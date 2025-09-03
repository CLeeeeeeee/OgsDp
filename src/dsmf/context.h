/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: context.h
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:09 CST
 */

#ifndef DSMF_CONTEXT_H
#define DSMF_CONTEXT_H

#include "ogs-core.h"
#include "ogs-pfcp.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsmf_sess_s dsmf_sess_t;
typedef struct dsmf_context_s dsmf_context_t;

typedef struct dsmf_sess_s {
    ogs_sbi_object_t sbi;
    ogs_pool_id_t id;

    uint32_t        index;              /* An index of this node */
    ogs_pool_id_t   *dsmf_dn4_seid_node;  /* A node of DSMF-DN4-SEID */

    ogs_fsm_t       sm;             /* A state machine */

    ogs_pfcp_sess_t pfcp;           /* PFCP session context */

    uint64_t        dsmf_dn4_seid;    /* DSMF SEID is derived from NODE */
    uint64_t        df_dn4_seid;      /* DF SEID is received from DF */

    struct {
        uint64_t seid;
        ogs_ip_t ip;
    } df_dn4_f_seid;
    
    ogs_pfcp_node_t *pfcp_node;

    /* RAN 相关信息 */
    char *gnb_id;
    char *session_id;
    ogs_sockaddr_t *ran_addr;
    uint32_t ran_teid;
    
    /* 会话状态 */
    bool ran_sync_completed;
    bool pfcp_established;
    
    /* SBI 相关 */
    char *session_ref;              /* 会话引用 */
    ogs_sbi_stream_t *stream;       /* SBI 流 */

    /* 定时器 */
    ogs_timer_t *t_establishment;   /* 会话建立定时器 */
    ogs_timer_t *t_deletion;        /* 会话删除定时器 */

    ogs_list_t lnode;
} dsmf_sess_t;

typedef struct dsmf_context_s {
    ogs_list_t sess_list;         /* 会话列表 */
    
    ogs_hash_t *dsmf_dn4_seid_hash; /* hash table (DSMF-DN4-SEID) */
    ogs_hash_t *df_dn4_seid_hash;   /* hash table (DF-DN4-SEID) */
    ogs_hash_t *df_dn4_f_seid_hash; /* hash table (DF-DN4-F-SEID) */
    ogs_hash_t *session_ref_hash;   /* hash table (Session Reference) */
    ogs_hash_t *gnb_session_hash;   /* hash table (gNB-Session) */
} dsmf_context_t;

dsmf_context_t *dsmf_self(void);

void dsmf_context_init(void);
void dsmf_context_final(void);

dsmf_sess_t *dsmf_sess_add(ogs_pfcp_f_seid_t *cp_f_seid);
int dsmf_sess_remove(dsmf_sess_t *sess);
dsmf_sess_t *dsmf_sess_find_by_id(ogs_pool_id_t id);
dsmf_sess_t *dsmf_sess_find_by_dsmf_dn4_seid(uint64_t seid);
dsmf_sess_t *dsmf_sess_find_by_df_dn4_seid(uint64_t seid);
dsmf_sess_t *dsmf_sess_find_by_df_dn4_f_seid(ogs_pfcp_f_seid_t *f_seid);
dsmf_sess_t *dsmf_sess_find_by_session_ref(const char *session_ref);
dsmf_sess_t *dsmf_sess_find_by_gnb_session(const char *gnb_id, const char *session_id);

/* RAN 同步函数 */
void dsmf_context_add_gnb(const char *gnb_id);
void dsmf_context_remove_gnb(const char *gnb_id);
void dsmf_context_add_gnb_with_ran_info(const char *gnb_id, const char *session_id,
                                       const char *ran_addr_str, uint16_t ran_port, uint32_t teid);
void dsmf_forward_ran_info_to_df(const char *gnb_id, const char *session_id,
                                 const char *ran_addr_str, uint16_t ran_port, uint32_t teid);

/* DF 发现函数 */
ogs_pfcp_node_t *dsmf_discover_df_node(void);

/* 会话管理函数 */
dsmf_sess_t *dsmf_sess_add_by_ran_sync(const char *gnb_id, const char *session_id,
                                       const char *ran_addr_str, uint16_t ran_port, uint32_t teid);
int dsmf_sess_establish(dsmf_sess_t *sess);
int dsmf_sess_modify(dsmf_sess_t *sess);
int dsmf_sess_delete(dsmf_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_CONTEXT_H */
