/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: sbi-path.c
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:10 CST
 */

#include "context.h"
#include "sbi-path.h"
#include "event.h"
#include "pfcp-path.h"
#include "dn4-handler.h"
#include "dsmf-sm.h"
#include "ogs-core.h"

/* 前向声明 */
int dsmf_sbi_server_handler(ogs_sbi_request_t *request, void *data);

int dsmf_sbi_open(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_nf_service_t *service = NULL;

    /*
     * DSMF 可以运行以支持 RAN 到 DF 的 PDU 会话管理
     */
    if (ogs_sbi_server_first() == NULL)
        return OGS_OK;

    /* 初始化 SELF NF 实例 */
    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_sbi_nf_fsm_init(nf_instance);

    /* 构建 NF 实例信息 */
    ogs_sbi_nf_instance_build_default(nf_instance);
    ogs_sbi_nf_instance_add_allowed_nf_type(nf_instance, OpenAPI_nf_type_SCP);
    ogs_sbi_nf_instance_add_allowed_nf_type(nf_instance, OpenAPI_nf_type_DMF);

    /* 构建 NF 服务信息（无条件注册 nsmf-pdusession） */
        service = ogs_sbi_nf_service_build_default(
                    nf_instance, OGS_SBI_SERVICE_NAME_NDSMF_PDUSESSION);
        ogs_assert(service);
        ogs_sbi_nf_service_add_version(
                    service, OGS_SBI_API_V1, OGS_SBI_API_V1_0_0, NULL);
    ogs_sbi_nf_service_add_allowed_nf_type(service, OpenAPI_nf_type_DMF);

    /* 初始化 NRF NF 实例 */
    nf_instance = ogs_sbi_self()->nrf_instance;
    if (nf_instance)
        ogs_sbi_nf_fsm_init(nf_instance);

    /* 设置订阅数据 */
    ogs_sbi_subscription_spec_add(OpenAPI_nf_type_SEPP, NULL);
    ogs_sbi_subscription_spec_add(
            OpenAPI_nf_type_NULL, OGS_SBI_SERVICE_NAME_NAMF_COMM);

    if (ogs_sbi_server_start_all(dsmf_sbi_server_handler) != OGS_OK)
        return OGS_ERROR;

    return OGS_OK;
}

void dsmf_sbi_close(void)
{
    ogs_sbi_client_stop_all();
    ogs_sbi_server_stop_all();
}

int dsmf_sbi_server_handler(ogs_sbi_request_t *request, void *data)
{
    ogs_sbi_stream_t *stream = ogs_sbi_stream_find_by_id((ogs_pool_id_t)(uintptr_t)data);
    ogs_sbi_message_t sbi_message;
    int rv;

    ogs_assert(request);
    ogs_assert(stream);

    rv = ogs_sbi_parse_request(&sbi_message, request);
    if (rv != OGS_OK) {
        ogs_error("Cannot parse HTTP request");
            ogs_assert(true ==
            ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                NULL, "Cannot parse HTTP request", NULL, NULL));
        return OGS_ERROR;
    }

    /* 处理来自 DMF 的 gNB 同步消息 */
    if (strcmp(sbi_message.h.resource.component[0], "gnb-sync") == 0) {
        dsmf_sbi_handle_gnb_sync_request(stream, &sbi_message, request);
    } else {
        ogs_error("Unknown resource: %s", sbi_message.h.resource.component[0]);
            ogs_assert(true ==
            ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                &sbi_message, "Unknown resource", 
                sbi_message.h.resource.component[0], NULL));
    }

    ogs_sbi_message_free(&sbi_message);
    return OGS_OK;
}

