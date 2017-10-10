#pragma once

#pragma comment(lib, "windowsapp") 

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>

#include "winrt\Windows.Foundation.h"
#include "winrt\Windows.Storage.Streams.h"
#include "winrt\Windows.Devices.Bluetooth.h"
#include "winrt\Windows.Devices.Bluetooth.Advertisement.h"
#include "winrt\Windows.Devices.Bluetooth.GenericAttributeProfile.h"
