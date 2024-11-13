#include "i2c_master_bus.hpp"
#include <sstream>

I2CMaster::I2CBus::I2CBus(
    i2c_port_t i2c_port,
    gpio_num_t sda_io_num,
    gpio_num_t scl_io_num,
    bool enable_internal_pullup,
    i2c_clock_source_t clk_source,
    uint8_t glitch_ignore_cnt)
    :m_i2c_master_config{
        .i2c_port = i2c_port,
        .sda_io_num = sda_io_num,
        .scl_io_num = scl_io_num,
        .clk_source = clk_source,
        .glitch_ignore_cnt = glitch_ignore_cnt,
        .flags{
            .enable_internal_pullup = enable_internal_pullup,
            }
        }
    {

    // TODO remplacer ESP_ERROR_CHECK par un autre mécanisme de gestion d'erreur (exception ?)
    //ESP_ERROR_CHECK(i2c_new_master_bus(&m_i2c_master_config, &m_bus_handle));
    esp_err_t err_code = i2c_new_master_bus(&m_i2c_master_config, &m_bus_handle);
    if(err_code != ESP_OK){
        // throw I2CDriverException(esp_err_to_name(err_code)); // doesn't work (char* problems)
        ESP_ERROR_CHECK_WITHOUT_ABORT(err_code);  // to have the error message
        throw I2CDriverException();
    }
}

I2CMaster::I2CBus::~I2CBus(){
    // TODO verifier si tous les devices ont été supprimés au préalable ?
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_del_master_bus(m_bus_handle));
}
