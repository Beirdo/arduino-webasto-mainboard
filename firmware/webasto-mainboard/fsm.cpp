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

int fsm_mode = 0;
bool batteryLow = false;
bool supplementalEnabled = false;
bool lockdown;
bool ignitionOn;
bool startRunSignalOn;

bool circulationPumpOn = false;

int combustionFanPercent = 0;
int vehicleFanPercent = 0;
int glowPlugPercent = 0;
bool fuelPumpOn = false;
int fuelPumpPeriodMs = 0;

bool glowPlugInEnable = false;   // mutually exclusive with glowPlugOutEnable
bool glowPlugOutEnable = false;  // mutually exclusive with glowPlugInEnable

bool flameLed = false;
bool operatingLed = false;

time_sensor_t ventilation_duration = {0};

int flameOutCount = 0;

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


void init_state_machine(void)
{
  mutex_init(&fsm_mutex);

  // Set input pins as inputs
  pinMode(PIN_START_RUN, INPUT);

  // Set output pins as outputs

  flameLed = false;
  pinMode(PIN_FLAME_LED, OUTPUT);
  digitalWrite(PIN_FLAME_LED, flameLed);

  operatingLed = false;
  pinMode(PIN_OPERATING_LED, OUTPUT);
  digitalWrite(PIN_OPERATING_LED, operatingLed);

  // KLineEN handled in kline.cpp

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
  e6.value = 0;
  WebastoControlFSM::dispatch(e6);

  pinMode(PIN_VEHICLE_FAN_RELAY, OUTPUT);
  VehicleFanEvent e7;
  e7.value = 0;
  WebastoControlFSM::dispatch(e7);

  CoreMutex m(&fsm_mutex);
  lockdown = fram_data.current.lockdown;
}

void WebastoControlFSM::react(GlowPlugInEnableEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  glowPlugInEnable = e.enable;
  if (glowPlugInEnable && glowPlugOutEnable) {
    glowPlugOutEnable = false;
    set_open_drain_pin(PIN_GLOW_PLUG_OUT_EN, glowPlugOutEnable);
  }
  set_open_drain_pin(PIN_GLOW_PLUG_IN_EN, glowPlugInEnable);
}

void WebastoControlFSM::react(GlowPlugOutEnableEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  glowPlugOutEnable = e.enable;
  if (glowPlugOutEnable && glowPlugInEnable) {
    glowPlugInEnable = false;
    set_open_drain_pin(PIN_GLOW_PLUG_IN_EN, glowPlugInEnable);
  }
  set_open_drain_pin(PIN_GLOW_PLUG_OUT_EN, glowPlugOutEnable);
}

void WebastoControlFSM::react(CirculationPumpEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  circulationPumpOn = e.enable;
  digitalWrite(PIN_CIRCULATION_PUMP, circulationPumpOn);
}

void WebastoControlFSM::react(CombustionFanEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  combustionFanPercent = clamp(e.value, 0, 100);
  analogWrite(PIN_COMBUSTION_FAN, combustionFanPercent * 255 / 100);
}

void WebastoControlFSM::react(GlowPlugOutEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  if (e.value && !glowPlugOutEnable) {
    return;
  } 
  
  glowPlugPercent = clamp(e.value, 0, 100);
  analogWrite(PIN_GLOW_PLUG_OUT, glowPlugPercent * 255 / 100);
}

void WebastoControlFSM::react(VehicleFanEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  vehicleFanPercent = clamp(e.value, 0, 100);
  analogWrite(PIN_VEHICLE_FAN_RELAY, vehicleFanPercent * 255 / 100);
}

void WebastoControlFSM::react(FuelPumpEvent const &e)
{
  int value = clamp(e.value, 0, MAX_RATED_POWER);
  fuelPumpTimer.setBurnPower(value);
}

void WebastoControlFSM::react(TimerEvent const &e)
{
  fsmCommonReact(e);
}

void WebastoControlFSM::react(FlameDetectEvent const &e)
{
  // We only care in two states, so we will override specifically there, and gobble em up otherwise
}

void WebastoControlFSM::react(EmergencyStopEvent const &e)
{
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
  CoreMutex m(&fsm_mutex);

  if (e.value > COOLANT_MAX_THRESHOLD) {
    // OVERHEAT!  let it cool back down
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = false;
    event.lockdown = false;
    dispatch(event);
  } else if (e.value > COOLANT_MIN_THRESHOLD && fsm_mode) {
    VehicleFanEvent event;
    event.value = map(e.value, COOLANT_MIN_THRESHOLD, COOLANT_MAX_THRESHOLD, 10, 100);
    dispatch(event);
  }
}

