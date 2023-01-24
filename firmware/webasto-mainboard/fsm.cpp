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
bool ignitionOn;
bool startRunSignalOn;

bool combustionFanOn = false;
bool circulationPumpOn = false;

int vehicleFanPercent = 0;
int glowPlugPercent = 0;
bool fuelPumpOn = false;
int fuelPumpPeriodMs = 0;

bool glowPlugInEnable = false;   // mutually exclusive with glowPlugOutEnable
bool glowPlugOutEnable = false;  // mutually exclusive with glowPlugInEnable

bool flameLed = false;
bool operatingLed = false;

int time_start_ms[5] = {0};
int time_minutes[5] = {0};
time_sensor_t ventilation_duration = {0};

mutex_t fsm_mutex;
int kline_remaining_ms = 0;

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
  e4.enable = false;
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
  combustionFanOn = e.enable;
  digitalWrite(PIN_COMBUSTION_FAN, combustionFanOn);
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
  switch (e.timerId) {
    case TIMER_TIMED_SHUT_DOWN:
      {
        ShutdownEvent event;
        event.mode = fsm_mode;
        dispatch(event);
      }
      break;
    default:
      break;
  }
}

void WebastoControlFSM::react(FlameDetectEvent const &e)
{

}

void WebastoControlFSM::react(StartRunEvent const &e)
{

}

void WebastoControlFSM::react(EmergencyStopEvent const &e)
{

}

void WebastoControlFSM::react(CoolantTempEvent const &e)
{

}

void WebastoControlFSM::react(OutdoorTempEvent const &e)
{

}

void WebastoControlFSM::react(ExhaustTempEvent const &e)
{

}

void WebastoControlFSM::react(InternalTempEvent const &e) 
{
  
}

void WebastoControlFSM::reach(BatteryLevelEvent const &e)
{

}

void WebastoControlFSM::react(FlameoutEvent const &e)
{
  
}

void WebastoControlFSM::react(RestartEvent const &e)
{
  
}



void WebastoControlFSM::react(ShutdownEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  int new_mode;
  int old_mode = fsm_mode;
  
  Log.notice("ShutdownEvent: mode 0x%02X", e.mode);
  if (new_mode == WEBASTO_MODE_DEFAULT) {
    if (ignitionOn){
      new_mode = WEBASTO_MODE_SUPPLEMENTAL_HEATER; 
    } else{
      new_mode = WEBASTO_MODE_PARKING_HEATER;
    }
  }
  
  switch(new_mode) {
    case WEBASTO_MODE_PARKING_HEATER:
      if (new_mode == fsm_mode) { 
        //transit<BLAH>();
      } else {
        Log.error("Can't shutdown the wrong mode!  0x%02X != 0x%02X", e.mode, fsm_mode);
      }
      break;
    case WEBASTO_MODE_SUPPLEMENTAL_HEATER:
      if (new_mode == fsm_mode) { 
        //transit<BLAH>();
      } else {
        Log.error("Can't shutdown the wrong mode!  0x%02X != 0x%02X", e.mode, fsm_mode);
      }
      break;
    case WEBASTO_MODE_VENTILATION:
      {
        CombustionFanEvent event;
        event.enable = false;
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
    int index = fsm_mode - WEBASTO_MODE_PARKING_HEATER;
    int start_time = time_start_ms[index];
    time_minutes[index] += minutes;
    kline_remaining_ms = time_minutes[index] * 60000 + start_time - millis();
    globalTimer.adjust_timer(TIMER_TIMED_SHUT_DOWN, minutes * 60000);
  }
}


void WebastoControlFSM::react(IgnitionEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  ignitionOn = e.enable;
  if (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER && !ignitionOn) {
    //transit<BLAH>();
  }
}

void WebastoControlFSM::react(StartupEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  Log.notice("StartupEvent: mode 0x%02X", e.mode);
  int new_mode = e.mode;
  int old_mode = fsm_mode;
  int minutes = e.minutes;
  
  if (new_mode == WEBASTO_MODE_DEFAULT) {
    if (ignitionOn){
      new_mode = WEBASTO_MODE_SUPPLEMENTAL_HEATER; 
    } else{
      new_mode = WEBASTO_MODE_PARKING_HEATER;
    }
  }

  fsm_mode = new_mode;

  if (minutes) {
    int index = fsm_mode - WEBASTO_MODE_PARKING_HEATER;
    int start_time = millis();
    time_start_ms[index] = start_time;
    time_minutes[index] = minutes;

    globalTimer.register_timer(TIMER_TIMED_SHUT_DOWN, minutes * 60000, &fsmTimerCallback);
  }
  
  switch(fsm_mode) {
    case WEBASTO_MODE_PARKING_HEATER:
      //transit<BLAH>();
      break;
    case WEBASTO_MODE_SUPPLEMENTAL_HEATER:
      if (ignitionOn) {
        //transit<BLAH>();
      }
      break;
    case WEBASTO_MODE_VENTILATION:
      {
        CombustionFanEvent event;
        event.enable = true;
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
  ignitionOn = ignitionSenseSensor->get_value();
  for (int i = 0; i < 5; i ++) {
    time_start_ms[i] = 0;
  }
}

void IdleState::react(IgnitionEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  ignitionOn = e.enable;
  if (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER && ignitionOn) {
    //transit<BLAH>();
  }
}

void IdleState::react(StartRunEvent const &e)
{
  CoreMutex m(&fsm_mutex);
  startRunSignalOn = e.enable;
  if (fsm_mode == 0 && startRunSignalOn) {
    // we want to start up, but only if idle
    StartupEvent event;
    event.mode = WEBASTO_MODE_DEFAULT;
    dispatch(event);
  } else if (!startRunSignalOn) {
    // we want to shut down
    //transit<BLAH>();
  }
}

void fsmTimerCallback(int timer_id, int delay)
{
  TimerEvent event;
  event.value = delay;
  event.timerId = timer_id;
  WebastoControlFSM::dispatch(event);
}

FSM_INITIAL_STATE(WebastoControlFSM, IdleState)

WebastoControlFSM fsm;
