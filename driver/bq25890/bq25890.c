/**
 *  @brief     An STM32 HAL library written for the BQ25890 charge controller IC
 * in C.
 *  @author    Sumant Khalate www.github.com/SumantKhalate/BQ25890
 *  @date      May 2023
 *  @copyright GPL-3.0 license.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "bq25890.h"

#include "bq25890_reg.h"

/*---------------------------------------- DEVICE ADDRESS
 * ---------------------------------------*/

/**
 * @brief Writes one byte of data to the designated BQ25890 register.
 * @param[in] reg Register address to write to.
 * @param[in] *data Pointer to a date variable to write from.
 * @return HAL_StatusTypeDef variable describing if it was successful or not.
 */
__weak HAL_StatusTypeDef BQ25890_WriteRegister(uint8_t reg, uint8_t* data) {
    // return HAL_I2C_Mem_Write(BQ25890_device, BQ25890_I2C_ADDR, reg,
    //                          I2C_MEMADD_SIZE_8BIT, data, 1, BQ25890_TIMEOUT);
    return HAL_OK;
}

/**
 * @brief Reads one byte of data from the designated BQ25890 register.
 * @param[in] reg Register address to read from.
 * @param[out] *data Pointer to a date variable to read to.
 * @return HAL_StatusTypeDef variable describing if it was successful or not.
 */
__weak HAL_StatusTypeDef BQ25890_ReadRegister(uint8_t reg, uint8_t* data) {
    // return HAL_I2C_Mem_Read(BQ25890_device, BQ25890_I2C_ADDR, reg,
    //                         I2C_MEMADD_SIZE_8BIT, data, 1, BQ25890_TIMEOUT);
    return HAL_OK;
}

/**
 * @brief Updates the designated bits with the data within the BQ25890 register
 * based on the mask.
 * @param[in] reg Register address to write to.
 * @param[in] mask Data mask.
 * @param[in] *data Pointer to a date variable.
 * @return HAL_StatusTypeDef variable describing if it was successful or not.
 */
HAL_StatusTypeDef BQ25890_UpdateBits(uint8_t reg, uint8_t mask, uint8_t* data) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(reg, &temp);
    if (status != HAL_OK)
        return status;
    temp &= ~mask;
    temp |= *data & mask;
    return BQ25890_WriteRegister(reg, &temp);
}

/**
 * @brief Set high impedance mode (EN_HIZ)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_SetHIZmode(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_ENHIZ_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_00, BQ25890_ENHIZ_MASK, &temp);
}

/**
 * @brief Get high impedance mode (EN_HIZ)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_GetHIZmode(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_00, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_ENHIZ_MASK) >> BQ25890_ENHIZ_BIT;
    return status;
}

/**
 * @brief Set input current limit mode (EN_ILIM)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetInputCurrentLimitMode(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_ENILIM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_00, BQ25890_ENILIM_MASK, &temp);
}

/**
 * @brief Get input current limit mode (EN_ILIM)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetInputCurrentLimitMode(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_00, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_ENILIM_MASK) >> BQ25890_ENILIM_BIT;
    return status;
}

/**
 * @brief Set input current limit (IINLIM[5-0])
 * @param[in] *current_ma Current in mA. Range: 100mA - 3250mA with increment of
 * 50mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 500mA
 */
HAL_StatusTypeDef BQ25890_SetInputCurrentLimit(uint16_t* current_ma) {
    uint8_t temp = (*current_ma - BQ25890_IINLIM_BASE) / BQ25890_IINLIM_LSB;
    temp <<= BQ25890_IINLIM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_00, BQ25890_IINLIM_MASK, &temp);
}

/**
 * @brief Get input current limit (IINLIM[5-0])
 * @param[out] *current_ma Current in mA. Range: 100mA - 3250mA with increment
 * of 50mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 500mA
 */
HAL_StatusTypeDef BQ25890_GetInputCurrentLimit(uint16_t* current_ma) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_00, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_IINLIM_MASK) >> BQ25890_IINLIM_BIT;
    *current_ma = (temp * BQ25890_IINLIM_LSB) + BQ25890_IINLIM_BASE;
    return status;
}

/**
 * @brief Set boost mode hot temperature monitor threshold (BHOT[1-0])
 * @param[in] *state #BQ25890_BHOT_34_75_PERCENT, #BQ25890_BHOT_37_75_PERCENT,
 * #BQ25890_BHOT_31_25_PERCENT or #BQ25890_BHOT_DISABLE
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_BHOT_34_75_PERCENT (00)
 */
HAL_StatusTypeDef BQ25890_SetBoostHotTempTH(BQ25890_BHOT* state) {
    uint8_t temp = *state << BQ25890_BHOT_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_01, BQ25890_BHOT_MASK, &temp);
}

/**
 * @brief Get boost mode hot temperature monitor threshold (BHOT[1-0])
 * @param[out] *state #BQ25890_BHOT_34_75_PERCENT, #BQ25890_BHOT_37_75_PERCENT,
 * #BQ25890_BHOT_31_25_PERCENT or #BQ25890_BHOT_DISABLE
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_BHOT_34_75_PERCENT (00)
 */
HAL_StatusTypeDef BQ25890_GetBoostHotTempTH(BQ25890_BHOT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_01, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BHOT_MASK) >> BQ25890_BHOT_BIT;
    return status;
}

/**
 * @brief Set boost mode cold temperature monitor threshold (BCOLD)
 * @param[in] *state #BQ25890_BCOLD_77_PERCENT and #BQ25890_BCOLD_80_PERCENT
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_BCOLD_77_PERCENT (0)
 */
HAL_StatusTypeDef BQ25890_SetBoostColdTempTH(BQ25890_BCOLD* state) {
    uint8_t temp = *state << BQ25890_BCOLD_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_01, BQ25890_BCOLD_MASK, &temp);
}

/**
 * @brief Get boost mode cold temperature monitor threshold (BCOLD)
 * @param[out] *state #BQ25890_BCOLD_77_PERCENT and #BQ25890_BCOLD_80_PERCENT
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_BCOLD_77_PERCENT (0)
 */
HAL_StatusTypeDef BQ25890_GetBoostColdTempTH(BQ25890_BCOLD* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_01, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BCOLD_MASK) >> BQ25890_BCOLD_BIT;
    return status;
}

