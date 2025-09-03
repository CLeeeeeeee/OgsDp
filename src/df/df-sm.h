/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: df-sm.h
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:04 CST
 */

#ifndef DF_SM_H
#define DF_SM_H

#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

void df_state_initial(ogs_fsm_t *s, df_event_t *e);
void df_state_final(ogs_fsm_t *s, df_event_t *e);
void df_state_operational(ogs_fsm_t *s, df_event_t *e);
void df_state_exception(ogs_fsm_t *s, df_event_t *e);

void df_pfcp_state_initial(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_final(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_will_associate(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_associated(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_exception(ogs_fsm_t *s, df_event_t *e);

#define df_sm_debug(__pe) \
    ogs_debug("%s(): %s", __func__, df_event_get_name(__pe))

#ifdef __cplusplus
}
#endif

#endif /* DF_SM_H */
