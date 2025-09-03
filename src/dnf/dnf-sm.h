/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dnf-sm.h
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#ifndef DNF_SM_H
#define DNF_SM_H

#include "ogs-core.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DNF 状态机函数 */
void dnf_state_initial(ogs_fsm_t *s, dnf_event_t *e);
void dnf_state_final(ogs_fsm_t *s, dnf_event_t *e);
void dnf_state_operational(ogs_fsm_t *s, dnf_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* DNF_SM_H */
