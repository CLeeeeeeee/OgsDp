#ifndef DSMF_DN4_BUILD_H
#define DSMF_DN4_BUILD_H

#include "ogs-pfcp.h"
#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 会话管理构建函数 */
ogs_pkbuf_t *dsmf_dn4_build_session_establishment_request(dsmf_sess_t *sess);
ogs_pkbuf_t *dsmf_dn4_build_session_modification_request(dsmf_sess_t *sess);
ogs_pkbuf_t *dsmf_dn4_build_session_deletion_request(dsmf_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_DN4_BUILD_H */
