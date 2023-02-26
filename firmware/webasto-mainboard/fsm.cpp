#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>

#include "fsm.h"
#include "fsm_state.h"
#include "fsm_events.h"
#include "analog.h"
#include "fuel_pump.h"
#include "global_timer.h"
#include "webasto.h"
#include "fram.h"
#include "beeper.h"
#include "canbus.h"
#include "sensor_registry.h"

uint8_t fsm_state = 0x00;
int fsm_mode = 0;
bool batteryLow = false;
bool vsysLow = false;
bool supplementalEnabled = false;
bool priming = false;
bool lockdown;
bool ignitionOn;
bool startRunSignalOn;

bool circulationPumpOn = false;

int combustionFanPercent = 0;
int vehicleFanPercent = 0;
int glowPlugPercent = 0;
float fuelNeedRequested = 0.0;

bool glowPlugInEnable = false;   // mutually exclusive with glowPlugOutEnable
bool glowPlugOutEnable = false;  // mutually exclusive with glowPlugInEnable

bool flameLed = false;
bool operatingLed = false;

time_sensor_t ventilation_duration = {0};

int flameOutCount = 0;
int exhaustTempPreBurn = 0;
int exhaustTempStable = 0;

bool fsm_init = false;
mutex_t fsm_mutex;

void set_open_drain_pin(int pinNum, int value)
{
  // open drain with external pullup, asserted low (negative logic)
  if (value) {
    pinMode(pinNum, INPUT);
    digitalWrite(pinNum, LOW);  // shutoff builtin pullup
  } else {
    pinMode(pinNum, OUTPUT);
    digitalWrite(pinNum, LOW);
  }
}

void WebastoControlFSM::react(GlowPlugInEnableEvent const &e)
{
  Log.notice("Received GlowPlugInEnableEvent: %d", e.enable);
  CoreMutex m(&fsm_mutex);

  LedChangeEvent e0;
  e0.operatingChange = false;
  e0.flameChange = true;
  e0.enable = e.enable;
  dispatch(e0);

  glowPlugInEnable = e.enable;
  if (glowPlugInEnable && glowPlugOutEnable) {
    glowPlugOutEnable = false;
    set_open_drain_pin(PIN_GLOW_PLUG_OUT_EN, glowPlugOutEnable);
  }
  set_open_drain_pin(PIN_GLOW_PLUG_IN_EN, glowPlugInEnable);
}

void WebastoControlFSM::react(GlowPlugOutEnableEvent const &e)
{
  Log.notice("Received GlowPlugInEnableEvent: %d", e.enable);
  CoreMutex m(&fsm_mutex);

  LedChangeEvent e0;
  e0.operatingChange = false;
  e0.flameChange = true;
  e0.enable = false;
  dispatch(e0);

  glowPlugOutEnable = e.enable;
  if (glowPlugOutEnable && glowPlugInEnable) {
    glowPlugInEnable = false;
    set_open_drain_pin(PIN_GLOW_PLUG_IN_EN, glowPlugInEnable);
  }
  set_open_drain_pin(PIN_GLOW_PLUG_OUT_EN, glowPlugOutEnable);
}

void WebastoControlFSM::react(LedChangeEvent const &e)
{
  Log.notice("Received LedChangeEvent: Operating: %d, Flame: %d, Enable: %d", e.operatingChange, e.flameChange, e.enable);
  CoreMutex m(&fsm_mutex);

  int pin;
  bool *store;

  if (e.operatingChange) {
    pin = PIN_OPERATING_LED;
    store = &operatingLed;
  } else if (e.flameChange) {
    pin = PIN_FLAME_LED;
    store = &flameLed;
  } else {
    return;
  }

  *store = e.enable;
  tca9534.digitalWrite(pin, e.enable);
}

void WebastoControlFSM::react(CirculationPumpEvent const &e)
{
  Log.notice("Received CirculationPumpEvent: %d", e.enable);
  CoreMutex m(&fsm_mutex);

  circulationPumpOn = e.enable;
  digitalWrite(PIN_CIRCULATION_PUMP, circulationPumpOn);
}

