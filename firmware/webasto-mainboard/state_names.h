#ifndef __state_names_
#define __state_names_

// Pulled from:  https://github.com/futuranet/libwbus/blob/master/wbus/wbus_const.h

#define WB_STATE_BO	0x00 /* Burn out */
#define	WB_STATE_DEACT1	0x01 /* Deactivation */
#define	WB_STATE_BOADR	0x02 /* Burn out ADR (has something to due with hazardous substances transportation) */
#define	WB_STATE_BORAMP	0x03 /* Burn out Ramp */
#define	WB_STATE_OFF	0x04 /* Off state */
#define	WB_STATE_CPL	0x05 /* Combustion process part load */
#define	WB_STATE_CFL	0x06 /* Combustion process full load */
#define	WB_STATE_FS	0x07 /* Fuel supply */
#define	WB_STATE_CAFS	0x08 /* Combustion air fan start */
#define	WB_STATE_FSI	0x09 /* Fuel supply interruption */
#define	WB_STATE_DIAG	0x0a /* Diagnostic state */
#define	WB_STATE_FPI	0x0b /* Fuel pump interruption */
#define	WB_STATE_EMF	0x0c /* EMF measurement */
#define	WB_STATE_DEB	0x0d /* Debounce */
#define	WB_STATE_DEACTE	0x0e /* Deactivation */
#define	WB_STATE_FDI	0x0f /* Flame detector interrogation */
#define	WB_STATE_FDC	0x10 /* Flame detector cooling */
#define	WB_STATE_FDM	0x11 /* Flame detector measuring phase */
#define	WB_STATE_FDMZ	0x12 /* Flame detector measuring phase ZUE */
#define	WB_STATE_FAN	0x13 /* Fan start up */
#define	WB_STATE_GPRAMP	0x14 /* Glow plug ramp */
#define	WB_STATE_LOCK	0x15 /* Heater interlock */
#define	WB_STATE_INIT	0x16 /* Initialization   */
#define	WB_STATE_BUBLE	0x17 /* Fuel bubble compensation */
#define	WB_STATE_FANC	0x18 /* Fan cold start-up */
#define	WB_STATE_COLDR	0x19 /* Cold start enrichment */
#define	WB_STATE_COOL	0x1a /* Cooling */
#define	WB_STATE_LCHGUP	0x1b /* Load change PL-FL */
#define	WB_STATE_VENT	0x1c /* Ventilation */
#define	WB_STATE_LCHGDN	0x1d /* Load change FL-PL */
#define	WB_STATE_NINIT	0x1e /* New initialization */
#define	WB_STATE_CTRL	0x1f /* Controlled operation */
#define	WB_STATE_CIDDLE	0x20 /* Control iddle period */
#define	WB_STATE_SSTART	0x21 /* Soft start */
#define	WB_STATE_STIME	0x22 /* Savety time */
#define	WB_STATE_PURGE	0x23 /* Purge */
#define	WB_STATE_START	0x24 /* Start */
#define	WB_STATE_STAB	0x25 /* Stabilization */
#define	WB_STATE_SRAMP	0x26 /* Start ramp    */
#define	WB_STATE_OOP	0x27 /* Out of power  */
#define	WB_STATE_LOCK2	0x28 /* Interlock     */
#define WB_STATE_LOCKADR	0x29 /* Interlock ADR (Australian design rules) */
#define	WB_STATE_STABT	0x2a /* Stabilization time */
#define	WB_STATE_CHGCTRL	0x2b /* Change to controlled operation */
#define	WB_STATE_DECIS	0x2c /* Decision state */
#define	WB_STATE_PSFS	0x2d /* Prestart fuel supply */
#define	WB_STATE_GLOW	0x2e /* Glowing */
#define	WB_STATE_GLOWP	0x2f /* Glowing power control */
#define	WB_STATE_DELAY	0x30 /* Delay lowering */
#define	WB_STATE_SLUG	0x31 /* Sluggish fan start */
#define	WB_STATE_AGLOW	0x32 /* Additional glowing */
#define	WB_STATE_IGNI	0x33 /* Ignition interruption */
#define	WB_STATE_IGN	0x34 /* Ignition */
#define	WB_STATE_IGNII	0x35 /* Intermittent glowing */
#define	WB_STATE_APMON	0x36 /* Application monitoring */
#define	WB_STATE_LOCKS	0x37 /* Interlock save to memory */
#define	WB_STATE_LOCKD	0x38 /* Heater interlock deactivation */
#define	WB_STATE_OUTCTL	0x39 /* Output control */
#define	WB_STATE_CPCTL	0x3a /* Circulating pump control */
#define	WB_STATE_INITUC	0x3b /* Initialization uP */
#define	WB_STATE_SLINT	0x3c /* Stray light interrogation */
#define	WB_STATE_PRES	0x3d /* Prestart */
#define	WB_STATE_PREIGN	0x3e /* Pre-ignition */
#define	WB_STATE_FIGN	0x3f /* Flame ignition */
#define	WB_STATE_FSTAB	0x40 /* Flame stabilization */
#define	WB_STATE_PH	0x41 /* Combustion process parking heating */
#define	WB_STATE_SH	0x42 /* Combustion process suppl. heating  */
#define	WB_STATE_PHFAIL	0x43 /* Combustion failure failure heating */
#define	WB_STATE_SHFAIL	0x44 /* Combustion failure suppl. heating  */
#define	WB_STATE_OFFR	0x45 /* Heater off after run */
#define	WB_STATE_CID	0x46 /* Control idle after run */
#define	WB_STATE_ARFAIL	0x47 /* After-run due to failure */
#define	WB_STATE_ARTCTL	0x48 /* Time-controlled after-run due to failure */
#define	WB_STATE_LOCKCP	0x49 /* Interlock circulation pump */
#define	WB_STATE_CIDPH	0x4a /* Control idle after parking heating */
#define	WB_STATE_CIDSH	0x4b /* Control idle after suppl. heating  */
#define	WB_STATE_CIDHCP	0x4c /* Control idle period suppl. heating with circulation pump */
#define	WB_STATE_CPNOH	0x4d /* Circulation pump without heating function */
#define	WB_STATE_OV	0x4e /* Waiting loop overvoltage */
#define	WB_STATE_MFAULT	0x4f /* Fault memory update */
#define	WB_STATE_WLOOP	0x50 /* Waiting loop */
#define	WB_STATE_CTEST	0x51 /* Component test */
#define	WB_STATE_BOOST	0x52 /* Boost */
#define	WB_STATE_COOL2	0x53 /* Cooling */
#define	WB_STATE_LOCKP	0x54 /* Heater interlock permanent */
#define	WB_STATE_FANIDL	0x55 /* Fan idle */
#define	WB_STATE_BA	0x56 /* Break away */
#define	WB_STATE_TINT	0x57 /* Temperature interrogation */
#define	WB_STATE_PREUV	0x58 /* Prestart undervoltage */
#define	WB_STATE_AINT	0x59 /* Accident interrogation */
#define	WB_STATE_ARSV	0x5a /* After-run solenoid valve */
#define	WB_STATE_MFLTSV	0x5b /* Fault memory update solenoid valve */
#define	WB_STATE_TCARSV	0x5c /* Timer-controlled after-run solenoid valve */
#define	WB_STATE_SA	0x5d /* Startup attempt */
#define	WB_STATE_PREEXT	0x5e /* Prestart extension */
#define	WB_STATE_COMBP	0x5f /* Combustion process */
#define	WB_STATE_TIARUV	0x60 /* Timer-controlled after-run due to undervoltage */
#define	WB_STATE_MFLTSW	0x61 /* Fault memory update prior switch off */
#define	WB_STATE_RAMPFL	0x62 /* Ramp full load */

#endif
