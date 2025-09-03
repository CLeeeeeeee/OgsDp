/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: timer.h
 * 组件: dnf
 * 添加时间: 2025年 08月 26日 星期一 14:38:00 CST
 */

#ifndef DNF_TIMER_H
#define DNF_TIMER_H

#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef enum {
    DNF_TIMER_BASE = OGS_MAX_NUM_OF_PROTO_TIMER,

    /* 数据转发相关定时器 */
    DNF_TIMER_DATA_FORWARD,
    DNF_TIMER_SESSION_CLEANUP,

    MAX_NUM_OF_DNF_TIMER,

} dnf_timer_e;

const char *dnf_timer_get_name(int timer_id);

/* 数据转发定时器函数 */
void dnf_timer_data_forward(void *data);
void dnf_timer_session_cleanup(void *data);

#ifdef __cplusplus
}
#endif

#endif /* DNF_TIMER_H */
