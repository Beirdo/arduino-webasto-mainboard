#ifndef __fsm_h_
#define __fsm_h_

#include "tinyfsm.hpp"
#include "fsm_events.h"
#include "fuel_pump.h"
#include "fram.h"

class WebastoControlFSM : public tinyfsm::Fsm<WebastoControlFSM>
{
  public:
    // default reaction for unhandled events */
    void react(tinyfsm::Event const &) { };

    virtual void react(TimerEvent         const &);
    
    virtual void react(FlameDetectEvent   const &);
    virtual void react(EmergencyStopEvent const &);
    virtual void react(CoolantTempEvent   const &);
    virtual void react(OutdoorTempEvent   const &);
    virtual void react(ExhaustTempEvent   const &);
    virtual void react(InternalTempEvent  const &);
    virtual void react(BatteryLevelEvent  const &);

    void react(RestartEvent               const &);
    void react(ShutdownEvent              const &);
    void react(StartupEvent               const &);
    void react(FlameoutEvent              const &);
    void react(AddTimeEvent               const &);

    void react(IgnitionEvent              const &);
    void react(StartRunEvent              const &);

    void react(GlowPlugInEnableEvent      const &);
    void react(GlowPlugOutEnableEvent     const &);
    void react(CirculationPumpEvent       const &);
    void react(CombustionFanEvent         const &);

    void react(GlowPlugOutEvent           const &);
    void react(FuelPumpEvent              const &);
    void react(VehicleFanEvent            const &);
    void react(LockdownEvent              const &);
    void react(OverheatEvent              const &);
    void react(LedChangeEvent             const &);

    uint8_t getStateNum(void) { return _state_num; };

    virtual void entry(void)  { };
    void exit(void)  { };

  protected:
    uint8_t _state_num;
};

extern int fsm_mode;
extern bool batteryLow;
extern bool supplementalEnabled;

extern bool ignitionOn;
extern bool startRunSignalOn;

extern bool circulationPumpOn;

extern int combustionFanPercent;
extern int vehicleFanPercent;
extern int glowPlugPercent;
extern float fuelNeedRequested;

extern bool glowPlugInEnable;   // mutually exclusive with glowPlugOutEnable
extern bool glowPlugOutEnable;  // mutually exclusive with glowPlugInEnable

extern time_sensor_t ventilation_duration;

extern int flameOutCount;

extern int exhaustTempPreBurn;
extern int exhaustTempStable;

extern mutex_t fsm_mutex;

extern WebastoControlFSM fsm;


void set_open_drain_pin(int pinNum, int value);
void fsmTimerCallback(int timer_id, int delay);
void fsmCommonReact(TimerEvent const&);
void kickRunTimer(void);
void increment_minutes(time_sensor_t *time);

#endif
