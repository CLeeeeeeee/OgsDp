/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dn3-path.c
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:05 CST
 */

/*
 * Copyright (C) 2019 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "context.h"
#include "dn3-path.h"

static ogs_sock_t *dn3_sock = NULL;

int df_dn3_open(void)
{
    ogs_sockaddr_t *addr = NULL;
    int rv;

    ogs_assert(dn3_sock == NULL);

    /* 创建 UDP socket 用于 GTP-U */
    dn3_sock = ogs_sock_socket(AF_INET, SOCK_DGRAM, 0);
    if (!dn3_sock) {
        ogs_error("Failed to create DN3 socket");
        return OGS_ERROR;
    }

    /* 绑定到 GTP-U 端口 (2153) */
    addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
    ogs_assert(addr);
    addr->ogs_sa_family = AF_INET;
    addr->sin.sin_port = htobe16(2153);
    addr->sin.sin_addr.s_addr = INADDR_ANY;

    rv = ogs_sock_bind(dn3_sock, addr);
    if (rv != OGS_OK) {
        ogs_error("Failed to bind DN3 socket");
        ogs_free(addr);
        ogs_sock_destroy(dn3_sock);
        dn3_sock = NULL;
        return rv;
    }

    ogs_free(addr);
    ogs_info("DN3 socket opened on port 2153");
    return OGS_OK;
}

void df_dn3_close(void)
{
    if (dn3_sock) {
        ogs_sock_destroy(dn3_sock);
        dn3_sock = NULL;
    }
    
    ogs_info("DN3 socket closed");
}

/* GTP-U 数据发送函数 */
int df_dn3_send(uint32_t teid, ogs_sockaddr_t *addr, void *data, int len)
{
    int rv;

    if (!addr || !data || len <= 0 || !dn3_sock) {
        ogs_error("Invalid parameters for df_dn3_send");
        return OGS_ERROR;
    }

    /* 发送数据 */
    rv = ogs_sendto(dn3_sock->fd, data, len, 0, addr);
    if (rv < 0) {
        ogs_error("Failed to send GTP-U packet");
        return OGS_ERROR;
    }

    ogs_debug("Sent GTP-U packet: TEID=0x%x, length=%d", teid, rv);
    return OGS_OK;
}

/* GTP-U 数据接收回调函数 */
void df_dn3_recv_cb(short when, ogs_socket_t fd, void *data)
{
    ogs_sockaddr_t from;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_gtp2_header_t gtpu_h;
    df_sess_t *sess = NULL;
    int len;
    char buf[OGS_ADDRSTRLEN];

    ogs_assert(fd == dn3_sock->fd);

    /* 接收 GTP-U 数据包 */
    pkbuf = ogs_pkbuf_alloc(NULL, OGS_MAX_SDU_LEN);
    ogs_assert(pkbuf);
    ogs_pkbuf_put(pkbuf, OGS_MAX_SDU_LEN);

    len = ogs_recvfrom(fd, pkbuf->data, pkbuf->len, 0, &from);
    if (len <= 0) {
        ogs_error("Failed to receive GTP-U packet");
        ogs_pkbuf_free(pkbuf);
        return;
    }

    ogs_pkbuf_trim(pkbuf, len);

    /* 解析 GTP-U 头部 */
    if (len < sizeof(gtpu_h)) {
        ogs_error("GTP-U packet too short: %d", len);
        ogs_pkbuf_free(pkbuf);
        return;
    }

    memcpy(&gtpu_h, pkbuf->data, sizeof(gtpu_h));
    ogs_pkbuf_pull(pkbuf, sizeof(gtpu_h));

    /* 验证 GTP-U 头部 */
    if (gtpu_h.version != 1) {
        ogs_error("Invalid GTP-U version: %d", gtpu_h.version);
        ogs_pkbuf_free(pkbuf);
        return;
    }

    /* 查找对应的会话 */
    sess = df_sess_find_by_teid(be32toh(gtpu_h.teid));
    if (!sess) {
        ogs_warn("Session not found for TEID: 0x%x", be32toh(gtpu_h.teid));
        ogs_pkbuf_free(pkbuf);
        return;
    }

    /* 处理不同类型的 GTP-U 消息 */
    switch (gtpu_h.type) {
    case OGS_GTPU_MSGTYPE_GPDU:
        /* 用户数据包 - 转发到 DNF */
        ogs_trace("DF received GPDU: TEID[0x%x] size[%d] from[%s]",
                be32toh(gtpu_h.teid), pkbuf->len, OGS_ADDR(&from, buf));

        /* 转发数据到 DNF */
        if (df_dn4_forward_to_dnf(sess, pkbuf) == OGS_OK) {
            ogs_trace("DF successfully forwarded data to DNF via DN4");
        } else {
            ogs_error("DF failed to forward data to DNF via DN4");
        }
        break;

    case OGS_GTPU_MSGTYPE_ECHO_REQ:
        /* Echo 请求 - 发送 Echo 响应 */
        ogs_trace("DF received Echo Request from [%s]", OGS_ADDR(&from, buf));
        df_dn3_send_echo_response(&from, be32toh(gtpu_h.teid));
        break;

    case OGS_GTPU_MSGTYPE_ECHO_RSP:
        /* Echo 响应 - 记录日志 */
        ogs_trace("DF received Echo Response from [%s]", OGS_ADDR(&from, buf));
        break;

    default:
        ogs_warn("DF received unknown GTP-U message type: %d from [%s]",
                gtpu_h.type, OGS_ADDR(&from, buf));
        break;
    }

    ogs_pkbuf_free(pkbuf);
}

/* 发送 Echo 响应 */
int df_dn3_send_echo_response(ogs_sockaddr_t *addr, uint32_t teid)
{
    ogs_gtp2_header_t gtpu_h;
    ogs_pkbuf_t *pkbuf = NULL;
    int rv;

    ogs_assert(addr);

    /* 构造 Echo 响应 */
    memset(&gtpu_h, 0, sizeof(gtpu_h));
    gtpu_h.version = 1;
    gtpu_h.type = OGS_GTPU_MSGTYPE_ECHO_RSP;
    gtpu_h.teid = htobe32(teid);
    gtpu_h.length = htobe16(0);

    /* 创建数据包 */
    pkbuf = ogs_pkbuf_alloc(NULL, sizeof(gtpu_h));
    ogs_assert(pkbuf);
    ogs_pkbuf_put(pkbuf, sizeof(gtpu_h));
    memcpy(pkbuf->data, &gtpu_h, sizeof(gtpu_h));

    /* 发送 Echo 响应 */
    rv = df_dn3_send(teid, addr, pkbuf->data, pkbuf->len);
    if (rv == OGS_OK) {
        char buf[OGS_ADDRSTRLEN];
        ogs_trace("DF sent Echo Response to [%s]", OGS_ADDR(addr, buf));
    } else {
        char buf[OGS_ADDRSTRLEN];
        ogs_error("DF failed to send Echo Response to [%s]", OGS_ADDR(addr, buf));
    }

    ogs_pkbuf_free(pkbuf);
    return rv;
}

/* 获取 DN3 socket 用于事件循环 */
ogs_sock_t *df_dn3_sock(void)
{
    return dn3_sock;
} 