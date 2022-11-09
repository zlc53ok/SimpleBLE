#include <simplebluez/Bluez.h>
#include <simplebluez/Exceptions.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
#include <string.h>

inline SimpleBluez::ByteArray MakeByteArray(const char* data)
{
	size_t size = strlen(data);
	SimpleBluez::ByteArray aa;
    aa.resize(size);
    memcpy(&aa[0], data, size);
    return aa;
}

SimpleBluez::Bluez bluez;

std::atomic_bool async_thread_active = true;
void async_thread_function() {
    while (async_thread_active) {
        bluez.run_async();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void millisecond_delay(int ms) {
    for (int i = 0; i < ms; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void print_byte_array(SimpleBluez::ByteArray& bytes) {
    for (auto byte : bytes) {
        std::cout << std::hex << std::setfill('0') << (uint32_t)((uint8_t)byte) << " ";
        break;
    }
    std::cout << std::endl;
}

std::vector<std::shared_ptr<SimpleBluez::Device>> peripherals;

int main(int argc, char* argv[]) {
    int selection = -1;

    bluez.init();
    std::thread* async_thread = new std::thread(async_thread_function);

    auto adapters = bluez.get_adapters();
    std::cout << "Available adapters:" << std::endl;
    for (int i = 0; i < adapters.size(); i++) {
        std::cout << "[" << i << "] " << adapters[i]->identifier() << " [" << adapters[i]->address() << "]"
                  << std::endl;
    }

    std::cout << "Please select an adapter to scan: ";
    std::cin >> selection;
    if (selection < 0 || selection >= adapters.size()) {
        std::cout << "Invalid selection" << std::endl;
        return 1;
    }

    auto adapter = adapters[selection];
    std::cout << "Scanning " << adapter->identifier() << " [" << adapter->address() << "]" << std::endl;

    SimpleBluez::Adapter::DiscoveryFilter filter;
    filter.Transport = SimpleBluez::Adapter::DiscoveryFilter::TransportType::LE;
    adapter->discovery_filter(filter);

    adapter->set_on_device_updated([](std::shared_ptr<SimpleBluez::Device> device) {
        if (device->name() == "") {
            return;
        }

        if (std::find(peripherals.begin(), peripherals.end(), device) == peripherals.end()) {
            std::cout << "Found device: " << device->name() << " [" << device->address() << "]" << std::endl;
            peripherals.push_back(device);
        }
    });

    adapter->discovery_start();
    millisecond_delay(3000);
    adapter->discovery_stop();

    std::cout << "The following devices were found:" << std::endl;
    for (int i = 0; i < peripherals.size(); i++) {
        std::cout << "[" << i << "] " << peripherals[i]->name() << " [" << peripherals[i]->address() << "]"
                  << std::endl;
    }

    std::cout << "Please select a device to connect to: ";
    std::cin >> selection;

    if (selection >= 0 && selection < peripherals.size()) {
        auto peripheral = peripherals[selection];
        std::cout << "Connecting to " << peripheral->name() << " [" << peripheral->address() << "]" << std::endl;

        for (int attempt = 0; attempt < 3; attempt++) {
            try {
                peripheral->connect();
            } catch (SimpleDBus::Exception::SendFailed& e) {
                millisecond_delay(100);
            }
        }

        if (!peripheral->connected() || !peripheral->services_resolved()) {
            std::cout << "Failed to connect to " << peripheral->name() << " [" << peripheral->address() << "]"
                      << std::endl;
            return 1;
        }

        std::cout << "Successfully connected, testing NUS service." << std::endl;

        auto characteristic_rx = peripheral->get_characteristic("6e400001-b5a3-f393-e0a9-e50e24dcca9e",
                                                                "6e400002-b5a3-f393-e0a9-e50e24dcca9e");

        characteristic_rx->write_command(MakeByteArray("Hello World"));

        auto characteristic_tx = peripheral->get_characteristic("6e400001-b5a3-f393-e0a9-e50e24dcca9e",
                                                                "6e400003-b5a3-f393-e0a9-e50e24dcca9e");

        characteristic_tx->set_on_value_changed([&](SimpleBluez::ByteArray new_value) { print_byte_array(new_value); });

        characteristic_tx->start_notify();
        millisecond_delay(3000);
        characteristic_tx->stop_notify();
        millisecond_delay(1000);

        peripheral->disconnect();

        // Sleep for an additional second before returning.
        // If there are any unexpected events, this example will help debug them.
        millisecond_delay(1000);
    }

    async_thread_active = false;
    while (!async_thread->joinable()) {
        millisecond_delay(10);
    }
    async_thread->join();
    delete async_thread;

    return 0;
}
