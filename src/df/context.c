/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: context.c
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:04 CST
 */

#include "context.h"
#include "ndf-handler.h"
#include "dn3-path.h"

static df_context_t df_context;

int __df_log_domain;

static OGS_POOL(df_sess_pool, df_sess_t);
static OGS_POOL(df_n4_seid_pool, ogs_pool_id_t);

void df_context_init(void)
{
    memset(&df_context, 0, sizeof(df_context));

    df_context.df_n4_seid_hash = ogs_hash_make();
    df_context.dsmf_n4_seid_hash = ogs_hash_make();
    df_context.dsmf_n4_f_seid_hash = ogs_hash_make();
    df_context.ipv4_hash = ogs_hash_make();
    df_context.ipv6_hash = ogs_hash_make();
    df_context.teid_hash = ogs_hash_make();

    ogs_list_init(&df_context.sess_list);

    /* 初始化 DNF 客户端 */
    df_context.dnf_client = ogs_sbi_client_add(OpenAPI_uri_scheme_http, (char*)"dnf", 0, NULL, NULL);
    ogs_assert(df_context.dnf_client);

    /* 设置默认 DN4 地址 (DF 到 DNF) */
    df_context.dn4_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
    ogs_assert(df_context.dn4_addr);
    df_context.dn4_addr->ogs_sa_family = AF_INET;
    df_context.dn4_addr->sin.sin_port = htobe16(2154);  /* DN4 GTP-U 端口 */
    df_context.dn4_addr->sin.sin_addr.s_addr = inet_addr("127.0.0.24"); /* DNF 地址 */
    df_context.dn4_port = 2154;

    ogs_log_install_domain(&__df_log_domain, "df", ogs_core()->log.level);
    /* Setup UP Function Features for PFCP Association */
    ogs_pfcp_self()->up_function_features.ftup = 1;
    ogs_pfcp_self()->up_function_features.empu = 1;
    ogs_pfcp_self()->up_function_features.mnop = 1;
    ogs_pfcp_self()->up_function_features.vtime = 1;
    ogs_pfcp_self()->up_function_features.frrt = 1;
    ogs_pfcp_self()->up_function_features_len = 4;
    ogs_pool_init(&df_sess_pool, ogs_app()->pool.sess);
    ogs_pool_init(&df_n4_seid_pool, ogs_app()->pool.sess);
    ogs_pool_random_id_generate(&df_n4_seid_pool);
    df_context.ipv4_framed_routes = NULL;
    df_context.ipv6_framed_routes = NULL;

    ogs_info("DF context initialized");
}

void df_context_final(void)
{
    df_sess_t *sess = NULL, *next_sess = NULL;

    ogs_list_for_each_safe(&df_context.sess_list, next_sess, sess) {
        df_sess_remove(sess);
    }

    if (df_context.df_n4_seid_hash)
        ogs_hash_destroy(df_context.df_n4_seid_hash);
    if (df_context.dsmf_n4_seid_hash)
        ogs_hash_destroy(df_context.dsmf_n4_seid_hash);
    if (df_context.dsmf_n4_f_seid_hash)
        ogs_hash_destroy(df_context.dsmf_n4_f_seid_hash);
    if (df_context.ipv4_hash)
        ogs_hash_destroy(df_context.ipv4_hash);
    if (df_context.ipv6_hash)
        ogs_hash_destroy(df_context.ipv6_hash);
    if (df_context.teid_hash)
        ogs_hash_destroy(df_context.teid_hash);

    /* 清理 DNF 客户端 */
    if (df_context.dnf_client) {
        ogs_sbi_client_remove(df_context.dnf_client);
        df_context.dnf_client = NULL;
    }
    if (df_context.dn4_addr) {
        ogs_free(df_context.dn4_addr);
        df_context.dn4_addr = NULL;
    }

    ogs_info("DF context finalized");
}

df_context_t *df_self(void)
{
    return &df_context;
}

