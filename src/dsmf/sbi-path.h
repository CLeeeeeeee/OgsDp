/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: sbi-path.h
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:10 CST
 */

#ifndef DSMF_SBI_PATH_H
#define DSMF_SBI_PATH_H

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

int dsmf_sbi_open(void);
void dsmf_sbi_close(void);

// DSMF 接收来自 DMF 的 gNB 同步消息
int dsmf_sbi_handle_gnb_sync_request(ogs_sbi_stream_t *stream, ogs_sbi_message_t *message, ogs_sbi_request_t *request);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_SBI_PATH_H */ 