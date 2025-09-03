/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: context.c
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#include "context.h"
#include "sbi-path.h"
#include "gtpu-path.h"
#include "dnf-sm.h"

dnf_self_t dnf_self;
int __dnf_log_domain;

int dnf_context_init(void)
{
    ogs_list_init(&dnf_self.sess_list);
    dnf_self.next_sess_id = 0;

    /* 安装日志域，保持与其他网元一致 */
    ogs_log_install_domain(&__dnf_log_domain, "dnf", ogs_core()->log.level);

    /* NRF 客户端：使用框架统一管理，无需在此显式创建 */

    ogs_info("DNF context initialized");
    return OGS_OK;
}

int dnf_context_parse_config(void)
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
        if ((!strcmp(root_key, "dnf")) &&
            (idx++ == ogs_app()->config_section_id)) {
            ogs_yaml_iter_t dnf_iter;
            ogs_yaml_iter_recurse(&root_iter, &dnf_iter);
            while (ogs_yaml_iter_next(&dnf_iter)) {
                const char *dnf_key = ogs_yaml_iter_key(&dnf_iter);
                ogs_assert(dnf_key);
                
                if (!strcmp(dnf_key, "sbi")) {
                    ogs_yaml_iter_t sbi_iter;
                    ogs_yaml_iter_recurse(&dnf_iter, &sbi_iter);
                    while (ogs_yaml_iter_next(&sbi_iter)) {
                        const char *sbi_key = ogs_yaml_iter_key(&sbi_iter);
                        ogs_assert(sbi_key);
                        
                        if (!strcmp(sbi_key, "server")) {
                            ogs_yaml_iter_t server_iter, server_array;
                            ogs_yaml_iter_recurse(&sbi_iter, &server_array);
                            do {
                                int num = 0;
                                const char *hostname[OGS_MAX_NUM_OF_HOSTNAME];

                                if (ogs_yaml_iter_type(&server_array) ==
                                        YAML_MAPPING_NODE) {
                                    memcpy(&server_iter, &server_array,
                                            sizeof(ogs_yaml_iter_t));
                                } else if (ogs_yaml_iter_type(&server_array) ==
                                    YAML_SEQUENCE_NODE) {
                                    if (!ogs_yaml_iter_next(&server_array))
                                        break;
                                    ogs_yaml_iter_recurse(
                                            &server_array, &server_iter);
                                } else if (ogs_yaml_iter_type(&server_array) ==
                                    YAML_SCALAR_NODE) {
                                    break;
                                } else
                                    ogs_assert_if_reached();

                                while (ogs_yaml_iter_next(&server_iter)) {
                                    const char *server_key =
                                        ogs_yaml_iter_key(&server_iter);
                                    ogs_assert(server_key);
                                    if (!strcmp(server_key, "address")) {
                                        ogs_yaml_iter_t hostname_iter;
                                        ogs_yaml_iter_recurse(
                                                &server_iter, &hostname_iter);
                                        ogs_assert(ogs_yaml_iter_type(
                                                    &hostname_iter) !=
                                                YAML_MAPPING_NODE);

                                        do {
                                            if (ogs_yaml_iter_type(
                                                        &hostname_iter) ==
                                                    YAML_SEQUENCE_NODE) {
                                                if (!ogs_yaml_iter_next(
                                                            &hostname_iter))
                                                    break;
                                            }

                                            ogs_assert(num <
                                                    OGS_MAX_NUM_OF_HOSTNAME);
                                            hostname[num] = ogs_yaml_iter_value(
                                                    &hostname_iter);
                                            num++;
                                        } while (ogs_yaml_iter_type(
                                                    &hostname_iter) ==
                                                YAML_SEQUENCE_NODE);

                                        if (num > 0) {
                                            dnf_self.sbi_addr = hostname[0];
                                        }
                                    } else if (!strcmp(server_key, "port")) {
                                        const char *v = ogs_yaml_iter_value(&server_iter);
                                        if (v) dnf_self.sbi_port = atoi(v);
                                    }
                                }
                            } while (ogs_yaml_iter_type(&server_array) ==
                                    YAML_MAPPING_NODE);
                        }
                    }
                } else if (!strcmp(dnf_key, "gtpu")) {
                    ogs_yaml_iter_t gtpu_iter;
                    ogs_yaml_iter_recurse(&dnf_iter, &gtpu_iter);
                    while (ogs_yaml_iter_next(&gtpu_iter)) {
                        const char *gtpu_key = ogs_yaml_iter_key(&gtpu_iter);
                        ogs_assert(gtpu_key);
                        
                        if (!strcmp(gtpu_key, "server")) {
                            ogs_yaml_iter_t server_iter, server_array;
                            ogs_yaml_iter_recurse(&gtpu_iter, &server_array);
                            do {
                                int num = 0;
                                const char *hostname[OGS_MAX_NUM_OF_HOSTNAME];

                                if (ogs_yaml_iter_type(&server_array) ==
                                        YAML_MAPPING_NODE) {
                                    memcpy(&server_iter, &server_array,
                                            sizeof(ogs_yaml_iter_t));
                                } else if (ogs_yaml_iter_type(&server_array) ==
                                    YAML_SEQUENCE_NODE) {
                                    if (!ogs_yaml_iter_next(&server_array))
                                        break;
                                    ogs_yaml_iter_recurse(
                                            &server_array, &server_iter);
                                } else if (ogs_yaml_iter_type(&server_array) ==
                                    YAML_SCALAR_NODE) {
                                    break;
                                } else
                                    ogs_assert_if_reached();

                                while (ogs_yaml_iter_next(&server_iter)) {
                                    const char *server_key =
                                        ogs_yaml_iter_key(&server_iter);
                                    ogs_assert(server_key);
                                    if (!strcmp(server_key, "address")) {
                                        ogs_yaml_iter_t hostname_iter;
                                        ogs_yaml_iter_recurse(
                                                &server_iter, &hostname_iter);
                                        ogs_assert(ogs_yaml_iter_type(
                                                    &hostname_iter) !=
                                                YAML_MAPPING_NODE);

                                        do {
                                            if (ogs_yaml_iter_type(
                                                        &hostname_iter) ==
                                                    YAML_SEQUENCE_NODE) {
                                                if (!ogs_yaml_iter_next(
                                                            &hostname_iter))
                                                    break;
                                            }

                                            ogs_assert(num <
                                                    OGS_MAX_NUM_OF_HOSTNAME);
                                            hostname[num] = ogs_yaml_iter_value(
                                                    &hostname_iter);
                                            num++;
                                        } while (ogs_yaml_iter_type(
                                                    &hostname_iter) ==
                                                YAML_SEQUENCE_NODE);

                                        if (num > 0) {
                                            dnf_self.gtpu_addr = hostname[0];
                                        }
                                    } else if (!strcmp(server_key, "port")) {
                                        const char *v = ogs_yaml_iter_value(&server_iter);
                                        if (v) dnf_self.gtpu_port = atoi(v);
                                    }
                                }
                            } while (ogs_yaml_iter_type(&server_array) ==
                                    YAML_MAPPING_NODE);
                        }
                    }
                } else if (!strcmp(dnf_key, "metrics")) {
                    ogs_yaml_iter_t metrics_iter;
                    ogs_yaml_iter_recurse(&dnf_iter, &metrics_iter);
                    while (ogs_yaml_iter_next(&metrics_iter)) {
                        const char *metrics_key = ogs_yaml_iter_key(&metrics_iter);
                        ogs_assert(metrics_key);
                        
                        if (!strcmp(metrics_key, "server")) {
                            ogs_yaml_iter_t server_iter, server_array;
                            ogs_yaml_iter_recurse(&metrics_iter, &server_array);
                            do {
                                int num = 0;
                                const char *hostname[OGS_MAX_NUM_OF_HOSTNAME];

                                if (ogs_yaml_iter_type(&server_array) ==
                                        YAML_MAPPING_NODE) {
                                    memcpy(&server_iter, &server_array,
                                            sizeof(ogs_yaml_iter_t));
                                } else if (ogs_yaml_iter_type(&server_array) ==
                                    YAML_SEQUENCE_NODE) {
                                    if (!ogs_yaml_iter_next(&server_array))
                                        break;
                                    ogs_yaml_iter_recurse(
                                            &server_array, &server_iter);
                                } else if (ogs_yaml_iter_type(&server_array) ==
                                    YAML_SCALAR_NODE) {
                                    break;
                                } else
                                    ogs_assert_if_reached();

                                while (ogs_yaml_iter_next(&server_iter)) {
                                    const char *server_key =
                                        ogs_yaml_iter_key(&server_iter);
                                    ogs_assert(server_key);
                                    if (!strcmp(server_key, "address")) {
                                        ogs_yaml_iter_t hostname_iter;
                                        ogs_yaml_iter_recurse(
                                                &server_iter, &hostname_iter);
                                        ogs_assert(ogs_yaml_iter_type(
                                                    &hostname_iter) !=
                                                YAML_MAPPING_NODE);

                                        do {
                                            if (ogs_yaml_iter_type(
                                                        &hostname_iter) ==
                                                    YAML_SEQUENCE_NODE) {
                                                if (!ogs_yaml_iter_next(
                                                            &hostname_iter))
                                                    break;
                                            }

                                            ogs_assert(num <
                                                    OGS_MAX_NUM_OF_HOSTNAME);
                                            hostname[num] = ogs_yaml_iter_value(
                                                    &hostname_iter);
                                            num++;
                                        } while (ogs_yaml_iter_type(
                                                    &hostname_iter) ==
                                                YAML_SEQUENCE_NODE);

                                        if (num > 0) {
                                            dnf_self.metrics_addr = hostname[0];
                                        }
                                    } else if (!strcmp(server_key, "port")) {
                                        const char *v = ogs_yaml_iter_value(&server_iter);
                                        if (v) dnf_self.metrics_port = atoi(v);
                                    }
                                }
                            } while (ogs_yaml_iter_type(&server_array) ==
                                    YAML_MAPPING_NODE);
                        }
                    }
                }
            }
        }
    }
    
    /* 设置默认值 */
    if (!dnf_self.sbi_addr) dnf_self.sbi_addr = "127.0.0.24";
    if (!dnf_self.sbi_port) dnf_self.sbi_port = 7777;
    if (!dnf_self.gtpu_addr) dnf_self.gtpu_addr = "127.0.0.24";
    if (!dnf_self.gtpu_port) dnf_self.gtpu_port = 2154;
    if (!dnf_self.metrics_addr) dnf_self.metrics_addr = "127.0.0.24";
    if (!dnf_self.metrics_port) dnf_self.metrics_port = 9090;
    
    dnf_self.domf_port = 2155; /* DOMF GTP-U 端口 */
    
    ogs_info("DNF configuration parsed successfully");
    return OGS_OK;
}

