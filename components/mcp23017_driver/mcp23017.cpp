#include <utility>
#include <sstream>
#include "esp_log.h"
#include "mcp23017.hpp"

#define TAG "MCP23017"

MCP23017::Reg_e MCP23017::operator+(const RegPair_e& rp, const Port_e& p){
    return static_cast<Reg_e>(std::to_underlying(rp)+std::to_underlying(p));
}

// constructor
MCP23017::MCP23017::MCP23017(
    I2CMaster::I2CBus& master_bus,
    SubAddress_e device_sub_address,
    uint32_t scl_speed_hz,
    int timeout_ms)
    :m_device(
        master_bus,
        MCP23017_I2C_base_address + std::to_underlying(device_sub_address), // I2C address (7 bits) : 00100nnn with the 3 nnn bits hardware dependants
        scl_speed_hz,
        I2C_ADDR_BIT_LEN_7),
    m_config{ // registers values at power-on/reset
        {RegPair_e::REGS_IODIR, {0xFF, 0xFF}},
        {RegPair_e::REGS_IPOL, {0x00, 0x00}},
        {RegPair_e::REGS_GPPU, {0x00, 0x00}}},
    m_timeout_ms{timeout_ms},
    m_status{Status_e::STS_DISCONNECTED}
{
}

// general single register read/write
auto MCP23017::MCP23017::read_register(const Reg_e reg) -> uint8_t
{
    try {
        auto data = m_device.transmit_receive(std::vector<uint8_t>{std::to_underlying(reg)}, 1, m_timeout_ms);
        return data[0];
    } catch (I2CMaster::I2CBusErrorException& e) {
        m_status = Status_e::STS_DISCONNECTED;
        return 0;
    } catch ( ... )
    {
        // everything else
        m_status = Status_e::STS_DISCONNECTED;
        std::stringstream ss;
        ss << "read_register failed: 1 byte @" << +std::to_underlying(reg);
        ESP_LOGI(TAG, "%s", ss.str().c_str());
        throw;
    }
}

void MCP23017::MCP23017::write_register(const Reg_e reg, const uint8_t value)
{
    try{
        m_device.transmit(std::vector<uint8_t>{std::to_underlying(reg), value}, m_timeout_ms);
    } catch (I2CMaster::I2CBusErrorException& e) {
        m_status = Status_e::STS_DISCONNECTED;
    } catch ( ... )
    {
        // everything else
        m_status = Status_e::STS_DISCONNECTED;
        std::stringstream ss;
        ss << "write_register failed: [" << std::hex << +value << std::dec << "] byte @" << +std::to_underlying(reg);
        ESP_LOGI(TAG, "%s", ss.str().c_str());
        throw;
    }
}

// general register pair read/write
auto MCP23017::MCP23017::read_registers(const RegPair_e regs) -> std::vector<uint8_t>
{
    try{
        return m_device.transmit_receive(std::vector<uint8_t>{std::to_underlying(regs)}, 2, m_timeout_ms);
    } catch (I2CMaster::I2CBusErrorException& e) {
        m_status = Status_e::STS_DISCONNECTED;
        return std::vector<uint8_t>{0, 0};
    } catch ( ... )
    {
        // everything else
        m_status = Status_e::STS_DISCONNECTED;
        std::stringstream ss;
        ss << "read_registers failed: 2 bytes @" << +std::to_underlying(regs);
        ESP_LOGI(TAG, "%s", ss.str().c_str());
        throw;
    }
}

void MCP23017::MCP23017::read_registers_into(const RegPair_e regs, std::vector<uint8_t>&values)
{
    try{
        m_device.transmit_receive_in(std::vector<uint8_t>{std::to_underlying(regs)}, values, m_timeout_ms);
    } catch (I2CMaster::I2CBusErrorException& e) {
        m_status = Status_e::STS_DISCONNECTED;
    } catch ( ... )
    {
        // everything else
        m_status = Status_e::STS_DISCONNECTED;
        std::stringstream ss;
        ss << "read_registers_into failed: " << +values.size() << " byte(s) @" << +std::to_underlying(regs);
        ESP_LOGI(TAG, "%s", ss.str().c_str());
        throw;
    }
}

