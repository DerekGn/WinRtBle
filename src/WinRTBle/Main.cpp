#include "pch.h"

using namespace winrt;
using namespace Windows::Devices::Bluetooth::Advertisement;

std::wstring guidToString(GUID g)
{
	std::wostringstream ret;

	ret << std::hex << std::setfill(L'0')
		<< std::setw(2) << g.Data1 << ":"
		<< std::setw(2) << g.Data2 << ":"
		<< std::setw(2) << g.Data3 << ":"
		<< std::setw(2) << g.Data4;

	return ret.str();
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

int main()
{
	init_apartment();

	std::atomic<unsigned long long> deviceAddress = 0;

	try
	{
		BluetoothLEAdvertisementWatcher watcher;

		std::wcout << std::hex <<
			"MaxOutOfRangeTimeout: [0x" << watcher.MaxOutOfRangeTimeout().count() << "]" << std::endl <<
			"MaxSamplingInterval:  [0x" << watcher.MaxSamplingInterval().count() << "]" << std::endl <<
			"MinOutOfRangeTimeout: [0x" << watcher.MinOutOfRangeTimeout().count() << "]" << std::endl <<
			"MinSamplingInterval:  [0x" << watcher.MinSamplingInterval().count() << "]" << std::endl <<
			std::endl;

		watcher.ScanningMode(BluetoothLEScanningMode::Active);

		watcher.Received([&](BluetoothLEAdvertisementWatcher watcher, BluetoothLEAdvertisementReceivedEventArgs eventArgs)
		{
			watcher.Stop();

			std::wcout <<
				"LocalName: [" << eventArgs.Advertisement().LocalName().c_str() << "]" <<
				"AdvertisementType: [" << advertisementTypeToString(eventArgs.AdvertisementType()) << "]" <<
				"BluetoothAddress: [0x" << std::hex << eventArgs.BluetoothAddress() << "]" <<
				"RawSignalStrengthInDBm: [" << std::dec << eventArgs.RawSignalStrengthInDBm() << "]" <<
				std::endl;

			auto serviceUuids = eventArgs.Advertisement().ServiceUuids();

			for (GUID const & g : serviceUuids)
				std::wcout << "ServiceUUID: [" << guidToString(g) << "]" << std::endl;

			deviceAddress = eventArgs.BluetoothAddress();
		});

		auto status = watcher.Status();
		
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
			std::cout << "Device found." << std::endl;
		else
			std::cout << "Device not found." << std::endl;
		
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}