/**
 * @brief Set input voltage limit offset(VINDPM_OS[4-0])
 * @param[in] *offset Offset in mV. Range: 0mV - 3100mV with increment of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 500mV
 */
HAL_StatusTypeDef BQ25890_SetInputVoltageLimitOffset(uint16_t* offset) {
    uint8_t temp = (*offset - BQ25890_VINDPMOS_BASE) / BQ25890_VINDPMOS_LSB;
    temp <<= BQ25890_VINDPMOS_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_01, BQ25890_VINDPMOS_MASK, &temp);
}

/**
 * @brief Get input voltage limit offset(VINDPM_OS[4-0])
 * @param[out] *offset Offset in mV. Range: 0mV - 3100mV with increment of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 500mV
 */
HAL_StatusTypeDef BQ25890_GetInputVoltageLimitOffset(uint16_t* offset) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_01, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_VINDPMOS_MASK) >> BQ25890_VINDPMOS_BIT;
    *offset = (temp * BQ25890_VINDPMOS_LSB) + BQ25890_VINDPMOS_BASE;
    return status;
}

/**
 * @brief Start ADC conversion (CONV_START)
 * @param[in] *state #BQ25890_DISABLED and #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 * @note This bit is read-only when CONV_RATE = 1. The bit stays high during ADC
 * conversion and during input source detection
 */
HAL_StatusTypeDef BQ25890_StartADCconversion(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_CONV_START_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_CONV_START_MASK, &temp);
}

/**
 * @brief Get status of ADC conversion (CONV_START)
 * @param[out] *state #BQ25890_DISABLED and #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 * @note This bit is read-only when CONV_RATE = 1. The bit stays high during ADC
 * conversion and during input source detection
 */
HAL_StatusTypeDef BQ25890_GetADCconversionStatus(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_CONV_START_MASK) >> BQ25890_CONV_START_BIT;
    return status;
}

/**
 * @brief Set ADC conversion mode (CONV_RATE)
 * @param[in] *state #BQ25890_ADC_ONE_SHOT and #BQ25890_ADC_CONTINUOUS
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ADC_ONE_SHOT (0)
 * @note When CONV_RATE = 1. The CONV_START bit is read-only
 */
HAL_StatusTypeDef BQ25890_SetADCconversionMode(BQ25890_CONV_RATE* state) {
    uint8_t temp = *state << BQ25890_CONV_RATE_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_CONV_RATE_MASK, &temp);
}

/**
 * @brief Get ADC conversion mode (CONV_RATE)
 * @param[out] *state #BQ25890_ADC_ONE_SHOT and #BQ25890_ADC_CONTINUOUS
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ADC_ONE_SHOT (0)
 * @note When CONV_RATE = 1. The CONV_START bit is read-only
 */
HAL_StatusTypeDef BQ25890_GetADCconversionMode(BQ25890_CONV_RATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_CONV_RATE_MASK) >> BQ25890_CONV_RATE_BIT;
    return status;
}

/**
 * @brief Set boost  mode conversion frequency (BOOST_FREQ)
 * @param[in] *state #BQ25890_BOOST_FREQ_1500K and #BQ25890_BOOST_FREQ_500K
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_BOOST_FREQ_500K (1)
 * @note Read-only when OTG_CONFIG = 1
 */
HAL_StatusTypeDef BQ25890_SetBoostFreq(BQ25890_BOOST_FREQ* state) {
    uint8_t temp = *state << BQ25890_BOOST_FREQ_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_BOOST_FREQ_MASK, &temp);
}

/**
 * @brief Get boost  mode conversion frequency (BOOST_FREQ)
 * @param[out] *state #BQ25890_BOOST_FREQ_1500K and #BQ25890_BOOST_FREQ_500K
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_BOOST_FREQ_500K (1)
 * @note Read-only when OTG_CONFIG = 1
 */
HAL_StatusTypeDef BQ25890_GetBoostFreq(BQ25890_BOOST_FREQ* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BOOST_FREQ_MASK) >> BQ25890_BOOST_FREQ_BIT;
    return status;
}

/**
 * @brief Set boost mode current limit (BOOST_LIM)
 * @param[in] *state BQ25890_BOOST_LIM_xxx
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default BQ25890_BOOST_LIM_1400mA (011)
 */
HAL_StatusTypeDef BQ25890_SetBoostCurrentLimit(BQ25890_BOOST_LIM* state) {
    uint8_t temp = *state << BQ25890_BOOST_LIM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_0A, BQ25890_BOOST_LIM_MASK, &temp);
}

/**
 * @brief Get boost mode current limit (BOOST_LIM)
 * @param[out] *state BQ25890_BOOST_LIM_xxx
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default BQ25890_BOOST_LIM_1400mA (011)
 */
HAL_StatusTypeDef BQ25890_GetBoostCurrentLimit(BQ25890_BOOST_LIM* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0A, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BOOST_LIM_MASK) >> BQ25890_BOOST_LIM_BIT;
    return status;
}

/**
 * @brief Set input current optimizer enable (ICO)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetInputCurrentOptimizer(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_ICO_EN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_ICO_EN_MASK, &temp);
}

/**
 * @brief Get input current optimizer enable (ICO)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetInputCurrentOptimizer(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_ICO_EN_MASK) >> BQ25890_ICO_EN_BIT;
    return status;
}

/**
 * @brief Set high voltage DCP enable (HVDCP_EN)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetHighVoltageDCP(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_HVDCP_EN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_HVDCP_EN_MASK, &temp);
}

/**
 * @brief Get high voltage DCP enable (HVDCP_EN)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetHighVoltageDCP(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_HVDCP_EN_MASK) >> BQ25890_HVDCP_EN_BIT;
    return status;
}

/**
 * @brief Set MaxCharge adapter enable (MAXC_EN)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetMaxCharge(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_MAXC_EN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_MAXC_EN_MASK, &temp);
}

/**
 * @brief Get MaxCharge adapter enable (MAXC_EN)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetMaxCharge(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_MAXC_EN_MASK) >> BQ25890_MAXC_EN_BIT;
    return status;
}

/**
 * @brief Set force D+/D- detection (FORCE_DPDM)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_SetForceDPDM(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_FORCE_DPDM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_FORCE_DPDM_MASK, &temp);
}

/**
 * @brief Get force D+/D- detection (FORCE_DPDM)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_GetForceDPDM(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_FORCE_DPDM_MASK) >> BQ25890_FORCE_DPDM_BIT;
    return status;
}

/**
 * @brief Set automatic D+/D- detection enable (AUTO_DPDM_EN)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetAutoDPDM(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_AUTO_DPDM_EN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_02, BQ25890_AUTO_DPDM_EN_MASK, &temp);
}

/**
 * @brief Get automatic D+/D- detection enable (AUTO_DPDM_EN)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetAutoDPDM(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_02, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_AUTO_DPDM_EN_MASK) >> BQ25890_AUTO_DPDM_EN_BIT;
    return status;
}

/**
 * @brief Set battery load enable (BAT_LOADEN)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_SetBatLoad(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_BAT_LOADEN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_03, BQ25890_BAT_LOADEN_MASK, &temp);
}

/**
 * @brief Get battery load enable (BAT_LOADEN)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_GetBatLoad(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_03, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BAT_LOADEN_MASK) >> BQ25890_BAT_LOADEN_BIT;
    return status;
}

/**
 * @brief Reset I2C watchdog timer (WD_RST)
 * @param Void #BQ25890_RESET (1) (Internally)
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @note Reverts back to 0 after timer reset
 */