void dnf_context_final(void)
{
    dnf_sess_t *sess = NULL, *next_sess = NULL;

    /* 清理会话 */
    ogs_list_for_each_safe(&dnf_self.sess_list, next_sess, sess) {
        dnf_sess_remove(sess);
    }

    /* 清理 NRF 客户端 */
    if (dnf_self.nrf_client) {
        ogs_sbi_client_remove(dnf_self.nrf_client);
        dnf_self.nrf_client = NULL;
    }

    ogs_info("DNF context finalized");
}

dnf_sess_t *dnf_sess_add(uint32_t teid, ogs_sockaddr_t *ran_addr)
{
    dnf_sess_t *sess = NULL;

    ogs_assert(ran_addr);

    sess = ogs_calloc(1, sizeof(dnf_sess_t));
    ogs_assert(sess);

    sess->id = ++dnf_self.next_sess_id;
    sess->index = sess->id;

    sess->teid = teid;
    ogs_copyaddrinfo(&sess->ran_addr, ran_addr);
    ogs_assert(sess->ran_addr);

    /* 设置 DOMF 转发地址（暂时使用固定地址） */
    sess->domf_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
    ogs_assert(sess->domf_addr);
    sess->domf_addr->ogs_sa_family = AF_INET;
    sess->domf_addr->sin.sin_port = htobe16(dnf_self.domf_port);
    sess->domf_addr->sin.sin_addr.s_addr = inet_addr("127.0.0.25"); /* DOMF 地址 */

    sess->active = true;
    sess->created = ogs_time_now();
    sess->last_activity = sess->created;

    ogs_list_add(&dnf_self.sess_list, sess);



    char buf1[OGS_ADDRSTRLEN], buf2[OGS_ADDRSTRLEN];
    ogs_info("DNF session added: TEID[0x%x] RAN[%s] DOMF[%s]",
            sess->teid,
            OGS_ADDR(sess->ran_addr, buf1),
            OGS_ADDR(sess->domf_addr, buf2));

    return sess;
}

