/**
 *  @brief     An STM32 HAL library written for the BQ25890 charge controller IC in C.
 *  @copyright GPL-3.0 license.
 */

#ifndef BQ25890_H
#define BQ25890_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/*------------------------------------ PORTING FUNCTIONS
 * -----------------------------------------*/

#define BQ25890_I2C_ADDR 0x6A
HAL_StatusTypeDef BQ25890_WriteRegister(uint8_t reg, uint8_t* data);
HAL_StatusTypeDef BQ25890_ReadRegister(uint8_t reg, uint8_t* data);

/*------------------------------------ ENUM DEFINATIONS
 * -----------------------------------------*/

typedef enum BQ25890_STATE { BQ25890_DISABLED, BQ25890_ENABLED } BQ25890_STATE;

typedef enum BQ25890_RESET_STATE {
    BQ25890_RESET_NORMAL,
    BQ25890_RESET
} BQ25890_RESET_STATE;

typedef enum BQ25890_FAULT_STATE {
    BQ25890_FAULT_NORMAL,
    BQ25890_FAULT
} BQ25890_FAULT_STATE;

typedef enum BQ25890_BHOT {
    BQ25890_BHOT_34_75_PERCENT,
    BQ25890_BHOT_37_75_PERCENT,
    BQ25890_BHOT_31_25_PERCENT,
    BQ25890_BHOT_DISABLE
} BQ25890_BHOT;

typedef enum BQ25890_BCOLD {
    BQ25890_BCOLD_77_PERCENT,
    BQ25890_BCOLD_80_PERCENT
} BQ25890_BCOLD;

typedef enum BQ25890_JEITA_ISET {
    BQ25890_JEITA_ISET_50_PERCENT_ICHG,
    BQ25890_JEITA_ISET_20_PERCENT_ICHG
} BQ25890_JEITA_ISET;

typedef enum BQ25890_JEITA_VSET {
    BQ25890_JEITA_VSET_VREG_DEC_200MV,
    BQ25890_JEITA_VSET_VREG,
} BQ25890_JEITA_VSET;

typedef enum BQ25890_BOOST_LIM {
    BQ25890_BOOST_LIM_500MA,
    BQ25890_BOOST_LIM_750MA,
    BQ25890_BOOST_LIM_1200MA,
    BQ25890_BOOST_LIM_1400MA,
    BQ25890_BOOST_LIM_1650MA,
    BQ25890_BOOST_LIM_1875MA,
    BQ25890_BOOST_LIM_2150MA,
    BQ25890_BOOST_LIM_2450MA
} BQ25890_BOOST_LIM;

typedef enum BQ25890_CONV_RATE {
    BQ25890_ADC_ONE_SHOT,
    BQ25890_ADC_CONTINUOUS
} BQ25890_CONV_RATE;

typedef enum BQ25890_BOOST_FREQ {
    BQ25890_BOOST_FREQ_1500K,
    BQ25890_BOOST_FREQ_500K
} BQ25890_BOOST_FREQ;

typedef enum BQ25890_BATLOWV {
    BQ25890_BATLOWV_2800MV,
    BQ25890_BATLOWV_3000MV
} BQ25890_BATLOWV;

typedef enum BQ25890_VRECHG {
    BQ25890_VRECHG_100MV,
    BQ25890_VRECHG_200MV
} BQ25890_VRECHG;

typedef enum BQ25890_WATCHDOG {
    BQ25890_WATCHDOG_DISABLE,
    BQ25890_WATCHDOG_40S,
    BQ25890_WATCHDOG_80S,
    BQ25890_WATCHDOG_160S
} BQ25890_WATCHDOG;

typedef enum BQ25890_CHG_TIMER {
    BQ25890_CHG_TIMER_5HOURS,
    BQ25890_CHG_TIMER_8HOURS,
    BQ25890_CHG_TIMER_12HOURS,
    BQ25890_CHG_TIMER_20HOURS
} BQ25890_CHG_TIMER;

