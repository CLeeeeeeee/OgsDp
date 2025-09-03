/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: context.c
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:05 CST
 */

/* dmf-context.c */
#include "context.h"

static dmf_context_t _dmf_self = {0};

dmf_context_t *dmf_context_self(void)
{
    return &_dmf_self;
}

void dmf_context_add_gnb(const char *gnb_id)
{
    dmf_gnb_t *gnb;
    ogs_list_for_each_entry(&_dmf_self.gnb_list, gnb, lnode) {
        if (strcmp(gnb->id, gnb_id) == 0)
            return; // 已存在
    }
    gnb = (dmf_gnb_t *)ogs_calloc(1, sizeof(dmf_gnb_t));
    strncpy(gnb->id, gnb_id, sizeof(gnb->id)-1);
    ogs_list_add(&_dmf_self.gnb_list, &gnb->lnode);
    ogs_info("[DMF] Added gNB: %s", gnb_id);
}

void dmf_context_remove_gnb(const char *gnb_id)
{
    dmf_gnb_t *gnb, *tmp;
    ogs_list_for_each_entry_safe(&_dmf_self.gnb_list, tmp, gnb, lnode) {
        if (strcmp(gnb->id, gnb_id) == 0) {
            if (gnb->ran_addr) {
                ogs_free(gnb->ran_addr);
                gnb->ran_addr = NULL;
            }
            ogs_list_remove(&_dmf_self.gnb_list, &gnb->lnode);
            ogs_free(gnb);
            ogs_info("[DMF] Removed gNB: %s", gnb_id);
            return;
        }
    }
}

// 修改：只查找 gNB，不创建
dmf_gnb_t *dmf_find_gnb(const char *gnb_id)
{
    dmf_gnb_t *gnb;
    
    // 只查找是否已存在
    ogs_list_for_each_entry(&_dmf_self.gnb_list, gnb, lnode) {
        if (strcmp(gnb->id, gnb_id) == 0)
            return gnb;
    }
    
    // 不存在则返回 NULL
    return NULL;
}

// 修改：更新 gNB 的 RAN 信息，如果不存在则记录错误
void dmf_update_gnb_ran_info(const char *gnb_id, 
                            const char *session_id,
                            const char *ran_addr_str,
                            uint16_t ran_port,
                            uint32_t teid)
{
    dmf_gnb_t *gnb = dmf_find_gnb(gnb_id);
    if (!gnb) {
        ogs_error("[DMF] Cannot update RAN info: gNB %s not found (must be registered by AMF first)", gnb_id);
        return;
    }
    
    // 释放旧的地址（如果存在）
    if (gnb->ran_addr) {
        ogs_free(gnb->ran_addr);
    }
    
    // 创建新的地址结构
    gnb->ran_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
    if (!gnb->ran_addr) {
        ogs_error("[DMF] Failed to allocate memory for RAN address");
        return;
    }
    
    // 设置地址信息
    gnb->ran_addr->ogs_sa_family = AF_INET;
    gnb->ran_addr->sin.sin_port = htobe16(ran_port);
    
    if (inet_pton(AF_INET, ran_addr_str, &gnb->ran_addr->sin.sin_addr) != 1) {
        ogs_error("[DMF] Invalid RAN address: %s", ran_addr_str);
        ogs_free(gnb->ran_addr);
        gnb->ran_addr = NULL;
        return;
    }
    
    gnb->teid = teid;
    strncpy(gnb->session_id, session_id, sizeof(gnb->session_id)-1);
    
    ogs_info("[DMF] Updated gNB %s with RAN info: %s:%d, TEID=0x%x, Session=%s", 
             gnb_id, ran_addr_str, ran_port, teid, session_id);
}

// 新增：转发 gNB 信息给 DSMF
void dmf_forward_to_dsmf(const char *gnb_id, const char *session_id, 
                         const char *ran_addr_str, uint16_t ran_port, uint32_t teid)
{
    ogs_sbi_xact_t *xact = NULL;
    ogs_sbi_service_type_e service_type = OGS_SBI_SERVICE_TYPE_NDSMF_PDUSESSION;
    
    ogs_assert(gnb_id);
    ogs_assert(session_id);
    ogs_assert(ran_addr_str);
    
    ogs_info("[DMF] dmf_forward_to_dsmf called: gnb=%s session=%s addr=%s:%d teid=0x%x",
             gnb_id, session_id, ran_addr_str, ran_port, teid);
    
    // 定义构建函数
    ogs_sbi_request_t *build(void *ctx, void *data) {
        ogs_sbi_message_t msg;
        char *body = NULL;
        ogs_sbi_request_t *req_local = NULL;
        memset(&msg, 0, sizeof(msg));
        msg.h.method = (char *)OGS_SBI_HTTP_METHOD_POST;
        msg.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NDSMF_PDUSESSION;
        msg.h.api.version = (char *)OGS_SBI_API_V1;
        msg.h.resource.component[0] = (char *)"gnb-sync";
        req_local = ogs_sbi_build_request(&msg);
        if (!req_local) return NULL;
        body = ogs_msprintf("{\"gnb_id\":\"%s\",\"action\":\"register\",\"session_id\":\"%s\",\"ran_addr\":\"%s\",\"ran_port\":%d,\"teid\":%u}",
            gnb_id, session_id, ran_addr_str, ran_port, teid);
        if (!body) { ogs_sbi_request_free(req_local); return NULL; }
        req_local->http.content = body;
        req_local->http.content_length = strlen(body);
        return req_local;
    }
    
    // 创建事务
    xact = ogs_sbi_xact_add(0, &_dmf_self.sbi, service_type, NULL,
                             (ogs_sbi_build_f)build, NULL, NULL);
    if (!xact) {
        ogs_error("[DMF] ogs_sbi_xact_add failed");
        return;
    }
    
    if (ogs_sbi_discover_and_send(xact) != OGS_OK) {
        ogs_error("[DMF] discover_and_send failed");
        ogs_sbi_xact_remove(xact);
        return;
    }
    ogs_info("[DMF] discover_and_send invoked");
    return;
}