HAL_StatusTypeDef BQ25890_ResetWatchdog(void) {
    uint8_t temp = BQ25890_RESET << BQ25890_WDT_RESET_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_03, BQ25890_WDT_RESET_MASK, &temp);
}

/**
 * @brief Set boost (OTG) mode configuration (OTG_CONFIG)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetOTGmode(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_OTG_CONFIG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_03, BQ25890_OTG_CONFIG_MASK, &temp);
}

/**
 * @brief Get boost (OTG) mode configuration (OTG_CONFIG)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetOTGmode(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_03, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_OTG_CONFIG_MASK) >> BQ25890_OTG_CONFIG_BIT;
    return status;
}

/**
 * @brief Set battery charging mode (CHG_CONFIG)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetChgMode(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_CHG_CONFIG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_03, BQ25890_CHG_CONFIG_MASK, &temp);
}

/**
 * @brief Get battery charging mode (CHG_CONFIG)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetChgMode(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_03, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_CHG_CONFIG_MASK) >> BQ25890_CHG_CONFIG_BIT;
    return status;
}

/**
 * @brief Set minimum system voltage limit (SYS_MIN[2-0])
 * @param[in] *voltage_mv Voltage in mV. Range: 3000mV - 3700mV with increment
 * of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 3500mV
 */
HAL_StatusTypeDef BQ25890_SetSysMinVoltage(uint16_t* voltage_mv) {
    uint8_t temp = (*voltage_mv - BQ25890_SYS_MINV_BASE) / BQ25890_SYS_MINV_LSB;
    temp <<= BQ25890_SYS_MINV_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_03, BQ25890_SYS_MINV_MASK, &temp);
}

/**
 * @brief Get minimum system voltage limit (SYS_MIN[2-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 3000mV - 3700mV with increment
 * of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 3500mV
 */
HAL_StatusTypeDef BQ25890_GetSysMinVoltage(uint16_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_03, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_SYS_MINV_MASK) >> BQ25890_SYS_MINV_BIT;
    *voltage_mv = (temp * BQ25890_SYS_MINV_LSB) + BQ25890_SYS_MINV_BASE;
    return status;
}

/**
 * @brief Set current pulse control mode (EN_PUMPX)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_SetCurrentPulseMode(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_EN_PUMPX_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_04, BQ25890_EN_PUMPX_MASK, &temp);
}

/**
 * @brief Get current pulse control mode (EN_PUMPX)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_GetCurrentPulseMode(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_04, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_EN_PUMPX_MASK) >> BQ25890_EN_PUMPX_BIT;
    return status;
}

/**
 * @brief Set fast charge current limit (ICHG[6-0])
 * @param[in] *current_ma Current in mA. Range: 0mA - 5056mA with increment of
 * 64mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 2048mA
 * @note ICHG = 0mA disables charge
 * @note ICHG > 5056mA is clamped to register value 5056mA
 */
HAL_StatusTypeDef BQ25890_SetFastChargeCurrent(uint16_t* current_ma) {
    uint8_t temp = (*current_ma - BQ25890_ICHG_BASE) / BQ25890_ICHG_LSB;
    temp <<= BQ25890_ICHG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_04, BQ25890_ICHG_MASK, &temp);
}

/**
 * @brief Get fast charge current limit (ICHG[6-0])
 * @param[out] *current_ma Current in mA. Range: 0mA - 5056mA with increment of
 * 64mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 2048mA
 * @note ICHG = 0mA disables charge
 * @note ICHG > 5056mA is clamped to register value 5056mA
 */
HAL_StatusTypeDef BQ25890_GetFastChargeCurrent(uint16_t* current_ma) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_04, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_ICHG_MASK) >> BQ25890_ICHG_BIT;
    *current_ma = (temp * BQ25890_ICHG_LSB) + BQ25890_ICHG_BASE;
    return status;
}

/**
 * @brief Set precharge current limit (IPRECHG[3-0])
 * @param[in] *current_ma Current in mA. Range: 64mA - 1024mA with increment of
 * 64mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 128mA
 */
HAL_StatusTypeDef BQ25890_SetPreChargeCurrent(uint16_t* current_ma) {
    uint8_t temp = (*current_ma - BQ25890_IPRECHG_BASE) / BQ25890_IPRECHG_LSB;
    temp <<= BQ25890_IPRECHG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_05, BQ25890_IPRECHG_MASK, &temp);
}

/**
 * @brief Get precharge current limit (IPRECHG[3-0])
 * @param[out] *current_ma Current in mA. Range: 64mA - 1024mA with increment of
 * 64mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 128mA
 */
HAL_StatusTypeDef BQ25890_GetPreChargeCurrent(uint16_t* current_ma) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_05, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_IPRECHG_MASK) >> BQ25890_IPRECHG_BIT;
    *current_ma = (temp * BQ25890_IPRECHG_LSB) + BQ25890_IPRECHG_BASE;
    return status;
}

/**
 * @brief Set termination current limit (ITERM[3-0])
 * @param[in] *current_ma Current in mA. Range: 64mA - 1024mA with increment of
 * 64mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 256mA
 */