int dsmf_sbi_handle_gnb_sync_request(ogs_sbi_stream_t *stream, ogs_sbi_message_t *message, ogs_sbi_request_t *request)
{
    ogs_sbi_response_t *response = NULL;
    /* 事件改为由 dsmf_context_add_gnb_with_ran_info 内部派发，这里不再直接使用 e */
    int rv;

    ogs_assert(stream);
    ogs_assert(message);
    ogs_assert(request);

    ogs_info("[DSMF] Rcv gNB sync request from DMF");

    /* 解析请求参数 */
    char *gnb_id = NULL;
    char *session_id = NULL;
    char *ran_addr_str = NULL;
    uint16_t ran_port = 0;
    uint32_t teid = 0;

    /* 从原始请求中解析 JSON 参数 */
    if (request->http.content && request->http.content_length > 0) {
        // 简单的JSON解析，参考DMF的实现
        char *gnb_id_start = strstr(request->http.content, "\"gnb_id\":\"");
        if (gnb_id_start) {
            gnb_id_start += 10;
            char *gnb_id_end = strchr(gnb_id_start, '"');
            if (gnb_id_end) {
                int len = gnb_id_end - gnb_id_start;
                gnb_id = ogs_malloc(len + 1);
                strncpy(gnb_id, gnb_id_start, len);
                gnb_id[len] = '\0';
            }
        }
        
        char *session_id_start = strstr(request->http.content, "\"session_id\":\"");
        if (session_id_start) {
            session_id_start += 14;
            char *session_id_end = strchr(session_id_start, '"');
            if (session_id_end) {
                int len = session_id_end - session_id_start;
                session_id = ogs_malloc(len + 1);
                strncpy(session_id, session_id_start, len);
                session_id[len] = '\0';
            }
        }
        
        char *ran_addr_start = strstr(request->http.content, "\"ran_addr\":\"");
        if (ran_addr_start) {
            ran_addr_start += 12;
            char *ran_addr_end = strchr(ran_addr_start, '"');
            if (ran_addr_end) {
                int len = ran_addr_end - ran_addr_start;
                ran_addr_str = ogs_malloc(len + 1);
                strncpy(ran_addr_str, ran_addr_start, len);
                ran_addr_str[len] = '\0';
            }
        }
        
        char *ran_port_start = strstr(request->http.content, "\"ran_port\":");
        if (ran_port_start) {
            ran_port_start = strchr(ran_port_start, ':') + 1;
            ran_port = atoi(ran_port_start);
        }
        
        char *teid_start = strstr(request->http.content, "\"teid\":");
        if (teid_start) {
            teid_start = strchr(teid_start, ':') + 1;
            teid = atoi(teid_start);
        }
    }

    /* 验证必要参数 */
    if (!gnb_id || !session_id || !ran_addr_str || ran_port == 0 || teid == 0) {
        ogs_error("Missing required parameters: gnb_id=%s, session_id=%s, ran_addr=%s, ran_port=%d, teid=%u",
                  gnb_id ? gnb_id : "NULL", session_id ? session_id : "NULL", 
                  ran_addr_str ? ran_addr_str : "NULL", ran_port, teid);
        ogs_assert(true ==
            ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                message, "Missing required parameters", NULL, NULL));
        return OGS_ERROR;
    }

    ogs_info("[DSMF] Parsed RAN addr: gnb=%s session=%s addr=%s:%d teid=0x%x",
             gnb_id, session_id, ran_addr_str, ran_port, teid);

    /* 先派发事件，确保流程不中断（即使 HTTP/2 回包失败） */
    dsmf_context_add_gnb_with_ran_info(gnb_id, session_id, ran_addr_str, ran_port, teid);
    ogs_info("[DSMF] Dispatched RAN addr via context: gnb=%s session=%s addr=%s:%d teid=0x%x",
             gnb_id, session_id, ran_addr_str, ran_port, teid);

    /* 再尝试回 200/空体，若远端已关闭则记录告警但不影响主流程 */
    response = ogs_sbi_response_new();
    ogs_assert(response);
    response->status = OGS_SBI_HTTP_STATUS_OK;
    response->http.content = ogs_strdup("{}");
    response->http.content_length = 2;
    ogs_sbi_header_set(response->http.headers,
            OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
    rv = ogs_sbi_server_send_response(stream, response);
    if (rv != OGS_OK) {
        ogs_warn("ogs_sbi_server_send_response() failed [%d], continue without response", rv);
    }

    /* 后续由 FSM 处理 RAN_SYNC 事件驱动 PFCP 建立，避免并发/重复触发 */
    return OGS_OK;
}