int dmf_context_parse_config(void)
{
    yaml_document_t *document = NULL;
    ogs_yaml_iter_t root_iter;
    int idx = 0;

    document = ogs_app()->document;
    ogs_assert(document);

    // 初始化gNB列表
    ogs_list_init(&_dmf_self.gnb_list);

    ogs_yaml_iter_init(&root_iter, document);
    while (ogs_yaml_iter_next(&root_iter)) {
        const char *root_key = ogs_yaml_iter_key(&root_iter);
        ogs_assert(root_key);
        if ((!strcmp(root_key, "dmf")) &&
            (idx++ == ogs_app()->config_section_id)) {
            ogs_yaml_iter_t dmf_iter;
            ogs_yaml_iter_recurse(&root_iter, &dmf_iter);
            while (ogs_yaml_iter_next(&dmf_iter)) {
                const char *dmf_key = ogs_yaml_iter_key(&dmf_iter);
                ogs_assert(dmf_key);
                
                if (!strcmp(dmf_key, "sbi")) {
                    ogs_yaml_iter_t sbi_iter;
                    ogs_yaml_iter_recurse(&dmf_iter, &sbi_iter);
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
                                            _dmf_self.sbi_addr = hostname[0];
                                        }
                                    } else if (!strcmp(server_key, "port")) {
                                        const char *v = ogs_yaml_iter_value(&server_iter);
                                        if (v) _dmf_self.sbi_port = atoi(v);
                                    }
                                }
                            } while (ogs_yaml_iter_type(&server_array) ==
                                    YAML_MAPPING_NODE);
                        }
                    }
                } else if (!strcmp(dmf_key, "metrics")) {
                    ogs_yaml_iter_t metrics_iter;
                    ogs_yaml_iter_recurse(&dmf_iter, &metrics_iter);
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
                                            _dmf_self.metrics_addr = hostname[0];
                                        }
                                    } else if (!strcmp(server_key, "port")) {
                                        const char *v = ogs_yaml_iter_value(&server_iter);
                                        if (v) _dmf_self.metrics_port = atoi(v);
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
    if (!_dmf_self.sbi_addr) _dmf_self.sbi_addr = "127.0.0.21";
    if (!_dmf_self.sbi_port) _dmf_self.sbi_port = 7777;
    if (!_dmf_self.metrics_addr) _dmf_self.metrics_addr = "127.0.0.21";
    if (!_dmf_self.metrics_port) _dmf_self.metrics_port = 9090;
    
    ogs_info("[DMF] Configuration parsed successfully");
    return OGS_OK;
}

int dmf_context_nf_info(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    
    // 获取DMF的NF实例
    nf_instance = ogs_sbi_self()->nf_instance;
    if (!nf_instance) {
        ogs_error("No NF instance");
        return OGS_ERROR;
    }
    
    // 设置DMF的NF信息
    ogs_sbi_nf_instance_build_default(nf_instance);
    ogs_sbi_nf_instance_add_allowed_nf_type(nf_instance, OpenAPI_nf_type_AMF);
    
    ogs_info("[DMF] NF info configured successfully");
    return OGS_OK;
}

void dmf_context_init(void)
{
    // 初始化DMF上下文
    memset(&_dmf_self, 0, sizeof(dmf_context_t));
    ogs_list_init(&_dmf_self.gnb_list);
    /* 初始化 SBI 对象 */
    memset(&_dmf_self.sbi, 0, sizeof(_dmf_self.sbi));
    ogs_list_init(&_dmf_self.sbi.xact_list);
    
    ogs_info("[DMF] Context initialized");
}

void dmf_context_final(void)
{
    dmf_gnb_t *gnb, *tmp;
    
    // 清理所有gNB
    ogs_list_for_each_entry_safe(&_dmf_self.gnb_list, tmp, gnb, lnode) {
        if (gnb->ran_addr) {
            ogs_free(gnb->ran_addr);
        }
        ogs_list_remove(&_dmf_self.gnb_list, &gnb->lnode);
        ogs_free(gnb);
    }
    
    ogs_info("[DMF] Context finalized");
}