HAL_StatusTypeDef BQ25890_SetTermChargeCurrent(uint16_t* current_ma) {
    uint8_t temp = (*current_ma - BQ25890_ITERM_BASE) / BQ25890_ITERM_LSB;
    temp <<= BQ25890_ITERM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_05, BQ25890_ITERM_MASK, &temp);
}

/**
 * @brief Get termination current limit (ITERM[3-0])
 * @param[out] *current_ma Current in mA. Range: 64mA - 1024mA with increment of
 * 64mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 256mA
 */
HAL_StatusTypeDef BQ25890_GetTermChargeCurrent(uint16_t* current_ma) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_05, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_ITERM_MASK) >> BQ25890_ITERM_BIT;
    *current_ma = (temp * BQ25890_ITERM_LSB) + BQ25890_ITERM_BASE;
    return status;
}

/**
 * @brief Set charge voltage limit (VREG[5-0])
 * @param[in] *voltage_mv Voltage in mV. Range: 3840mV - 4608mV with increment
 * of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 4208mV
 * @note Value clamped to 4608mV when VREG > 4608mV
 */
HAL_StatusTypeDef BQ25890_SetChargeVoltage(uint16_t* voltage_mv) {
    uint8_t temp = (*voltage_mv - BQ25890_VREG_BASE) / BQ25890_VREG_LSB;
    temp <<= BQ25890_VREG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_06, BQ25890_VREG_MASK, &temp);
}

/**
 * @brief Get charge voltage limit (VREG[5-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 3840mV - 4608mV with increment
 * of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST
 * @default 4208mV
 * @note Value clamped to 4608mV when VREG > 4608mV
 */
HAL_StatusTypeDef BQ25890_GetChargeVoltage(uint16_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_06, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_VREG_MASK) >> BQ25890_VREG_BIT;
    *voltage_mv = (temp * BQ25890_VREG_LSB) + BQ25890_VREG_BASE;
    return status;
}

/**
 * @brief Set battery precharge to fast charge threshold (BATLOWV)
 * @param[in] *state #BQ25890_BATLOWV_2800MV or #BQ25890_BATLOWV_3000MV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_BATLOWV_3000MV (1)
 */
HAL_StatusTypeDef BQ25890_SetPreFastChargeTH(BQ25890_BATLOWV* state) {
    uint8_t temp = *state << BQ25890_BATLOWV_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_06, BQ25890_BATLOWV_MASK, &temp);
}

/**
 * @brief Get battery precharge to fast charge threshold (BATLOWV)
 * @param[out] *state #BQ25890_BATLOWV_2800MV or #BQ25890_BATLOWV_3000MV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_BATLOWV_3000MV (1)
 */
HAL_StatusTypeDef BQ25890_GetPreFastChargeTH(BQ25890_BATLOWV* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_06, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BATLOWV_MASK) >> BQ25890_BATLOWV_BIT;
    return status;
}

/**
 * @brief Set battery recharge threshold offset, below charge voltage limit
 * (VRECHG)
 * @param[in] *state #BQ25890_VRECHG_100MV or #BQ25890_VRECHG_200MV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_VRECHG_100MV (0)
 */
HAL_StatusTypeDef BQ25890_SetRechargeThOffset(BQ25890_VRECHG* state) {
    uint8_t temp = *state << BQ25890_VRECHG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_06, BQ25890_VRECHG_MASK, &temp);
}

/**
 * @brief Get battery recharge threshold offset, below charge voltage limit
 * (VRECHG)
 * @param[out] *state #BQ25890_VRECHG_100MV or #BQ25890_VRECHG_200MV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_VRECHG_100MV (0)
 */
HAL_StatusTypeDef BQ25890_GetRechargeThOffset(BQ25890_VRECHG* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_06, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_VRECHG_MASK) >> BQ25890_VRECHG_BIT;
    return status;
}

/**
 * @brief Set charging termination mode (EN_TERM)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetChargingTermination(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_EN_TERM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_07, BQ25890_EN_TERM_MASK, &temp);
}

/**
 * @brief Get charging termination mode (EN_TERM)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetChargingTermination(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_07, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_EN_TERM_MASK) >> BQ25890_EN_TERM_BIT;
    return status;
}

/**
 * @brief Set STAT pin function mode (STAT_DIS)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (0)
 * @note *state is internally inverted to become #BQ25890_ENABLED (0)
 */
HAL_StatusTypeDef BQ25890_SetSTATPinMode(BQ25890_STATE* state) {
    uint8_t temp = (!(*state)) << BQ25890_STAT_DIS_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_07, BQ25890_STAT_DIS_MASK, &temp);
}

/**
 * @brief Get STAT pin function mode (STAT_DIS)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (0)
 * @note *state is internally inverted to become #BQ25890_ENABLED (0)
 */
HAL_StatusTypeDef BQ25890_GetSTATPinMode(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_07, &temp);
    if (status != HAL_OK)
        return status;
    *state = !((temp & BQ25890_STAT_DIS_MASK) >> BQ25890_STAT_DIS_BIT);
    return status;
}

/**
 * @brief Set I2C watchdog timer (WATCHDOG[1-0])
 * @param[in] *state #BQ25890_WATCHDOG_DISABLE, #BQ25890_WATCHDOG_40S,
 * #BQ25890_WATCHDOG_80S or #BQ25890_WATCHDOG_160S
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_WATCHDOG_40S (01)
 */
HAL_StatusTypeDef BQ25890_SetWatchdogTimer(BQ25890_WATCHDOG* state) {
    uint8_t temp = *state << BQ25890_WATCHDOG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_07, BQ25890_WATCHDOG_MASK, &temp);
}

/**
 * @brief Get I2C watchdog timer (WATCHDOG[1-0])
 * @param[out] *state #BQ25890_WATCHDOG_DISABLE, #BQ25890_WATCHDOG_40S,
 * #BQ25890_WATCHDOG_80S or #BQ25890_WATCHDOG_160S
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_WATCHDOG_40S (01)
 */
HAL_StatusTypeDef BQ25890_GetWatchdogTimer(BQ25890_WATCHDOG* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_07, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_WATCHDOG_MASK) >> BQ25890_WATCHDOG_BIT;
    return status;
}