typedef enum BQ25890_TREG {
    BQ25890_TREG_60C,
    BQ25890_TREG_80C,
    BQ25890_TREG_100C,
    BQ25890_TREG_120C
} BQ25890_TREG;

typedef enum BQ25890_VBUS_STAT {
    BQ25890_NO_INPUT,
    BQ25890_USB_SDP,
    BQ25890_USB_CDP,
    BQ25890_USB_DCP,
    BQ25890_MAX_CHARGE_DCP,
    BQ25890_UNKNOWN,
    BQ25890_NON_STANDARD,
    BQ25890_OTG
} BQ25890_VBUS_STAT;

typedef enum BQ25890_CHRG_STAT {
    BQ25890_NOT_CHARGING,
    BQ25890_PRE_CHARGE,
    BQ25890_FAST_CHARGE,
    BQ25890_CHARGE_TERMINATION
} BQ25890_CHRG_STAT;

typedef enum BQ25890_PG_STAT {
    BQ25890_NO_POWER_GOOD,
    BQ25890_POWER_GOOD
} BQ25890_PG_STAT;

typedef enum BQ25890_VSYS_STAT {
    BQ25890_NO_REGULATION,
    BQ25890_IN_REGULATION
} BQ25890_VSYS_STAT;

typedef enum BQ25890_CHRG_FAULT {
    BQ25890_CHG_NORMAL,
    BQ25890_INPUT_FAULT,
    BQ25890_THERMAL_SHUTDOWN,
    BQ25890_SAFETY_TIMER
} BQ25890_CHRG_FAULT;

typedef enum BQ25890_NTC_FAULT {
    BQ25890_NTC_NORMAL,
    BQ25890_BUCK_TS_COLD = 1,
    BQ25890_BUCK_TS_HOT = 2,
    BQ25890_BOOST_TS_COLD = 5,
    BQ25890_BOOST_TS_HOT = 6
} BQ25890_NTC_FAULT;

typedef enum BQ25890_FORCE_VINDPM {
    BQ25890_RELATIVE_VINDPM,
    BQ25890_ABSOLUTE_VINDPM
} BQ25890_FORCE_VINDPM;

typedef enum BQ25890_THERM_STAT {
    BQ25890_NO_THERMAL_REGULATION,
    BQ25890_IN_THERMAL_REGULATION
} BQ25890_THERM_STAT;

typedef enum BQ25890_VBUS_GD {
    BQ25890_NO_VBUS,
    BQ25890_VBUS_PRESENT
} BQ25890_VBUS_GD;

typedef enum BQ_DEVICE { DEVICE_BQ25890 = 3, DEVICE_BQ25892 = 0 } BQ_DEVICE;

