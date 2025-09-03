/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: timer.h
 * 组件: df
 * 添加时间: 2025年 08月 26日 星期一 14:38:00 CST
 */

#ifndef DF_TIMER_H
#define DF_TIMER_H

#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef enum {
    DF_TIMER_BASE = OGS_MAX_NUM_OF_PROTO_TIMER,

    /* PFCP 相关定时器 */
    DF_TIMER_PFCP_ASSOCIATION,
    DF_TIMER_PFCP_NO_HEARTBEAT,

    /* 数据转发相关定时器 */
    DF_TIMER_DATA_FORWARD,
    DF_TIMER_DNF_DISCOVERY,

    MAX_NUM_OF_DF_TIMER,

} df_timer_e;

const char *df_timer_get_name(int timer_id);

/* PFCP 定时器函数 */
void df_timer_pfcp_association(void *data);
void df_timer_pfcp_no_heartbeat(void *data);

/* 数据转发定时器函数 */
void df_timer_data_forward(void *data);
void df_timer_dnf_discovery(void *data);



#ifdef __cplusplus
}
#endif

#endif /* DF_TIMER_H */
