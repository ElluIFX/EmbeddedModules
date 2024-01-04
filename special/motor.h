/**
 * @file motor.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 2.0
 * @date 2021-12-18
 *
 * THINK DIFFERENTLY
 */

#ifndef __MOTOR_H
#define __MOTOR_H
#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"
#include "pid.h"

/****************** 常量定义 ******************/
// 电机参数相关
#define SPEED_RATIO 30.0         // 齿轮组减速比
#define ENCODER_RESOLUTION 11.0  // 编码器线数
#define ENCODER_MULTIPLE 2.0  // 编码器倍频（个数*计数方式）（UP）
#define PULSE_PER_ROTATION \
  (ENCODER_RESOLUTION * SPEED_RATIO * ENCODER_MULTIPLE)  // 每圈编码器脉冲数
#define WHEEL_PERIMETER 0.42097                          // 轮子周长（m）
#define WHEEL_BASE 0.163                                 // 轮子轴距
#define PULSE_PER_METER (PULSE_PER_ROTATION / WHEEL_PERIMETER)  // 每米脉冲数
#define MOTOR_LAUNCH_PWM_DUTY 0  // 电机启动PWM占空比

// 功能相关
#define MOTOR_PWM_FREQ 21000              // 电机PWM频率
#define MOTOR_PWM_PRECISION 2000          // 电机PWM精度
#define MOTOR_TIM_CLOCK_FREQ_HZ 84000000  // TIM时钟频率
// 编码器TIM周期, 用于计算溢出 16bit:65535 32bit:4294967295
#define ENCODER_TIM_PERIOD 65535
#define ENCODER_MID_VALUE ENCODER_TIM_PERIOD / 2  // 编码器中值
#define SPEED_FILTER 0.2                          // 速度更新滤波系数

enum MOTOR_MODES {
  MOTOR_MODE_BRAKE = 0,    // 电机刹车模式
  MOTOR_MODE_SLIDE = 1,    // 电机滑行模式
  MOTOR_MODE_RUN_SPD = 2,  // 电机速度环模式
  MOTOR_MODE_RUN_POS = 3,  // 电机位置环模式
  MOTOR_MODE_MANUAL = 4    // 电机手动模式
};
/****************** 数据类型定义 ******************/

typedef struct {  // 电机闭环控制结构体
  enum MOTOR_MODES mode;  // 电机模式(0:刹车 1:滑行 2:速度环 3:位置环 4:手动)
  float speed;                    // 速度 (rpm)
  int32_t pos;                    // 位置 (pulse)
  int32_t lastPos;                // 上一次位置
  pid_t spdPID;                   // 速度环PID
  pid_t posPID;                   // 位置环PID
  float posTargetSpd;             // 位置环目标速度
  float pwmDuty;                  // PWM占空比
  TIM_HandleTypeDef *timEncoder;  // 编码器定时器
  uint8_t encoderReverse;         // 编码器反转
  TIM_HandleTypeDef *timPWM;      // PWM定时器
  uint32_t pwmChannel;            // PWM通道
  GPIO_TypeDef *forwardGPIOx;     // 正向GPIO
  uint16_t forwardGPIOpin;        // 正向GPIO引脚
  GPIO_TypeDef *reverseGPIOx;     // 反向GPIO
  uint16_t reverseGPIOpin;        // 反向GPIO引脚
} motor_t;

/****************** 带参宏定义 ******************/

// 设置电机模式
#define MOTOR_SET_MODE(motor, to_mode) motor.mode = to_mode

// 设置速度环速度（前提是没使能位置环）
#define MOTOR_SET_SPEED(motor, speed) motor.spdPID.setPoint = speed

// 设置速度环速度 m/s
#define MOTOR_SET_SPEED_MPS(motor, speed) \
  motor.spdPID.setPoint = ((speed) * 60 / WHEEL_PERIMETER)

#define MOTOR_GET_SPEED_MPS(motor) ((motor.speed) * WHEEL_PERIMETER / 60)

#define MOTOR_SET_POS_SPEED_MPS(motor, speed) \
  motor.posTargetSpd = ((speed) * 60 / WHEEL_PERIMETER)

// 设置位置环位置（以中立位为基准）
#define MOTOR_SET_POS(motor, pos) motor.posPID.setPoint = pos

// 获取当前位置（以中立位为基准）
#define MOTOR_GET_POS(motor) (motor.pos)

// 前进一定脉冲数（使能位置环）
#define MOTOR_GO_POS(motor, pos) motor.posPID.setPoint += pos

// 设置位置环角度（以中立位为基准）
#define MOTOR_SET_DEGREE(motor, degree) \
  motor.posPID.setPoint = (int32_t)((degree) * PULSE_PER_ROTATION / 360.0)

// 获取当前角度（以中立位为基准）
#define MOTOR_GET_DEGREE(motor) ((double)motor.pos * 360.0 / PULSE_PER_ROTATION)

// 重置中立位（以当前位置为中立位）
#define MOTOR_RESET(motor)                                    \
  motor.pos = 0;                                              \
  motor.lastPos = 0;                                          \
  __HAL_TIM_SET_COUNTER(motor.timEncoder, ENCODER_MID_VALUE); \
  motor.posPID.setPoint = 0;                                  \
  PID_ResetStartPoint(&motor.posPID, 0)

// 按角度前进(使能位置环)
#define MOTOR_GO_DEGREE(motor, degree) \
  motor.posPID.setPoint += (int32_t)((degree) * PULSE_PER_ROTATION / 360.0)

// 按米前进（使能位置环）
#define MOTOR_GO_METER(motor, meter) \
  motor.posPID.setPoint += meter * PULSE_PER_METER

// 输出停止（会越过32的PWM刹车功能）
#define MOTOR_STOP(motor) HAL_TIM_PWM_Stop(motor.timPWM, motor.pwmChannel)

// 输出启动
#define MOTOR_START(motor) HAL_TIM_PWM_Start(motor.timPWM, motor.pwmChannel)

// 清空双环PID的误差累计
#define CLEAR_MOTOR_PID_ERROR(motor)     \
  PID_ResetStartPoint(&motor.spdPID, 0); \
  PID_ResetStartPoint(&motor.posPID, motor.pos)

/****************** 函数声明 ******************/
/**
 * @brief  电机初始化
 * @param  motor 电机结构体
 */
extern void Motor_Setup(motor_t *motor);

/**
 * @brief  定时计算电机速度
 * @param  motor 电机结构体
 */
extern void Motor_Encoder_Tick(motor_t *motor, const float runTimeHz);

/**
 * @brief  电机运行
 * @param  motor 电机结构体
 */
extern void Motor_Run(motor_t *motor);

/**
 * @brief  计算双轮车的目标速度
 * @param  V              车辆线速度
 * @param  angular_velocity 车辆角速度
 * @param  target_rpm_left  目标左轮rpm
 * @param  target_rpm_right 目标右轮rpm
 */
void Two_Wheel_Speed_Calc(float V, float angular_velocity,
                          float *target_rpm_left, float *target_rpm_right);
#ifdef __cplusplus
}
#endif
#endif  // __MOTOR_H
