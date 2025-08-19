#ifndef DF_DN3_PATH_H
#define DF_DN3_PATH_H

#include "ogs-gtp.h"

#ifdef __cplusplus
extern "C" {
#endif

int df_dn3_open(void);
void df_dn3_close(void);

/* GTP-U 数据发送函数 */
int df_dn3_send(uint32_t teid, ogs_sockaddr_t *addr, void *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* DF_DN3_PATH_H */ 