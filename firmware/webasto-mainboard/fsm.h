#ifndef __fsm_h_
#define __fsm_h_

#include "tinyfsm.hpp"
#include "fsm_events.h"
#include "fuel_pump.h"

class WebastoControlFSM : public tinyfsm::Fsm<WebastoControlFSM>
{
  public:
    // default reaction for unhandled events */
    void react(tinyfsm::Event const &) { };

    virtual void react(TimerEvent         const &);
    
    virtual void react(FlameDetectEvent   const &);
    virtual void react(IgnitionEvent      const &);
    virtual void react(StartRunEvent      const &);
    virtual void react(EmergencyStopEvent const &);
    virtual void react(CoolantTempEvent   const &);
    virtual void react(OutdoorTempEvent   const &);
    virtual void react(ExhaustTempEvent   const &);
    virtual void react(InternalTempEvent  const &);
    virtual void reach(BatteryLevelEvent  const &);
    
    virtual void react(FlameoutEvent      const &);
    virtual void react(RestartEvent       const &);
    virtual void react(ShutdownEvent      const &);
    virtual void react(StartupEvent       const &);
    virtual void react(AddTimeEvent       const &);

    void react(GlowPlugInEnableEvent      const &);
    void react(GlowPlugOutEnableEvent     const &);
    void react(CirculationPumpEvent       const &);
    void react(CombustionFanEvent         const &);

    void react(GlowPlugOutEvent           const &);
    void react(FuelPumpEvent              const &);
    void react(VehicleFanEvent            const &);

    uint8_t getStateNum(void) { return _state_num; };

    virtual void entry(void)  { };
    void exit(void)  { };

  protected:
    uint8_t _state_num;
};

extern int mode;
extern bool ignitionOn;
extern bool startRunSignalOn;

extern bool combustionFanOn;
extern bool circulationPumpOn;

extern int vehicleFanPercent;
extern int glowPlugPercent;
extern bool fuelPumpOn;
extern int fuelPumpPeriodMs;

extern bool glowPlugInEnable;   // mutually exclusive with glowPlugOutEnable
extern bool glowPlugOutEnable;  // mutually exclusive with glowPlugInEnable

extern int time_start_ms[5];
extern int time_minutes[5];
extern timerItem_t *timer[5];

extern int kline_remaining_ms;


void set_open_drain_pin(int pinNum, int value);

#endif
