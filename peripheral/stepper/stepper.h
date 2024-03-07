/**
 * @file stepper.h
 * @brief see stepper.c
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2022-12-12
 *
 * THINK DIFFERENTLY
 */

#ifndef __STEPPER_H
#define __STEPPER_H
#ifdef __cplusplus
extern "C" {
#endif
#include <modules.h>

#if __has_include("tim.h")
#include "tim.h"

/****************** 常量定义 ******************/
// 电机参数相关
#define STEPPER_BASE_PULSE 200  // 零细分脉冲数
#define STEPPER_SUBDIVISION 32  // 细分倍数
#define STEPPER_PULSE_PER_ROUND \
  (STEPPER_BASE_PULSE * STEPPER_SUBDIVISION)  // 一圈脉冲数
#define STEPPER_DIR_LOGIC_REVERSE \
  0  // 反转方向控制逻辑(顺时针时DIR默认为高电平)

// 功能相关
#define STEPPER_TIM_BASE_CLK 240000000  // 定时器时钟频率
#define STEPPER_PWM_MAX_FREQ 20000      // PWM最大频率(防止丢步)
// 从定时器 16bit=65535 32bit=4294967295
#define STEPPER_SLAVE_TIM_MAX_CNT 65535

/****************** 数据类型定义 ******************/

typedef struct {                 // 步进电机控制结构体
  double speed;                  // 速度, deg/s
  double angle;                  // 当前角度, deg
  double angleTarget;            // 目标角度, deg
  uint8_t rotating;              // 是否正在转动
  uint8_t dir;                   // 转动方向 (0:逆时针, 1:顺时针)
  uint32_t slaveTimReload;       // 从定时器重装载值
  uint32_t slaveTimITCnt;        // 从定时器溢出中断计数
  TIM_HandleTypeDef *timMaster;  // 主定时器句柄(用于PWM输出)
  TIM_HandleTypeDef *timSlave;   // 从定时器句柄(用于脉冲计数)
  uint32_t timMasterCh;          // 主定时器通道
  GPIO_TypeDef *dirPort;         // 方向控制端口
  uint16_t dirPin;               // 方向控制引脚
  uint8_t dirLogic;              // 方向控制逻辑
} step_ctrl_t;

/****************** 宏函数声明 ******************/

#define __STEPPER_START_PWM(step) \
  HAL_TIM_PWM_Start_IT(step->timMaster, step->timMasterCh)
#define __STEPPER_STOP_PWM(step) \
  HAL_TIM_PWM_Stop_IT(step->timMaster, step->timMasterCh)

/****************** 函数声明 ******************/

extern void Stepper_Init(step_ctrl_t *step, TIM_HandleTypeDef *timMaster,
                         TIM_HandleTypeDef *timSlave, uint32_t timMasterCh,
                         GPIO_TypeDef *dirPort, uint16_t dirPin,
                         uint8_t dirLogic);
extern void Stepper_IT_Handler(step_ctrl_t *step, TIM_HandleTypeDef *htim);
extern void Stepper_Set_Speed(step_ctrl_t *step, double speed);
extern void Stepper_Rotate(step_ctrl_t *step, double angle);
extern void Stepper_Rotate_Abs(step_ctrl_t *step, double angle);
extern void Stepper_Set_Angle(step_ctrl_t *step, double angle);
extern double Stepper_Get_Angle(step_ctrl_t *step);
extern void Stepper_Stop(step_ctrl_t *step);

#endif

#ifdef __cplusplus
}
#endif
#endif