void WebastoControlFSM::react(CombustionFanEvent const &e)
{
  Log.notice("Received CombustionFanEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  combustionFanPercent = clamp<int>(e.value, 0, 100);
  analogWrite(PIN_COMBUSTION_FAN, combustionFanPercent * 255 / 100);
}

void WebastoControlFSM::react(GlowPlugOutEvent const &e)
{
  Log.notice("Received GlowPlugOutEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  if (e.value && !glowPlugOutEnable) {
    return;
  }

  glowPlugPercent = clamp<int>(e.value, 0, 100);
  analogWrite(PIN_GLOW_PLUG_OUT, glowPlugPercent * 255 / 100);
}

void WebastoControlFSM::react(VehicleFanEvent const &e)
{
  Log.notice("Received VehicleFanEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  vehicleFanPercent = clamp<int>(e.value, 0, 100);
  uint8_t value = (uint8_t)vehicleFanPercent;
  canbus_output_value<uint8_t>(CANBUS_ID_VEHICLE_FAN, value);
}

void WebastoControlFSM::react(FuelPumpEvent const &e)
{
  Log.notice("Received FuelPumpEvent: %f", e.value);
  CoreMutex m(&fsm_mutex);

  if (e.value == 0.0) {
    fuelNeedRequested = 0.0;
  } else if (priming) {
    fuelNeedRequested = clamp<int>(e.value, MIN_FUEL_NEED, MAX_FUEL_NEED_PRIMING);
  } else {
    fuelNeedRequested = clamp<int>(e.value, MIN_FUEL_NEED, MAX_FUEL_NEED_BURNING);
  }
  fuelPumpTimer.setFuelNeed(fuelNeedRequested);
}

void WebastoControlFSM::react(TimerEvent const &e)
{
  fsmCommonReact(e);
}

void WebastoControlFSM::react(FlameDetectEvent const &e)
{
  // We only care in one state, so we will override specifically there, and gobble em up otherwise
}

void WebastoControlFSM::react(EmergencyStopEvent const &e)
{
  Log.notice("Received EmergencyStopEvent: %d", e.enable);

  if (!e.enable) {
    return;
  }

  CoreMutex m(&fsm_mutex);

  // TODO: what if we missed the edge?
  if (lockdown && !fsm_mode) {
    LockdownEvent event;
    event.enable = false;
    dispatch(event);
  } else {
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = true;
    event.lockdown = true;
    dispatch(event);
  }
}

void WebastoControlFSM::react(CoolantTempEvent const &e)
{
  Log.notice("Received CoolantTempEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  if (e.value > COOLANT_MAX_THRESHOLD) {
    fram_add_error(0x06);

    // OVERHEAT!  let it cool back down
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = false;
    event.lockdown = false;
    dispatch(event);
  } else if (e.value > COOLANT_MIN_THRESHOLD && fsm_mode) {
    VehicleFanEvent event;
    event.value = map<int>(e.value, COOLANT_MIN_THRESHOLD, COOLANT_MAX_THRESHOLD, 10, 100);
    dispatch(event);
  }
}

void WebastoControlFSM::react(OutdoorTempEvent const &e)
{
  Log.notice("Received OutdoorTempEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  if (e.value >= SUPPLEMENTAL_MIN_TEMP && e.value <= SUPPLEMENTAL_MAX_TEMP) {
    supplementalEnabled = true;
    if (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER) {
      StartupEvent event;
      event.mode = fsm_mode;
      dispatch(event);
    }
  } else {
    supplementalEnabled = false;
    if (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER) {
      ShutdownEvent event;
      event.mode = fsm_mode;
      event.emergency = false;
      event.lockdown = false;
      dispatch(event);
    }
  }
}

void WebastoControlFSM::react(ExhaustTempEvent const &e)
{
  Log.notice("Received ExhaustTempEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  if (e.value > EXHAUST_MAX_TEMP && fsm_mode) {
    fram_add_error(0x06);

    // OVERHEAT!  let it cool back down
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = false;
    event.lockdown = false;
    dispatch(event);
  }
}

void WebastoControlFSM::react(InternalTempEvent const &e)
{
  Log.notice("Received InternalTempEvent: %d", e.value);

  CoreMutex m(&fsm_mutex);

  if (e.value > INTERNAL_MAX_TEMP && fsm_mode) {
    // OVERHEAT!  let it cool back down
    fram_add_error(0x06);

    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = false;
    event.lockdown = false;
    dispatch(event);
  }
}

void WebastoControlFSM::react(BatteryLevelEvent const &e)
{
  Log.notice("Received BatteryLevelEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  if (batteryLow) {
    if (e.value > BATTERY_LOW_THRESHOLD) {
      batteryLow = false;
      transit<IdleState>();
    }
    return;
  }

  if (e.value < BATTERY_LOW_THRESHOLD) {
    batteryLow = true;

    fram_add_error(0x84);

    if (fsm_mode) {
      ShutdownEvent event;
      event.mode = fsm_mode;
      event.emergency = false;
      event.lockdown = false;
      dispatch(event);
    }
  }
}

void WebastoControlFSM::react(VSYSLevelEvent const &e)
{
  Log.notice("Received VSYSLevelEvent: %d", e.value);
  CoreMutex m(&fsm_mutex);

  if (vsysLow) {
    if (e.value > VSYS_LOW_THRESHOLD) {
      vsysLow = false;
      transit<IdleState>();
    }
    return;
  }

  if (e.value < VSYS_LOW_THRESHOLD) {
    vsysLow = true;

    fram_add_error(0x84);

    if (fsm_mode) {
      ShutdownEvent event;
      event.mode = fsm_mode;
      event.emergency = false;
      event.lockdown = false;
      dispatch(event);
    }
  }
}


void WebastoControlFSM::react(FlameoutEvent const &e)
{
  Log.notice("Received FlameoutEvent: reset: %d", e.resetCount);
  CoreMutex m(&fsm_mutex);

  if (e.resetCount) {
    flameOutCount = 0;
  }

  // Make sure to shut off the flame sensor
  GlowPlugInEnableEvent e1;
  e1.enable = false;
  dispatch(e1);

  fram_add_error(0x83);

  if (++flameOutCount > MAX_FLAMEOUT_COUNT) {
    fram_add_error(0x02);
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = false;
    event.lockdown = true;
    dispatch(event);
  } else {
    RestartEvent event;
    dispatch(event);
  }
}

void WebastoControlFSM::react(RestartEvent const &e)
{
  Log.notice("Received RestartEvent");
  transit<PurgingState>();
}

void WebastoControlFSM::react(OverheatEvent const &e)
{
  Log.notice("Received OverheatEvent");
  CoreMutex m(&fsm_mutex);

  fram_add_error(0x06);

  ShutdownEvent event;
  event.mode = fsm_mode;
  event.emergency = false;
  event.lockdown = false;
  dispatch(event);
}



void WebastoControlFSM::react(ShutdownEvent const &e)
{
  Log.notice("Received ShutdownEvent: mode %d, emergency:%d, lockdown:%d", e.mode, e.emergency, e.lockdown);
  CoreMutex m(&fsm_mutex);

  int new_mode;
  int old_mode = fsm_mode;

  if (e.lockdown) {
    LockdownEvent event;
    event.enable = true;
    dispatch(event);
  }

  if (e.emergency) {
    transit<EmergencyOffState>();
    return;
  }

  if (!e.mode || !fsm_mode) {
    return;
  }

  if (new_mode == WEBASTO_MODE_DEFAULT) {
    if (ignitionOn){
      new_mode = WEBASTO_MODE_SUPPLEMENTAL_HEATER;
    } else{
      new_mode = WEBASTO_MODE_PARKING_HEATER;
    }
  }

  switch(new_mode) {
    case WEBASTO_MODE_PARKING_HEATER:
    case WEBASTO_MODE_SUPPLEMENTAL_HEATER:
      if (new_mode == fsm_mode) {
        transit<CooldownState>();
      } else {
        Log.error("Can't shutdown the wrong mode!  %d != %d", e.mode, fsm_mode);
        fram_add_error(0x11);
      }
      break;
    case WEBASTO_MODE_VENTILATION:
      {
        CombustionFanEvent event;
        event.value = 0;
        dispatch(event);
      }
      break;
    case WEBASTO_MODE_CIRCULATION_PUMP:
      {
        CirculationPumpEvent event;
        event.enable = false;
        dispatch(event);
      }
      break;
    default:
      Log.error("Received an unsupported mode: %d", e.mode);
      fsm_mode = old_mode;
      break;
  }
}

void WebastoControlFSM::react(AddTimeEvent const &e)
{
  Log.notice("Received AddTimeEvent: mode %d, %d minutes", e.mode, e.minutes);
  CoreMutex m(&fsm_mutex);
  int new_mode = e.mode;
  int minutes = e.minutes;

  if (new_mode == WEBASTO_MODE_DEFAULT) {
    if (ignitionOn){
      new_mode = WEBASTO_MODE_SUPPLEMENTAL_HEATER;
    } else{
      new_mode = WEBASTO_MODE_PARKING_HEATER;
    }
  }

  if (minutes) {
    globalTimer.adjust_timer(TIMER_TIMED_SHUT_DOWN, minutes * 60000);
  }
}

void WebastoControlFSM::react(IgnitionEvent const &e)
{
  Log.notice("Received IgnitionEvent: %d", e.enable);

  CoreMutex m(&fsm_mutex);

  ignitionOn = e.enable;
  if (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER) {
    if (ignitionOn) {
      StartupEvent event;
      event.mode = fsm_mode;
      dispatch(event);
    } else {
      ShutdownEvent event;
      event.mode = fsm_mode;
      event.emergency = false;
      event.lockdown = false;
      dispatch(event);
    }
  }
}

void WebastoControlFSM::react(StartRunEvent const &e)
{
  Log.notice("Received StartRunEvent: %d", e.enable);

  CoreMutex m(&fsm_mutex);
  startRunSignalOn = e.enable;

  if (startRunSignalOn) {
    StartupEvent event;
    event.mode = WEBASTO_MODE_DEFAULT;
    dispatch(event);
  } else if (fsm_mode) {
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.lockdown = false;
    event.emergency = false;
    dispatch(event);
  }
}

void WebastoControlFSM::react(LockdownEvent const &e)
{
  Log.notice("Received LockdownEvent: %d", e.enable);

  if (!e.enable) {
    return;
  }

  CoreMutex m(&fsm_mutex);
  lockdown |= e.enable;

  CoreMutex m0(&fram_mutex);
  fram_add_error(0x07);
  fram_data.current.lockdown = lockdown;
  fram_dirty = true;

  transit<LockdownState>();
}


void WebastoControlFSM::react(StartupEvent const &e)
{
  Log.notice("Received StartupEvent: mode %d, lockdown: %d", e.mode, lockdown);
  CoreMutex m(&fsm_mutex);

  int new_mode = e.mode;
  int old_mode = fsm_mode;
  int minutes = e.minutes;

  if (batteryLow) {
    Log.warning("Will not start:  battery too low");
    return;
  }

  if (lockdown) {
    Log.warning("Will not startup without clearing lockdown (using EmergencyStop while not running)");
    return;
  }

  if (new_mode == WEBASTO_MODE_DEFAULT) {
    if (ignitionOn){
      new_mode = WEBASTO_MODE_SUPPLEMENTAL_HEATER;
    } else{
      new_mode = WEBASTO_MODE_PARKING_HEATER;
    }
  }

  fsm_mode = new_mode;

  LedChangeEvent e0;
  e0.operatingChange = true;
  e0.flameChange = false;
  e0.enable = true;
  dispatch(e0);

  if (minutes) {
    globalTimer.register_timer(TIMER_TIMED_SHUT_DOWN, minutes * 60000, &fsmTimerCallback);
  }

  switch(fsm_mode) {
    case WEBASTO_MODE_PARKING_HEATER:
      transit<PurgingState>();
      break;
    case WEBASTO_MODE_SUPPLEMENTAL_HEATER:
      if (ignitionOn && supplementalEnabled) {
        transit<PurgingState>();
      }
      break;
    case WEBASTO_MODE_VENTILATION:
      {
        CombustionFanEvent event;
        event.value = 100;
        dispatch(event);
      }
      break;
    case WEBASTO_MODE_CIRCULATION_PUMP:
      {
        CirculationPumpEvent event;
        event.enable = true;
        dispatch(event);
      }
      break;
    default:
      Log.error("Received an unsupported mode: %d", e.mode);
      fsm_mode = old_mode;
      break;
  }
}

void StartupState::entry()
{
  // Do nothing, we will prime this with a timer event.
}

void StartupState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_FSM_STARTUP:
      Log.notice("StartupState -> IdleState");
      transit<IdleState>();
      break;
    default:
      break;
  }
}