/**
 * @brief Set charging safety timer mode (EN_TIMER)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetSafetyTimer(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_EN_TIMER_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_07, BQ25890_EN_TIMER_MASK, &temp);
}

/**
 * @brief Get charging safety timer mode (EN_TIMER)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetSafetyTimer(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_07, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_EN_TIMER_MASK) >> BQ25890_EN_TIMER_BIT;
    return status;
}

/**
 * @brief Set fast charging timer (CHG_TIMER[1-0])
 * @param[in] *state #BQ25890_CHG_TIMER_5HOURS, #BQ25890_CHG_TIMER_8HOURS,
 * #BQ25890_CHG_TIMER_12HOURS or #BQ25890_CHG_TIMER_20HOURS
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_CHG_TIMER_12HOURS (10)
 */
HAL_StatusTypeDef BQ25890_SetFastChargeTimer(BQ25890_CHG_TIMER* state) {
    uint8_t temp = *state << BQ25890_CHG_TIMER_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_07, BQ25890_CHG_TIMER_MASK, &temp);
}

/**
 * @brief Get fast charging timer (CHG_TIMER[1-0])
 * @param[out] *state #BQ25890_CHG_TIMER_5HOURS, #BQ25890_CHG_TIMER_8HOURS,
 * #BQ25890_CHG_TIMER_12HOURS or #BQ25890_CHG_TIMER_20HOURS
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_CHG_TIMER_12HOURS (10)
 */
HAL_StatusTypeDef BQ25890_GetFastChargeTimer(BQ25890_CHG_TIMER* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_07, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_CHG_TIMER_MASK) >> BQ25890_CHG_TIMER_BIT;
    return status;
}

/**
 * @brief Set IR compensation resistor setting (BAT_COMP[2-0])
 * @param[in] *ohms_mohm Resistance in mOhm. Range: 0mΩ - 140mΩ with increment
 * of 20mΩ
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 0Ω (i.e. Disable IRComp)
 */
HAL_StatusTypeDef BQ25890_SetIRCompResistance(uint8_t* ohms_mohm) {
    uint8_t temp = (*ohms_mohm - BQ25890_BAT_COMP_BASE) / BQ25890_BAT_COMP_LSB;
    temp <<= BQ25890_BAT_COMP_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_08, BQ25890_BAT_COMP_MASK, &temp);
}

/**
 * @brief Get IR compensation resistor setting (BAT_COMP[2-0])
 * @param[out] *ohms_mohm Resistance in mOhm. Range: 0mΩ - 140mΩ with increment
 * of 20mΩ
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 0Ω (i.e. Disable IRComp)
 */
HAL_StatusTypeDef BQ25890_GetIRCompResistance(uint8_t* ohms_mohm) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_08, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_BAT_COMP_MASK) >> BQ25890_BAT_COMP_BIT;
    *ohms_mohm = (temp * BQ25890_BAT_COMP_LSB) + BQ25890_BAT_COMP_BASE;
    return status;
}

/**
 * @brief Set IR compensation voltage clamp (VCLAMP[2-0])
 * @param[in] *voltage_mv Voltage in mV. Range: 0mΩ - 224mV with increment of
 * 32mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 0mV
 */
HAL_StatusTypeDef BQ25890_SetIRCompVoltage(uint8_t* voltage_mv) {
    uint8_t temp = (*voltage_mv - BQ25890_VCLAMP_BASE) / BQ25890_VCLAMP_LSB;
    temp <<= BQ25890_VCLAMP_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_08, BQ25890_VCLAMP_MASK, &temp);
}

/**
 * @brief Get IR compensation voltage clamp (VCLAMP[2-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 0mΩ - 224mV with increment of
 * 32mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 0mV
 */
HAL_StatusTypeDef BQ25890_GetIRCompVoltage(uint8_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_08, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_VCLAMP_MASK) >> BQ25890_VCLAMP_BIT;
    *voltage_mv = (temp * BQ25890_VCLAMP_LSB) + BQ25890_VCLAMP_BASE;
    return status;
}

/**
 * @brief Set thermal regulation threshold (TREG[1-0])
 * @param[in] *threshold #BQ25890_TREG_60C, #BQ25890_TREG_80C,
 * #BQ25890_TREG_100C or #BQ25890_TREG_120C
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_TREG_120C (11)
 */
HAL_StatusTypeDef BQ25890_SetThermalRegulationTH(BQ25890_TREG* threshold) {
    uint8_t temp = *threshold << BQ25890_TREG_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_08, BQ25890_TREG_MASK, &temp);
}

/**
 * @brief Get thermal regulation threshold (TREG[1-0])
 * @param[out] *threshold #BQ25890_TREG_60C, #BQ25890_TREG_80C,
 * #BQ25890_TREG_100C or #BQ25890_TREG_120C
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_TREG_120C (11)
 */
HAL_StatusTypeDef BQ25890_GetThermalRegulationTH(BQ25890_TREG* threshold) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_08, &temp);
    if (status != HAL_OK)
        return status;
    *threshold = (temp & BQ25890_TREG_MASK) >> BQ25890_TREG_BIT;
    return status;
}

/**
 * @brief Force start input current optimizer (FORCE_ICO)
 * @param Void #BQ25890_RESET (1) (Internally)
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by REG_RST, by Watchdog
 * @note This bit can only be set and always returns to 0 after ICO starts
 */
HAL_StatusTypeDef BQ25890_ForceICO(void) {
    uint8_t temp = BQ25890_RESET << BQ25890_FORCE_ICO_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_09, BQ25890_FORCE_ICO_MASK, &temp);
}

/**
 * @brief Set 2x safety timer setting during DPM or thermal regulation
 * (TMR2X_EN)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_SetDPM2xSafetyTimer(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_TMR2X_EN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_09, BQ25890_TMR2X_EN_MASK, &temp);
}

/**
 * @brief Get the state of 2x safety timer setting during DPM or thermal
 * regulation (TMR2X_EN)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_ENABLED (1)
 */
HAL_StatusTypeDef BQ25890_GetDPM2xSafetyTimer(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_09, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_TMR2X_EN_MASK) >> BQ25890_TMR2X_EN_BIT;
    return status;
}

