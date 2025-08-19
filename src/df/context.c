#include "context.h"
#include "pfcp-path.h"

static df_context_t self;

int __df_log_domain;

static OGS_POOL(df_sess_pool, df_sess_t);
static OGS_POOL(df_n4_seid_pool, ogs_pool_id_t);

static int context_initialized = 0;

void df_context_init(void)
{
    ogs_assert(context_initialized == 0);
    memset(&self, 0, sizeof(df_context_t));
    ogs_log_install_domain(&__df_log_domain, "df", ogs_core()->log.level);
    ogs_list_init(&self.sess_list);
    ogs_pool_init(&df_sess_pool, ogs_app()->pool.sess);
    ogs_pool_init(&df_n4_seid_pool, ogs_app()->pool.sess);
    ogs_pool_random_id_generate(&df_n4_seid_pool);
    self.df_n4_seid_hash = ogs_hash_make();
    self.dsmf_n4_seid_hash = ogs_hash_make();
    self.dsmf_n4_f_seid_hash = ogs_hash_make();
    self.ipv4_hash = ogs_hash_make();
    self.ipv6_hash = ogs_hash_make();
    self.teid_hash = ogs_hash_make();
    ogs_assert(self.df_n4_seid_hash);
    ogs_assert(self.dsmf_n4_seid_hash);
    ogs_assert(self.dsmf_n4_f_seid_hash);
    ogs_assert(self.ipv4_hash);
    ogs_assert(self.ipv6_hash);
    ogs_assert(self.teid_hash);
    self.ipv4_framed_routes = NULL;
    self.ipv6_framed_routes = NULL;
    context_initialized = 1;
}

static void free_df_route_trie_node(struct df_route_trie_node *node)
{
    if (!node) return;
    free_df_route_trie_node(node->left);
    free_df_route_trie_node(node->right);
    ogs_free(node);
}

void df_context_final(void)
{
    ogs_assert(context_initialized == 1);
    df_sess_remove_all();
    ogs_assert(self.df_n4_seid_hash);
    ogs_hash_destroy(self.df_n4_seid_hash);
    ogs_assert(self.dsmf_n4_seid_hash);
    ogs_hash_destroy(self.dsmf_n4_seid_hash);
    ogs_assert(self.dsmf_n4_f_seid_hash);
    ogs_hash_destroy(self.dsmf_n4_f_seid_hash);
    ogs_assert(self.ipv4_hash);
    ogs_hash_destroy(self.ipv4_hash);
    ogs_assert(self.ipv6_hash);
    ogs_hash_destroy(self.ipv6_hash);
    ogs_assert(self.teid_hash);
    ogs_hash_destroy(self.teid_hash);
    free_df_route_trie_node(self.ipv4_framed_routes);
    free_df_route_trie_node(self.ipv6_framed_routes);
    ogs_pool_final(&df_sess_pool);
    ogs_pool_final(&df_n4_seid_pool);
    context_initialized = 0;
}

df_context_t *df_self(void)
{
    return &self;
}

static int df_context_prepare(void)
{
    return OGS_OK;
}

static int df_context_validation(void)
{
    return OGS_OK;
}

int df_context_parse_config(void)
{
    int rv;
    yaml_document_t *document = NULL;
    ogs_yaml_iter_t root_iter;

    document = ogs_app()->document;
    ogs_assert(document);

    rv = df_context_prepare();
    if (rv != OGS_OK) return rv;

    ogs_yaml_iter_init(&root_iter, document);
    while (ogs_yaml_iter_next(&root_iter)) {
        const char *root_key = ogs_yaml_iter_key(&root_iter);
        ogs_assert(root_key);
        if (!strcmp(root_key, "df")) {
            ogs_yaml_iter_t df_iter;
            ogs_yaml_iter_recurse(&root_iter, &df_iter);
            while (ogs_yaml_iter_next(&df_iter)) {
                const char *df_key = ogs_yaml_iter_key(&df_iter);
                ogs_assert(df_key);
                if (!strcmp(df_key, "gtpu")) {
                    /* handle config in gtp library */
                } else if (!strcmp(df_key, "pfcp")) {
                    /* handle config in pfcp library */
                } else if (!strcmp(df_key, "dsmf")) {
                    /* handle config in pfcp library */
                } else if (!strcmp(df_key, "session")) {
                    /* handle config in pfcp library */
                } else if (!strcmp(df_key, "metrics")) {
                    /* handle config in metrics library */
                } else if (!strcmp(df_key, "dn3")) {
                    /* handle config in gtp library */
                } else
                    ogs_warn("unknown key `%s`", df_key);
            }
        }
    }

    rv = df_context_validation();
    if (rv != OGS_OK) return rv;

    return OGS_OK;
}

