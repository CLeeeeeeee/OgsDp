/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: ndf-build.h
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:02 CST
 */

#ifndef DF_N4_BUILD_H
#define DF_N4_BUILD_H

#include "ogs-gtp.h"

#ifdef __cplusplus
extern "C" {
#endif

ogs_pkbuf_t *df_n4_build_session_establishment_response(uint8_t type,
    df_sess_t *sess, ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr);
ogs_pkbuf_t *df_n4_build_session_modification_response(uint8_t type,
    df_sess_t *sess, ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr);
ogs_pkbuf_t *df_n4_build_session_deletion_response(uint8_t type,
    df_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* DF_N4_BUILD_H */
