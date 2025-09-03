/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: sbi-path.h
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#ifndef DNF_SBI_PATH_H
#define DNF_SBI_PATH_H

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SBI 服务器管理 */
int dnf_sbi_init(void);
int dnf_sbi_open(void);
void dnf_sbi_close(void);

/* SBI 请求处理 */
int dnf_sbi_server_handler(ogs_sbi_request_t *request, void *data);

#ifdef __cplusplus
}
#endif

#endif /* DNF_SBI_PATH_H */
