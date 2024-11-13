#pragma once
#include <map>
#include <utility>

#include "i2c_master_bus.hpp"
#include "i2c_master_device.hpp"

namespace MCP23017{

    enum class SubAddress_e : uint16_t
    {
        SUBADDR_0 = 0,
        SUBADDR_1,
        SUBADDR_2,
        SUBADDR_3,
        SUBADDR_4,
        SUBADDR_5,
        SUBADDR_6,
        SUBADDR_7,
    };

    //Device status
    enum class Status_e
    {    
        STS_DISCONNECTED,
        STS_CONNECTED,
        STS_READY,
    };

    // GPIO Ports
    enum class Port_e : uint8_t
    {
        PORT_A,
        PORT_B
    };

    // Registers (for register pair access)
    enum class RegPair_e: uint8_t
    {
        REGS_IODIR   = 0x00, // I/O direction registers
        REGS_IPOL    = 0x02, // Inputs polarity registers
        REGS_GPINTEN = 0x04, // Interupt enable registers
        REGS_DEFVAL  = 0x06, // Default value registers
        REGS_INTCON  = 0x08, // Interrupt-on-change control register
        REGS_ICON    = 0x0A, // Configuration registers
        REGS_GPPU    = 0x0C, // pull-up resistors enable
        REGS_INTFA   = 0x0E, // Interrupt flags
        REGS_INTCAP  = 0x10, // Interrupt captured values for port registers
        REGS_GPIO    = 0x12, // GPIO port registers
        REGS_OLAT    = 0x14, // Output latch registers
    };

    // Registers (for single register access)
    enum class Reg_e: uint8_t
    {
        REG_IODIRA = 0x00,
        REG_IODIRB,
        REG_IPOLA,
        REG_IPOLB,
        REG_GPINTENA,
        REG_GPINTENB,
        REG_DEFVALA,
        REG_DEFVALB,
        REG_INTCONA,
        REG_INTCONB,
        REG_ICONA,
        REG_ICONB,
        REG_GPPUA,
        REG_GPPUB,
        REG_INTFA,
        REG_INTFB,
        REG_INTCAPA,
        REG_INTCAPB,
        REG_GPIOA,
        REG_GPIOB,
        REG_OLATA,
        REG_OLATB
    };

    Reg_e operator+(const RegPair_e& rp, const Port_e& p);

    // I2C address (7 bits) : 00100nnn with the 3 nnn bits being hardware dependants
    inline constexpr uint16_t MCP23017_I2C_base_address = 0x20;

    class MCP23017{
        I2CMaster::I2CDevice m_device;
        std::map<RegPair_e, std::vector<uint8_t>> m_config;
        int m_timeout_ms;
        Status_e m_status;

    public:
        MCP23017(
            I2CMaster::I2CBus& master_bus,
            SubAddress_e device_sub_address, // 0..7 hardware configuration
            uint32_t scl_speed_hz = 100000UL,
            int timeout_ms=-1);

        MCP23017(const MCP23017&) = delete;
        MCP23017& operator=(const MCP23017&) = delete;

        ~MCP23017(){};

        // general single register read/write
        auto read_register(const Reg_e reg) -> uint8_t;
        void write_register(const Reg_e reg, const uint8_t value);
        // general register pair read/write
        auto read_registers(const RegPair_e regs) -> std::vector<uint8_t>;
        void read_registers_into(const RegPair_e regs, std::vector<uint8_t>&values);
        void write_registers(const RegPair_e regs, const uint8_t value_port_a, const uint8_t value_port_b);

        // Ports state
        auto read_port(const Port_e port) -> uint8_t;
        auto read_ports(void) -> std::vector<uint8_t>;

        // Device configuration
        // Ports direction
        void set_port_direction(const Port_e port, const uint8_t direction);
        void set_ports_direction(const uint8_t direction_port_a, const uint8_t direction_port_b);
        // Ports polarity
        void set_port_polarity(const Port_e port, const uint8_t polarity);
        void set_ports_polarity(const uint8_t polarity_port_a, const uint8_t polarity_port_b);
        // Ports pullups
        void set_port_pullups(const Port_e port, const uint8_t pullups);
        void set_ports_pullups(const uint8_t pullups_port_a, const uint8_t pullups_port_b);

        void read_config(void);
        void set_config(
            const uint8_t direction_port_a, const uint8_t direction_port_b,
            const uint8_t polarity_port_a, const uint8_t polarity_port_b,
            const uint8_t pullups_port_a, const uint8_t pullups_port_b);
        void write_config(void);

        auto get_status(void) -> Status_e{return m_status;}
        void check_status(void);
    };

} // namespace
