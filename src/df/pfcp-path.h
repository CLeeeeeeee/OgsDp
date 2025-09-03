/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: pfcp-path.h
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:05 CST
 */

#ifndef DF_PFCP_PATH_H
#define DF_PFCP_PATH_H

#include "ogs-gtp.h"

#ifdef __cplusplus
extern "C" {
#endif

int df_pfcp_open(void);
void df_pfcp_close(void);

void df_pfcp_handle_message(ogs_pfcp_node_t *pfcp_node, ogs_pfcp_message_t *message);

int df_pfcp_send_session_establishment_response(
        ogs_pfcp_xact_t *xact, df_sess_t *sess,
        ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr);
int df_pfcp_send_session_modification_response(
        ogs_pfcp_xact_t *xact, df_sess_t *sess,
        ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr);
int df_pfcp_send_session_deletion_response(ogs_pfcp_xact_t *xact,
        df_sess_t *sess);

int df_pfcp_send_session_report_request(
        df_sess_t *sess, ogs_pfcp_user_plane_report_t *report);

#ifdef __cplusplus
}
#endif

#endif /* DF_PFCP_PATH_H */ 