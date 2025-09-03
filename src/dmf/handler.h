/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: handler.h
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:05 CST
 */

#ifndef DMF_HANDLER_H
#define DMF_HANDLER_H

#include "ogs-app.h"
#include "context.h"
#include "event.h"

void dmf_handle_report(const char *gnb_id, bool is_register);
void dmf_handle_gnb_sync(const char *gnb_id, bool is_register);
void dmf_handle_gnb_deregister(const char *gnb_id);
void dmf_handle_gnb_timer_expiry(const char *gnb_id);

#endif /* DMF_HANDLER_H */ 
