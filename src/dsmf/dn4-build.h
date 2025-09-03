/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dn4-build.h
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:10 CST
 */

#ifndef DSMF_DN4_BUILD_H
#define DSMF_DN4_BUILD_H

#include "ogs-pfcp.h"
#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 会话管理构建函数 */
ogs_pkbuf_t *dsmf_dn4_build_session_establishment_request(dsmf_sess_t *sess);
ogs_pkbuf_t *dsmf_dn4_build_session_modification_request(dsmf_sess_t *sess);
ogs_pkbuf_t *dsmf_dn4_build_session_deletion_request(dsmf_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_DN4_BUILD_H */