void MCP23017::MCP23017::write_registers(const RegPair_e regs, const uint8_t value_port_a, const uint8_t value_port_b)
{
    try{
        m_device.transmit(std::vector<uint8_t>{std::to_underlying(regs), value_port_a, value_port_b}, m_timeout_ms);
    } catch (I2CMaster::I2CBusErrorException& e) {
        m_status = Status_e::STS_DISCONNECTED;
    } catch ( ... )
    {
        // everything else
        m_status = Status_e::STS_DISCONNECTED;
        std::stringstream ss;
        ss << "write_registers failed: [" << std::hex << +value_port_a << ", " << +value_port_b << std::dec << "] bytes @" << +std::to_underlying(regs);
        ESP_LOGI(TAG, "%s", ss.str().c_str());
        throw;
    }
}

// Ports state
auto MCP23017::MCP23017::read_port(const Port_e port) -> uint8_t
{
    return read_register(RegPair_e::REGS_GPIO + port);
}

auto MCP23017::MCP23017::read_ports(void) -> std::vector<uint8_t>
{
    return read_registers(RegPair_e::REGS_GPIO);
}

// Ports direction
void MCP23017::MCP23017::set_port_direction(const Port_e port, const uint8_t direction)
{
    m_config[RegPair_e::REGS_IODIR][std::to_underlying(port)] = direction;
    write_register(RegPair_e::REGS_IODIR + port, direction);
}

void MCP23017::MCP23017::set_ports_direction(const uint8_t direction_port_a, const uint8_t direction_port_b)
{
    m_config[RegPair_e::REGS_IODIR][0] = direction_port_a;
    m_config[RegPair_e::REGS_IODIR][1] = direction_port_b;
    write_registers(RegPair_e::REGS_IODIR, direction_port_a, direction_port_b);
}

// Ports polarity
void MCP23017::MCP23017::set_port_polarity(const Port_e port, const uint8_t polarity)
{
    m_config[RegPair_e::REGS_IPOL][std::to_underlying(port)] = polarity;
    write_register(RegPair_e::REGS_IPOL + port, polarity);
}

void MCP23017::MCP23017::set_ports_polarity(const uint8_t polarity_port_a, const uint8_t polarity_port_b)
{
    m_config[RegPair_e::REGS_IPOL][0] = polarity_port_a;
    m_config[RegPair_e::REGS_IPOL][1] = polarity_port_b;
    write_registers(RegPair_e::REGS_IPOL, polarity_port_a, polarity_port_b);
}

// Ports pullups
void MCP23017::MCP23017::set_port_pullups(const Port_e port, const uint8_t pullups)
{
    m_config[RegPair_e::REGS_GPPU][std::to_underlying(port)] = pullups;
    write_register(RegPair_e::REGS_GPPU + port, pullups);
}

void MCP23017::MCP23017::set_ports_pullups(const uint8_t pullups_port_a, const uint8_t pullups_port_b)
{
    m_config[RegPair_e::REGS_GPPU][0] = pullups_port_a;
    m_config[RegPair_e::REGS_GPPU][1] = pullups_port_b;
    write_registers(RegPair_e::REGS_GPPU, pullups_port_a, pullups_port_b);
}

void MCP23017::MCP23017::read_config(void)
{
    for (auto& cfg_pair : m_config){
        read_registers_into(cfg_pair.first, cfg_pair.second);
    }
}

void MCP23017::MCP23017::set_config(
    const uint8_t direction_port_a, const uint8_t direction_port_b,
    const uint8_t polarity_port_a, const uint8_t polarity_port_b,
    const uint8_t pullups_port_a, const uint8_t pullups_port_b)
{
    m_config[RegPair_e::REGS_IODIR][0] = direction_port_a;
    m_config[RegPair_e::REGS_IODIR][1] = direction_port_b;
    m_config[RegPair_e::REGS_IPOL][0] = polarity_port_a;
    m_config[RegPair_e::REGS_IPOL][1] = polarity_port_b;
    m_config[RegPair_e::REGS_GPPU][0] = pullups_port_a;
    m_config[RegPair_e::REGS_GPPU][1] = pullups_port_b;
    write_config();
}

void MCP23017::MCP23017::write_config(void)
{
    m_status = Status_e::STS_READY;
    // if the upcoming write fail, m_status will be set to STS_DISCONNECTED
    for (const auto& [rp, rp_values] : m_config){
        write_registers(rp, rp_values[0], rp_values[1]);
    }
}

void MCP23017::MCP23017::check_status(void)
{
    if (m_status == Status_e::STS_DISCONNECTED){
        m_status = Status_e::STS_CONNECTED;
        // if the read fail, m_status will be set to STS_DISCONNECTED
        uint8_t reg_icon_a = read_register(Reg_e::REG_ICONA);
    }
    if (m_status == Status_e::STS_CONNECTED){
        write_config(); // sets m_status to STS_READY / STS_DISCONNECTED if fail
    }
}