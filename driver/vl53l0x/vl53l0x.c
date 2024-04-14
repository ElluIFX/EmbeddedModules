#include "vl53l0x.h"

#define LOG_MODULE "vl53l0x"
#include "log.h"

/*
    ST doesn't provide a good documentation.
    So use the open examples to make large ST's driver alive on tiny
   microcontollers.

    For some ideas many thanks to https://www.artfulbytes.com/vl53l0x-post
*/

/*
 * I2C interface - reference registers
 * The registers can be used to validate the user I2C interface.
 **/
#define REF_REG_0 (0xC0)
#define REF_REG_0_VAL (0xEE)
#define REF_REG_1 (0xC1)
#define REF_REG_1_VAL (0xAA)
#define REF_REG_2 (0xC2)
#define REF_REG_2_VAL (0x10)
#define REF_REG_3 (0x51)
#define REF_REG_3_VAL (0x0099)
#define REF_REG_4 (0x61)
#define REF_REG_4_VAL (0x0000)

/*
    The stop variable is used for initiating the stop sequence when doing range
   measurement. It's not obvious why this stop variable exists
*/
static uint8_t stop_variable = 0;

/* Register addresses from ST API file vl53l0x_device.h */
enum {
    SYSRANGE_START = 0x00,
    SYSTEM_THRESH_HIGH = 0x0C,
    SYSTEM_THRESH_LOW = 0x0E,
    SYSTEM_SEQUENCE_CONFIG = 0x01,
    SYSTEM_RANGE_CONFIG = 0x09,
    SYSTEM_INTERMEASUREMENT_PERIOD = 0x04,
    SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0A,
    GPIO_HV_MUX_ACTIVE_HIGH = 0x84,
    SYSTEM_INTERRUPT_CLEAR = 0x0B,
    RESULT_INTERRUPT_STATUS = 0x13,
    RESULT_RANGE_STATUS = 0x14,
    RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN = 0xBC,
    RESULT_CORE_RANGING_TOTAL_EVENTS_RTN = 0xC0,
    RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF = 0xD0,
    RESULT_CORE_RANGING_TOTAL_EVENTS_REF = 0xD4,
    RESULT_PEAK_SIGNAL_RATE_REF = 0xB6,
    ALGO_PART_TO_PART_RANGE_OFFSET_MM = 0x28,
    I2C_SLAVE_DEVICE_ADDRESS = 0x8A,
    MSRC_CONFIG_CONTROL = 0x60,
    PRE_RANGE_CONFIG_MIN_SNR = 0x27,
    PRE_RANGE_CONFIG_VALID_PHASE_LOW = 0x56,
    PRE_RANGE_CONFIG_VALID_PHASE_HIGH = 0x57,
    PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT = 0x64,
    FINAL_RANGE_CONFIG_MIN_SNR = 0x67,
    FINAL_RANGE_CONFIG_VALID_PHASE_LOW = 0x47,
    FINAL_RANGE_CONFIG_VALID_PHASE_HIGH = 0x48,
    FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44,
    PRE_RANGE_CONFIG_SIGMA_THRESH_HI = 0x61,
    PRE_RANGE_CONFIG_SIGMA_THRESH_LO = 0x62,
    PRE_RANGE_CONFIG_VCSEL_PERIOD = 0x50,
    PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x51,
    PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO = 0x52,
    SYSTEM_HISTOGRAM_BIN = 0x81,
    HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT = 0x33,
    HISTOGRAM_CONFIG_READOUT_CTRL = 0x55,
    FINAL_RANGE_CONFIG_VCSEL_PERIOD = 0x70,
    FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x71,
    FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO = 0x72,
    CROSSTALK_COMPENSATION_PEAK_RATE_MCPS = 0x20,
    MSRC_CONFIG_TIMEOUT_MACROP = 0x46,
    SOFT_RESET_GO2_SOFT_RESET_N = 0xBF,
    IDENTIFICATION_MODEL_ID = 0xC0,
    IDENTIFICATION_REVISION_ID = 0xC2,
    OSC_CALIBRATE_VAL = 0xF8,
    GLOBAL_CONFIG_VCSEL_WIDTH = 0x32,
    GLOBAL_CONFIG_SPAD_ENABLES_REF_0 = 0xB0,
    GLOBAL_CONFIG_SPAD_ENABLES_REF_1 = 0xB1,
    GLOBAL_CONFIG_SPAD_ENABLES_REF_2 = 0xB2,
    GLOBAL_CONFIG_SPAD_ENABLES_REF_3 = 0xB3,
    GLOBAL_CONFIG_SPAD_ENABLES_REF_4 = 0xB4,
    GLOBAL_CONFIG_SPAD_ENABLES_REF_5 = 0xB5,
    GLOBAL_CONFIG_REF_EN_START_SELECT = 0xB6,
    DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD = 0x4E,
    DYNAMIC_SPAD_REF_EN_START_OFFSET = 0x4F,
    POWER_MANAGEMENT_GO1_POWER_FORCE = 0x80,
    VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV = 0x89,
    ALGO_PHASECAL_LIM = 0x30,
    ALGO_PHASECAL_CONFIG_TIMEOUT = 0x30,
};

