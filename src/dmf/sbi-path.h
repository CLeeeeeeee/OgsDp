#ifndef DMF_SBI_PATH_H
#define DMF_SBI_PATH_H

#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

int dmf_sbi_open(void);
void dmf_sbi_close(void);

// DMF接收AMF基站同步消息的SBI接口
int dmf_sbi_server_handler(ogs_sbi_request_t *request, void *data);

#ifdef __cplusplus
}
#endif

#endif /* DMF_SBI_PATH_H */ 