void IdleState::entry()
{
  Log.notice("Entering IdleState");

  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;

  LedChangeEvent e0;
  e0.operatingChange = true;
  e0.flameChange = false;
  e0.enable = false;
  dispatch(e0);

  fsm_mode = 0;
  batteryLow = false;
  ignitionOn = ignitionSenseSensor->get_value();

  // Make sure the vehicle fan is off
  VehicleFanEvent event;
  event.value = 0;
  dispatch(event);

  if (lockdown) {
    transit<LockdownState>();
  }
}

void PurgingState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  int exhaustTemp = exhaustTempSensor->get_value();

  if (exhaustTemp > EXHAUST_PURGE_THRESHOLD) {
    // Turn on Combustion Fan at 80%
    CombustionFanEvent e1;
    e1.value = PURGE_FAN;
    dispatch(e1);

    // Stay in this state for 3s
    globalTimer.register_timer(TIMER_STAGE_COMPLETE, 5000, &fsmTimerCallback);
  } else {
    transit<StandbyState>();
  }
}

void PurgingState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<StandbyState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}


void StandbyState::entry()
{

  kickRunTimer();

  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  exhaustTempPreBurn = exhaustTempSensor->get_value();

  // Turn on the combustion fan - 70%
  CombustionFanEvent e1;
  e1.value = 70;
  dispatch(e1);

  // Turn on the circulation pump
  CirculationPumpEvent e2;
  e2.enable = true;
  dispatch(e2);

  // Set the glow plug to 100%
  GlowPlugOutEvent e3;
  e3.value = 100;
  dispatch(e3);

  // Turn ON the glow plug out.
  GlowPlugOutEnableEvent e4;
  e4.enable = true;
  dispatch(e4);

  // Stay in this state for 30s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 30000, &fsmTimerCallback);

  CoreMutex m1(&fram_mutex);

  if (fsm_mode == WEBASTO_MODE_PARKING_HEATER) {
    fram_data.current.start_counter_parking_heater++;
    fram_data.current.total_start_counter++;
    fram_dirty = true;
  } else if (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER) {
    fram_data.current.start_counter_supplemental_heater++;
    fram_data.current.total_start_counter++;
    fram_dirty = true;
  }
}


