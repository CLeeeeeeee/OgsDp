/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: gtpu-path.h
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#ifndef DNF_GTPU_PATH_H
#define DNF_GTPU_PATH_H

#include "ogs-gtp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GTP-U 服务器管理 */
int dnf_gtpu_init(void);
int dnf_gtpu_open(void);
void dnf_gtpu_close(void);

/* GTP-U 数据发送 */
int dnf_gtpu_send(ogs_sockaddr_t *addr, ogs_pkbuf_t *pkbuf);

/* GTP-U 数据接收回调 */
void dnf_gtpu_recv_cb(short when, ogs_socket_t fd, void *data);

#ifdef __cplusplus
}
#endif

#endif /* DNF_GTPU_PATH_H */
