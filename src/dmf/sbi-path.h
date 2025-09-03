/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: sbi-path.h
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:07 CST
 */

#ifndef DMF_SBI_PATH_H
#define DMF_SBI_PATH_H

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

int dmf_sbi_open(void);
void dmf_sbi_close(void);

// DMF接收AMF基站同步消息的SBI接口
int dmf_sbi_server_handler(ogs_sbi_request_t *request, void *data);

#ifdef __cplusplus
}
#endif

#endif /* DMF_SBI_PATH_H */ 
