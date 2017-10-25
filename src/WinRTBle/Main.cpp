#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace winrt::Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;

static const GUID UUID_SERIAL_CHARACTERISTIC = { 0x49535343, 0x1E4D, 0x4BD9,{ 0xBA, 0x61, 0x23, 0xC6, 0x47, 0x24, 0x96, 0x16 } };

static const GUID UUID_SERIAL_SERVICE = { 0x49535343, 0xFE7D, 0x4AE5,{ 0x8F, 0xA9, 0x9F, 0xAF, 0xD2, 0x05, 0xE4, 0x55 } };

std::string guidToString(GUID uuid)
{
	std::string guid;
	CHAR* wszUuid = NULL;
	if (::UuidToStringA(&uuid, (RPC_CSTR*) &wszUuid) == RPC_S_OK)
	{
		guid = wszUuid;
		::RpcStringFree((RPC_WSTR*) &wszUuid);
	}

	return guid;
}

std::string advertisementTypeToString(BluetoothLEAdvertisementType advertisementType)
{
	std::string ret;

	switch (advertisementType)
	{
	case BluetoothLEAdvertisementType::ConnectableUndirected:
		ret = "ConnectableUndirected";
		break;
	case BluetoothLEAdvertisementType::ConnectableDirected:
		ret = "ConnectableDirected";
		break;
	case BluetoothLEAdvertisementType::ScannableUndirected:
		ret = "ScannableUndirected";
		break;
	case BluetoothLEAdvertisementType::NonConnectableUndirected:
		ret = "NonConnectableUndirected";
		break;
	case BluetoothLEAdvertisementType::ScanResponse:
		ret = "ScanResponse";
		break;
	default:
		break;
	}

	return ret;
}

std::string bluetoothAddressTypeToString(BluetoothAddressType bluetoothAddressType)
{
	std::string ret;

	switch (bluetoothAddressType)
	{
	case BluetoothAddressType::Public:
		ret = "Public";
		break;
	case BluetoothAddressType::Random:
		ret = "Random";
		break;
	case BluetoothAddressType::Unspecified:
		ret = "Unspecified";
		break;
	default:
		break;
	}

	return ret;
}

std::string gattCommunicationStatusToString(GattCommunicationStatus status)
{
	std::string result;

	switch (status)
	{
	case GattCommunicationStatus::AccessDenied:
		result = "AccessDenied";
		break;
	case GattCommunicationStatus::ProtocolError:
		result = "ProtocolError";
		break;
	case GattCommunicationStatus::Success:
		result = "Success";
		break;
	case GattCommunicationStatus::Unreachable:
		result = "Unreachable";
		break;
	default:
		break;
	}

	return result;
}

