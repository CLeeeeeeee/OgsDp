/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: context.h
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:06 CST
 */

#ifndef DMF_CONTEXT_H
#define DMF_CONTEXT_H

#include "ogs-app.h"
#include "ogs-core.h"
#include "ogs-sbi.h"

typedef struct dmf_gnb_s {
    char id[64];
    ogs_sockaddr_t *ran_addr;     /* RAN GTP-U 地址 */
    uint32_t teid;                /* RAN TEID */
    char session_id[64];          /* 关联的会话 ID */
    ogs_lnode_t lnode;
} dmf_gnb_t;

typedef struct dmf_context_s {
    ogs_list_t gnb_list;
    ogs_sbi_object_t sbi; /* 用于 ogs_sbi_xact_add/发现与发送 */
    
    /* SBI 服务器配置 */
    const char *sbi_addr;
    uint16_t sbi_port;
    
    /* Metrics 服务器配置 */
    const char *metrics_addr;
    uint16_t metrics_port;
} dmf_context_t;

dmf_context_t *dmf_context_self(void);
void dmf_context_add_gnb(const char *gnb_id);
void dmf_context_remove_gnb(const char *gnb_id);

// 新增函数声明
int dmf_context_parse_config(void);
int dmf_context_nf_info(void);
void dmf_context_init(void);
void dmf_context_final(void);

// 新增 gNB 管理函数
dmf_gnb_t *dmf_find_gnb(const char *gnb_id);
void dmf_update_gnb_ran_info(const char *gnb_id, 
                            const char *session_id,
                            const char *ran_addr_str,
                            uint16_t ran_port,
                            uint32_t teid);
void dmf_forward_to_dsmf(const char *gnb_id,
                        const char *session_id,
                        const char *ran_addr_str,
                        uint16_t ran_port,
                        uint32_t teid);
/* 供 xact 构建函数的数据结构 */
typedef struct dmf_gnb_sync_req_s {
    const char *gnb_id;
    const char *session_id;
    const char *ran_addr_str;
    uint16_t ran_port;
    uint32_t teid;
} dmf_gnb_sync_req_t;

#endif /* DMF_CONTEXT_H */