void dnf_sess_remove(dnf_sess_t *sess)
{
    ogs_assert(sess);

    ogs_list_remove(&dnf_self.sess_list, sess);

    if (sess->ran_addr)
        ogs_freeaddrinfo(sess->ran_addr);
    if (sess->domf_addr)
        ogs_free(sess->domf_addr);

    ogs_free(sess);
}

dnf_sess_t *dnf_sess_find_by_teid(uint32_t teid)
{
    dnf_sess_t *sess = NULL;

    ogs_list_for_each(&dnf_self.sess_list, sess) {
        if (sess->teid == teid)
            return sess;
    }

    return NULL;
}

dnf_sess_t *dnf_sess_find_by_id(uint64_t id)
{
    dnf_sess_t *sess = NULL;

    ogs_list_for_each(&dnf_self.sess_list, sess) {
        if (sess->id == id)
            return sess;
    }

    return NULL;
}

int dnf_forward_data(dnf_sess_t *sess, ogs_pkbuf_t *pkbuf)
{
    ogs_gtp2_header_t gtpu_h;
    ogs_pkbuf_t *sendbuf = NULL;
    int rv;

    ogs_assert(sess);
    ogs_assert(pkbuf);
    ogs_assert(sess->domf_addr);

    /* 更新统计信息 */
    sess->rx_packets++;
    sess->rx_bytes += pkbuf->len;
    sess->last_activity = ogs_time_now();

    /* 构造 GTP-U 头部 */
    memset(&gtpu_h, 0, sizeof(gtpu_h));
    gtpu_h.type = OGS_GTPU_MSGTYPE_GPDU;
    gtpu_h.teid = htobe32(sess->teid);
    gtpu_h.length = htobe16(pkbuf->len);

    /* 复制数据包并添加 GTP-U 头部 */
    sendbuf = ogs_pkbuf_copy(pkbuf);
    ogs_assert(sendbuf);

    ogs_pkbuf_push(sendbuf, sizeof(gtpu_h));
    memcpy(sendbuf->data, &gtpu_h, sizeof(gtpu_h));

    /* 发送到 DOMF */
    rv = dnf_gtpu_send(sess->domf_addr, sendbuf);
    if (rv == OGS_OK) {
        sess->tx_packets++;
        sess->tx_bytes += pkbuf->len;
        char buf[OGS_ADDRSTRLEN];
        ogs_trace("DNF forwarded data: TEID[0x%x] size[%d] to DOMF[%s]",
                sess->teid, pkbuf->len, OGS_ADDR(sess->domf_addr, buf));
    } else {
        char buf[OGS_ADDRSTRLEN];
        ogs_error("DNF failed to forward data: TEID[0x%x] to DOMF[%s]",
                sess->teid, OGS_ADDR(sess->domf_addr, buf));
    }

    ogs_pkbuf_free(sendbuf);
    return rv;
}