#define VL53L0X_VCSEL_PERIOD_PRE_RANGE \
    (0) /* Identifies the pre-range vcsel period. */
#define VL53L0X_VCSEL_PERIOD_FINAL_RANGE \
    (1) /* Identifies the final range vcsel period. */

#define RANGE_SEQUENCE_STEP_TCC (0x10)  /* Target CentreCheck */
#define RANGE_SEQUENCE_STEP_MSRC (0x04) /* Minimum Signal Rate Check */
#define RANGE_SEQUENCE_STEP_DSS (0x28)  /* Dynamic SPAD selection */
#define RANGE_SEQUENCE_STEP_PRE_RANGE (0x40)
#define RANGE_SEQUENCE_STEP_FINAL_RANGE (0x80)

#define VL53L0X_DEFAULT_MAX_LOOP 2000

/** mask existing bit in #VL53L0X_REG_SYSRANGE_START*/
#define VL53L0X_REG_SYSRANGE_MODE_MASK 0x0F
/** bit 0 in #VL53L0X_REG_SYSRANGE_START write 1 toggle state in
 * continuous mode and arm next shot in single shot mode
 */
#define VL53L0X_REG_SYSRANGE_MODE_START_STOP 0x01
/** bit 1 write 0 in #VL53L0X_REG_SYSRANGE_START set single shot mode */
#define VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT 0x00
/** bit 1 write 1 in #VL53L0X_REG_SYSRANGE_START set back-to-back
 *  operation mode
 */
#define VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK 0x02
/** bit 2 write 1 in #VL53L0X_REG_SYSRANGE_START set timed operation
 *  mode
 */
#define VL53L0X_REG_SYSRANGE_MODE_TIMED 0x04
/** bit 3 write 1 in #VL53L0X_REG_SYSRANGE_START set histogram operation
 *  mode
 */
#define VL53L0X_REG_SYSRANGE_MODE_HISTOGRAM 0x08

typedef enum {
    CALIBRATION_TYPE_VHV,
    CALIBRATION_TYPE_PHASE
} calibration_type_t;

typedef enum {
    VL53L0X_SINGLE_RANGING = 0,
    VL53L0X_CONTINUOUS_RANGING = 1,
    VL53L0X_SINGLE_RANGING_NO_POLLING,
} vl53l0x_measure_mode_t;

/* There are two types of SPAD: aperture and non-aperture. My understanding
 * is that aperture ones let it less light (they have a smaller opening),
 * similar to how you can change the aperture on a digital camera. Only 1/4 th
 * of the SPADs are of type non-aperture. */
#define SPAD_TYPE_APERTURE (0x01)
/* The total SPAD array is 16x16, but we can only activate a quadrant spanning
 * 44 SPADs at a time. In the ST api code they have (for some reason) selected
 * 0xB4 (180) as a starting point (lies in the middle and spans non-aperture
 * (3rd) quadrant and aperture (4th) quadrant). */
#define SPAD_START_SELECT (0xB4)
/* The total SPAD map is 16x16, but we should only activate an area of 44 SPADs
 * at a time. */
#define SPAD_MAX_COUNT (44)
/* The 44 SPADs are represented as 6 bytes where each bit represents a single
 * SPAD. 6x8 = 48, so the last four bits are unused. */
#define SPAD_MAP_ROW_COUNT (6)
#define SPAD_ROW_SIZE (8)
/* Since we start at 0xB4 (180), there are four quadrants (three aperture, one
 * aperture), and each quadrant contains 256 / 4 = 64 SPADs, and the third
 * quadrant is non-aperture, the offset to the aperture quadrant is (256 - 64 -
 * 180) = 12 */
