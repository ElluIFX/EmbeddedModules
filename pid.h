/**
 * @file pid.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-04-03
 *
 * THINK DIFFERENTLY
 */

/**
 * @note PID功能说明(pid_t结构体参数介绍)
 * 1.基础使用(必填参数):
 *  1.1.设定目标 setPoint, 采样周期 Ts: 与常规PID参数一致
 *  1.2.三个K值 proportion,integral,derivative: 与常规PID参数一致,
 *      结构体中没有反转系统的选项,将K值均设为负数既是负反馈
 *  1.3.基础值 base: 决定PID初始的*输出*值,无需求设为0即可,
 *      base值在调用PID_Reset_StartPoint(下述)时也会被更新
 *
 * 2.可选参数(这部分参数设为0即可不生效):
 *  2.1.误差死区 deadBand: 误差小于该值时维持输出稳态
 *  2.2.输出限幅 maxOutput,minOutput: 限制最终的输出,限幅状态通过limitFlag查看
 *  2.3.积分限幅 sumILimit: 限制积分项贡献大小,其大小单位和输出单位一致,
 *      一般用于调整时间较长的系统,但这些情况更推荐使用下述的积分分离或变速积分
 *
 * 3.积分计算模式 integMode:
 *  (三种模式积分常数均为integral,计算方式不同)
 *  3.1.常数积分: integMode=0,与常规PID一致
 *  3.2.积分分离: integMode=1,当|error|<=iModeK1时积分,否则不积分
 *  3.3.变速积分: integMode=2,当|error|>=iModeK1时积分常数为0,
 *      当|error|<iModeK1时积分常数从0开始线性变化,直至|error|<=iModeK2时积分常数等于integral
 *
 * 4.比例计算模式 propMode:
 *  (三种模式比例常数均为proportion,计算方式不同)
 *  4.1.常数比例: propMode=0,与常规PID一致
 *  4.2.PonM(基于测量值的比例): propMode=1,其计算时误差项为当前值减去系统初始值,
 *      用于主要依赖积分项进行控制的系统(如加热),可以减少超调,避免积分震荡
 *  4.3.自适应: propMode=2,其Kp=proportion*log(pModeK1*|error|+pModeK2),
 *      实现在误差较小时比例常数增大,误差较大时比例常数减小
 *
 * 5.初始化:
 *  小贴士: C99之后,一个结构体可以被以下面的格式初始化:
 *     pid_t motor_pid = {
 *     .setPoint = 12, // 指定你想设置的值
 *     .proportion = 34,
 *     .integral = 56,
 *     .derivative = 78,
 *     // 其他未指定参数均初始化为0, 在此方法下, 你可以不用调用PID_Reset()函数
 *     };
 *  掌握上述方法,就没必要写一个XXX_Init(k1,k2,...)函数了,十几二十个参数也太丑了
 *  或者(不推荐):
 *  定义一个结构体,想办法填入上述参数,随后调用PID_Reset()函数(或PID_Reset_StartPoint(),
 *  可以同时设定系统初始状态base) 来初始化PID结构体,传入参数为结构体指针
 *
 * 6.获取输出:
 *  调用PID_Calculate()函数即可返回给定PID的输出,结构体中存有其他输出内容:
 *  6.1.output: 即PID_Calculate()上一次的输出值
 *  6.2.error_0(error_1): 即上(上)一次的误差值
 *  6.3.sumI: 总的积分项贡献值
 *  6.4.sumP: PonM模式下的比例项贡献值
 *  6.5.lastPoint: 上一次的输入值
 *
 * 7.PID控制的暂停与平滑恢复:
 *  暂停本质上不需要做任何额外操作,不去调用PID_Calculate()更新数据即可,
 *  但恢复时,为了避免系统震荡,最好调用PID_Reset_StartPoint()函数,传入系统
 *  当前状态应对应的输出值,这样PID会平滑地从当前状态开始工作
 *
 * 8.更改PID参数:
 *  可以调用PID_Set_Tunings()或直接设置结构体中的参数,积分过程经过优化,任何参数
 *  的修改均不会导致输出突变,但积分/比例模式的变化会导致积分/比例项的突变,所以修改
 *  后需要调用PID_Reset_StartPoint()来保证平滑过渡
 *
 * 9.动态分配malloc()相关:
 *  pid_t为了兼容繁多的功能,内部有大量float,所以占用较大(92+),
 *  如果提供的堆内存(heap)较小,很容易malloc()失败,所以建议尽量
 *  定义为静态变量,而不是动态分配
 *
 * 10.PID_TimeAdaptive_Calculate():
 *  本质是通过获取系统时间来自动设置采样周期Ts,但其精度直接取决于系统时间的精度,
 *  比如HAL_GetTick()的精度为1ms,那么Ts的精度也只有1ms,因此更推荐使用定时器
 *  或调度器来固定间隔计算PID,保证积分/微分项的精度
 *
 * 11.PID_FeedForward_Calculate():
 *  前馈控制,本质是在PID输出前加入一个前馈项,用于提前预测输出,从而快速响应系统变化,
 *  但前馈项的计算需要知道系统的模型,因此需要用户根据控制系统自己实现
 *
 * 12.PID_QuickInc_Calculate():
 *  一个用于超高速计算的增量式PID,其计算过程中没有除法,函数调用内联,计算速度十倍于
 *  位置式PID,适用于超高频率(1000Hz+)的控制,但没有上述额外功能,所有参数编译期就被固定
 */