df_sess_t *df_sess_add(ogs_pfcp_f_seid_t *cp_f_seid)
{
    df_sess_t *sess = NULL;
    ogs_assert(cp_f_seid);
    ogs_pool_id_calloc(&df_sess_pool, &sess);
    ogs_assert(sess);
    ogs_pfcp_pool_init(&sess->pfcp);
    ogs_pool_alloc(&df_n4_seid_pool, &sess->df_n4_seid_node);
    ogs_assert(sess->df_n4_seid_node);
    sess->df_n4_seid = *(sess->df_n4_seid_node);
    ogs_hash_set(self.df_n4_seid_hash, &sess->df_n4_seid, sizeof(sess->df_n4_seid), sess);
    sess->dsmf_n4_f_seid.seid = cp_f_seid->seid;
    ogs_assert(OGS_OK == ogs_pfcp_f_seid_to_ip(cp_f_seid, &sess->dsmf_n4_f_seid.ip));
    ogs_hash_set(self.dsmf_n4_f_seid_hash, &sess->dsmf_n4_f_seid, sizeof(sess->dsmf_n4_f_seid), sess);
    ogs_hash_set(self.dsmf_n4_seid_hash, &sess->dsmf_n4_f_seid.seid, sizeof(sess->dsmf_n4_f_seid.seid), sess);
    ogs_list_add(&self.sess_list, sess);
    return sess;
}

int df_sess_remove(df_sess_t *sess)
{
    ogs_assert(sess);
    ogs_list_remove(&self.sess_list, sess);
    ogs_pfcp_sess_clear(&sess->pfcp);
    ogs_hash_set(self.df_n4_seid_hash, &sess->df_n4_seid, sizeof(sess->df_n4_seid), NULL);
    ogs_hash_set(self.dsmf_n4_seid_hash, &sess->dsmf_n4_f_seid.seid, sizeof(sess->dsmf_n4_f_seid.seid), NULL);
    ogs_hash_set(self.dsmf_n4_f_seid_hash, &sess->dsmf_n4_f_seid, sizeof(sess->dsmf_n4_f_seid), NULL);
    if (sess->ipv4) {
        ogs_hash_set(self.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), NULL);
        ogs_pfcp_ue_ip_free(sess->ipv4);
    }
    if (sess->ipv6) {
        ogs_hash_set(self.ipv6_hash, sess->ipv6->addr, sizeof(sess->ipv6->addr), NULL);
        ogs_pfcp_ue_ip_free(sess->ipv6);
    }
    if (sess->ran_teid) {
        ogs_hash_set(self.teid_hash, &sess->ran_teid, sizeof(sess->ran_teid), NULL);
    }
    // 路由释放略
    ogs_pfcp_pool_final(&sess->pfcp);
    ogs_pool_free(&df_n4_seid_pool, sess->df_n4_seid_node);
    ogs_pool_id_free(&df_sess_pool, sess);
    if (sess->apn_dnn)
        ogs_free(sess->apn_dnn);
    return OGS_OK;
}

void df_sess_remove_all(void)
{
    df_sess_t *sess = NULL, *next = NULL;
    ogs_list_for_each_safe(&self.sess_list, next, sess) {
        df_sess_remove(sess);
    }
}