void WebastoControlFSM::react(OutdoorTempEvent const &e)
{
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
  CoreMutex m(&fsm_mutex);

  if (e.value > EXHAUST_MAX_TEMP && fsm_mode) {
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
  CoreMutex m(&fsm_mutex);

  if (e.value > INTERNAL_MAX_TEMP && fsm_mode) {
    // OVERHEAT!  let it cool back down
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = false;
    event.lockdown = false;
    dispatch(event);
  }  
}

void WebastoControlFSM::react(BatteryLevelEvent const &e)
{
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

    ShutdownEvent event;
    event.mode = fsm_mode;
    event.emergency = false;
    event.lockdown = false;
    dispatch(event);
  }
}

void WebastoControlFSM::react(FlameoutEvent const &e)
{
  CoreMutex m(&fsm_mutex);

  if (e.resetCount) {
    flameOutCount = 0;
  }

  // Make sure to shut off the flame sensor
  GlowPlugInEnableEvent e1;
  e1.enable = false;
  dispatch(e1);
  
  if (++flameOutCount > MAX_FLAMEOUT_COUNT) {
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
  transit<StandbyState>();
}



void WebastoControlFSM::react(ShutdownEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  int new_mode;
  int old_mode = fsm_mode;
  
  Log.notice("ShutdownEvent: mode 0x%02X, emergency:%d, lockdown:%d", e.mode, e.emergency, e.lockdown);
  if (e.lockdown) {
    LockdownEvent event;
    event.enable = true;
    dispatch(event);
  }

  if (e.emergency) {
    // transit<EmergencyOffState>();
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
        Log.error("Can't shutdown the wrong mode!  0x%02X != 0x%02X", e.mode, fsm_mode);
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
      Log.error("Received an unsupported mode: 0x%02X", e.mode);
      fsm_mode = old_mode;
      break;
  }
}

void WebastoControlFSM::react(AddTimeEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  Log.notice("AddTimeEvent: mode 0x%02X, %d minutes", e.mode, e.minutes);
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
  CoreMutex m(&fsm_mutex);

  ignitionOn = e.enable;
  if (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER) {
    if (ignitionOn) {
      StartupEvent event;
      event.mode = fsm_mode;
      dispatch(event);
    } else {
      ShutdownEvent event;
      event.emergency = false;
      event.lockdown = false;
      dispatch(event);
    }
  }
}

void WebastoControlFSM::react(StartRunEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  startRunSignalOn = e.enable;

  if (startRunSignalOn) {
    StartupEvent event;
    event.mode = WEBASTO_MODE_DEFAULT;
    dispatch(event);
  } else {
    ShutdownEvent event;
    event.mode = fsm_mode;
    event.lockdown = false;
    event.emergency = false;
    dispatch(event);
  }
}

void WebastoControlFSM::react(LockdownEvent const &e)
{
  CoreMutex m(&fram_mutex);

  lockdown = e.enable;
  fram_data.current.lockdown = lockdown;
  fram_dirty = true;
}


void WebastoControlFSM::react(StartupEvent const &e)
{
  CoreMutex m(&fsm_mutex);

  Log.notice("StartupEvent: mode 0x%02X, lockdown: %d", e.mode, lockdown);
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

  if (minutes) {
    globalTimer.register_timer(TIMER_TIMED_SHUT_DOWN, minutes * 60000, &fsmTimerCallback);
  }
  
  switch(fsm_mode) {
    case WEBASTO_MODE_PARKING_HEATER:
      transit<StandbyState>();
      break;
    case WEBASTO_MODE_SUPPLEMENTAL_HEATER:
      if (ignitionOn && supplementalEnabled) {
        transit<StandbyState>();
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
      Log.error("Received an unsupported mode: 0x%02X", e.mode);
      fsm_mode = old_mode;
      break;
  }
}


void IdleState::entry()
{
  CoreMutex m(&fsm_mutex);

  Log.notice("Entering IdleState");
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

void StandbyState::entry()
{
  kickRunTimer();

  // Turn on the combustion fan - 100%  
  CombustionFanEvent e1;
  e1.value = 100;
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
  // Turn off Combustion Fan
  CombustionFanEvent e1;
  e1.value = 0;
  dispatch(e1);    

  // Turn on Fuel Pump at 1000W - will give about 80mL of fuel in 3s
  FuelPumpEvent e2;
  e2.value = 1000;
  dispatch(e2);

  // Stay in this state for 3s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 3000, &fsmTimerCallback);
}

void PrefuelState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<InitialRampUpState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}


void InitialRampUpState::entry()
{
  // Turn on Combustion Fan - 40%
  CombustionFanEvent e1;
  e1.value = 40;
  dispatch(e1);    

  // Turn on Fuel Pump at 2000W - first ramp
  FuelPumpEvent e2;
  e2.value = 2000;
  dispatch(e2);

  // Stay in this ramp for 28s
  globalTimer.register_timer(TIMER_RAMP_STEP, 28000, &fsmTimerCallback);
}

void InitialRampUpState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_RAMP_STEP:
      {
        // Turn on Combustion Fan - 70%
        CombustionFanEvent e1;
        e1.value = 70;
        dispatch(e1);    

        // Turn on Fuel Pump at 3000W - second ramp
        FuelPumpEvent e2;
        e2.value = 3000;
        dispatch(e2);

        // Stay in this ramp for 28s
        globalTimer.register_timer(TIMER_STAGE_COMPLETE, 28000, &fsmTimerCallback);
      }
      break;
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
  // Leave Combustion Fan, Fuel Pump, Glow Plug and Circulation Pump alone!
  // Stay in this stage for 15s
  globalTimer.register_timer(TIMER_RAMP_STEP, 15000, &fsmTimerCallback);
}

void StabilizationState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<SecondRampUpState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}