/**
 * @brief Set BATFET off to enable ship mode (BATFET_DIS)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_SetShipMode(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_BATFET_DIS_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_09, BQ25890_BATFET_DIS_MASK, &temp);
}

/**
 * @brief Get the ship mode status (BATFET_DIS)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_DISABLED (0)
 */
HAL_StatusTypeDef BQ25890_GetShipMode(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_09, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BATFET_DIS_MASK) >> BQ25890_BATFET_DIS_BIT;
    return status;
}

/**
 * @brief Set BATFET turn off delay control (BATFET_DLY)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_DISABLED (0)
 * @note When enabled, the BATFET is turned off after a delay of 10 - 15
 * seconds.
 */
HAL_StatusTypeDef BQ25890_SetShipModeDelay(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_BATFET_DLY_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_09, BQ25890_BATFET_DLY_MASK, &temp);
}

/**
 * @brief Get the status of BATFET turn off delay control (BATFET_DLY)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_DISABLED (0)
 * @note When enabled, the BATFET is turned off after a delay of 10 - 15
 * seconds.
 */
HAL_StatusTypeDef BQ25890_GetShipModeDelay(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_09, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BATFET_DLY_MASK) >> BQ25890_BATFET_DLY_BIT;
    return status;
}

/**
 * @brief Set BATFET functions as a load reset switch (BATFET_DLY)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_ENABLED (1)
 * @note It only works when the system is powered through battery and when input
 * source is not plugged-in
 */
HAL_StatusTypeDef BQ25890_SetSystemResetFunction(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_BATFET_RST_EN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_09, BQ25890_BATFET_RST_EN_MASK,
                              &temp);
}

/**
 * @brief Get BATFET functions as a load reset switch status (BATFET_DLY)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_ENABLED (1)
 * @note It only works when the system is powered through battery and when input
 * source is not plugged-in
 */
HAL_StatusTypeDef BQ25890_GetSystemResetFunction(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_09, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BATFET_RST_EN_MASK) >> BQ25890_BATFET_RST_EN_BIT;
    return status;
}

/**
 * @brief Set current pulse control voltage up enable (PUMPX_UP)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_DISABLED (0)
 * @note This bit is can only be set when EN_PUMPX bit is set and returns to 0
 * after current pulse control sequence is completed
 */
HAL_StatusTypeDef BQ25890_SetCurrentPulseVoltageUp(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_PUMPX_UP_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_09, BQ25890_PUMPX_UP_MASK, &temp);
}

/**
 * @brief Get current pulse control voltage up enable sttaus (PUMPX_UP)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_DISABLED (0)
 * @note This bit is can only be set when EN_PUMPX bit is set and returns to 0
 * after current pulse control sequence is completed
 */
HAL_StatusTypeDef BQ25890_GetCurrentPulseVoltageUp(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_09, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_PUMPX_UP_MASK) >> BQ25890_PUMPX_UP_BIT;
    return status;
}

/**
 * @brief Set current pulse control voltage down enable (PUMPX_DN)
 * @param[in] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_DISABLED (0)
 * @note This bit is can only be set when EN_PUMPX bit is set and returns to 0
 * after current pulse control sequence is completed
 */
HAL_StatusTypeDef BQ25890_SetCurrentPulseVoltageDown(BQ25890_STATE* state) {
    uint8_t temp = *state << BQ25890_PUMPX_DN_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_09, BQ25890_PUMPX_DN_MASK, &temp);
}

/**
 * @brief Get current pulse control voltage down enable status (PUMPX_DN)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default #BQ25890_DISABLED (0)
 * @note This bit is can only be set when EN_PUMPX bit is set and returns to 0
 * after current pulse control sequence is completed
 */
HAL_StatusTypeDef BQ25890_GetCurrentPulseVoltageDown(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_09, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_PUMPX_DN_MASK) >> BQ25890_PUMPX_DN_BIT;
    return status;
}

/**
 * @brief Set boost mode voltage regulation (BOOSTV[3-0])
 * @param[in] *voltage_mv Voltage in mV. Range: 4550mV - 5510mV with increment
 * of 64mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 5126mV
 */
HAL_StatusTypeDef BQ25890_SetBoostModeVoltage(uint16_t* voltage_mv) {
    uint8_t temp = (*voltage_mv - BQ25890_BOOSTV_BASE) / BQ25890_BOOSTV_LSB;
    temp <<= BQ25890_BOOSTV_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_0A, BQ25890_BOOSTV_MASK, &temp);
}

/**
 * @brief Get boost mode voltage regulation (BOOSTV[3-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 4550mV - 5510mV with increment
 * of 64mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software, by Watchdog
 * @default 5126mV
 */
HAL_StatusTypeDef BQ25890_GetBoostModeVoltage(uint16_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0A, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_BOOSTV_MASK) >> BQ25890_BOOSTV_BIT;
    *voltage_mv = (temp * BQ25890_BOOSTV_LSB) + BQ25890_BOOSTV_BASE;
    return status;
}

/**
 * @brief Get VBUS status (VBUS_STAT[2-0])
 * @param[out] *state #BQ25890_NO_INPUT, #BQ25890_USB_SDP, #BQ25890_USB_CDP,
 * #BQ25890_USB_DCP, #BQ25890_MAX_CHARGE_DCP, #BQ25890_UNKNOWN,
 * #BQ25890_NON_STANDARD or #BQ25890_OTG
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @note Software current limit is reported in IINLIM register
 */
HAL_StatusTypeDef BQ25890_GetVBUSStatus(BQ25890_VBUS_STAT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0B, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_VBUS_STAT_MASK) >> BQ25890_VBUS_STAT_BIT;
    return status;
}

/**
 * @brief Get charging status (CHRG_STAT[1-0])
 * @param[out] *state #BQ25890_NOT_CHARGING, #BQ25890_PRE_CHARGE,
 * #BQ25890_FAST_CHARGE or #BQ25890_CHARGE_TERMINATION
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetChargingStatus(BQ25890_CHRG_STAT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0B, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_CHRG_STAT_MASK) >> BQ25890_CHRG_STAT_BIT;
    return status;
}

/**
 * @brief Get power good status (PG_STAT)
 * @param[out] *state #BQ25890_NO_POWER_GOOD or #BQ25890_POWER_GOOD
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetPowerGoodStatus(BQ25890_PG_STAT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0B, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_PG_STAT_MASK) >> BQ25890_PG_STAT_BIT;
    return status;
}

/**
 * @brief Get VSYS regulation status (VSYS_STAT)
 * @param[out] *state #BQ25890_NO_REGULATION or #BQ25890_IN_REGULATION
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetVSYSRegulationStatus(BQ25890_VSYS_STAT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0B, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_VSYS_STAT_MASK) >> BQ25890_VSYS_STAT_BIT;
    return status;
}

/**
 * @brief Get watchdog fault status (WATCHDOG_FAULT)
 * @param[out] *state #BQ25890_FAULT_NORMAL or #BQ25890_FAULT
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @note #BQ25890_FAULT means watchdog timer has expired
 */