df_sess_t *df_sess_find_by_dsmf_n4_seid(uint64_t seid)
{
    return ogs_hash_get(self.dsmf_n4_seid_hash, &seid, sizeof(seid));
}
df_sess_t *df_sess_find_by_dsmf_n4_f_seid(ogs_pfcp_f_seid_t *f_seid)
{
    struct { uint64_t seid; ogs_ip_t ip; } key;
    ogs_assert(f_seid);
    ogs_assert(OGS_OK == ogs_pfcp_f_seid_to_ip(f_seid, &key.ip));
    key.seid = f_seid->seid;
    return ogs_hash_get(self.dsmf_n4_f_seid_hash, &key, sizeof(key));
}
df_sess_t *df_sess_find_by_df_n4_seid(uint64_t seid)
{
    return ogs_hash_get(self.df_n4_seid_hash, &seid, sizeof(seid));
}
df_sess_t *df_sess_find_by_ipv4(uint32_t addr)
{
    df_sess_t *ret = NULL;
    struct df_route_trie_node *trie = self.ipv4_framed_routes;
    const int nbits = sizeof(addr) << 3;
    int i;
    ogs_assert(self.ipv4_hash);
    ret = ogs_hash_get(self.ipv4_hash, &addr, OGS_IPV4_LEN);
    if (ret) return ret;
    for (i =  0; i <= nbits; i++) {
        int bit = nbits - i - 1;
        if (!trie) break;
        if (trie->sess) ret = trie->sess;
        if (i == nbits) break;
        if ((1 << bit) & be32toh(addr)) trie = trie->right;
        else trie = trie->left;
    }
    return ret;
}
df_sess_t *df_sess_find_by_ipv6(uint32_t *addr6)
{
    df_sess_t *ret = NULL;
    struct df_route_trie_node *trie = self.ipv6_framed_routes;
    int i;
    const int chunk_size = sizeof(*addr6) << 3;
    ogs_assert(self.ipv6_hash);
    ogs_assert(addr6);
    ret = ogs_hash_get(self.ipv6_hash, addr6, OGS_IPV6_DEFAULT_PREFIX_LEN >> 3);
    if (ret) return ret;
    for (i = 0; i <= OGS_IPV6_128_PREFIX_LEN; i++) {
        int part = i / chunk_size;
        int bit = (OGS_IPV6_128_PREFIX_LEN - i - 1) % chunk_size;
        if (!trie) break;
        if (trie->sess) ret = trie->sess;
        if (i == OGS_IPV6_128_PREFIX_LEN) break;
        if ((1 << bit) & be32toh(addr6[part])) trie = trie->right;
        else trie = trie->left;
    }
    return ret;
}
df_sess_t *df_sess_find_by_id(ogs_pool_id_t id)
{
    return ogs_pool_find_by_id(&df_sess_pool, id);
}
df_sess_t *df_sess_add_by_message(ogs_pfcp_message_t *message)
{
    df_sess_t *sess = NULL;
    ogs_pfcp_f_seid_t *f_seid = NULL;
    ogs_pfcp_session_establishment_request_t *req = &message->pfcp_session_establishment_request;;
    f_seid = req->cp_f_seid.data;
    if (req->cp_f_seid.presence == 0 || f_seid == NULL) {
        ogs_error("No CP F-SEID");
        return NULL;
    }
    if (f_seid->ipv4 == 0 && f_seid->ipv6 == 0) {
        ogs_error("No IPv4 or IPv6");
        return NULL;
    }
    f_seid->seid = be64toh(f_seid->seid);
    sess = df_sess_find_by_dsmf_n4_f_seid(f_seid);
    if (!sess) {
        sess = df_sess_add(f_seid);
        if (!sess) {
            ogs_error("No Session Context");
            return NULL;
        }
    }
    ogs_assert(sess);
    return sess;
}
uint8_t df_sess_set_ue_ip(df_sess_t *sess, uint8_t session_type, ogs_pfcp_pdr_t *pdr)
{
    ogs_pfcp_ue_ip_addr_t *ue_ip = NULL;
    char buf1[OGS_ADDRSTRLEN];
    char buf2[OGS_ADDRSTRLEN];
    uint8_t cause_value = OGS_PFCP_CAUSE_REQUEST_ACCEPTED;
    ogs_assert(sess);
    ogs_assert(pdr);
    ogs_assert(pdr->ue_ip_addr_len);
    ue_ip = &pdr->ue_ip_addr;
    ogs_assert(ue_ip);
    if (sess->ipv4) {
        ogs_hash_set(self.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), NULL);
        ogs_pfcp_ue_ip_free(sess->ipv4);
    }
    if (sess->ipv6) {
        ogs_hash_set(self.ipv6_hash, sess->ipv6->addr, sizeof(sess->ipv6->addr), NULL);
        ogs_pfcp_ue_ip_free(sess->ipv6);
    }
    if (session_type == OGS_PDU_SESSION_TYPE_IPV4) {
        if (ue_ip->ipv4 || pdr->dnn) {
            sess->ipv4 = ogs_pfcp_ue_ip_alloc(&cause_value, AF_INET, pdr->dnn, (uint8_t *)&(ue_ip->addr));
            if (!sess->ipv4) {
                ogs_error("ogs_pfcp_ue_ip_alloc() failed[%d]", cause_value);
                ogs_assert(cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED);
                return cause_value;
            }
            ogs_hash_set(self.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), sess);
        } else {
            ogs_warn("Cannot support PDN-Type[%d], [IPv4:%d IPv6:%d DNN:%s]", session_type, ue_ip->ipv4, ue_ip->ipv6, pdr->dnn ? pdr->dnn : "");
        }
    } else if (session_type == OGS_PDU_SESSION_TYPE_IPV6) {
        if (ue_ip->ipv6 || pdr->dnn) {
            sess->ipv6 = ogs_pfcp_ue_ip_alloc(&cause_value, AF_INET6, pdr->dnn, ue_ip->addr6);
            if (!sess->ipv6) {
                ogs_error("ogs_pfcp_ue_ip_alloc() failed[%d]", cause_value);
                ogs_assert(cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED);
                return cause_value;
            }
            ogs_hash_set(self.ipv6_hash, sess->ipv6->addr, OGS_IPV6_DEFAULT_PREFIX_LEN >> 3, sess);
        } else {
            ogs_warn("Cannot support PDN-Type[%d], [IPv4:%d IPv6:%d DNN:%s]", session_type, ue_ip->ipv4, ue_ip->ipv6, pdr->dnn ? pdr->dnn : "");
        }
    } else if (session_type == OGS_PDU_SESSION_TYPE_IPV4V6) {
        if (ue_ip->ipv4 || pdr->dnn) {
            sess->ipv4 = ogs_pfcp_ue_ip_alloc(&cause_value, AF_INET, pdr->dnn, (uint8_t *)&(ue_ip->both.addr));
            if (!sess->ipv4) {
                ogs_error("ogs_pfcp_ue_ip_alloc() failed[%d]", cause_value);
                ogs_assert(cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED);
                return cause_value;
            }
            ogs_hash_set(self.ipv4_hash, sess->ipv4->addr, OGS_IPV4_LEN, sess);
        } else {
            ogs_warn("Cannot support PDN-Type[%d], [IPv4:%d IPv6:%d DNN:%s]", session_type, ue_ip->ipv4, ue_ip->ipv6, pdr->dnn ? pdr->dnn : "");
        }
        if (ue_ip->ipv6 || pdr->dnn) {
            sess->ipv6 = ogs_pfcp_ue_ip_alloc(&cause_value, AF_INET6, pdr->dnn, ue_ip->both.addr6);
            if (!sess->ipv6) {
                ogs_error("ogs_pfcp_ue_ip_alloc() failed[%d]", cause_value);
                ogs_assert(cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED);
                if (sess->ipv4) {
                    ogs_hash_set(self.ipv4_hash, sess->ipv4->addr, OGS_IPV4_LEN, NULL);
                    ogs_pfcp_ue_ip_free(sess->ipv4);
                    sess->ipv4 = NULL;
                }
                return cause_value;
            }
            ogs_hash_set(self.ipv6_hash, sess->ipv6->addr, OGS_IPV6_DEFAULT_PREFIX_LEN >> 3, sess);
        } else {
            ogs_warn("Cannot support PDN-Type[%d], [IPv4:%d IPv6:%d DNN:%s]", session_type, ue_ip->ipv4, ue_ip->ipv6, pdr->dnn ? pdr->dnn : "");
        }
    } else {
        ogs_error("Invalid PDN-Type[%d], [IPv4:%d IPv6:%d DNN:%s]", session_type, ue_ip->ipv4, ue_ip->ipv6, pdr->dnn ? pdr->dnn : "");
        return OGS_PFCP_CAUSE_SERVICE_NOT_SUPPORTED;
    }
    ogs_info("UE F-SEID[DF:0x%lx DSMF:0x%lx] APN[%s] PDN-Type[%d] IPv4[%s] IPv6[%s]", (long)sess->df_n4_seid, (long)sess->dsmf_n4_f_seid.seid, pdr->dnn, session_type, sess->ipv4 ? OGS_INET_NTOP(&sess->ipv4->addr, buf1) : "", sess->ipv6 ? OGS_INET6_NTOP(&sess->ipv6->addr, buf2) : "");
    return cause_value;
}

df_sess_t *df_sess_find_by_teid(uint32_t teid)
{
    return (df_sess_t *)ogs_hash_get(self.teid_hash, &teid, sizeof(teid));
}

uint8_t df_sess_set_ue_ipv4_framed_routes(df_sess_t *sess, char *framed_routes[])
{
    ogs_assert(sess);
    ogs_assert(framed_routes);

    /* 暂时简化实现 */
    ogs_debug("Setting IPv4 framed routes for session");
    return OGS_OK;
}

uint8_t df_sess_set_ue_ipv6_framed_routes(df_sess_t *sess, char *framed_routes[])
{
    ogs_assert(sess);
    ogs_assert(framed_routes);

    /* 暂时简化实现 */
    ogs_debug("Setting IPv6 framed routes for session");
    return OGS_OK;
} 