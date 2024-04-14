/**
 * @file motor.c
 * @brief 直流电机闭环控制器
 * @author Ellu (ellu.grif@gmail.com)
 * @version 3.0
 * @date 2023年4月29日
 *
 * THINK DIFFERENTLY
 */

#include "motor.h"

#include "macro.h"

#if __has_include("tim.h")

void Motor_Setup(motor_t* motor) {
    motor->mode = MOTOR_MODE_BRAKE;
    motor->pwmDuty = 0;
    motor->speed = 0;
    motor->pos = 0;
    motor->lastPos = 0;
    motor->posPID.setPoint = 0;
    motor->spdPID.setPoint = 0;
    motor->spdPID.maxOutput = 100;
    motor->spdPID.minOutput = -100;
    PID_ResetStartPoint(&motor->spdPID, 0);
    PID_ResetStartPoint(&motor->posPID, ENCODER_MID_VALUE);
    HAL_GPIO_WritePin(motor->forwardGPIOx, motor->forwardGPIOpin,
                      GPIO_PIN_RESET);
    HAL_GPIO_WritePin(motor->reverseGPIOx, motor->reverseGPIOpin,
                      GPIO_PIN_RESET);

    HAL_TIM_Encoder_Start(motor->timEncoder, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(motor->timEncoder, ENCODER_MID_VALUE);
    __HAL_TIM_SET_PRESCALER(
        motor->timPWM,
        (MOTOR_TIM_CLOCK_FREQ_HZ / MOTOR_PWM_PRECISION / MOTOR_PWM_FREQ) - 1);
    __HAL_TIM_SET_AUTORELOAD(motor->timPWM, MOTOR_PWM_PRECISION - 1);
    __HAL_TIM_SET_COMPARE(motor->timPWM, motor->pwmChannel, 0);
    HAL_TIM_Base_Start(motor->timPWM);
    HAL_TIM_PWM_Start(motor->timPWM, motor->pwmChannel);
}

void Motor_Encoder_Tick(motor_t* motor, const float runTimeHz) {
    float speed = 0;
    if (motor->encoderReverse)
        motor->pos += (int32_t)__HAL_TIM_GET_COUNTER(motor->timEncoder) -
                      ENCODER_MID_VALUE;
    else
        motor->pos -= (int32_t)__HAL_TIM_GET_COUNTER(motor->timEncoder) -
                      ENCODER_MID_VALUE;
    __HAL_TIM_SET_COUNTER(motor->timEncoder, ENCODER_MID_VALUE);
    speed = (float)(motor->pos - motor->lastPos) * 60.0 * runTimeHz /
            PULSE_PER_ROTATION;
    motor->lastPos = motor->pos;
    motor->speed += (speed - motor->speed) * SPEED_FILTER;
}

void Motor_Run(motor_t* motor) {
    float pwmDuty = 0;
    if (motor->mode == MOTOR_MODE_RUN_POS) {
        motor->posPID.maxOutput = motor->posTargetSpd;
        motor->posPID.minOutput = -motor->posTargetSpd;
        motor->spdPID.setPoint = PID_Calculate(&motor->posPID, motor->pos);
    } else {
        PID_ResetStartPoint(&motor->posPID, 0);
    }
    if (motor->mode == MOTOR_MODE_BRAKE) {
        motor->pwmDuty = 0;
        PID_ResetStartPoint(&motor->spdPID, 0);
    } else if (motor->mode == MOTOR_MODE_SLIDE) {
        motor->pwmDuty = 0;
        PID_ResetStartPoint(&motor->spdPID, 0);
        __HAL_TIM_SET_COMPARE(motor->timPWM, motor->pwmChannel, 0);
        HAL_GPIO_WritePin(motor->forwardGPIOx, motor->forwardGPIOpin,
                          GPIO_PIN_SET);
        HAL_GPIO_WritePin(motor->reverseGPIOx, motor->reverseGPIOpin,
                          GPIO_PIN_SET);
        return;
    } else if (motor->mode == MOTOR_MODE_MANUAL) {
        pwmDuty = motor->pwmDuty;
    } else {
        pwmDuty = PID_Calculate(&motor->spdPID, motor->speed);
#if MOTOR_LAUNCH_PWM_DUTY != 0
        if (pwmDuty > 0.1)
            pwmDuty = MAP(pwmDuty, 0, 100, MOTOR_LAUNCH_PWM_DUTY, 100);
        else if (pwmDuty < -0.1)
            pwmDuty = MAP(pwmDuty, -100, 0, -100, -MOTOR_LAUNCH_PWM_DUTY);
        else
            pwmDuty = 0;
#endif
    }
    if (pwmDuty > 0) {
        __HAL_TIM_SET_COMPARE(motor->timPWM, motor->pwmChannel,
                              pwmDuty * MOTOR_PWM_PRECISION / 100);
        HAL_GPIO_WritePin(motor->forwardGPIOx, motor->forwardGPIOpin,
                          GPIO_PIN_SET);
        HAL_GPIO_WritePin(motor->reverseGPIOx, motor->reverseGPIOpin,
                          GPIO_PIN_RESET);
    } else if (pwmDuty == 0) {
        __HAL_TIM_SET_COMPARE(motor->timPWM, motor->pwmChannel, 0);
        HAL_GPIO_WritePin(motor->forwardGPIOx, motor->forwardGPIOpin,
                          GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motor->reverseGPIOx, motor->reverseGPIOpin,
                          GPIO_PIN_RESET);
    } else {
        __HAL_TIM_SET_COMPARE(motor->timPWM, motor->pwmChannel,
                              -pwmDuty * MOTOR_PWM_PRECISION / 100);
        HAL_GPIO_WritePin(motor->forwardGPIOx, motor->forwardGPIOpin,
                          GPIO_PIN_RESET);
        HAL_GPIO_WritePin(motor->reverseGPIOx, motor->reverseGPIOpin,
                          GPIO_PIN_SET);
    }
    if (motor->mode != MOTOR_MODE_MANUAL)
        motor->pwmDuty = pwmDuty;
}

void Two_Wheel_Speed_Calc(float V, float angular_velocity,
                          float* target_rpm_left, float* target_rpm_right) {
    // 计算车辆线速度和角速度
    float V_vehicle = V;
    float angular_velocity_rad =
        angular_velocity * (3.14159265 / 180);  // 将角速度从度转换为弧度

    // 计算左右轮的目标线速度
    float V_left = V_vehicle + (angular_velocity_rad * WHEEL_BASE / 2);
    float V_right = V_vehicle - (angular_velocity_rad * WHEEL_BASE / 2);

    // 计算左右轮的目标角速度
    float omega_left = V_left / WHEEL_PERIMETER;
    float omega_right = V_right / WHEEL_PERIMETER;

    // 转换为rpm
    *target_rpm_left = omega_left * 60;
    *target_rpm_right = omega_right * 60;
}

#endif  // __has_include("tim.h")
