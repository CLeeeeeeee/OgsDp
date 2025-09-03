/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dmf-sm.h
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:06 CST
 */

//
// Created by admin on 25-6-21.
//

#ifndef DMF_SM_H
#define DMF_SM_H

#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

void dmf_state_initial(ogs_fsm_t *s, dmf_event_t *e);
void dmf_state_final(ogs_fsm_t *s, dmf_event_t *e);
void dmf_state_operational(ogs_fsm_t *s, dmf_event_t *e);

#define dmf_sm_debug(__pe) \
    ogs_debug("%s(): %s", __func__, dmf_event_get_name(__pe))

#ifdef __cplusplus
}
#endif

#endif /* DMF_SM_H */