int dnf_nrf_register(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_nf_service_t *service = NULL;

    /* 使用自有 NF 实例（与 DF/SMF 一致），若为空则跳过（由 dnf_sbi_open 构建） */
    nf_instance = ogs_sbi_self()->nf_instance;
    if (!nf_instance) {
        ogs_warn("dnf_nrf_register: nf_instance is NULL, skip (will be built in sbi_open)");
        return OGS_OK;
    }
    ogs_sbi_nf_fsm_init(nf_instance);
    ogs_sbi_nf_instance_build_default(nf_instance);
    nf_instance->nf_type = OpenAPI_nf_type_DNF;

    /* 构建 DNF 的服务 */
    service = ogs_sbi_nf_service_build_default(
                nf_instance, OGS_SBI_SERVICE_NAME_NDNF_DATA_FORWARDING);
    ogs_assert(service);
    ogs_sbi_nf_service_add_version(
                service, OGS_SBI_API_V1, OGS_SBI_API_V1_0_0, NULL);
    ogs_sbi_nf_service_add_allowed_nf_type(service, OpenAPI_nf_type_DF);

    /* 初始化 NRF NF 实例（如存在） */
    nf_instance = ogs_sbi_self()->nrf_instance;
    if (nf_instance)
        ogs_sbi_nf_fsm_init(nf_instance);
    ogs_info("DNF registered to NRF (FSM-driven)");
    return OGS_OK;
}

int dnf_nrf_deregister(void)
{
    /* 交由框架在停止时处理 */
    ogs_info("DNF deregistration requested (handled by framework)");
    return OGS_OK;
} 