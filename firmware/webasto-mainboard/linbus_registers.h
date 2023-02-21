#ifndef __linbus_registers_h_
#define __linbus_registers_h_

#include <Arduino.h>

typedef enum {
  BOARD_TYPE_VALVE_CONTROL = 0,
  BOARD_TYPE_COOLANT_TEMP,
  BOARD_TYPE_FLOW_SENSOR,
  BOARD_TYPE_FAN_CONTROL,
  BOARD_TYPE_PUMP_CONTROL,
  BOARD_TYPE_PELTIER_CONTROL,
} board_types_t;


class LINBusRegister {
  public:
    LINBusRegister(uint8_t ident, uint8_t location) : _ident(ident), _location(location), _last_millis(0), _read_addr(0) {
      _last_values = new uint8_t[2];
    };
    
    ~LINBusRegister() {
      if (_last_values) {
        delete [] _last_values;
      }
    };

    void init(void);
    void update(void);
    void feedback(void);

    int32_t get_value(uint8_t index);

    uint8_t get8u(uint8_t index);
    uint16_t get16u(uint8_t index);
    int8_t get8s(uint8_t index);
    int16_t get16s(uint8_t index);

    void write_register(uint8_t index, uint8_t value);

  protected:
    virtual const uint8_t _max_register(void) { return 2; };
    void reset_read_addr(uint8_t new_addr);
    uint16_t read_register_pair(uint8_t addr);

    uint8_t _ident;
    uint8_t _location;
    int _last_millis;
    uint8_t _read_addr;
    uint8_t *_last_values;

    static const uint16_t width_8u  = BIT(1) | BIT(0);
    static const uint16_t width_16u = 0;
    static const uint16_t width_8s  = 0;
    static const uint16_t width_16s = 0;
};


class LINBusRegisterValveControl : public LINBusRegister {
  public:
    LINBusRegisterValveControl(uint8_t ident, uint8_t location) : LINBusRegister(ident, location) {};
    enum Index {
      REG_BOARD_TYPE = 0,
      REG_LOCATION,
      REG_VALVE_CONTROL,
      REG_VALVE_STATUS,
      REG_VALVE_CURRENT_HI,
      REG_VALVE_CURRENT_LO,
      MAX_REGISTERS,
    };

    static const uint16_t width_8u  = BIT(3) | BIT(2) | BIT(1) | BIT(0);
    static const uint16_t width_16u = BIT(4);
    static const uint16_t width_8s  = 0;
    static const uint16_t width_16s = 0;

    const uint8_t _max_register(void) { return MAX_REGISTERS; };
};


class LINBusRegisterCoolantTemperature : public LINBusRegister {
  public:
    LINBusRegisterCoolantTemperature(uint8_t ident, uint8_t location) : LINBusRegister(ident, location) {};
    enum Index {
      REG_BOARD_TYPE = 0,
      REG_LOCATION,
      REG_COOLANT_TEMP0_HI,
      REG_COOLANT_TEMP0_LO,
      REG_COOLANT_TEMP1_HI,
      REG_COOLANT_TEMP1_LO,
      MAX_REGISTERS,
    };

    static const uint16_t width_8u  = BIT(1) | BIT(0);
    static const uint16_t width_16u = 0;
    static const uint16_t width_8s  = 0;
    static const uint16_t width_16s = BIT(4) | BIT(2);

    const uint8_t _max_register(void) { return MAX_REGISTERS; };
};


class LINBusRegisterFlowSensor : public LINBusRegister {
  public:
    LINBusRegisterFlowSensor(uint8_t ident, uint8_t location) : LINBusRegister(ident, location) {};
    enum Index {
      REG_BOARD_TYPE = 0,
      REG_LOCATION,
      REG_FLOW_RATE0_HI,
      REG_FLOW_RATE0_LO,
      REG_FLOW_RATE1_HI,
      REG_FLOW_RATE1_LO,
      MAX_REGISTERS,
    };

    static const uint16_t width_8u  = BIT(1) | BIT(0);
    static const uint16_t width_16u = BIT(4) | BIT(2);
    static const uint16_t width_8s  = 0;
    static const uint16_t width_16s = 0;

    const uint8_t _max_register(void) { return MAX_REGISTERS; };
};


class LINBusRegisterFanControl : public LINBusRegister {
  public:
    LINBusRegisterFanControl(uint8_t ident, uint8_t location) : LINBusRegister(ident, location) {};
    enum Index {
      REG_BOARD_TYPE = 0,
      REG_LOCATION,
      REG_FAN_CONTROL,
      REG_FAN_RPM_HI,
      REG_FAN_RPM_LO,
      REG_INTERNAL_TEMP,
      REG_EXTERNAL_TEMP_HI,
      REG_EXTERNAL_TEMP_LO,
      REG_ALERT_STATUS,
      REG_ALERT_MASK,
      REG_POR_STATUS,
      MAX_REGISTERS,
    };

    static const uint16_t width_8u  = BIT(10) | BIT(9) | BIT(8) | BIT(2) | BIT(1) | BIT(0);
    static const uint16_t width_16u = BIT(3);
    static const uint16_t width_8s  = BIT(5);
    static const uint16_t width_16s = BIT(6);

    const uint8_t _max_register(void) { return MAX_REGISTERS; };
};


class LINBusRegisterPumpControl : public LINBusRegister {
  public:
    LINBusRegisterPumpControl(uint8_t ident, uint8_t location) : LINBusRegister(ident, location) {};
    enum Index {
      REG_BOARD_TYPE = 0,
      REG_LOCATION,
      REG_PUMP_CONTROL,
      MAX_REGISTERS,
    };

    static const uint16_t width_8u  = BIT(2) | BIT(1) | BIT(0);
    static const uint16_t width_16u = 0;
    static const uint16_t width_8s  = 0;
    static const uint16_t width_16s = 0;
    const uint8_t _max_register(void) { return MAX_REGISTERS; };
};


class LINBusRegisterPeltierControl : public LINBusRegister {
  public:
    LINBusRegisterPeltierControl(uint8_t ident, uint8_t location) : LINBusRegister(ident, location) {};
    enum Index {
      REG_BOARD_TYPE = 0,
      REG_LOCATION,
      REG_PELTIER_CONTROL,
      MAX_REGISTERS,
    };

    static const uint16_t width_8u  = BIT(2) | BIT(1) | BIT(0);
    static const uint16_t width_16u = 0;
    static const uint16_t width_8s  = 0;
    static const uint16_t width_16s = 0;
    const uint8_t _max_register(void) { return MAX_REGISTERS; };
};


extern uint8_t addr_linbus_bridge;

#endif