#ifndef __PID_H
#define __PID_H
#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"

typedef struct {  // PID结构体(位置式)
  /** 设置参数 **/
  float setPoint;    // 设定目标
  float proportion;  // 设定比例常数
  float integral;    // 设定积分常数
  float derivative;  // 设定微分常数
  float Ts;          // 设定采样周期(必须>0)
  float deadBand;    // 设定误差死区(>0生效)
  float maxOutput;   // 设定输出上限(max>min生效)
  float minOutput;   // 设定输出下限(max>min生效)
  float base;        // 设定基准值
  float sumILimit;   // 设定积分限幅(>0生效)
  uint8_t integMode;  // 设定积分计算模式:0:常数,1:积分分离,2:变速积分
  float iModeK1;  // 设定非常数积分系数1(分离阈值(>0)/变速积分K=0点(K1>K2>0))
  float iModeK2;  // 设定非常数积分系数2(None/变速积分K=1点(K1>K2>0))
  uint8_t propMode;  // 设定比例计算模式:0:常数,1:PonM,2:自适应
  float pModeK1;     // 设定自适应比例常数1(>0)
  float pModeK2;     // 设定自适应比例常数2(>0)
  /** 计算结果 **/
  float output;     // 输出值
  float error_0;    // error
  float error_1;    // error[-1]
  float sumI;       // 误差积分
  float sumP;       // 比例积分
  float lastPoint;  // 上次输入
  int limitFlag;    // 输出限幅标志(0:未限幅,1:上限,-1:下限)
} pid_t;

typedef struct {      // 时间自适应PID结构体
  pid_t pid;          // PID结构体
  m_time_t lastTime;  // 上次计算时间
} pid_tad_t;

/**
 * @brief 计算位置式PID
 * @retval 输出
 */
extern float PID_Calculate(pid_t *PIDx, float nextPoint);

/**
 * @brief 位置式PID(调度间隔自适应)
 * @retval 输出
 * @note 暂停->恢复时，pid->lastTime需要手动置为0
 */
extern float PID_TimeAdaptive_Calculate(pid_tad_t *PIDx, float nextPoint);

/**
 * @brief 前馈PID
 * @note 前馈环节的传递函数需根据实际情况整定，无通用模型
 */
extern float PID_FeedForward_Calculate(pid_t *PIDx, float nextPoint);

/**
 * @brief 快速增量式PID
 * @retval 增量输出
 * @note 该函数内联且不可重入，只能被单一控制器专用
 */
extern float PID_QuickInc_Calculate(float setPoint, float nextPoint);

/**
 * @brief 重置PID初始值并清空PID状态
 * @param  startPoint      PID初始值, 防止PID输出突变
 */
extern void PID_ResetStartPoint(pid_t *PIDx, float startPoint);

/**
 * @brief 清空PID状态
 */
extern void PID_Reset(pid_t *PIDx);

/**
 * @brief 设置PID参数
 */
extern void PID_SetTuning(pid_t *PIDx, float kp, float ki, float kd);

#ifdef __cplusplus
}
#endif

#endif  // __PID_H
