#ifndef DSMF_SBI_PATH_H
#define DSMF_SBI_PATH_H

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

int dsmf_sbi_open(void);
void dsmf_sbi_close(void);

// DSMF 接收来自 DMF 的 gNB 同步消息
int dsmf_sbi_handle_gnb_sync_request(ogs_sbi_stream_t *stream, ogs_sbi_message_t *message, ogs_sbi_request_t *request);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_SBI_PATH_H */ 