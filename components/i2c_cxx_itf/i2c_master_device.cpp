extern "C" {
#include "driver/i2c_master.h"
}
#include "i2c_master_device.hpp"

I2CMaster::I2CDevice::I2CDevice(
    I2CBus& master_bus,
    uint16_t device_address,
    uint32_t scl_speed_hz,
    i2c_addr_bit_len_t dev_addr_length)
    :m_master_bus{master_bus},
    m_i2c_device_config{
        .dev_addr_length = dev_addr_length,
        .device_address = device_address,
        .scl_speed_hz = scl_speed_hz,
    }
{
    esp_err_t err_code = i2c_master_bus_add_device(
        m_master_bus.m_bus_handle,
        &m_i2c_device_config,
        &m_device_handle);
    if(err_code != ESP_OK){
        // throw I2CDriverException(esp_err_to_name(err_code)); // doesn't work (char* problems)
        ESP_ERROR_CHECK_WITHOUT_ABORT(err_code);  // to have the error message
        throw I2CDriverException();
    }
}

I2CMaster::I2CDevice::~I2CDevice(){
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_bus_rm_device(m_device_handle));
}

auto I2CMaster::I2CDevice::transmit(
    const std::vector<uint8_t>& data,
    const int timeout_ms) -> void
{
    esp_err_t err_code = i2c_master_transmit(
        m_device_handle,
        data.data(),
        data.size(),
        timeout_ms);
    if ((err_code == ESP_ERR_TIMEOUT) || (err_code == ESP_ERR_INVALID_STATE)){
        throw I2CBusErrorException();
    }
    if(err_code != ESP_OK){
        // throw I2CDriverException(esp_err_to_name(err_code)); // doesn't work (char* problems)
        ESP_ERROR_CHECK_WITHOUT_ABORT(err_code);  // to have the error message
        throw I2CDriverException();
    }
}

auto I2CMaster::I2CDevice::receive(
    const std::size_t nb_data_to_read,
    const int timeout_ms) -> std::vector<uint8_t>
{
    // initialisation d'un vector de taille nb_data_to_read
    std::vector<uint8_t> data_to_read(nb_data_to_read);

    esp_err_t err_code = i2c_master_receive(
        m_device_handle,
        data_to_read.data(),
        data_to_read.size(),
        timeout_ms);
    if ((err_code == ESP_ERR_TIMEOUT) || (err_code == ESP_ERR_INVALID_STATE)){
        throw I2CBusErrorException();
    }
    if(err_code != ESP_OK){
        // throw I2CDriverException(esp_err_to_name(err_code)); // doesn't work (char* problems)
        ESP_ERROR_CHECK_WITHOUT_ABORT(err_code);  // to have the error message
        throw I2CDriverException();
    }

    return data_to_read;
}

auto I2CMaster::I2CDevice::receive_in(
    std::vector<uint8_t>& data_to_read,
    const int timeout_ms) -> void
    {
    esp_err_t err_code = i2c_master_receive(
        m_device_handle,
        data_to_read.data(),
        data_to_read.size(),
        timeout_ms);
    if ((err_code == ESP_ERR_TIMEOUT) || (err_code == ESP_ERR_INVALID_STATE)){
        throw I2CBusErrorException();
    }
    if(err_code != ESP_OK){
        // throw I2CDriverException(esp_err_to_name(err_code)); // doesn't work (char* problems)
        ESP_ERROR_CHECK_WITHOUT_ABORT(err_code);  // to have the error message
        throw I2CDriverException();
    }
}

auto I2CMaster::I2CDevice::transmit_receive(
    const std::vector<uint8_t>& data_to_write,
    const std::size_t nb_data_to_read,
    const int timeout_ms) -> std::vector<uint8_t>
    {
    // initialisation d'un vector de taille nb_data_to_read
    std::vector<uint8_t> data_to_read(nb_data_to_read);

    esp_err_t err_code = i2c_master_transmit_receive(
        m_device_handle,
        data_to_write.data(),
        data_to_write.size(),
        data_to_read.data(),
        data_to_read.size(),
        timeout_ms);
    if ((err_code == ESP_ERR_TIMEOUT) || (err_code == ESP_ERR_INVALID_STATE)){
        throw I2CBusErrorException();
    }
    if(err_code != ESP_OK){
        // throw I2CDriverException(esp_err_to_name(err_code)); // doesn't work (char* problems)
        ESP_ERROR_CHECK_WITHOUT_ABORT(err_code);  // to have the error message
        throw I2CDriverException();
    }

    return data_to_read;
}

auto I2CMaster::I2CDevice::transmit_receive_in(
    const std::vector<uint8_t>& data_to_write,
    std::vector<uint8_t>& data_to_read,
    const int timeout_ms) -> void
    {
    esp_err_t err_code = i2c_master_transmit_receive(
        m_device_handle,
        data_to_write.data(),
        data_to_write.size(),
        data_to_read.data(),
        data_to_read.size(),
        timeout_ms);
    if ((err_code == ESP_ERR_TIMEOUT) || (err_code == ESP_ERR_INVALID_STATE)){
        throw I2CBusErrorException();
    }
    if(err_code != ESP_OK){
        // throw I2CDriverException(esp_err_to_name(err_code)); // doesn't work (char* problems)
        ESP_ERROR_CHECK_WITHOUT_ABORT(err_code);  // to have the error message
        throw I2CDriverException();
    }
}
