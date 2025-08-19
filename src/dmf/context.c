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
    ogs_assert(ogs_app()->file);
    
    // 初始化gNB列表
    ogs_list_init(&_dmf_self.gnb_list);
    
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