void StandbyState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<PrefuelState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}

void PrefuelState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  priming = true;

  // Turn on Combustion Fan at 15%
  CombustionFanEvent e1;
  e1.value = 15;
  dispatch(e1);

  // Turn on Fuel Pump to prime
  FuelPumpEvent e2;
  int exhaustTemp = exhaustTempSensor->get_value();
  e2.value = map<double>(exhaustTemp, PRIMING_LOW_THRESHOLD, PRIMING_HIGH_THRESHOLD, 3.5, 2.0);
  dispatch(e2);

  // Stay in this state for 3s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 3000, &fsmTimerCallback);
}

void PrefuelState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<FuelOffState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}


void FuelOffState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  priming = false;

  // Turn off Fuel Pump
  FuelPumpEvent event;
  event.value = 0.0;
  dispatch(event);

  // Stay in this stage for 54s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 54000, &fsmTimerCallback);
}

void FuelOffState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<StabilizationState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}

void StabilizationState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  exhaustTempStable = exhaustTempSensor->get_value();

  // Shutdown the glow plug
  GlowPlugOutEnableEvent e1;
  e1.enable = false;
  dispatch(e1);

  GlowPlugOutEvent e2;
  e2.value = 0;
  dispatch(e2);

  // Leave Combustion Fan, Fuel Pump and Circulation Pump alone!
  // Stay in this stage for 15s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 2000, &fsmTimerCallback);
}