int df_context_parse_config(void)
{
    yaml_document_t *document = NULL;
    ogs_yaml_iter_t root_iter;
    int idx = 0;

    document = ogs_app()->document;
    ogs_assert(document);

    ogs_yaml_iter_init(&root_iter, document);
    while (ogs_yaml_iter_next(&root_iter)) {
        const char *root_key = ogs_yaml_iter_key(&root_iter);
        ogs_assert(root_key);
        if ((!strcmp(root_key, "df")) &&
            (idx++ == ogs_app()->config_section_id)) {
            ogs_yaml_iter_t df_iter;
            ogs_yaml_iter_recurse(&root_iter, &df_iter);
            while (ogs_yaml_iter_next(&df_iter)) {
                if (ogs_yaml_iter_type(&df_iter) != YAML_SCALAR_NODE)
                    continue;
                const char *df_key = ogs_yaml_iter_key(&df_iter);
                ogs_assert(df_key);
                if (!strcmp(df_key, "dn4")) {
                    ogs_yaml_iter_t dn4_iter;
                    ogs_yaml_iter_recurse(&df_iter, &dn4_iter);
                    while (ogs_yaml_iter_next(&dn4_iter)) {
                        if (ogs_yaml_iter_type(&dn4_iter) != YAML_SCALAR_NODE)
                            continue;
                        const char *dn4_key = ogs_yaml_iter_key(&dn4_iter);
                        ogs_assert(dn4_key);
                        if (!strcmp(dn4_key, "client")) {
                            ogs_yaml_iter_t client_iter;
                            ogs_yaml_iter_recurse(&dn4_iter, &client_iter);
                            while (ogs_yaml_iter_next(&client_iter)) {
                                if (ogs_yaml_iter_type(&client_iter) != YAML_SCALAR_NODE)
                                    continue;
                                const char *client_key = ogs_yaml_iter_key(&client_iter);
                                ogs_assert(client_key);
                                if (!strcmp(client_key, "address")) {
                                    const char *addr = ogs_yaml_iter_value(&client_iter);
                                    if (addr) {
                                        if (!df_context.dn4_addr) {
                                            df_context.dn4_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
                                            ogs_assert(df_context.dn4_addr);
                                        }
                                        df_context.dn4_addr->sin.sin_family = AF_INET;
                                        df_context.dn4_addr->sin.sin_addr.s_addr = inet_addr(addr);
                                    }
                                } else if (!strcmp(client_key, "port")) {
                                    const char *port = ogs_yaml_iter_value(&client_iter);
                                    if (port) {
                                        df_context.dn4_port = atoi(port);
                                        if (df_context.dn4_addr) {
                                            df_context.dn4_addr->sin.sin_port = htobe16(df_context.dn4_port);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    char buf[OGS_ADDRSTRLEN];
    ogs_info("DF DN4 client configured: %s:%d", 
            OGS_ADDR(df_context.dn4_addr, buf), df_context.dn4_port);
    return OGS_OK;
}

df_sess_t *df_sess_add(ogs_pfcp_f_seid_t *f_seid)
{
    df_sess_t *sess = NULL;

    ogs_assert(f_seid);

    ogs_pool_alloc(&df_sess_pool, &sess);
    ogs_assert(sess);
    memset(sess, 0, sizeof *sess);

    sess->df_n4_seid = f_seid->seid;
    sess->dsmf_n4_f_seid.seid = f_seid->seid;
    sess->dsmf_n4_f_seid.ip.addr = f_seid->ipv4;
    sess->dsmf_n4_f_seid.ip.ipv4 = 1;

    ogs_pfcp_pool_init(&sess->pfcp);

    ogs_list_add(&df_context.sess_list, sess);

    ogs_hash_set(df_context.df_n4_seid_hash, &sess->df_n4_seid, sizeof(sess->df_n4_seid), sess);
    ogs_hash_set(df_context.dsmf_n4_f_seid_hash, &sess->dsmf_n4_f_seid, sizeof(sess->dsmf_n4_f_seid), sess);

    /* 默认启用 DNF 转发 */
    sess->forward_to_dnf = true;
    sess->dn4_teid = sess->df_n4_seid & 0xFFFFFFFF; /* 使用 DF SEID 的低 32 位作为 DN4 TEID */

    ogs_info("DF session added: SEID[%ld]", sess->df_n4_seid);
    return sess;
}

int df_sess_remove(df_sess_t *sess)
{
    ogs_assert(sess);

    ogs_list_remove(&df_context.sess_list, sess);

    ogs_hash_set(df_context.df_n4_seid_hash, &sess->df_n4_seid, sizeof(sess->df_n4_seid), NULL);
    ogs_hash_set(df_context.dsmf_n4_f_seid_hash, &sess->dsmf_n4_f_seid, sizeof(sess->dsmf_n4_f_seid), NULL);

    if (sess->ipv4)
        ogs_hash_set(df_context.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), NULL);
    if (sess->ipv6)
        ogs_hash_set(df_context.ipv6_hash, sess->ipv6->addr, sizeof(sess->ipv6->addr), NULL);

    if (sess->ran_teid)
        ogs_hash_set(df_context.teid_hash, &sess->ran_teid, sizeof(sess->ran_teid), NULL);

    ogs_pfcp_sess_clear(&sess->pfcp);

    if (sess->apn_dnn)
        ogs_free(sess->apn_dnn);

    if (sess->ran_addr)
        ogs_freeaddrinfo(sess->ran_addr);

    ogs_pool_free(&df_sess_pool, sess);

    return OGS_OK;
}

void df_sess_remove_all(void)
{
    df_sess_t *sess = NULL, *next_sess = NULL;

    ogs_list_for_each_safe(&df_context.sess_list, next_sess, sess) {
        df_sess_remove(sess);
    }
}

df_sess_t *df_sess_find_by_dsmf_n4_seid(uint64_t seid)
{
    return ogs_hash_get(df_context.dsmf_n4_seid_hash, &seid, sizeof(seid));
}

df_sess_t *df_sess_find_by_dsmf_n4_f_seid(ogs_pfcp_f_seid_t *f_seid)
{
    ogs_assert(f_seid);
    return ogs_hash_get(df_context.dsmf_n4_f_seid_hash, f_seid, sizeof(*f_seid));
}

df_sess_t *df_sess_find_by_df_n4_seid(uint64_t seid)
{
    return ogs_hash_get(df_context.df_n4_seid_hash, &seid, sizeof(seid));
}

df_sess_t *df_sess_find_by_ipv4(uint32_t addr)
{
    return ogs_hash_get(df_context.ipv4_hash, &addr, sizeof(addr));
}

df_sess_t *df_sess_find_by_ipv6(uint32_t *addr6)
{
    ogs_assert(addr6);
    return ogs_hash_get(df_context.ipv6_hash, addr6, sizeof(addr6));
}

df_sess_t *df_sess_find_by_id(ogs_pool_id_t id)
{
    return ogs_pool_find(&df_sess_pool, id);
}

df_sess_t *df_sess_add_by_message(ogs_pfcp_message_t *message)
{
    df_sess_t *sess = NULL;
    ogs_pfcp_session_establishment_request_t *req = &message->pfcp_session_establishment_request;
    ogs_pfcp_f_seid_t *f_seid;

    ogs_assert(message);
    ogs_assert(message->h.type == OGS_PFCP_SESSION_ESTABLISHMENT_REQUEST_TYPE);

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

    sess = df_sess_add(f_seid);
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
    ue_ip = &pdr->ue_ip_addr;
    ogs_assert(ue_ip);
    if (sess->ipv4) {
        ogs_hash_set(df_context.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), NULL);
        ogs_pfcp_ue_ip_free(sess->ipv4);
    }
    if (sess->ipv6) {
        ogs_hash_set(df_context.ipv6_hash, sess->ipv6->addr, sizeof(sess->ipv6->addr), NULL);
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
            ogs_hash_set(df_context.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), sess);
        } else {
            ogs_warn("Cannot support PDN-Type[%d], [IPv4:%d IPv6:%d DNN:%s]", session_type, ue_ip->ipv4, ue_ip->ipv6, pdr->dnn ? pdr->dnn : "");
        }
    } else if (session_type == OGS_PDU_SESSION_TYPE_IPV6) {
        if (ue_ip->ipv6 || pdr->dnn) {
            sess->ipv6 = ogs_pfcp_ue_ip_alloc(&cause_value, AF_INET6, pdr->dnn, (uint8_t*)&ue_ip->addr);
            if (!sess->ipv6) {
                ogs_error("ogs_pfcp_ue_ip_alloc() failed[%d]", cause_value);
                ogs_assert(cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED);
                return cause_value;
            }
            ogs_hash_set(df_context.ipv6_hash, sess->ipv6->addr, sizeof(sess->ipv6->addr), sess);
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
            ogs_hash_set(df_context.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), sess);
        } else {
            ogs_warn("Cannot support PDN-Type[%d], [IPv4:%d IPv6:%d DNN:%s]", session_type, ue_ip->ipv4, ue_ip->ipv6, pdr->dnn ? pdr->dnn : "");
        }
        if (ue_ip->ipv6 || pdr->dnn) {
            sess->ipv6 = ogs_pfcp_ue_ip_alloc(&cause_value, AF_INET6, pdr->dnn, (uint8_t*)&ue_ip->both.addr);
            if (!sess->ipv6) {
                ogs_error("ogs_pfcp_ue_ip_alloc() failed[%d]", cause_value);
                ogs_assert(cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED);
                if (sess->ipv4) {
                    ogs_hash_set(df_context.ipv4_hash, &sess->ipv4->addr, sizeof(sess->ipv4->addr), NULL);
                    ogs_pfcp_ue_ip_free(sess->ipv4);
                    sess->ipv4 = NULL;
                }
                return cause_value;
            }
            ogs_hash_set(df_context.ipv6_hash, sess->ipv6->addr, sizeof(sess->ipv6->addr), sess);
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
    return ogs_hash_get(df_context.teid_hash, &teid, sizeof(teid));
}

 

int df_dn4_forward_to_dnf(df_sess_t *sess, ogs_pkbuf_t *pkbuf)
{
    ogs_gtp2_header_t gtpu_h;
    ogs_pkbuf_t *sendbuf = NULL;
    int rv;

    ogs_assert(sess);
    ogs_assert(pkbuf);
    ogs_assert(df_context.dn4_addr);

    if (!sess->forward_to_dnf) {
        ogs_debug("DN4 forwarding disabled for session");
        return OGS_OK;
    }

    /* 尝试重新发现 DNF */
    df_rediscover_dnf_if_needed();

    /* 构造 GTP-U 头部 */
    memset(&gtpu_h, 0, sizeof(gtpu_h));
    gtpu_h.type = OGS_GTPU_MSGTYPE_GPDU;
    gtpu_h.teid = htobe32(sess->dn4_teid);
    gtpu_h.length = htobe16(pkbuf->len);

    /* 复制数据包并添加 GTP-U 头部 */
    sendbuf = ogs_pkbuf_copy(pkbuf);
    ogs_assert(sendbuf);

    ogs_pkbuf_push(sendbuf, sizeof(gtpu_h));
    memcpy(sendbuf->data, &gtpu_h, sizeof(gtpu_h));

    /* 发送到 DNF (DN4 接口) */
    char buf[OGS_ADDRSTRLEN];
    rv = df_dn3_send(sess->dn4_teid, df_context.dn4_addr, sendbuf->data, sendbuf->len);
    if (rv == OGS_OK) {
        ogs_trace("DF forwarded data to DNF via DN4: TEID[0x%x] size[%d] DNF[%s]",
                sess->dn4_teid, pkbuf->len, OGS_ADDR(df_context.dn4_addr, buf));
    } else {
        ogs_error("DF failed to forward data to DNF via DN4: TEID[0x%x] DNF[%s]",
                sess->dn4_teid, OGS_ADDR(df_context.dn4_addr, buf));
    }

    ogs_pkbuf_free(sendbuf);
    return rv;
}

int df_discover_dnf(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_nf_service_t *service = NULL;

    ogs_assert(df_context.dnf_client);

    /* 查找 DNF 实例 */
    nf_instance = ogs_sbi_nf_instance_find_by_service_type(OGS_SBI_SERVICE_TYPE_NDNF_DATA_FORWARDING, OpenAPI_nf_type_DF);
    if (!nf_instance) {
        ogs_warn("DNF instance not found, using default configuration");
        return OGS_OK;
    }

    /* 查找 DNF 的数据转发服务 */
    service = ogs_sbi_nf_service_find_by_name(nf_instance, (char*)"ndnf-data-forwarding");
    if (!service) {
        ogs_warn("DNF data forwarding service not found");
        return OGS_OK;
    }

    /* 更新 DNF 地址配置 */
    if (service->scheme) {
        /* 这里可以根据服务发现结果更新 DNF 地址 */
        ogs_info("DNF discovered: %s", service->scheme == OpenAPI_uri_scheme_http ? "HTTP" : "HTTPS");
    }

    return OGS_OK;
}

/* 在需要时重新发现 DNF */
int df_rediscover_dnf_if_needed(void)
{
    static int last_discovery_time = 0;
    int current_time = ogs_time_now();
    
    /* 每30秒尝试重新发现一次 DNF */
    if (current_time - last_discovery_time > 30) {
        last_discovery_time = current_time;
        return df_discover_dnf();
    }
    
    return OGS_OK;
} 