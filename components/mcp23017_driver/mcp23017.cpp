#include <utility>
#include "mcp23017.hpp"

MCP23017::Reg_e MCP23017::operator+(const RegPair_e& rp, const Port_e& p){
    return static_cast<Reg_e>(std::to_underlying(rp)+std::to_underlying(p));
}

// constructor
MCP23017::MCP23017::MCP23017(
    I2CMaster::I2CBus& master_bus,
    SubAddress_e device_sub_address,
    uint32_t scl_speed_hz)
    :m_device(
        master_bus,
        MCP23017_I2C_base_address + std::to_underlying(device_sub_address), // I2C address (7 bits) : 00100nnn with the 3 nnn bits hardware dependants
        scl_speed_hz,
        I2C_ADDR_BIT_LEN_7)
{
}

// general single register read/write
auto MCP23017::MCP23017::read_register(const Reg_e reg, const int timeout_ms) -> uint8_t
{
    auto data = m_device.transmit_receive(std::vector<uint8_t>{std::to_underlying(reg)}, 1, timeout_ms);
    return data[0];
}

void MCP23017::MCP23017::write_register(const Reg_e reg, const uint8_t value, const int timeout_ms)
{
    m_device.transmit(std::vector<uint8_t>{std::to_underlying(reg), value}, timeout_ms);
}

// general register pair read/write
auto MCP23017::MCP23017::read_registers(const RegPair_e regs, const int timeout_ms) -> std::vector<uint8_t>
{
    return m_device.transmit_receive(std::vector<uint8_t>{std::to_underlying(regs)}, 2, timeout_ms);
}

void MCP23017::MCP23017::write_registers(const RegPair_e regs, const uint8_t value_port_a, const uint8_t value_port_b, const int timeout_ms)
{
    m_device.transmit(std::vector<uint8_t>{std::to_underlying(regs), value_port_a, value_port_b}, timeout_ms);
}

// Ports state
auto MCP23017::MCP23017::read_port(const Port_e port, const int timeout_ms) -> uint8_t
{
    return read_register(RegPair_e::REGS_GPIO + port, timeout_ms);
}

auto MCP23017::MCP23017::read_ports(const int timeout_ms) -> std::vector<uint8_t>
{
    return read_registers(RegPair_e::REGS_GPIO, timeout_ms);
}

// Ports direction
void MCP23017::MCP23017::set_port_direction(const Port_e port, const uint8_t direction, const int timeout_ms)
{
    write_register(RegPair_e::REGS_IODIR + port, direction, timeout_ms);
}

void MCP23017::MCP23017::set_ports_direction(const uint8_t direction_port_a, const uint8_t direction_port_b, const int timeout_ms)
{
    write_registers(RegPair_e::REGS_IODIR, direction_port_a, direction_port_b, timeout_ms);
}

// Ports polarity
void MCP23017::MCP23017::set_port_polarity(const Port_e port, const uint8_t polarity, const int timeout_ms)
{
    write_register(RegPair_e::REGS_IPOL + port, polarity, timeout_ms);
}

void MCP23017::MCP23017::set_ports_polarity(const uint8_t polarity_port_a, const uint8_t polarity_port_b, const int timeout_ms)
{
    write_registers(RegPair_e::REGS_IPOL, polarity_port_a, polarity_port_b, timeout_ms);
}

// Ports pullups
void MCP23017::MCP23017::set_port_pullups(const Port_e port, const uint8_t pullups, const int timeout_ms)
{
    write_register(RegPair_e::REGS_GPPU + port, pullups, timeout_ms);
}

void MCP23017::MCP23017::set_ports_pullups(const uint8_t pullups_port_a, const uint8_t pullups_port_b, const int timeout_ms)
{
    write_registers(RegPair_e::REGS_GPPU, pullups_port_a, pullups_port_b, timeout_ms);
}