void StabilizationState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<TestBurnState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}

void TestBurnState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  FuelPumpEvent e1;
  e1.value = START_FUEL(exhaustTempStable);
  dispatch(e1);

  CombustionFanEvent e2;
  if (combustionFanPercent < START_FAN) {
    e2.value = combustionFanPercent + 1;
    globalTimer.register_timer(TIMER_FUEL_FAN_DELTA, 333, &fsmTimerCallback);
  } else {
    e2.value = START_FAN;
  }
  dispatch(e2);

  // Stay in this state for 15s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 15000, &fsmTimerCallback);
}

void TestBurnState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_FUEL_FAN_DELTA:
      {
        CoreMutex m(&fsm_mutex);

        CombustionFanEvent event;
        if (combustionFanPercent < START_FAN) {
          event.value = combustionFanPercent + 1;
          globalTimer.register_timer(TIMER_FUEL_FAN_DELTA, 333, &fsmTimerCallback);
        } else {
          event.value = START_FAN;
        }
        dispatch(event);
      }
      break;
    case TIMER_STAGE_COMPLETE:
      transit<FlameMeasureState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}

void FlameMeasureState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;

  // Turn on the Flame Sensor
  GlowPlugInEnableEvent e1;
  e1.enable = true;
  dispatch(e1);

  // Stay in this state for 20s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 20000, &fsmTimerCallback);
}