#define SPAD_APERTURE_START_INDEX (12)

/**
 * Wait for strobe value to be set. This is used when we read values
 * from NVM (non volatile memory).
 *
 * Returns VL53L0X_OK if OK
 */
static int read_strobe(vl53l0x_dev_t* dev) {
    uint8_t strobe = 0;
    int timeout_cycles = 0;

    dev->ll->i2c_write_reg(dev->addr, 0x83, 0x00);

    /* polling using timeout to avoid deadlock */
    while (strobe == 0) {
        strobe = dev->ll->i2c_read_reg(dev->addr, 0x83);
        if (strobe != 0) {
            break;
        }

        dev->ll->delay_ms(2);
        if (timeout_cycles >= VL53L0X_DEFAULT_MAX_LOOP) {
            LOG_E("VL: read_strobe timeout");
            return VL53L0X_FAIL;
        }
        ++timeout_cycles;
    }

    dev->ll->i2c_write_reg(dev->addr, 0x83, 0x01);
    return VL53L0X_OK;
}

/**
 * Gets the spad count, spad type och "good" spad map stored by ST in NVM at
 * their production line.
 * .
 * According to the datasheet, ST runs a calibration (without cover glass) and
 * saves a "good" SPAD map to NVM (non volatile memory). The SPAD array has two
 * types of SPADs: aperture and non-aperture. By default, all of the
 * good SPADs are enabled, but we should only enable a subset of them to get
 * an optimized signal rate. We should also only enable either only aperture
 * or only non-aperture SPADs. The number of SPADs to enable and which type
 * are also saved during the calibration step at ST factory and can be retrieved
 * from NVM.
 */
static int get_spad_info_from_nvm(vl53l0x_dev_t* dev, uint8_t* spad_count,
                                  uint8_t* spad_type,
                                  uint8_t good_spad_map[6]) {
    uint8_t tmp_data8 = 0;
    uint32_t tmp_data32 = 0;

    /* Setup to read from NVM */
    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x06);
    tmp_data8 = dev->ll->i2c_read_reg(dev->addr, 0x83);
    dev->ll->i2c_write_reg(dev->addr, 0x83, tmp_data8 | 0x04);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x07);
    dev->ll->i2c_write_reg(dev->addr, 0x81, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x01);

    /* Get the SPAD count and type */
    dev->ll->i2c_write_reg(dev->addr, 0x94, 0x6b);

    if (read_strobe(dev) == VL53L0X_FAIL) {
        return VL53L0X_FAIL;
    }

    tmp_data32 = dev->ll->i2c_read_reg_32bit(dev->addr, 0x90);

    *spad_count = (tmp_data32 >> 8) & 0x7f;
    *spad_type = (tmp_data32 >> 15) & 0x01;

    /* Since the good SPAD map is already stored in
   * GLOBAL_CONFIG_SPAD_ENABLES_REF_0 we can simply read that register instead
   * of doing the below */
#if 0
    /* Get the first part of the SPAD map */
    dev->ll->i2c_write_reg(dev->addr,0x94, 0x24);

    if (read_strobe(dev) == VL53L0X_FAIL) {
        return VL53L0X_FAIL;
    }
    tmp_data32 = dev->ll->i2c_read_reg_32bit(dev->addr,0x90);

    good_spad_map[0] = (uint8_t)((tmp_data32 >> 24) & 0xFF);
    good_spad_map[1] = (uint8_t)((tmp_data32 >> 16) & 0xFF);
    good_spad_map[2] = (uint8_t)((tmp_data32 >> 8) & 0xFF);
    good_spad_map[3] = (uint8_t)(tmp_data32 & 0xFF);

    /* Get the second part of the SPAD map */
    dev->ll->i2c_write_reg(dev->addr,0x94, 0x25);

    if (read_strobe(dev) == VL53L0X_FAIL) {
        return VL53L0X_FAIL;
    }

    tmp_data32 = dev->ll->i2c_read_reg_32bit(dev->addr,0x90);

    good_spad_map[4] = (uint8_t)((tmp_data32 >> 24) & 0xFF);
    good_spad_map[5] = (uint8_t)((tmp_data32 >> 16) & 0xFF);

