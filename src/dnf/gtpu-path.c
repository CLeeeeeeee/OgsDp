/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: gtpu-path.c
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#include "context.h"
#include "gtpu-path.h"
#include "dnf-sm.h"

static ogs_sock_t *gtpu_sock = NULL;
static ogs_poll_t *gtpu_poll = NULL;

int dnf_gtpu_init(void)
{
    ogs_info("DNF GTP-U initialized");
    return OGS_OK;
}

int dnf_gtpu_open(void)
{
    ogs_sockaddr_t *addr = NULL;
    int rv;

    ogs_assert(gtpu_sock == NULL);

    /* 创建 UDP socket 用于 GTP-U */
    gtpu_sock = ogs_sock_socket(AF_INET, SOCK_DGRAM, 0);
    if (!gtpu_sock) {
        ogs_error("Failed to create DNF GTP-U socket");
        return OGS_ERROR;
    }

    /* 绑定到 DNF GTP-U 端口 */
    addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
    ogs_assert(addr);
    addr->ogs_sa_family = AF_INET;
    addr->sin.sin_port = htobe16(dnf_self.gtpu_port);
    addr->sin.sin_addr.s_addr = INADDR_ANY;

    rv = ogs_sock_bind(gtpu_sock, addr);
    if (rv != OGS_OK) {
        ogs_error("Failed to bind DNF GTP-U socket");
        ogs_free(addr);
        ogs_sock_destroy(gtpu_sock);
        gtpu_sock = NULL;
        return rv;
    }

    /* 添加到事件循环 */
    gtpu_poll = ogs_pollset_add(ogs_app()->pollset,
            OGS_POLLIN, gtpu_sock->fd, dnf_gtpu_recv_cb, NULL);
    ogs_assert(gtpu_poll);

    ogs_free(addr);
    ogs_info("DNF GTP-U socket opened on port %d", dnf_self.gtpu_port);
    return OGS_OK;
}

void dnf_gtpu_close(void)
{
    if (gtpu_poll) {
        ogs_pollset_remove(gtpu_poll);
        gtpu_poll = NULL;
    }

    if (gtpu_sock) {
        ogs_sock_destroy(gtpu_sock);
        gtpu_sock = NULL;
    }

    ogs_info("DNF GTP-U socket closed");
}

int dnf_gtpu_send(ogs_sockaddr_t *addr, ogs_pkbuf_t *pkbuf)
{
    ssize_t sent;

    ogs_assert(gtpu_sock);
    ogs_assert(addr);
    ogs_assert(pkbuf);

    sent = ogs_sendto(gtpu_sock->fd, pkbuf->data, pkbuf->len, 0, addr);
    if (sent < 0 || sent != pkbuf->len) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno,
                "DNF GTP-U sendto() failed");
        return OGS_ERROR;
    }

    return OGS_OK;
}

void dnf_gtpu_recv_cb(short when, ogs_socket_t fd, void *data)
{
    ogs_sockaddr_t from;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_gtp2_header_t gtpu_h;
    dnf_sess_t *sess = NULL;
    int len;

    ogs_assert(fd != INVALID_SOCKET);

    pkbuf = ogs_pkbuf_alloc(NULL, OGS_MAX_PKT_LEN);
    ogs_assert(pkbuf);
    ogs_pkbuf_put(pkbuf, OGS_MAX_PKT_LEN);

    len = ogs_recvfrom(fd, pkbuf->data, pkbuf->len, 0, &from);
    if (len <= 0) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno,
                "DNF GTP-U recvfrom() failed");
        ogs_pkbuf_free(pkbuf);
        return;
    }

    ogs_pkbuf_trim(pkbuf, len);

    /* 解析 GTP-U 头部 */
    if (pkbuf->len < sizeof(gtpu_h)) {
        ogs_error("DNF GTP-U packet too short: %d", pkbuf->len);
        ogs_pkbuf_free(pkbuf);
        return;
    }

    memcpy(&gtpu_h, pkbuf->data, sizeof(gtpu_h));
    ogs_pkbuf_pull(pkbuf, sizeof(gtpu_h));

    /* 查找对应的会话 */
    sess = dnf_sess_find_by_teid(be32toh(gtpu_h.teid));
    if (!sess) {
        ogs_error("DNF GTP-U no session found for TEID[0x%x]",
                be32toh(gtpu_h.teid));
        ogs_pkbuf_free(pkbuf);
        return;
    }

    /* 处理数据包 */
    if (gtpu_h.type == OGS_GTPU_MSGTYPE_GPDU) {
        char buf[OGS_ADDRSTRLEN];
        ogs_trace("DNF GTP-U received data: TEID[0x%x] size[%d] from[%s]",
                be32toh(gtpu_h.teid), pkbuf->len, OGS_ADDR(&from, buf));

        /* 转发数据到 DOMF */
        dnf_forward_data(sess, pkbuf);
    } else if (gtpu_h.type == OGS_GTPU_MSGTYPE_ECHO_REQ) {
        ogs_pkbuf_t *echo_rsp;

        char buf[OGS_ADDRSTRLEN];
        ogs_debug("DNF GTP-U Echo Request from [%s]", OGS_ADDR(&from, buf));
        echo_rsp = ogs_gtp2_handle_echo_req(pkbuf);
        ogs_expect(echo_rsp);
        if (echo_rsp) {
            ssize_t sent;

            /* Echo reply */
            char buf[OGS_ADDRSTRLEN];
            ogs_debug("DNF GTP-U Echo Response to [%s]", OGS_ADDR(&from, buf));

            sent = ogs_sendto(fd, echo_rsp->data, echo_rsp->len, 0, &from);
            if (sent < 0 || sent != echo_rsp->len) {
                ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno,
                        "DNF GTP-U sendto() for echo failed");
            }
            ogs_pkbuf_free(echo_rsp);
        }
    } else {
        ogs_error("DNF GTP-U unsupported message type: %d", gtpu_h.type);
    }

    ogs_pkbuf_free(pkbuf);
}