void FlameMeasureState::react(FlameDetectEvent const &e)
{
  CoreMutex m(&fsm_mutex);

  if (e.value < FLAME_DETECT_THRESHOLD) {
    // OK, this means our flame must be out, there's not enough heat here
    FlameoutEvent e1;
    e1.resetCount = true;
    dispatch(e1);
  }
}

void FlameMeasureState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      {
        CoreMutex m(&fsm_mutex);

        int exhaustTemp = exhaustTempSensor->get_value();

        LocalSensor<uint16_t> *flameDetectorSensor = static_cast<LocalSensor<uint16_t> *>(sensorRegistry.get(CANBUS_ID_FLAME_DETECTOR));
        int flameSensor = flameDetectorSensor->get_value();

        if (exhaustTemp - exhaustTempStable >= EXHAUST_TEMP_RISE || flameSensor > FLAME_DETECT_THRESHOLD) {
          transit<AutoBurnState>();
        } else {
          globalTimer.register_timer(TIMER_STAGE_RETRY, 20000, &fsmTimerCallback);
        }
      }
      break;
    case TIMER_STAGE_RETRY:
      {
        CoreMutex m(&fsm_mutex);

        int exhaustTemp = exhaustTempSensor->get_value();

        LocalSensor<uint16_t> *flameDetectorSensor = static_cast<LocalSensor<uint16_t> *>(sensorRegistry.get(CANBUS_ID_FLAME_DETECTOR));
        int flameSensor = flameDetectorSensor->get_value();

        if (exhaustTemp - exhaustTempStable >= EXHAUST_TEMP_RISE || flameSensor > FLAME_DETECT_THRESHOLD) {
          transit<AutoBurnState>();
        } else {
          // Need a restart over here!
          FlameoutEvent event;
          event.resetCount = false;
          dispatch(event);
        }
      }
    default:
      fsmCommonReact(e);
      break;
  }
}

void AutoBurnState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;

  // Turn off the Flame Sensor
  GlowPlugInEnableEvent e1;
  e1.enable = false;
  dispatch(e1);

  // Stay in this state for 15s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 15000, &fsmTimerCallback);
  globalTimer.register_timer(TIMER_FUEL_FAN_DELTA, 500, &fsmTimerCallback);
}

void AutoBurnState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_FUEL_FAN_DELTA:
      {
        CoreMutex m(&fsm_mutex);

        int coolantTemp = coolantTempSensor->get_value();
        int externalTemp = externalTempSensor->get_value();
        int exhaustTemp = exhaustTempSensor->get_value();

        int fanRequest = combustionFanPercent;
        double fuelRequest = fuelNeedRequested;

        if (coolantTemp <= COOLANT_COLD_THRESHOLD) {  // 40C
          if (fanRequest < THROTTLE_HIGH_FAN) {
            fanRequest++;
          }

          if (fuelRequest < THROTTLE_HIGH_FUEL(externalTemp)) {
            fuelRequest++;
          }
        } else if (coolantTemp >= COOLANT_IDLE_THRESHOLD || exhaustTemp > EXHAUST_IDLE_THRESHOLD) {
          fanRequest = THROTTLE_IDLE_FAN;
          fuelRequest = THROTTLE_IDLE_FUEL;
        } else {
          int fanTarget;
          double fuelTarget;

          if (coolantTemp < COOLANT_MIN_THRESHOLD) {
            fuelTarget = THROTTLE_HIGH_FUEL(externalTemp);
            fanTarget = THROTTLE_HIGH_FAN;
          }

          if (coolantTemp >= COOLANT_MIN_THRESHOLD && coolantTemp < COOLANT_TARGET_TEMP) {
            fuelTarget = THROTTLE_STEADY_FUEL;
            fanTarget = THROTTLE_STEADY_FAN;
          }

          if (coolantTemp >= COOLANT_TARGET_TEMP && coolantTemp < COOLANT_IDLE_THRESHOLD) {
            fuelTarget = THROTTLE_LOW_FUEL;
            fanTarget = THROTTLE_LOW_FAN;
          }

          if (fuelRequest < fuelTarget) {
            fuelRequest += 0.01;
          } else if (fuelRequest > fuelTarget) {
            fuelRequest -= 0.01;
          }

          if (fanRequest < fanTarget) {
            fanRequest += 1;
          } else if (fanRequest > fanTarget) {
            fanRequest -= 1;
          }
        }

        CombustionFanEvent e1;
        e1.value = fanRequest;
        dispatch(e1);

        FuelPumpEvent e2;
        e2.value = fuelRequest;
        dispatch(e2);

        globalTimer.register_timer(TIMER_FUEL_FAN_DELTA, 500, &fsmTimerCallback);
      }
      break;
    case TIMER_STAGE_COMPLETE:
      {
        int coolantTemp = coolantTempSensor->get_value();
        int exhaustTemp = exhaustTempSensor->get_value();

        if (exhaustTemp > 4000 && coolantTemp > exhaustTemp) {
          FlameoutEvent event;
          event.resetCount = true;
          dispatch(event);
        } else {
          transit<FlameMeasureState>();
        }
      }
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}


