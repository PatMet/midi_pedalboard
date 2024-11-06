#pragma once
#include <vector>
#include "driver/i2c_master.h"
#include "i2c_master_bus.hpp"

namespace I2CMaster{

    class I2CDevice{
        I2CBus& m_master_bus; // TODO utile à conserve comme membre ? ou jetable ?
        i2c_device_config_t m_i2c_device_config; // TODO utile à conserve comme membre ? ou jetable ?
        i2c_master_dev_handle_t m_device_handle;
    public:
        I2CDevice(
            I2CBus& master_bus,
            uint16_t device_address,
            uint32_t scl_speed_hz = 100000UL,
            i2c_addr_bit_len_t dev_addr_length = I2C_ADDR_BIT_LEN_7);

        I2CDevice(const I2CDevice&) = delete;
        I2CDevice& operator=(const I2CDevice&) = delete;

        ~I2CDevice();

        auto transmit(const std::vector<uint8_t>& data, const int timeout_ms=-1) -> void;
        auto receive(const std::size_t nb_data_to_read, const int timeout_ms=-1) -> std::vector<uint8_t>;
        auto receive_in(std::vector<uint8_t>& data_to_read, const int timeout_ms=-1) -> void;
        auto transmit_receive(const std::vector<uint8_t>& data_to_write, const std::size_t nb_data_to_read, const int timeout_ms=-1) -> std::vector<uint8_t>;
        auto transmit_receive_in(const std::vector<uint8_t>& data_to_write, std::vector<uint8_t>& data_to_read, const int timeout_ms=-1) -> void;

    };

} // namespace
