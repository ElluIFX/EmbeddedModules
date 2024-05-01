// 这个文件是从机模式的I2C的演示文件
// 模拟了一个256字节的EEPROM

#include "i2c_slave.h"

#define LOG_MODULE "slave demo"
#define LOG_LEVEL LOG_LEVEL_TRACE
#include "log.h"

#define SLAVE_INST I2C1

void main(void) {

    // balabala

    slave_i2c_init(SLAVE_INST);
    // slave_i2c_set_address(SLAVE_INST, 0x2C); // manual or what you set in CubeMX

    // balabala

    while (1) {
        // balabala
    }
}

static uint8_t ee_buf[256] = {0};
static uint8_t ee_addr = 0;
static enum {
    EE_IDLE,
    EE_WAITADDR,
    EE_OPERATION,
} ee_state = EE_IDLE;

void slave_i2c_transmit_begin_handler(I2C_TypeDef* I2Cx) {
    // 主机发送START信号时触发
    ee_state = EE_WAITADDR;
    LOG_TRACE("START");
}

void slave_i2c_transmit_end_handler(I2C_TypeDef* I2Cx) {
    // 主机发送STOP信号时触发 (重启数据传输时不会触发)
    ee_state = EE_IDLE;
    LOG_TRACE("STOP");
}

bool slave_i2c_transmit_in_handler(I2C_TypeDef* I2Cx, uint8_t in_data) {
    // 主机每写入一个字节数据时触发
    switch (ee_state) {
        case EE_WAITADDR:
            ee_addr = in_data;
            ee_state = EE_OPERATION;
            LOG_TRACE("ADDR: %X", ee_addr);
            break;
        case EE_OPERATION:
            ee_buf[ee_addr++] = in_data;
            LOG_TRACE("WR: %X", in_data);
            break;
        default:
            break;
    }

    return true;  // true: ACK, false: NACK
}

bool slave_i2c_transmit_out_handler(I2C_TypeDef* I2Cx, uint8_t* out_data) {
    // 主机每读取一个字节数据时触发
    *data = ee_buf[ee_addr++];
    LOG_TRACE("RD: %X", *out_data);

    return true;  // true: ACK, false: NACK
}