void CooldownState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  int currentPower = fuelPumpTimer.getBurnPower();

  // Shut off the fuel pump
  FuelPumpEvent e1;
  e1.value = 0;
  dispatch(e1);

  // Turn on the combustion fan at 100%
  CombustionFanEvent e2;
  e2.value = 100;
  dispatch(e2);

  // Ensure the circulation pump is off
  CirculationPumpEvent e3;
  e3.enable = false;
  dispatch(e3);

  // Make sure to shut off the glow plug
  GlowPlugOutEnableEvent e4;
  e4.enable = false;
  dispatch(e4);

  // Make sure to shut off the flame sensor
  GlowPlugInEnableEvent e5;
  e5.enable = false;
  dispatch(e5);

  // Clear out the ventilation duration
  ventilation_duration.hours = 0;
  ventilation_duration.minutes = 0;

  // Stay in this state for 100s if at partial power, 175s at full power (gonna prorate)
  int timeout = map<int>(currentPower, MIN_POWER, MAX_RATED_POWER, 100000, 175000);
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, timeout, &fsmTimerCallback);
}

void CooldownState::react(TimerEvent const &e)
{
  CoreMutex m(&fsm_mutex);

  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<IdleState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}

void LockdownState::entry()
{
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;
  Log.warning("Entering lockdown mode.  Toggle EmergencyStop to clear");
  beeper.register_beeper(10, 500, 500);
  globalTimer.register_timer(TIMER_RESTART_BEEPS, 20000, &fsmTimerCallback);
}

void LockdownState::react(TimerEvent const &e)
{
  CoreMutex m(&fsm_mutex);

  switch (e.timerId) {
    case TIMER_RESTART_BEEPS:
      beeper.register_beeper(10, 500, 500);
      globalTimer.register_timer(TIMER_RESTART_BEEPS, 20000, &fsmTimerCallback);
      break;

    default:
      fsmCommonReact(e);
      break;
  }
}


