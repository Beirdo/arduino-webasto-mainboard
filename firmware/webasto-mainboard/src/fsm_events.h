#ifndef __fsm_events_h_
#define __fsm_events_h_

#include "tinyfsm.hpp"

struct BooleanEvent       : tinyfsm::Event {
  bool enable;
};
struct IgnitionEvent            : BooleanEvent { };
struct StartRunEvent            : BooleanEvent { };
struct EmergencyStopEvent       : BooleanEvent { };
struct GlowPlugInEnableEvent    : BooleanEvent { };
struct GlowPlugOutEnableEvent   : BooleanEvent { };
struct CirculationPumpEvent     : BooleanEvent { };
struct LockdownEvent            : BooleanEvent { };
struct LedChangeEvent           : BooleanEvent {
  bool operatingChange;
  bool flameChange;
};

struct IntegerEvent       : tinyfsm::Event {
  int value;
};
struct CombustionFanEvent : IntegerEvent { };
struct GlowPlugOutEvent   : IntegerEvent { };
struct VehicleFanEvent    : IntegerEvent { };
struct CoolantTempEvent   : IntegerEvent { };
struct OutdoorTempEvent   : IntegerEvent { };
struct ExhaustTempEvent   : IntegerEvent { };
struct InternalTempEvent  : IntegerEvent { };
struct FlameDetectEvent   : IntegerEvent { };
struct BatteryLevelEvent  : IntegerEvent { };
struct VSYSLevelEvent     : IntegerEvent { };

struct TimerEvent         : IntegerEvent {
  int timerId;
};

struct DoubleEvent       : tinyfsm::Event {
  double value;
};
struct FuelPumpEvent      : DoubleEvent { };

struct FlameoutEvent      : tinyfsm::Event {
  bool resetCount;
};
struct RestartEvent       : tinyfsm::Event { };
struct OverheatEvent      : tinyfsm::Event { };

struct ModeChangeEvent    : tinyfsm::Event {
  int mode;
  int minutes;
};
struct ShutdownEvent      : ModeChangeEvent {
  bool emergency;
  bool lockdown;
};
struct StartupEvent       : ModeChangeEvent { };
struct AddTimeEvent       : ModeChangeEvent { };

#endif

