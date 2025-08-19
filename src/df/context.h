#ifndef DF_CONTEXT_H
#define DF_CONTEXT_H

#include "ogs-gtp.h"
#include "ogs-pfcp.h"
#include "ogs-app.h"
#include "df-sm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int __df_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __df_log_domain

struct df_route_trie_node;

typedef struct df_context_s {
    ogs_hash_t *df_n4_seid_hash;
    ogs_hash_t *dsmf_n4_seid_hash;
    ogs_hash_t *dsmf_n4_f_seid_hash;
    ogs_hash_t *ipv4_hash;
    ogs_hash_t *ipv6_hash;
    ogs_hash_t *teid_hash;        /* hash table for TEID */

    struct df_route_trie_node *ipv4_framed_routes;
    struct df_route_trie_node *ipv6_framed_routes;

    ogs_list_t sess_list;
} df_context_t;

struct df_route_trie_node {
    struct df_route_trie_node *left;
    struct df_route_trie_node *right;
    df_sess_t *sess;
};

typedef struct df_sess_s {
    ogs_lnode_t     lnode;
    ogs_pool_id_t   id;
    ogs_pool_id_t   *df_n4_seid_node;  /* A node of DF-N4-SEID */

    ogs_pfcp_sess_t pfcp;

    uint64_t        df_n4_seid;        /* DF SEID is derived from NODE */
    struct {
        uint64_t    seid;
        ogs_ip_t    ip;
    } dsmf_n4_f_seid;                  /* DSMF SEID is received from Peer */

    /* APN Configuration */
    ogs_pfcp_ue_ip_t *ipv4;
    ogs_pfcp_ue_ip_t *ipv6;

    ogs_ipsubnet_t   *ipv4_framed_routes;
    ogs_ipsubnet_t   *ipv6_framed_routes;

    ogs_pfcp_node_t *pfcp_node;
    char            *apn_dnn;          /* APN/DNN Item */

    /* RAN Connection */
    ogs_sockaddr_t  *ran_addr;         /* RAN address */
    uint32_t        ran_teid;          /* RAN TEID */
} df_sess_t;

void df_context_init(void);
void df_context_final(void);
df_context_t *df_self(void);

int df_context_parse_config(void);

df_sess_t *df_sess_add_by_message(ogs_pfcp_message_t *message);

df_sess_t *df_sess_add(ogs_pfcp_f_seid_t *f_seid);
int df_sess_remove(df_sess_t *sess);
void df_sess_remove_all(void);
df_sess_t *df_sess_find_by_dsmf_n4_seid(uint64_t seid);
df_sess_t *df_sess_find_by_dsmf_n4_f_seid(ogs_pfcp_f_seid_t *f_seid);
df_sess_t *df_sess_find_by_df_n4_seid(uint64_t seid);
df_sess_t *df_sess_find_by_ipv4(uint32_t addr);
df_sess_t *df_sess_find_by_ipv6(uint32_t *addr6);
df_sess_t *df_sess_find_by_id(ogs_pool_id_t id);
df_sess_t *df_sess_find_by_teid(uint32_t teid);

uint8_t df_sess_set_ue_ip(df_sess_t *sess, uint8_t session_type, ogs_pfcp_pdr_t *pdr);
uint8_t df_sess_set_ue_ipv4_framed_routes(df_sess_t *sess, char *framed_routes[]);
uint8_t df_sess_set_ue_ipv6_framed_routes(df_sess_t *sess, char *framed_routes[]);

#ifdef __cplusplus
}
#endif

#endif /* DF_CONTEXT_H */ 