#endif

    /* Restore after reading from NVM */
    dev->ll->i2c_write_reg(dev->addr, 0x81, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x06);
    tmp_data8 = dev->ll->i2c_read_reg(dev->addr, 0x83);
    dev->ll->i2c_write_reg(dev->addr, 0x83, tmp_data8 & 0xfb);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x00);

    /* When we haven't configured the SPAD map yet, the SPAD map register actually
   * contains the good SPAD map, so we can retrieve it straight from this
   * register instead of reading it from the NVM. */
    dev->ll->i2c_read_reg_multi(dev->addr, GLOBAL_CONFIG_SPAD_ENABLES_REF_0,
                                good_spad_map, 6);

    return VL53L0X_OK;
}

/**
 * Sets the SPADs according to the value saved to NVM by ST during production.
 * Assuming similar conditions (e.g. no cover glass), this should give
 * reasonable readings and we can avoid running ref spad management (tedious
 * code).
 *
 * Returns VL53L0X_OK if success
 */
static int set_spads_from_nvm(vl53l0x_dev_t* dev) {
    uint8_t spad_map[SPAD_MAP_ROW_COUNT] = {0};
    uint8_t good_spad_map[SPAD_MAP_ROW_COUNT] = {0};
    uint8_t spads_enabled_count = 0;
    uint8_t spads_to_enable_count = 0;
    uint8_t spad_type = 0;
    volatile uint32_t total_val = 0;

    if (get_spad_info_from_nvm(dev, &spads_to_enable_count, &spad_type,
                               good_spad_map) == VL53L0X_FAIL) {
        LOG_E("VL: get_spad_info_from_nvm failed");
        return VL53L0X_FAIL;
    }

    for (int i = 0; i < 6; i++) {
        total_val += good_spad_map[i];
    }

    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    dev->ll->i2c_write_reg(dev->addr, DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD,
                           0x2C);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, GLOBAL_CONFIG_REF_EN_START_SELECT,
                           SPAD_START_SELECT);

    uint8_t offset =
        (spad_type == SPAD_TYPE_APERTURE) ? SPAD_APERTURE_START_INDEX : 0;

    /* Create a new SPAD array by selecting a subset of the SPADs suggested by the
   * good SPAD map. The subset should only have the number of type enabled as
   * suggested by the reading from the NVM (spads_to_enable_count and
   * spad_type). */
    for (int row = 0; row < SPAD_MAP_ROW_COUNT; row++) {
        for (int column = 0; column < SPAD_ROW_SIZE; column++) {
            int index = (row * SPAD_ROW_SIZE) + column;
            if (index >= SPAD_MAX_COUNT) {
                LOG_E("VL: set_spads_from_nvm: index out of bounds");
                return VL53L0X_FAIL;
            }
            if (spads_enabled_count == spads_to_enable_count) {
                /* We are done */
                break;
            }
            if (index < offset) {
                continue;
            }
            if ((good_spad_map[row] >> column) & 0x1) {
                spad_map[row] |= (1 << column);
                spads_enabled_count++;
            }
        }
        if (spads_enabled_count == spads_to_enable_count) {
            /* To avoid looping unnecessarily when we are already done. */
            break;
        }
    }

    if (spads_enabled_count != spads_to_enable_count) {
        LOG_E(
            "VL: set_spads_from_nvm: spads_enabled_count != "
            "spads_to_enable_count");
        return VL53L0X_FAIL;
    }

    /* Write the new SPAD configuration */
    dev->ll->i2c_write_reg_multi(dev->addr, GLOBAL_CONFIG_SPAD_ENABLES_REF_0,
                                 spad_map, SPAD_MAP_ROW_COUNT);

    return VL53L0X_OK;
}

/* Returns 0 if OK */
static int check_args(vl53l0x_dev_t* dev) {
    if (!dev || !dev->ll->delay_ms || !dev->ll->i2c_write_reg ||
        !dev->ll->i2c_write_reg_16bit || !dev->ll->i2c_write_reg_32bit ||
        !dev->ll->i2c_read_reg || !dev->ll->i2c_read_reg_16bit ||
        !dev->ll->i2c_read_reg_32bit) {
        return 1;
    }

    return 0;
}