void EmergencyOffState::entry()
{
  Log.notice("Entering EmergencyOffState");
  CoreMutex m(&fsm_mutex);

  fsm_state = _state_num;

  // Shut off the fuel pump
  FuelPumpEvent e1;
  e1.value = 0;
  dispatch(e1);

  // Turn off the combustion fan
  CombustionFanEvent e2;
  e2.value = 0;
  dispatch(e2);

  // Ensure the circulation pump is off
  CirculationPumpEvent e3;
  e3.enable = false;
  dispatch(e3);

  // Make sure to shut off the glow plug
  GlowPlugOutEnableEvent e4;
  e4.enable = false;
  dispatch(e4);

  // Make sure to shut off the flame sensor
  GlowPlugInEnableEvent e5;
  e5.enable = false;
  dispatch(e5);

  // Shut off the vehicle fan
  VehicleFanEvent e6;
  e6.value = 0;
  dispatch(e6);

  LockdownEvent e7;
  e7.enable = true;
  dispatch(e7);

  fram_add_error(0x5F);

  CoreMutex m1(&fram_mutex);

  fram_data.current.counter_emergency_shutdown++;
  fram_dirty = true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////


void fsmTimerCallback(int timer_id, int delay)
{
  Log.info("FSM callback: %d", timer_id);

  TimerEvent event;
  event.value = delay;
  event.timerId = timer_id;
  WebastoControlFSM::dispatch(event);
}

void kickRunTimer(void)
{
  globalTimer.register_timer(TIMER_RUN_TIME_MINUTE, 60000, &fsmTimerCallback);
}

void increment_minutes(time_sensor_t *time)
{
  if (!time) {
    return;
  }

  time->minutes++;
  while (time->minutes >= 60) {
    time->minutes -= 60;
    time->hours++;
  }
}

void fsmCommonReact(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_TIMED_SHUT_DOWN:
      {
        ShutdownEvent event;
        event.mode = fsm_mode;
        event.emergency = false;
        event.lockdown = false;
        WebastoControlFSM::dispatch(event);
      }
      break;
    case TIMER_RUN_TIME_MINUTE:
      {
        CoreMutex m(&fsm_mutex);
        if (!fsm_mode) {
          break;
        }

        CoreMutex m2(&fram_mutex);

        kickRunTimer();
        time_sensor_t *time;
        int currentPower = fuelPumpTimer.getBurnPower();
        int bin = currentPower * 3 / MAX_RATED_POWER;

        switch (fsm_mode) {
          case WEBASTO_MODE_PARKING_HEATER:
            if (currentPower) {
              time = &fram_data.current.burn_duration_parking_heater[bin];
              increment_minutes(time);
              time = &fram_data.current.total_burn_duration;
              increment_minutes(time);
            }
            time = &fram_data.current.working_duration_parking_heater;
            increment_minutes(time);
            time = &fram_data.current.total_working_duration;
            increment_minutes(time);
            fram_dirty = true;
            break;

          case WEBASTO_MODE_SUPPLEMENTAL_HEATER:
            if (currentPower) {
              time = &fram_data.current.burn_duration_supplemental_heater[bin];
              increment_minutes(time);
              time = &fram_data.current.total_burn_duration;
              increment_minutes(time);
            }
            time = &fram_data.current.working_duration_supplemental_heater;
            increment_minutes(time);
            time = &fram_data.current.total_working_duration;
            increment_minutes(time);
            fram_dirty = true;
            break;

          default:
            break;
        }

        if (!currentPower && combustionFanPercent) {
          time = &ventilation_duration;
          increment_minutes(time);
        }
      }
      break;

    default:
      break;
  }
}

void init_fsm(void)
{
  Log.notice("Starting FSM");
  mutex_init(&fsm_mutex);

  // Set output pins as outputs
  flameLed = false;
  tca9534.digitalWrite(PIN_FLAME_LED, flameLed);

  operatingLed = false;
  tca9534.digitalWrite(PIN_OPERATING_LED, operatingLed);

  WebastoControlFSM::start();

  // KLineEN handled in wbus.cpp

  // this is an open drain output.  default is off = input with pullup
  pinMode(PIN_GLOW_PLUG_IN_EN, INPUT);
  GlowPlugInEnableEvent e1;
  e1.enable = false;
  WebastoControlFSM::dispatch(e1);

  // this is an open drain output.  default is off = input with pullup
  pinMode(PIN_GLOW_PLUG_OUT_EN, INPUT);
  GlowPlugOutEnableEvent e2;
  e2.enable = false;
  WebastoControlFSM::dispatch(e2);

  pinMode(PIN_CIRCULATION_PUMP, OUTPUT);
  CirculationPumpEvent e3;
  e3.enable = false;
  WebastoControlFSM::dispatch(e3);

  pinMode(PIN_COMBUSTION_FAN, OUTPUT);
  CombustionFanEvent e4;
  e4.value = 0;
  WebastoControlFSM::dispatch(e4);

  pinMode(PIN_GLOW_PLUG_OUT, OUTPUT);
  GlowPlugOutEvent e5;
  e5.value = 0;
  WebastoControlFSM::dispatch(e5);

  // Setup is in the FuelPumpTimer class
  FuelPumpEvent e6;
  e6.value = 0.0;
  WebastoControlFSM::dispatch(e6);

  VehicleFanEvent e7;
  e7.value = 0;
  WebastoControlFSM::dispatch(e7);

  CoreMutex m(&fsm_mutex);
  //CoreMutex m(&fram_mutex);
  //lockdown = fram_data.current.lockdown;
  lockdown = false;

  fsm_init = true;
  globalTimer.register_timer(TIMER_FSM_STARTUP, 10, &fsmTimerCallback);
}

FSM_INITIAL_STATE(WebastoControlFSM, StartupState)