HAL_StatusTypeDef BQ25890_SetHIZmode(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetHIZmode(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetInputCurrentLimitMode(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetInputCurrentLimitMode(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetInputCurrentLimit(uint16_t* current_ma);
HAL_StatusTypeDef BQ25890_GetInputCurrentLimit(uint16_t* current_ma);

HAL_StatusTypeDef BQ25890_SetBoostHotTempTH(BQ25890_BHOT* state);
HAL_StatusTypeDef BQ25890_GetBoostHotTempTH(BQ25890_BHOT* state);

HAL_StatusTypeDef BQ25890_SetBoostColdTempTH(BQ25890_BCOLD* state);
HAL_StatusTypeDef BQ25890_GetBoostColdTempTH(BQ25890_BCOLD* state);

HAL_StatusTypeDef BQ25890_SetInputVoltageLimitOffset(uint16_t* offset);
HAL_StatusTypeDef BQ25890_GetInputVoltageLimitOffset(uint16_t* offset);

HAL_StatusTypeDef BQ25890_StartADCconversion(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetADCconversionStatus(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetADCconversionMode(BQ25890_CONV_RATE* state);
HAL_StatusTypeDef BQ25890_GetADCconversionMode(BQ25890_CONV_RATE* state);

HAL_StatusTypeDef BQ25890_SetBoostFreq(BQ25890_BOOST_FREQ* state);
HAL_StatusTypeDef BQ25890_GetBoostFreq(BQ25890_BOOST_FREQ* state);

HAL_StatusTypeDef BQ25890_SetBoostCurrentLimit(BQ25890_BOOST_LIM* state);
HAL_StatusTypeDef BQ25890_GetBoostCurrentLimit(BQ25890_BOOST_LIM* state);

HAL_StatusTypeDef BQ25890_SetInputCurrentOptimizer(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetInputCurrentOptimizer(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetHighVoltageDCP(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetHighVoltageDCP(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetMaxCharge(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetMaxCharge(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetForceDPDM(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetForceDPDM(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetAutoDPDM(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetAutoDPDM(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetBatLoad(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetBatLoad(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_ResetWatchdog(void);

HAL_StatusTypeDef BQ25890_SetOTGmode(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetOTGmode(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetChgMode(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetChgMode(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetSysMinVoltage(uint16_t* voltage_mv);
HAL_StatusTypeDef BQ25890_GetSysMinVoltage(uint16_t* voltage_mv);

HAL_StatusTypeDef BQ25890_SetCurrentPulseMode(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetCurrentPulseMode(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetFastChargeCurrent(uint16_t* current_ma);
HAL_StatusTypeDef BQ25890_GetFastChargeCurrent(uint16_t* current_ma);

HAL_StatusTypeDef BQ25890_SetPreChargeCurrent(uint16_t* current_ma);
HAL_StatusTypeDef BQ25890_GetPreChargeCurrent(uint16_t* current_ma);

HAL_StatusTypeDef BQ25890_SetTermChargeCurrent(uint16_t* current_ma);
HAL_StatusTypeDef BQ25890_GetTermChargeCurrent(uint16_t* current_ma);

HAL_StatusTypeDef BQ25890_SetChargeVoltage(uint16_t* voltage_mv);
HAL_StatusTypeDef BQ25890_GetChargeVoltage(uint16_t* voltage_mv);

HAL_StatusTypeDef BQ25890_SetPreFastChargeTH(BQ25890_BATLOWV* state);
HAL_StatusTypeDef BQ25890_GetPreFastChargeTH(BQ25890_BATLOWV* state);

HAL_StatusTypeDef BQ25890_SetRechargeThOffset(BQ25890_VRECHG* state);
HAL_StatusTypeDef BQ25890_GetRechargeThOffset(BQ25890_VRECHG* state);

HAL_StatusTypeDef BQ25890_SetChargingTermination(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetChargingTermination(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetSTATPinMode(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetSTATPinMode(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetWatchdogTimer(BQ25890_WATCHDOG* state);
HAL_StatusTypeDef BQ25890_GetWatchdogTimer(BQ25890_WATCHDOG* state);

HAL_StatusTypeDef BQ25890_SetSafetyTimer(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetSafetyTimer(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetFastChargeTimer(BQ25890_CHG_TIMER* state);
HAL_StatusTypeDef BQ25890_GetFastChargeTimer(BQ25890_CHG_TIMER* state);

HAL_StatusTypeDef BQ25890_SetIRCompResistance(uint8_t* ohms_mohm);
HAL_StatusTypeDef BQ25890_GetIRCompResistance(uint8_t* ohms_mohm);

HAL_StatusTypeDef BQ25890_SetIRCompVoltage(uint8_t* voltage_mv);
HAL_StatusTypeDef BQ25890_GetIRCompVoltage(uint8_t* voltage_mv);

HAL_StatusTypeDef BQ25890_SetThermalRegulationTH(BQ25890_TREG* threshold);
HAL_StatusTypeDef BQ25890_GetThermalRegulationTH(BQ25890_TREG* threshold);

HAL_StatusTypeDef BQ25890_ForceICO(void);

HAL_StatusTypeDef BQ25890_SetDPM2xSafetyTimer(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetDPM2xSafetyTimer(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetShipMode(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetShipMode(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetShipModeDelay(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetShipModeDelay(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetSystemResetFunction(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetSystemResetFunction(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetCurrentPulseVoltageUp(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetCurrentPulseVoltageUp(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetCurrentPulseVoltageDown(BQ25890_STATE* state);
HAL_StatusTypeDef BQ25890_GetCurrentPulseVoltageDown(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_SetBoostModeVoltage(uint16_t* voltage_mv);
HAL_StatusTypeDef BQ25890_GetBoostModeVoltage(uint16_t* voltage_mv);

HAL_StatusTypeDef BQ25890_GetVBUSStatus(BQ25890_VBUS_STAT* state);

HAL_StatusTypeDef BQ25890_GetChargingStatus(BQ25890_CHRG_STAT* state);

HAL_StatusTypeDef BQ25890_GetPowerGoodStatus(BQ25890_PG_STAT* state);

HAL_StatusTypeDef BQ25890_GetVSYSRegulationStatus(BQ25890_VSYS_STAT* state);

HAL_StatusTypeDef BQ25890_GetWatchdogFaultStatus(BQ25890_FAULT_STATE* state);

HAL_StatusTypeDef BQ25890_GetBoostFaultStatus(BQ25890_FAULT_STATE* state);

HAL_StatusTypeDef BQ25890_GetChargeFaultStatus(BQ25890_CHRG_FAULT* state);

HAL_StatusTypeDef BQ25890_GetBatteryFaultStatus(BQ25890_FAULT_STATE* state);

HAL_StatusTypeDef BQ25890_GetNTCFaultStatus(BQ25890_NTC_FAULT* state);

HAL_StatusTypeDef BQ25890_SetForceVINDPM(BQ25890_FORCE_VINDPM* state);
HAL_StatusTypeDef BQ25890_GetForceVINDPM(BQ25890_FORCE_VINDPM* state);

HAL_StatusTypeDef BQ25890_SetAbsoluteVINPDMTh(uint16_t* voltage_mv);
HAL_StatusTypeDef BQ25890_GetAbsoluteVINPDMTh(uint16_t* voltage_mv);

HAL_StatusTypeDef BQ25890_GetThermalRegulationStatus(BQ25890_THERM_STAT* state);

HAL_StatusTypeDef BQ25890_GetBatteryVoltage(uint16_t* voltage_mv);

HAL_StatusTypeDef BQ25890_GetSystemVoltage(uint16_t* voltage_mv);

HAL_StatusTypeDef BQ25890_GetTSVoltage(uint16_t* percent);

HAL_StatusTypeDef BQ25890_GetVBUSGoodStatus(BQ25890_VBUS_GD* state);

HAL_StatusTypeDef BQ25890_GetVBUSVoltage(uint16_t* voltage_mv);

HAL_StatusTypeDef BQ25890_GetChargeCurrent(uint16_t* current_ma);

HAL_StatusTypeDef BQ25890_GetVINDPMStatus(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_GetIINDPMStatus(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_GetICO_IIDPMCurrent(uint16_t* current_ma);

HAL_StatusTypeDef BQ25890_ResetChip(void);

HAL_StatusTypeDef BQ25890_GetICOStatus(BQ25890_STATE* state);

HAL_StatusTypeDef BQ25890_GetDevice(BQ_DEVICE* device);

HAL_StatusTypeDef BQ25890_GetTSProfile(uint8_t* profile);

HAL_StatusTypeDef BQ25890_GetDevRev(uint8_t* rev);

#ifdef __cplusplus
}
#endif

#endif /* BQ25890_INCLUDE_BQ25890_H_ */