HAL_StatusTypeDef BQ25890_GetWatchdogFaultStatus(BQ25890_FAULT_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0C, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_WATCHDOG_FAULT_MASK) >> BQ25890_WATCHDOG_FAULT_BIT;
    return status;
}

/**
 * @brief Get boost mode fault status (BOOST_FAULT)
 * @param[out] *state #BQ25890_FAULT_NORMAL or #BQ25890_FAULT
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @note #BQ25890_FAULT means VBUS overloaded in OTG, or VBUS OVP, or battery is
 * too low in boost mode
 */
HAL_StatusTypeDef BQ25890_GetBoostFaultStatus(BQ25890_FAULT_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0C, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BOOST_FAULT_MASK) >> BQ25890_BOOST_FAULT_BIT;
    return status;
}

/**
 * @brief Get charge fault status (CHRG_FAULT[1-0])
 * @param[out] *state #BQ25890_CHG_NORMAL, #BQ25890_INPUT_FAULT,
 * #BQ25890_THERMAL_SHUTDOWN or #BQ25890_SAFETY_TIMER
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetChargeFaultStatus(BQ25890_CHRG_FAULT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0C, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_CHRG_FAULT_MASK) >> BQ25890_CHRG_FAULT_BIT;
    return status;
}

/**
 * @brief Get battery fault status (BAT_FAULT)
 * @param[out] *state #BQ25890_FAULT_NORMAL or #BQ25890_FAULT
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @note #BQ25890_FAULT means VBAT > VBATOVP i.e., 104%
 */
HAL_StatusTypeDef BQ25890_GetBatteryFaultStatus(BQ25890_FAULT_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0C, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_BAT_FAULT_MASK) >> BQ25890_BAT_FAULT_BIT;
    return status;
}

/**
 * @brief Get NTC fault status (NTC_FAULT[2-0])
 * @param[out] *state #BQ25890_NTC_NORMAL, #BQ25890_BUCK_TS_COLD,
 * #BQ25890_BUCK_TS_HOT, #BQ25890_BOOST_TS_COLD or #BQ25890_BOOST_TS_HOT
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetNTCFaultStatus(BQ25890_NTC_FAULT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0C, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_NTC_FAULT_MASK) >> BQ25890_NTC_FAULT_BIT;
    return status;
}

/**
 * @brief Set force VINDPM threshold setting method (FORCE_VINDPM)
 * @param[in] *state #BQ25890_RELATIVE_VINDPM or #BQ25890_ABSOLUTE_VINDPM
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_ABSOLUTE_VINDPM (1)
 */
HAL_StatusTypeDef BQ25890_SetForceVINDPM(BQ25890_FORCE_VINDPM* state) {
    uint8_t temp = *state << BQ25890_FORCE_VINDPM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_0D, BQ25890_FORCE_VINDPM_MASK, &temp);
}

/**
 * @brief Get force VINDPM threshold setting method (FORCE_VINDPM)
 * @param[out] *state #BQ25890_RELATIVE_VINDPM or #BQ25890_ABSOLUTE_VINDPM
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default #BQ25890_ABSOLUTE_VINDPM (1)
 */
HAL_StatusTypeDef BQ25890_GetForceVINDPM(BQ25890_FORCE_VINDPM* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0D, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_FORCE_VINDPM_MASK) >> BQ25890_FORCE_VINDPM_BIT;
    return status;
}

/**
 * @brief Set absolute VINDPM threshold (VINDPM[6-0])
 * @param[in] *voltage_mv Voltage in mV. Range: 3900mV - 15300mV with increment
 * of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default 4400mV
 * @note voltage_mv < 3900 is clamped to 3900V
 * @note Register is read only when FORCE_VINDPM=0 and can be written by
 * internal control based on relative VINDPM threshold setting. Register can be
 * read/write when FORCE_VINDPM = 1
 */
HAL_StatusTypeDef BQ25890_SetAbsoluteVINPDMTh(uint16_t* voltage_mv) {
    uint16_t temp = *voltage_mv;
    if (temp < 3900)
        temp = 3900;
    temp = (temp - BQ25890_VINDPM_BASE) / BQ25890_VINDPM_LSB;
    temp <<= BQ25890_VINDPM_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_0D, BQ25890_VINDPM_MASK,
                              (uint8_t*)&temp);
}

/**
 * @brief Get absolute VINDPM threshold (VINDPM[6-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 3900mV - 15300mV with increment
 * of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @reset by Software
 * @default 4400mV
 * @note If voltage_mv < 3900 the its clamped to 3900V
 * @note Register is read only when FORCE_VINDPM=0 and can be written by
 * internal control based on relative VINDPM threshold setting. Register can be
 * read/write when FORCE_VINDPM = 1
 */
HAL_StatusTypeDef BQ25890_GetAbsoluteVINPDMTh(uint16_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0D, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_VINDPM_MASK) >> BQ25890_VINDPM_BIT;
    *voltage_mv = (temp * BQ25890_VINDPM_LSB) + BQ25890_VINDPM_BASE;
    return status;
}

/**
 * @brief Get thermal regulation status (THERM_STAT)
 * @param[out] *state #BQ25890_NO_THERMAL_REGULATION or
 * #BQ25890_IN_THERMAL_REGULATION
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetThermalRegulationStatus(
    BQ25890_THERM_STAT* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0E, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_THERM_STAT_MASK) >> BQ25890_THERM_STAT_BIT;
    return status;
}

/**
 * @brief Get battery voltage (VBAT) (BATV[6-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 2304mV - 4848mV with increment
 * of 20mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @default 2304mV
 */