/* Returns 0 if OK */
static int check_i2c_comm(vl53l0x_dev_t* dev) {
    int ret = 0;

    if (dev->ll->i2c_read_reg(dev->addr, REF_REG_0) != REF_REG_0_VAL) {
        ++ret;
    }

    if (dev->ll->i2c_read_reg(dev->addr, REF_REG_1) != REF_REG_1_VAL) {
        ++ret;
    }

    if (dev->ll->i2c_read_reg(dev->addr, REF_REG_2) != REF_REG_2_VAL) {
        ++ret;
    }

    if (dev->ll->i2c_read_reg_16bit(dev->addr, REF_REG_3) != REF_REG_3_VAL) {
        ++ret;
    }

    if (dev->ll->i2c_read_reg_16bit(dev->addr, REF_REG_4) != REF_REG_4_VAL) {
        ++ret;
    }
    return ret;
}

static int data_init(vl53l0x_dev_t* dev) {
    /* USE_I2C_2V8 */
    dev->ll->i2c_write_reg(
        dev->addr, VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV,
        (dev->ll->i2c_read_reg(dev->addr, VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV) &
         0xFE) |
            0x01);

    dev->ll->i2c_write_reg(dev->addr, 0x88,
                           0x00); /* Set I2C standard mode (400KHz) */

    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x00);
    stop_variable = dev->ll->i2c_read_reg(dev->addr, 0x91);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x00);

    return VL53L0X_OK;
}

static void set_sequence_steps_enabled(vl53l0x_dev_t* dev,
                                       uint8_t sequence_step) {
    dev->ll->i2c_write_reg(dev->addr, SYSTEM_SEQUENCE_CONFIG, sequence_step);
}

static int perform_single_ref_calibration(vl53l0x_dev_t* dev,
                                          calibration_type_t calib_type) {
    uint8_t interrupt_status = 0;
    uint8_t sysrange_start = 0;
    uint8_t sequence_config = 0;
    int timeout_cycles = 0;

    switch (calib_type) {
        case CALIBRATION_TYPE_VHV:
            sequence_config = 0x01;
            sysrange_start = 0x01 | 0x40;
            break;
        case CALIBRATION_TYPE_PHASE:
            sequence_config = 0x02;
            sysrange_start = 0x01 | 0x00;
            break;
    }

    dev->ll->i2c_write_reg(dev->addr, SYSTEM_SEQUENCE_CONFIG, sequence_config);
    dev->ll->i2c_write_reg(dev->addr, SYSRANGE_START, sysrange_start);

    /* Wait for interrupt */
    do {
        if (timeout_cycles >= VL53L0X_DEFAULT_MAX_LOOP) {
            LOG_E("VL: perform_single_ref_calibration timeout");
            return VL53L0X_FAIL;
        }
        ++timeout_cycles;
        interrupt_status =
            dev->ll->i2c_read_reg(dev->addr, RESULT_INTERRUPT_STATUS);
    } while ((interrupt_status & 0x07) == 0);

    dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CLEAR, 0x01);
    dev->ll->i2c_write_reg(dev->addr, SYSRANGE_START, 0x00);

    return VL53L0X_OK;
}

static int perform_ref_calibration(vl53l0x_dev_t* dev) {
    if (perform_single_ref_calibration(dev, CALIBRATION_TYPE_VHV)) {
        return 1;
    }

    if (perform_single_ref_calibration(dev, CALIBRATION_TYPE_PHASE)) {
        return 1;
    }

    /* restore the previous Sequence Config */
    set_sequence_steps_enabled(dev, RANGE_SEQUENCE_STEP_DSS +
                                        RANGE_SEQUENCE_STEP_PRE_RANGE +
                                        RANGE_SEQUENCE_STEP_FINAL_RANGE);

    return 0;
}

