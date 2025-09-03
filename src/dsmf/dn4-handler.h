/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dn4-handler.h
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:09 CST
 */

#ifndef DSMF_DN4_HANDLER_H
#define DSMF_DN4_HANDLER_H

#include "ogs-pfcp.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

void dsmf_dn4_handle_message(ogs_pool_id_t xact_id, dsmf_event_t *e);
void dsmf_dn4_handle_timer(ogs_pool_id_t xact_id, dsmf_event_t *e);

/* 会话管理函数 */
void dsmf_dn4_handle_session_establishment(dsmf_sess_t *sess);
void dsmf_dn4_handle_session_modification(dsmf_sess_t *sess);
void dsmf_dn4_handle_session_deletion(dsmf_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_DN4_HANDLER_H */
