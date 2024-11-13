#pragma once
#include <exception>
extern "C" {
#include "driver/i2c_types.h"
#include "driver/i2c_master.h"
}

namespace I2CMaster{

    class I2CDriverException : public std::exception{
//        private:
//        char * message;

    public:
//        I2CDriverException(char * msg) : message(msg) {}
        char * what () {
//            return message;
            return "I2C driver error:";
        }
    };

    class I2CBusErrorException : public std::exception{
    public:
        char * what () {
            return "I2C bus error:";
        }
    };

    class I2CBus{
        i2c_master_bus_config_t m_i2c_master_config; // TODO utile à conserve comme membre ? ou jetable ?
        i2c_master_bus_handle_t m_bus_handle;
    public:
        I2CBus(
            i2c_port_t i2c_port,
            gpio_num_t sda_io_num,
            gpio_num_t scl_io_num,
            bool enable_internal_pullup,
            i2c_clock_source_t clk_source = I2C_CLK_SRC_DEFAULT,
            uint8_t glitch_ignore_cnt = 7);

        I2CBus(const I2CBus&) = delete;
        I2CBus& operator=(const I2CBus&) = delete;

        ~I2CBus();

        friend class I2CDevice;
    };

} // namespace
