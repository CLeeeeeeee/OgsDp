/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: ndf-handler.h
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:04 CST
 */

#ifndef DF_N4_HANDLER_H
#define DF_N4_HANDLER_H

#include "ogs-gtp.h"

#ifdef __cplusplus
extern "C" {
#endif

void df_n4_handle_session_establishment_request(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_establishment_request_t *req);
void df_n4_handle_session_modification_request(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_modification_request_t *req);
void df_n4_handle_session_deletion_request(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_deletion_request_t *req);

void df_n4_handle_session_report_response(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_report_response_t *rsp);

#ifdef __cplusplus
}
#endif

#endif /* DF_N4_HANDLER_H */ 