std::string gattCharacteristicProperties(GattCharacteristicProperties properties)
{
	std::string result;

	result.append("[");
	if((properties & GattCharacteristicProperties::AuthenticatedSignedWrites) == GattCharacteristicProperties::AuthenticatedSignedWrites)
		result.append(" AuthenticatedSignedWrites ");

	if ((properties & GattCharacteristicProperties::Broadcast) == GattCharacteristicProperties::Broadcast)
		result.append(" Broadcast ");

	if ((properties & GattCharacteristicProperties::ExtendedProperties) == GattCharacteristicProperties::ExtendedProperties)
		result.append(" ExtendedProperties ");

	if ((properties & GattCharacteristicProperties::Indicate) == GattCharacteristicProperties::Indicate)
		result.append(" Indicate ");

	if ((properties & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify)
		result.append(" Notify ");

	if ((properties & GattCharacteristicProperties::Read) == GattCharacteristicProperties::Read)
		result.append(" Read ");

	if ((properties & GattCharacteristicProperties::ReliableWrites) == GattCharacteristicProperties::ReliableWrites)
		result.append(" ReliableWrites " );

	if ((properties & GattCharacteristicProperties::WritableAuxiliaries) == GattCharacteristicProperties::WritableAuxiliaries)
		result.append(" WritableAuxiliaries ");

	if ((properties & GattCharacteristicProperties::Write) == GattCharacteristicProperties::Write)
		result.append(" Write ");

	if ((properties & GattCharacteristicProperties::WriteWithoutResponse) == GattCharacteristicProperties::WriteWithoutResponse)
		result.append(" WriteWithoutResponse ");

	result.append("]");
	return result;
}

IAsyncAction OpenDevice(unsigned long long deviceAddress)
{
	auto device = co_await BluetoothLEDevice::FromBluetoothAddressAsync(deviceAddress);

	std::cout << std::hex <<
		"\tDevice Information: " << std::endl <<
		"\t\tBluetoothAddress: [" << device.BluetoothAddress() << "]" << std::endl <<
		"\t\tBluetoothAddressType: [" << bluetoothAddressTypeToString(device.BluetoothAddressType()) << "]" << std::endl <<
		"\t\tConnectionStatus: [" << (device.ConnectionStatus() == BluetoothConnectionStatus::Connected ? "Connected" : "Disconnected") << "]" << std::endl <<
		"\t\tDeviceId: [" << device.DeviceId().c_str() << "]" << std::endl <<
		std::endl;

	auto services = co_await device.GetGattServicesAsync();

	std::cout << "Services Count: " << services.Services().Size() << std::endl;

	for (GenericAttributeProfile::GattDeviceService const & s : services.Services())
	{
		std::cout << std::hex <<
			"\t\tService - Guid: [" << guidToString(s.Uuid()) << "]" << std::endl;

		auto characteristics = co_await s.GetCharacteristicsAsync();

		for (GenericAttributeProfile::GattCharacteristic const & c : characteristics.Characteristics())
		{
			std::cout << std::hex <<
				"\t\t\tCharacteristic - Guid: [" << guidToString(c.Uuid()) << "]" << gattCharacteristicProperties(c.CharacteristicProperties()) << std::endl;

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

			if (c.Uuid() == UUID_SERIAL_CHARACTERISTIC)
			{
				c.ValueChanged([&](GattCharacteristic characteristic, GattValueChangedEventArgs eventArgs)
				{
					std::cout << "Event" << std::endl;

					auto reader = DataReader::FromBuffer(eventArgs.CharacteristicValue());

					std::cout << "serial data read: [" << std::hex << reader.ReadByte() << "]";
				});

				DataWriter writer;
				writer.WriteByte(0x56);
				auto status = co_await c.WriteValueWithResultAsync(writer.DetachBuffer());
				
				std::cout << "Status: " << gattCommunicationStatusToString(status.Status()) << std::endl;

				std::this_thread::sleep_for(std::chrono::seconds(1));

				/*auto readResult = co_await c.ReadValueAsync();

				std::wcout << "Status: " << gattCommunicationStatusToString(readResult.Status()) << std::endl;

				if (readResult.Status() == GattCommunicationStatus::Success)
				{
					auto reader = DataReader::FromBuffer(readResult.Value());

					std::cout << "serial data read: [" << std::hex << reader.ReadByte() << "]";
				}*/
			}
		}
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

		watcher.ScanningMode(BluetoothLEScanningMode::Active);

		watcher.Received([&](BluetoothLEAdvertisementWatcher watcher, BluetoothLEAdvertisementReceivedEventArgs eventArgs)
		{
			watcher.Stop();

			std::cout << std::endl <<
				"AdvertisementReceived:" << std::endl <<
				"\tLocalName: [" << eventArgs.Advertisement().LocalName().c_str() << "]" << std::endl <<
				"\tAdvertisementType: [" << advertisementTypeToString(eventArgs.AdvertisementType()) << "]" << std::endl <<
				"\tBluetoothAddress: [0x" << std::hex << eventArgs.BluetoothAddress() << "]" << std::endl <<
				"\tRawSignalStrengthInDBm: [" << std::dec << eventArgs.RawSignalStrengthInDBm() << "]" <<
				std::endl;

			for (GUID const & g : eventArgs.Advertisement().ServiceUuids())
				std::cout << "ServiceUUID: [" << guidToString(g) << "]" << std::endl;

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
			OpenDevice(deviceAddress).get();
		else
			std::cout << "Device not found." << std::endl;

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}