/* Return 0 if OK */
static int static_init(vl53l0x_dev_t* dev) {
    if (set_spads_from_nvm(dev) == VL53L0X_FAIL) {
        LOG_E("VL: set_spads_from_nvm failed");
        return VL53L0X_FAIL;
    }

    /* Same as default tuning settings provided by ST api code */
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x09, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x10, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x11, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x24, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x25, 0xFF);
    dev->ll->i2c_write_reg(dev->addr, 0x75, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x4E, 0x2C);
    dev->ll->i2c_write_reg(dev->addr, 0x48, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x30, 0x20);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x30, 0x09);
    dev->ll->i2c_write_reg(dev->addr, 0x54, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x31, 0x04);
    dev->ll->i2c_write_reg(dev->addr, 0x32, 0x03);
    dev->ll->i2c_write_reg(dev->addr, 0x40, 0x83);
    dev->ll->i2c_write_reg(dev->addr, 0x46, 0x25);
    dev->ll->i2c_write_reg(dev->addr, 0x60, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x27, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x50, 0x06);
    dev->ll->i2c_write_reg(dev->addr, 0x51, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x52, 0x96);
    dev->ll->i2c_write_reg(dev->addr, 0x56, 0x08);
    dev->ll->i2c_write_reg(dev->addr, 0x57, 0x30);
    dev->ll->i2c_write_reg(dev->addr, 0x61, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x62, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x64, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x65, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x66, 0xA0);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x22, 0x32);
    dev->ll->i2c_write_reg(dev->addr, 0x47, 0x14);
    dev->ll->i2c_write_reg(dev->addr, 0x49, 0xFF);
    dev->ll->i2c_write_reg(dev->addr, 0x4A, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x7A, 0x0A);
    dev->ll->i2c_write_reg(dev->addr, 0x7B, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x78, 0x21);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x23, 0x34);
    dev->ll->i2c_write_reg(dev->addr, 0x42, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x44, 0xFF);
    dev->ll->i2c_write_reg(dev->addr, 0x45, 0x26);
    dev->ll->i2c_write_reg(dev->addr, 0x46, 0x05);
    dev->ll->i2c_write_reg(dev->addr, 0x40, 0x40);
    dev->ll->i2c_write_reg(dev->addr, 0x0E, 0x06);
    dev->ll->i2c_write_reg(dev->addr, 0x20, 0x1A);
    dev->ll->i2c_write_reg(dev->addr, 0x43, 0x40);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x34, 0x03);
    dev->ll->i2c_write_reg(dev->addr, 0x35, 0x44);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x31, 0x04);
    dev->ll->i2c_write_reg(dev->addr, 0x4B, 0x09);
    dev->ll->i2c_write_reg(dev->addr, 0x4C, 0x05);
    dev->ll->i2c_write_reg(dev->addr, 0x4D, 0x04);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x44, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x45, 0x20);
    dev->ll->i2c_write_reg(dev->addr, 0x47, 0x08);
    dev->ll->i2c_write_reg(dev->addr, 0x48, 0x28);
    dev->ll->i2c_write_reg(dev->addr, 0x67, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x70, 0x04);
    dev->ll->i2c_write_reg(dev->addr, 0x71, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x72, 0xFE);
    dev->ll->i2c_write_reg(dev->addr, 0x76, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x77, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x0D, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x01, 0xF8);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x8E, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x00);

    /*
   * Set interrupt config to new sample ready
   * See VL53L0X_SetGpioConfig() ST API func.
   **/
    dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CONFIG_GPIO,
                           VL53L0X_GPIO_FUNC_NEW_MEASURE_READY);
    dev->ll->i2c_write_reg(
        dev->addr, GPIO_HV_MUX_ACTIVE_HIGH,
        dev->ll->i2c_read_reg(dev->addr, GPIO_HV_MUX_ACTIVE_HIGH) &
            ~0x10); /* polarity low */
    dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CLEAR, 0x01);

    dev->gpio_func = VL53L0X_GPIO_FUNC_NEW_MEASURE_READY;

    set_sequence_steps_enabled(dev, RANGE_SEQUENCE_STEP_DSS +
                                        RANGE_SEQUENCE_STEP_PRE_RANGE +
                                        RANGE_SEQUENCE_STEP_FINAL_RANGE);

    return VL53L0X_OK;
}

vl53l0x_ret_t vl53l0x_init(vl53l0x_dev_t* dev) {
    if (check_args(dev)) {
        LOG_E("VL: check_args failed");
        return VL53L0X_FAIL;
    }

    /* HW reset */
    if (dev->ll->xshut_reset && dev->ll->xshut_set) {
        dev->ll->delay_ms(100);
        vl53l0x_shutdown(dev);
        dev->ll->delay_ms(100);
        vl53l0x_power_up(dev);
        /* Wait 1.2 ms max (according to the spec) until vl53l0x fw boots */
        dev->ll->delay_ms(2);
    } else {
        vl53l0x_soft_reset(dev);
    }
    uint8_t ret;
    uint8_t retry = 3;
    while (--retry) {
        if ((ret = check_i2c_comm(dev)) >= 4) {
            LOG_E("VL: check_i2c_comm failed: %d", ret);
            vl53l0x_soft_reset(dev);
        } else {
            retry = 1;
            break;
        }
    }
    if (retry == 0) {
        LOG_E("VL: check_i2c_comm retry failed");
        return VL53L0X_FAIL;
    }

    if (data_init(dev)) {
        LOG_E("VL: data_init failed");
        return VL53L0X_FAIL;
    }

    if (static_init(dev)) {
        LOG_E("VL: static_init failed");
        return VL53L0X_FAIL;
    }

    /**
   * Temperature calibration needs to be run again if the temperature changes by
   * more than 8 degrees according to the datasheet.
   */
    if (perform_ref_calibration(dev)) {
        LOG_E("VL: perform_ref_calibration failed");
        return VL53L0X_FAIL;
    }

    return VL53L0X_OK;
}