HAL_StatusTypeDef BQ25890_GetBatteryVoltage(uint16_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0E, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_BATV_MASK) >> BQ25890_BATV_BIT;
    *voltage_mv = (temp * BQ25890_BATV_LSB) + BQ25890_BATV_BASE;
    return status;
}

/**
 * @brief Get system voltage (VSYS) (SYSV[6-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 2304mV - 4848mV with increment
 * of 20mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @default 2304mV
 */
HAL_StatusTypeDef BQ25890_GetSystemVoltage(uint16_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_0F, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_SYSV_MASK) >> BQ25890_SYSV_BIT;
    *voltage_mv = (temp * BQ25890_SYSV_LSB) + BQ25890_SYSV_BASE;
    return status;
}

/**
 * @brief Get TS voltage (TS) as percentage of REGN (TSPCT[6-0])
 * @param[out] *percent Percentage in multiple of 100. Range: 21% (2100) - 80%
 * (8000)with increment of 0.465% (46)
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @default 21% (2100)
 * @note 0.465% LSB is rounded off to 0.46% to ease the calculation
 * @note The above round off introduces an error of -0.64% at full scale
 * compared to the float calculation.
 */
HAL_StatusTypeDef BQ25890_GetTSVoltage(uint16_t* percent) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_10, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_TSPCT_MASK) >> BQ25890_TSPCT_BIT;
    *percent = (*percent * BQ25890_TSPCT_LSB) + BQ25890_TSPCT_BASE;
    return status;
}

/**
 * @brief Get VBUS good status (VBUS_GD)
 * @param[out] *state #BQ25890_NO_VBUS or #BQ25890_VBUS_PRESENT
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetVBUSGoodStatus(BQ25890_VBUS_GD* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_11, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_VBUS_GD_MASK) >> BQ25890_VBUS_GD_BIT;
    return status;
}

/**
 * @brief Get VBUS voltage (VBUSV[6-0])
 * @param[out] *voltage_mv Voltage in mV. Range: 2600mV - 15300mV with increment
 * of 100mV
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @default 2600mV
 */
HAL_StatusTypeDef BQ25890_GetVBUSVoltage(uint16_t* voltage_mv) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_11, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_VBUSV_MASK) >> BQ25890_VBUSV_BIT;
    *voltage_mv = (temp * BQ25890_VBUSV_LSB) + BQ25890_VBUSV_BASE;
    return status;
}

/**
 * @brief Get charge current (ICHGR[6-0])
 * @param[out] *current_ma Current in mA. Range: 0mA - 6350mA with increment of
 * 50mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @default 0mA
 * @note This register returns 0000000 for VBAT < VBATSHORT
 */
HAL_StatusTypeDef BQ25890_GetChargeCurrent(uint16_t* current_ma) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_12, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_ICHGR_MASK) >> BQ25890_ICHGR_BIT;
    *current_ma = (temp * BQ25890_ICHGR_LSB) + BQ25890_ICHGR_BASE;
    return status;
}

/**
 * @brief Get VINDPM status (VDPM_STAT)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetVINDPMStatus(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_13, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_VDPM_STAT_MASK) >> BQ25890_VDPM_STAT_BIT;
    return status;
}

/**
 * @brief Get IINDPM status (IDPM_STAT)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetIINDPMStatus(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_13, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_IDPM_STAT_MASK) >> BQ25890_IDPM_STAT_BIT;
    return status;
}

/**
 * @brief Get input current limit in effect while Input Current Optimizer (ICO)
 * is enabled (IDPM_LIM[5-0])
 * @param[out] *current_ma Current in mA. Range: 100mA - 3250mA with increment
 * of 50mA
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetICO_IIDPMCurrent(uint16_t* current_ma) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_13, &temp);
    if (status != HAL_OK)
        return status;
    temp = (temp & BQ25890_IDPM_LIM_MASK) >> BQ25890_IDPM_LIM_BIT;
    *current_ma = (temp * BQ25890_IDPM_LIM_LSB) + BQ25890_IDPM_LIM_BASE;
    return status;
}

/**
 * @brief Register reset (REG_RST)
 * @param Void #BQ25890_RESET (1) (Internally)
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 * @note This bit can only be set and always returns to 0 after register reset
 * is completed
 */
HAL_StatusTypeDef BQ25890_ResetChip(void) {
    uint8_t temp = BQ25890_RESET << BQ25890_RESET_BIT;
    return BQ25890_UpdateBits(BQ25890_REG_14, BQ25890_RESET_MASK, &temp);
}

/**
 * @brief Get Input Current Optimizer (ICO) status (ICO_OPTIMIZED)
 * @param[out] *state #BQ25890_DISABLED or #BQ25890_ENABLED
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetICOStatus(BQ25890_STATE* state) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_14, &temp);
    if (status != HAL_OK)
        return status;
    *state = (temp & BQ25890_ICO_OPTIMIZED_MASK) >> BQ25890_ICO_OPTIMIZED_BIT;
    return status;
}

/**
 * @brief Get device configuration (PN[2-0])
 * @param[out] *state #DEVICE_BQ25890 or #DEVICE_BQ25892
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetDevice(BQ_DEVICE* device) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_14, &temp);
    if (status != HAL_OK)
        return status;
    *device = (temp & BQ25890_PN_MASK) >> BQ25890_PN_BIT;
    return status;
}

/**
 * @brief Get temperature profile (TS_PROFILE)
 * @param[out] *profile 0 = JEITA (default)
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetTSProfile(uint8_t* profile) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_14, &temp);
    if (status != HAL_OK)
        return status;
    *profile = (temp & BQ25890_TS_PROFILE_MASK) >> BQ25890_TS_PROFILE_BIT;
    return status;
}

/**
 * @brief Get device revision (DEV_REV)
 * @param[out] *rev 1 = Device Revision 01 (default)
 * @retval HAL_StatusTypeDef variable describing if it was successful or not
 */
HAL_StatusTypeDef BQ25890_GetDevRev(uint8_t* rev) {
    HAL_StatusTypeDef status;
    uint8_t temp;
    status = BQ25890_ReadRegister(BQ25890_REG_14, &temp);
    if (status != HAL_OK)
        return status;
    *rev = (temp & BQ25890_DEV_REV_MASK) >> BQ25890_DEV_REV_BIT;
    return status;
}

#ifdef __cplusplus
}
#endif
