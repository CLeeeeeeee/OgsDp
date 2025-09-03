/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dn3-path.h
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:05 CST
 */

#ifndef DF_DN3_PATH_H
#define DF_DN3_PATH_H

#include "ogs-gtp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DN3 路径管理 */
int df_dn3_open(void);
void df_dn3_close(void);

/* GTP-U 数据发送 */
int df_dn3_send(uint32_t teid, ogs_sockaddr_t *addr, void *data, int len);

/* GTP-U 数据接收回调 */
void df_dn3_recv_cb(short when, ogs_socket_t fd, void *data);

/* Echo 响应 */
int df_dn3_send_echo_response(ogs_sockaddr_t *addr, uint32_t teid);

/* 获取 DN3 socket */
ogs_sock_t *df_dn3_sock(void);

#ifdef __cplusplus
}
#endif

#endif /* DF_DN3_PATH_H */ 