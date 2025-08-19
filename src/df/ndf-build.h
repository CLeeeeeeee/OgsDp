#ifndef DF_N4_BUILD_H
#define DF_N4_BUILD_H

#include "ogs-gtp.h"

#ifdef __cplusplus
extern "C" {
#endif

ogs_pkbuf_t *df_n4_build_session_establishment_response(uint8_t type,
    df_sess_t *sess, ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr);
ogs_pkbuf_t *df_n4_build_session_modification_response(uint8_t type,
    df_sess_t *sess, ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr);
ogs_pkbuf_t *df_n4_build_session_deletion_response(uint8_t type,
    df_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* DF_N4_BUILD_H */
