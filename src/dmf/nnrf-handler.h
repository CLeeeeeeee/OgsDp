/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: nnrf-handler.h
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:05 CST
 */

#ifndef DMF_NNRF_HANDLER_H
#define DMF_NNRF_HANDLER_H

#include "ogs-sbi.h"

void dmf_nnrf_handle_nf_discover(
        ogs_sbi_xact_t *xact, ogs_sbi_message_t *recvmsg);

#endif /* DMF_NNRF_HANDLER_H */