void SecondRampUpState::entry()
{
  // Turn on Combustion Fan - 80%
  CombustionFanEvent e1;
  e1.value = 80;
  dispatch(e1);    

  // Turn on Fuel Pump at max power (5200W)
  FuelPumpEvent e2;
  e2.value = MAX_RATED_POWER;
  dispatch(e2);

  // Stay in this state for 50s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 50000, &fsmTimerCallback);
}

void SecondRampUpState::react(TimerEvent const &e)
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
  // Turn on Combustion Fan - 100%
  CombustionFanEvent e1;
  e1.value = 100;
  dispatch(e1);    

  // Turn off the Glow Plug
  GlowPlugOutEnableEvent e2;
  e2.enable = false;
  dispatch(e2);

  // And set the value to 0% too
  GlowPlugOutEvent e3;
  e3.value = 0;
  dispatch(e3);

  // Stay in this state for 15s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 15000, &fsmTimerCallback);
}

void TestBurnState::react(FlameDetectEvent const &e)
{
  CoreMutex m(&fsm_mutex);

  if (e.value < FLAME_DETECT_THRESHOLD) {
    // OK, this means our flame must be out, there's not enough heat here
    FlameoutEvent e1;
    e1.resetCount = false;
    dispatch(e1);
  }
}

void TestBurnState::react(TimerEvent const &e)
{
  switch (e.timerId) {
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
  // Turn on the Flame Sensor
  GlowPlugInEnableEvent e1;
  e1.enable = true;
  dispatch(e1);

  // Stay in this state for 30s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 30000, &fsmTimerCallback);
}

void FlameMeasureState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<AutoBurnState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}


void AutoBurnState::entry()
{
  // Turn off the Flame Sensor
  GlowPlugInEnableEvent e1;
  e1.enable = false;
  dispatch(e1);

  // Stay in this state for 15s
  globalTimer.register_timer(TIMER_STAGE_COMPLETE, 15000, &fsmTimerCallback);
}

void AutoBurnState::react(FlameDetectEvent const &e)
{
  CoreMutex m(&fsm_mutex);

  if (e.value < FLAME_DETECT_THRESHOLD) {
    // OK, this means our flame must be out, there's not enough heat here
    FlameoutEvent e1;
    e1.resetCount = true;
    dispatch(e1);
  }
}

void AutoBurnState::react(TimerEvent const &e)
{
  switch (e.timerId) {
    case TIMER_STAGE_COMPLETE:
      transit<FlameMeasureState>();
      break;
    default:
      fsmCommonReact(e);
      break;
  }
}


void CooldownState::entry()
{
  int currentPower = fuelPumpTimer.getBurnPower();

  // Shut off the fuel pump
  FuelPumpEvent e1;
  e1.value = 0;
  dispatch(e1);

  // Turn on the combustion fan at 100%
  CombustionFanEvent e2;
  e2.value = 100;
  dispatch(e2);

  // Ensure the circulation pump is on
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
  int timeout = map(currentPower, MIN_POWER, MAX_RATED_POWER, 100000, 175000);
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
  Log.warning("Entering lockdown mode.  Toggle EmergencyStop to clear");
}


void fsmTimerCallback(int timer_id, int delay)
{
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

FSM_INITIAL_STATE(WebastoControlFSM, IdleState)

WebastoControlFSM fsm;
