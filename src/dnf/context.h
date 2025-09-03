/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: context.h
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#ifndef DNF_CONTEXT_H
#define DNF_CONTEXT_H

#include "ogs-core.h"
#include "ogs-gtp.h"
#include "ogs-sbi.h"
#ifdef __cplusplus
extern "C" {
#endif

extern int __dnf_log_domain;
#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __dnf_log_domain

typedef struct dnf_sess_s {
    ogs_lnode_t lnode;

    uint64_t id;
    uint32_t index;

    /* GTP-U 隧道信息 */
    uint32_t teid;
    ogs_sockaddr_t *ran_addr;  /* 来自 DF 的 RAN 地址 */
    ogs_sockaddr_t *domf_addr; /* 转发到 DOMF 的地址 */

    /* 数据转发统计 */
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;

    /* 会话状态 */
    bool active;
    ogs_time_t created;
    ogs_time_t last_activity;

} dnf_sess_t;

typedef struct dnf_self_s {
    ogs_list_t sess_list;
    uint64_t next_sess_id;

    /* SBI 服务器配置 */
    const char *sbi_addr;
    uint16_t sbi_port;

    /* GTP-U 服务器配置 */
    const char *gtpu_addr;
    uint16_t gtpu_port;

    /* Metrics 服务器配置 */
    const char *metrics_addr;
    uint16_t metrics_port;

    /* DOMF 转发配置 */
    uint16_t domf_port;

    /* NRF 客户端配置 */
    ogs_sbi_client_t *nrf_client;

} dnf_self_t;

extern dnf_self_t dnf_self;

/* 上下文管理函数 */
int dnf_context_init(void);
int dnf_context_parse_config(void);
void dnf_context_final(void);

/* 会话管理函数 */
dnf_sess_t *dnf_sess_add(uint32_t teid, ogs_sockaddr_t *ran_addr);
void dnf_sess_remove(dnf_sess_t *sess);
dnf_sess_t *dnf_sess_find_by_teid(uint32_t teid);
dnf_sess_t *dnf_sess_find_by_id(uint64_t id);

/* 数据转发函数 */
int dnf_forward_data(dnf_sess_t *sess, ogs_pkbuf_t *pkbuf);

/* NRF 注册函数 */
int dnf_nrf_register(void);
int dnf_nrf_deregister(void);

#ifdef __cplusplus
}
#endif

#endif /* DNF_CONTEXT_H */ 