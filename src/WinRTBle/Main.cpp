#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace winrt::Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;

std::wstring guidToString(GUID uuid)
{
	std::wstring guid;
	WCHAR* wszUuid = NULL;
	if (::UuidToString(&uuid, (RPC_WSTR*) &wszUuid) == RPC_S_OK)
	{
		guid = wszUuid;
		::RpcStringFree((RPC_WSTR*) &wszUuid);
	}

	return guid;
}

std::wstring advertisementTypeToString(BluetoothLEAdvertisementType advertisementType)
{
	std::wstring ret;

	switch (advertisementType)
	{
	case BluetoothLEAdvertisementType::ConnectableUndirected:
		ret = L"ConnectableUndirected";
		break;
	case BluetoothLEAdvertisementType::ConnectableDirected:
		ret = L"ConnectableDirected";
		break;
	case BluetoothLEAdvertisementType::ScannableUndirected:
		ret = L"ScannableUndirected";
		break;
	case BluetoothLEAdvertisementType::NonConnectableUndirected:
		ret = L"NonConnectableUndirected";
		break;
	case BluetoothLEAdvertisementType::ScanResponse:
		ret = L"ScanResponse";
		break;
	default:
		break;
	}

	return ret;
}

std::wstring bluetoothAddressTypeToString(BluetoothAddressType bluetoothAddressType)
{
	std::wstring ret;

	switch (bluetoothAddressType)
	{
	case BluetoothAddressType::Public:
		ret = L"Public";
		break;
	case BluetoothAddressType::Random:
		ret = L"Random";
		break;
	case BluetoothAddressType::Unspecified:
		ret = L"Unspecified";
		break;
	default:
		break;
	}

	return ret;
}

IAsyncAction OpenDevice(unsigned long long deviceAddress)
{
	auto device = co_await BluetoothLEDevice::FromBluetoothAddressAsync(deviceAddress);

	std::wcout << std::hex <<
		"\tDevice Information: " << std::endl <<
		"\tBluetoothAddress: [" << device.BluetoothAddress() << "]" << std::endl <<
		"\tBluetoothAddressType: [" << bluetoothAddressTypeToString(device.BluetoothAddressType()) << "]" << std::endl <<
		"\tConnectionStatus: [" << (device.ConnectionStatus() == BluetoothConnectionStatus::Connected ? "Connected" : "Disconnected") << "]" << std::endl <<
		"\tDeviceId: [" << device.DeviceId().c_str() << "]" << std::endl <<
		std::endl;

	auto services = co_await device.GetGattServicesAsync();

	for (GenericAttributeProfile::GattDeviceService const & s : services.Services())
	{
		std::wcout << std::hex <<
			"\t\tService - Guid: [" << guidToString(s.Uuid()) << "]" << std::endl;

		auto characteristics = co_await s.GetCharacteristicsAsync();

		for (GenericAttributeProfile::GattCharacteristic const & c : characteristics.Characteristics())
		{
			std::wcout << std::hex <<
				"\t\t\tCharacteristic - Guid: [" << guidToString(c.Uuid()) << "]" << std::endl;

			if (c.CharacteristicProperties() == GattCharacteristicProperties::Read)
			{
				auto readResult = co_await c.ReadValueAsync();

				if (readResult.Status() == GattCommunicationStatus::Success)
				{
					std::wcout << "\t\t\tCharacteristic Data - Size: [" << readResult.Value().Length() << "]" << std::endl;

					DataReader reader = DataReader::FromBuffer(readResult.Value());

					std::wcout << "\t\t\tCharacteristic Data - [" << reader.ReadString(readResult.Value().Length()).c_str() << "]" << std::endl;
				}
			}

			if (c.CharacteristicProperties() == GattCharacteristicProperties::Notify)
			{

				c.ValueChanged([](GattCharacteristic const& charateristic, GattValueChangedEventArgs const& args)
				{
					std::wcout << std::hex <<
						"\t\tNotified GattCharacteristic - Guid: [" << guidToString(charateristic.Uuid()) << "]" << std::endl;

					DataReader reader = DataReader::FromBuffer(args.CharacteristicValue());

					// Note this assumes value is string the characteristic type must be determined before reading
					// This can display junk or maybe blowup
					std::wcout << "\t\t\tCharacteristic Data - [" << reader.ReadString(args.CharacteristicValue().Length()).c_str() << "]" << std::endl;
				});
			}
		}
	}

	auto uartService = services.Services().GetAt(4);

	auto txService = (co_await uartService.GetCharacteristicsAsync()).Characteristics().GetAt(0);

	auto rxService = (co_await uartService.GetCharacteristicsAsync()).Characteristics().GetAt(1);

	auto writer = Windows::Storage::Streams::DataWriter();
	writer.WriteByte(0x56);
	auto status = co_await txService.WriteValueWithResultAsync(writer.DetachBuffer(),winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteOption::WriteWithoutResponse);
	
	if (status.Status() == GattCommunicationStatus::Success)
	{
	}
	else
	{
		std::wcout << "Write Failed";
	}

	device.Close();
}

int main()
{
	init_apartment();

	std::atomic<unsigned long long> deviceAddress = 0;

	try
	{
		BluetoothLEAdvertisementWatcher watcher;

		std::wcout << std::hex <<
			"BluetoothLEAdvertisementWatcher:" << std::endl <<
			"\tMaxOutOfRangeTimeout: [0x" << watcher.MaxOutOfRangeTimeout().count() << "]" << std::endl <<
			"\tMaxSamplingInterval:  [0x" << watcher.MaxSamplingInterval().count() << "]" << std::endl <<
			"\tMinOutOfRangeTimeout: [0x" << watcher.MinOutOfRangeTimeout().count() << "]" << std::endl <<
			"\tMinSamplingInterval:  [0x" << watcher.MinSamplingInterval().count() << "]" << std::endl <<
			std::endl;

		watcher.ScanningMode(BluetoothLEScanningMode::Passive);

		watcher.Received([&](BluetoothLEAdvertisementWatcher watcher, BluetoothLEAdvertisementReceivedEventArgs eventArgs)
		{
			watcher.Stop();

			std::wcout <<
				"AdvertisementReceived:" << std::endl <<
				"\tLocalName: [" << eventArgs.Advertisement().LocalName().c_str() << "]" <<
				"\tAdvertisementType: [" << advertisementTypeToString(eventArgs.AdvertisementType()) << "]" <<
				"\tBluetoothAddress: [0x" << std::hex << eventArgs.BluetoothAddress() << "]" <<
				"\tRawSignalStrengthInDBm: [" << std::dec << eventArgs.RawSignalStrengthInDBm() << "]" <<
				std::endl;

			for (GUID const & g : eventArgs.Advertisement().ServiceUuids())
				std::wcout << "ServiceUUID: [" << guidToString(g) << "]" << std::endl;

			deviceAddress = eventArgs.BluetoothAddress();
		});

		std::cout << "Waiting for device: ";

		watcher.Start();

		int count = 0;

		while ((count++ < 10) && deviceAddress == 0)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << '.';
		}

		std::cout << std::endl << "Finished waiting for device." << std::endl;

		if (deviceAddress != 0)
		{
			std::cout << "Device found." << std::endl;

			OpenDevice(deviceAddress).get();
		}
		else
			std::cout << "Device not found." << std::endl;
		

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}
