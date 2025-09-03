/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: timer.h
 * 组件: dmf
 * 添加时间: 2025年 08月 26日 星期一 14:38:00 CST
 */

#ifndef DMF_TIMER_H
#define DMF_TIMER_H

#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef enum {
    DMF_TIMER_BASE = OGS_MAX_NUM_OF_PROTO_TIMER,

    /* gNB 管理相关定时器 */
    DMF_TIMER_GNB_SYNC,
    DMF_TIMER_GNB_HEARTBEAT,

    MAX_NUM_OF_DMF_TIMER,

} dmf_timer_e;

const char *dmf_timer_get_name(int timer_id);

/* gNB 管理定时器函数 */
void dmf_timer_gnb_sync(void *data);
void dmf_timer_gnb_heartbeat(void *data);

#ifdef __cplusplus
}
#endif

#endif /* DMF_TIMER_H */
