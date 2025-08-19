#ifndef DSMF_PFCP_PATH_H
#define DSMF_PFCP_PATH_H

#include "ogs-pfcp.h"

#ifdef __cplusplus
extern "C" {
#endif

int dsmf_pfcp_open(void);
void dsmf_pfcp_close(void);

// DSMF 与 DF 之间的 PFCP 会话管理
int dsmf_pfcp_send_session_establishment_request(
    ogs_pfcp_node_t *df_node,
    const char *gnb_id,
    const char *session_id,
    const char *ran_addr_str,
    uint16_t ran_port,
    uint32_t teid);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_PFCP_PATH_H */