void vl53l0x_shutdown(vl53l0x_dev_t* dev) {
    if (!dev->ll->xshut_set || !dev->ll->xshut_reset) {
        return;
    }

    dev->ll->xshut_reset();
}

void vl53l0x_power_up(vl53l0x_dev_t* dev) {
    if (!dev->ll->xshut_set || !dev->ll->xshut_reset) {
        return;
    }

    dev->ll->xshut_set();
}

void vl53l0x_soft_reset(vl53l0x_dev_t* dev) {
    uint8_t cnt = 0;
    dev->ll->i2c_write_reg(dev->addr, SOFT_RESET_GO2_SOFT_RESET_N, 0x00);
    while (dev->ll->i2c_read_reg(dev->addr, IDENTIFICATION_MODEL_ID) != 0x00 &&
           cnt++ < 10) {
        dev->ll->delay_ms(1);
    }
    dev->ll->delay_ms(3);
    dev->ll->i2c_write_reg(dev->addr, SOFT_RESET_GO2_SOFT_RESET_N, 0x01);
    cnt = 0;
    while (dev->ll->i2c_read_reg(dev->addr, IDENTIFICATION_MODEL_ID) == 0x00 &&
           cnt++ < 10)
        dev->ll->delay_ms(1);
    dev->ll->delay_ms(3);
}

vl53l0x_ret_t vl53l0x_do_measurement(vl53l0x_dev_t* dev,
                                     vl53l0x_measure_mode_t mode) {
    uint8_t sysrange_start = 0;
    uint8_t interrupt_status = 0;
    int timeout_cycles = 0;

    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x91, stop_variable);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x80, 0x00);

    switch (mode) {
        case VL53L0X_SINGLE_RANGING:
            dev->ll->i2c_write_reg(dev->addr, SYSRANGE_START,
                                   VL53L0X_REG_SYSRANGE_MODE_START_STOP);

            do {
                if (timeout_cycles >= VL53L0X_DEFAULT_MAX_LOOP) {
                    LOG_E("VL: vl53l0x_do_measurement timeout");
                    return VL53L0X_FAIL;
                }
                ++timeout_cycles;
                sysrange_start =
                    dev->ll->i2c_read_reg(dev->addr, SYSRANGE_START);
            } while (sysrange_start & 0x01);

            timeout_cycles = 0;
            if (dev->gpio_func == VL53L0X_GPIO_FUNC_OFF) {
                do {
                    if (timeout_cycles >= VL53L0X_DEFAULT_MAX_LOOP) {
                        LOG_E("VL: vl53l0x_do_measurement timeout 2");
                        return VL53L0X_FAIL;
                    }
                    ++timeout_cycles;

                    interrupt_status =
                        dev->ll->i2c_read_reg(dev->addr, RESULT_RANGE_STATUS);
                } while ((interrupt_status & 0x01) == 0);
            }

            timeout_cycles = 0;
            if (dev->gpio_func == VL53L0X_GPIO_FUNC_NEW_MEASURE_READY) {
                do {
                    if (timeout_cycles >= VL53L0X_DEFAULT_MAX_LOOP) {
                        LOG_E("VL: vl53l0x_do_measurement timeout 3");
                        return VL53L0X_FAIL;
                    }
                    ++timeout_cycles;

                    interrupt_status = dev->ll->i2c_read_reg(
                        dev->addr, RESULT_INTERRUPT_STATUS);
                } while ((interrupt_status & 0x07) == 0);
            }

            break;

        case VL53L0X_CONTINUOUS_RANGING:
            /* continuous back-to-back mode */
            dev->ll->i2c_write_reg(dev->addr, SYSRANGE_START,
                                   VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK);
            break;

        case VL53L0X_SINGLE_RANGING_NO_POLLING:
            dev->ll->i2c_write_reg(dev->addr, SYSRANGE_START,
                                   VL53L0X_REG_SYSRANGE_MODE_START_STOP);
            break;

        default:
            LOG_E("VL: vl53l0x_do_measurement unknown mode");
            return VL53L0X_FAIL;
    }

    return VL53L0X_OK;
}

