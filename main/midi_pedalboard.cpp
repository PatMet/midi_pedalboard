#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <bitset>

#include "rgb_led.hpp"

#include "i2c_master_bus.hpp"
//#include "i2c_master_device.hpp"
#include "mcp23017.hpp"

#include "usb.hpp"
#include "usb_midi.hpp"

using namespace std::chrono_literals;

#define LED_GPIO 18  // GPIO18 on SAOLA-1 devboard

#define PDB_FIRST_MIDI_NOTE 0x3C

template<std::size_t N>
std::bitset<N>& operator<<(std::bitset<N>& bits, const uint8_t& byte){
    bits <<= 8;
    bits |= byte;
    return bits;
}

template<std::size_t N>
std::bitset<N>& operator<<(std::bitset<N>& bits, const std::vector<uint8_t>& vect){
    std::for_each(vect.crbegin(), vect.crend(), [&bits](const uint8_t byte) { bits << byte; });
    return bits;
}

extern "C" void app_main(void)
{

    RGBLed led = RGBLed(LED_GPIO);

    I2CMaster::I2CBus i2c_bus{
        I2C_NUM_0,
        GPIO_NUM_9, // SDA pin
        GPIO_NUM_8, // SCL pin
        false,      // enable internal pullups
    };
    MCP23017::MCP23017 gpio0{i2c_bus, MCP23017::SubAddress_e::SUBADDR_0};
    MCP23017::MCP23017 gpio1{i2c_bus, MCP23017::SubAddress_e::SUBADDR_1};

    gpio0.set_ports_direction(0xFF, 0xFF); // all pins as inputs
    gpio0.set_ports_polarity(0xFF, 0xFF); // inverted polarity
    gpio0.set_ports_pullups(0xFF, 0xFF); // pull-up resistors enable
    std::cout << "gpio0 config done." << std::endl;
    gpio0.set_config(
        0xFF, 0xFF, // all pins as inputs (default)
        0xFF, 0xFF, // inverted polarity
        0xFF, 0xFF); // pull-up resistors enable
    std::cout << "gpio0 set_config done." << std::endl;

    gpio1.set_config(
        0xFF, 0xFF, // all pins as inputs (default)
        0xFF, 0xFF, // inverted polarity
        0xFF, 0xFF); // pull-up resistors enable
    std::cout << "gpio1 set_config done." << std::endl;

    led.blink(1);

    usb_itf_install();
    UsbHostMidiClient usb_midi;
    usb_midi.activate_pass_through(true);

    led.blink(0);

    std::vector<uint8_t> gpio_reg{18};
    std::bitset<30> pedals_status;
    std::bitset<30> pedals_status_prec;
    std::bitset<30> note_on_mask;
    std::bitset<30> note_off_mask;

    bool midi_config_sent = false;

    while (true) {
        // time measurements for performance monitoring
        const auto start_time = std::chrono::high_resolution_clock::now();

        // Pedals status update

        // GPIO1 (MSB) first
        pedals_status_prec = pedals_status;
        if (gpio1.get_status() == MCP23017::Status_e::STS_READY){
            pedals_status << gpio1.read_ports();
        } else {
            gpio1.check_status();
            pedals_status << 0x00;  // default pedals states if gpio unavailable
        }
        // GPIO0 (LSB)
        if (gpio0.get_status() == MCP23017::Status_e::STS_READY){
            pedals_status << gpio0.read_ports();
        } else {
            gpio0.check_status();
            pedals_status << 0x00;  // default pedals states if gpio unavailable
        }

        // Pedals status changed
        if (pedals_status != pedals_status_prec){

            // note off detection
            // 0 -> 0 : 0
            // 0 -> 1 : 0
            // 1 -> 0 : 1
            // 1 -> 1 : 0
            note_off_mask = pedals_status_prec & ~pedals_status;

            // note on detection
            // 0 -> 0 : 0
            // 0 -> 1 : 1
            // 1 -> 0 : 0
            // 1 -> 1 : 0
            note_on_mask = ~pedals_status_prec & pedals_status;

            if (midi_config_sent){
                // sending note OFF
                for (int b=0; b<30; b++){
                    if (note_off_mask.test(b)){
                        std::cout << "Note OFF : " << b << std::endl;
                        usb_midi.send_note(false, PDB_FIRST_MIDI_NOTE + b);
                    }
                }
                // sending note ON
                for (int b=0; b<30; b++){
                    if (note_on_mask.test(b)){
                        std::cout << "Note ON : " << b << std::endl;
                        usb_midi.send_note(true, PDB_FIRST_MIDI_NOTE + b);
                    }
                }
                //usb_midi.send_local_control(note_on);
            }

            std::cout << pedals_status << std::endl;
            const std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - start_time;
            std::cout << "traitement : " << elapsed << std::endl;
        }

        // USB device status management
        if (usb_midi.connected()){
            if (midi_config_sent == false){
                // Midi device connection
                led.blink(1);

                // sending MIDI config (
                // disable local control + activate pass through
                // disable local control ? auto at connection ? 
                // bank select, select)
                // TODO add usb_midi.send... command...

                midi_config_sent = true;
            }
        }
        else
        {
            if (midi_config_sent){
                // Midi device disconnection.
                led.blink(0);

                //Get ready to send config after next connection
                midi_config_sent = false;
            }
        }

        std::this_thread::sleep_for(10ms);
    }
}
