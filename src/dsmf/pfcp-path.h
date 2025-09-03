/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: pfcp-path.h
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:11 CST
 */

#ifndef DSMF_PFCP_PATH_H
#define DSMF_PFCP_PATH_H

#include "ogs-pfcp.h"

#ifdef __cplusplus
extern "C" {
#endif

int dsmf_pfcp_open(void);
void dsmf_pfcp_close(void);

// DSMF 与 DF 之间的 PFCP 会话管理
int dsmf_pfcp_send_session_establishment_request(
    const char *gnb_id,
    const char *session_id,
    const char *ran_addr_str,
    uint16_t ran_port,
    uint32_t teid);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_PFCP_PATH_H */