vl53l0x_ret_t vl53l0x_stop_measurement(vl53l0x_dev_t* dev) {
    dev->ll->i2c_write_reg(dev->addr, SYSRANGE_START,
                           VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x91, 0x00);
    dev->ll->i2c_write_reg(dev->addr, 0x00, 0x01);
    dev->ll->i2c_write_reg(dev->addr, 0xFF, 0x00);

    return VL53L0X_OK;
}

vl53l0x_ret_t vl53l0x_activate_gpio_interrupt(vl53l0x_dev_t* dev,
                                              vl53l0x_gpio_func_t int_type) {
    /*
   * Set interrupt config to new sample ready
   * See VL53L0X_SetGpioConfig() ST API func.
   **/
    dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CONFIG_GPIO, int_type);
    dev->ll->i2c_write_reg(
        dev->addr, GPIO_HV_MUX_ACTIVE_HIGH,
        dev->ll->i2c_read_reg(dev->addr, GPIO_HV_MUX_ACTIVE_HIGH) &
            ~0x10); /* polarity low */
    dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CLEAR, 0x01);

    dev->gpio_func = int_type;

    return VL53L0X_OK;
}

vl53l0x_ret_t vl53l0x_deactivate_gpio_interrupt(vl53l0x_dev_t* dev) {
    dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CONFIG_GPIO,
                           VL53L0X_GPIO_FUNC_OFF);
    dev->gpio_func = VL53L0X_GPIO_FUNC_OFF;

    return VL53L0X_OK;
}

vl53l0x_ret_t vl53l0x_clear_flag_gpio_interrupt(vl53l0x_dev_t* dev) {
    uint8_t byte = 0xff;
    int cycles = 0;

    while ((byte & 0x07) != 0x00) {
        dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CLEAR, 0x01);
        dev->ll->i2c_write_reg(dev->addr, SYSTEM_INTERRUPT_CLEAR, 0x00);
        byte = dev->ll->i2c_read_reg(dev->addr, RESULT_INTERRUPT_STATUS);

        if (cycles >= VL53L0X_DEFAULT_MAX_LOOP) {
            LOG_E("VL: vl53l0x_clear_flag_gpio_interrupt timeout");
            return VL53L0X_FAIL;
        }
        ++cycles;
    }

    return VL53L0X_OK;
}

/* Based on VL53L0X_PerformSingleRangingMeasurement() */
vl53l0x_ret_t vl53l0x_read_in_oneshot_mode(vl53l0x_dev_t* dev,
                                           uint16_t* range) {
    vl53l0x_ret_t ret = VL53L0X_FAIL;

    /* VL53L0X_PerformSingleMeasurement */
    ret = vl53l0x_do_measurement(dev, VL53L0X_SINGLE_RANGING);

    /* Based on VL53L0X_GetRangingMeasurementData() */
    *range = dev->ll->i2c_read_reg_16bit(dev->addr, RESULT_RANGE_STATUS + 10);

    /* clear IRQ flag in vl53l0 status register */
    ret |= vl53l0x_clear_flag_gpio_interrupt(dev);

    return ret;
}

vl53l0x_ret_t vl53l0x_start_continuous_measurements(vl53l0x_dev_t* dev) {
    return vl53l0x_do_measurement(dev, VL53L0X_CONTINUOUS_RANGING);
}

vl53l0x_ret_t vl53l0x_start_single_measurements(vl53l0x_dev_t* dev) {
    return vl53l0x_do_measurement(dev, VL53L0X_SINGLE_RANGING_NO_POLLING);
}

vl53l0x_ret_t vl53l0x_get_range_mm(vl53l0x_dev_t* dev, uint16_t* range) {
    *range = dev->ll->i2c_read_reg_16bit(dev->addr, RESULT_RANGE_STATUS + 10);
    return VL53L0X_OK;
}
