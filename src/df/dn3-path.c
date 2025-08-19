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