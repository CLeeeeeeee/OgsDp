
#include "dmf-sm.h"
#include "handler.h"
#include "context.h"
#include "event.h"
#include "nnrf-handler.h"

void dmf_state_initial(ogs_fsm_t *s, dmf_event_t *e)
{
    dmf_sm_debug(e);
    ogs_assert(s);
    OGS_FSM_TRAN(s, &dmf_state_operational);
}

void dmf_state_final(ogs_fsm_t *s, dmf_event_t *e)
{
    dmf_sm_debug(e);
    ogs_assert(s);
}

void dmf_state_operational(ogs_fsm_t *s, dmf_event_t *e)
{
    int rv;
    ogs_sbi_stream_t *stream = NULL;
    ogs_sbi_response_t *response = NULL;
    ogs_sbi_message_t sbi_message;
    
    dmf_sm_debug(e);
    ogs_assert(s);
    
    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("[DMF] Enter operational state");
        break;
        
    case OGS_FSM_EXIT_SIG:
        ogs_info("[DMF] Exit operational state");
        break;
        
    case OGS_EVENT_SBI_SERVER:
        ogs_assert(e->h.sbi.request);
        {
            ogs_pool_id_t stream_id = OGS_POINTER_TO_UINT(e->h.sbi.data);
            ogs_assert(stream_id >= OGS_MIN_POOL_ID && stream_id <= OGS_MAX_POOL_ID);
            stream = ogs_sbi_stream_find_by_id(stream_id);
            if (!stream) {
                ogs_error("STREAM has already been removed [%d]", stream_id);
                break;
            }
        }
        
        rv = ogs_sbi_parse_request(&sbi_message, e->h.sbi.request);
        if (rv != OGS_OK) {
            ogs_error("Cannot parse HTTP request");
            ogs_assert(true ==
                ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                    NULL, "Cannot parse HTTP request", NULL, NULL));
            break;
        }
        
        SWITCH(sbi_message.h.service.name)
        CASE(OGS_SBI_SERVICE_NAME_NDMF_GNB_SYNC)
            // 处理gNB同步消息
            if (strcmp(sbi_message.h.resource.component[0], "gnb-sync") == 0) {
                char *gnb_id = NULL;
                char *action = NULL;
                char *session_id = NULL;
                char *ran_addr_str = NULL;
                uint16_t ran_port = 0;
                uint32_t teid = 0;
                bool responded = false;
                if (e->h.sbi.request->http.content) {
                    
                    // 简单的JSON解析
                    char *gnb_id_start = strstr(e->h.sbi.request->http.content, "\"gnb_id\":\"");
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
                    
                    char *action_start = strstr(e->h.sbi.request->http.content, "\"action\":\"");
                    if (action_start) {
                        action_start += 10;
                        char *action_end = strchr(action_start, '"');
                        if (action_end) {
                            int len = action_end - action_start;
                            action = ogs_malloc(len + 1);
                            strncpy(action, action_start, len);
                            action[len] = '\0';
                        }
                    }
                    
                    // 解析扩展字段
                    char *session_id_start = strstr(e->h.sbi.request->http.content, "\"session_id\":\"");
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
                    
                    char *ran_addr_start = strstr(e->h.sbi.request->http.content, "\"ran_addr\":\"");
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
                    
                    char *ran_port_start = strstr(e->h.sbi.request->http.content, "\"ran_port\":");
                    if (ran_port_start) {
                        ran_port_start = strchr(ran_port_start, ':') + 1;
                        ran_port = atoi(ran_port_start);
                    }
                    
                    char *teid_start = strstr(e->h.sbi.request->http.content, "\"teid\":");
                    if (teid_start) {
                        teid_start = strchr(teid_start, ':') + 1;
                        teid = atoi(teid_start);
                    }
                    
                    // 处理请求
                    if (gnb_id && action) {
                        if (strcmp(action, "register") == 0) {
                            if (session_id && ran_addr_str) {
                                // 新的完整格式：包含会话级 RAN 地址信息
                                dmf_update_gnb_ran_info(gnb_id, session_id, ran_addr_str, ran_port, teid);
                                ogs_info("[DMF] Rcv RAN addr: gnb=%s session=%s addr=%s:%d teid=0x%x", gnb_id, session_id, ran_addr_str, ran_port, teid);
                                if (!responded) {
                                    response = ogs_sbi_response_new();
                                    ogs_assert(response);
                                    response->status = OGS_SBI_HTTP_STATUS_OK;
                                    response->http.content = ogs_strdup("{}");
                                    response->http.content_length = 2;
                                    ogs_sbi_header_set(response->http.headers,
                                            OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
                                    rv = ogs_sbi_server_send_response(stream, response);
                                    if (rv != OGS_OK) ogs_error("dmf rsp send failed [%d]", rv);
                                    responded = true;
                                }
                                // 异步转发给 DSMF（在响应发送之后）
                                dmf_forward_to_dsmf(gnb_id, session_id, ran_addr_str, ran_port, teid);
                            } else {
                                // 旧的简单格式：只有 gNB ID，这是 AMF 的注册请求
                                dmf_context_add_gnb(gnb_id);
                                if (!responded) {
                                    response = ogs_sbi_response_new();
                                    ogs_assert(response);
                                    response->status = OGS_SBI_HTTP_STATUS_OK;
                                    response->http.content = ogs_strdup("{}");
                                    response->http.content_length = 2;
                                    ogs_sbi_header_set(response->http.headers,
                                            OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
                                    rv = ogs_sbi_server_send_response(stream, response);
                                    if (rv != OGS_OK) ogs_error("dmf rsp send failed [%d]", rv);
                                    responded = true;
                                }
                            }
                        } else if (strcmp(action, "deregister") == 0) {
                            dmf_context_remove_gnb(gnb_id);
                            if (!responded) {
                                response = ogs_sbi_response_new();
                                ogs_assert(response);
                                response->status = OGS_SBI_HTTP_STATUS_OK;
                                response->http.content = ogs_strdup("{}");
                                response->http.content_length = 2;
                                ogs_sbi_header_set(response->http.headers,
                                        OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
                                rv = ogs_sbi_server_send_response(stream, response);
                                if (rv != OGS_OK) ogs_error("dmf rsp send failed [%d]", rv);
                                responded = true;
                            }
                        }
                    }
                }
                
                if (gnb_id) ogs_free(gnb_id);
                if (action) ogs_free(action);
                if (session_id) ogs_free(session_id);
                if (ran_addr_str) ogs_free(ran_addr_str);
                break;
            } else {
                // 未知资源
                ogs_error("Unknown resource: %s", sbi_message.h.resource.component[0]);
                ogs_assert(true ==
                    ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                        &sbi_message, "Unknown resource", 
                        sbi_message.h.resource.component[0], NULL));
            }
            break;
            
        DEFAULT
            ogs_error("Invalid API name [%s]", sbi_message.h.service.name);
            ogs_assert(true ==
                ogs_sbi_server_send_error(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST,
                    &sbi_message, "Invalid API name", 
                    sbi_message.h.service.name, NULL));
        END
        
        ogs_sbi_message_free(&sbi_message);
        break;
        
    case OGS_EVENT_SBI_CLIENT:
        {
            ogs_sbi_message_t client_msg;
            int crv;
            memset(&client_msg, 0, sizeof(client_msg));
            ogs_assert(e->h.sbi.response);
            crv = ogs_sbi_parse_response(&client_msg, e->h.sbi.response);
            if (crv != OGS_OK) {
                ogs_error("cannot parse HTTP response");
                break;
            }
            if (!strcmp(client_msg.h.service.name, OGS_SBI_SERVICE_NAME_NNRF_NFM) &&
                !strcmp(client_msg.h.resource.component[0], OGS_SBI_RESOURCE_NAME_NF_INSTANCES)) {
                ogs_sbi_nf_instance_t *nrf_instance = ogs_sbi_self()->nrf_instance;
                ogs_assert(nrf_instance);
                e->h.sbi.message = &client_msg;
                ogs_fsm_dispatch(&nrf_instance->sm, (ogs_event_t *)e);
            } else if (!strcmp(client_msg.h.service.name, OGS_SBI_SERVICE_NAME_NNRF_DISC) &&
                       !strcmp(client_msg.h.resource.component[0], OGS_SBI_RESOURCE_NAME_NF_INSTANCES)) {
                /* 使用与AMF/SMF一致的处理：交给本模块的 NNFR handler 完成缓存和选择 */
                ogs_sbi_xact_t *xact = NULL;
                ogs_pool_id_t xact_id = 0;
                xact_id = OGS_POINTER_TO_UINT(e->h.sbi.data);
                if (xact_id >= OGS_MIN_POOL_ID && xact_id <= OGS_MAX_POOL_ID)
                    xact = ogs_sbi_xact_find_by_id(xact_id);
                if (!xact) {
                    ogs_error("SBI transaction missing for discovery response");
                } else {
                    dmf_nnrf_handle_nf_discover(xact, &client_msg);
                }
            }
            ogs_sbi_message_free(&client_msg);
            break;
        }

    case OGS_EVENT_SBI_TIMER:
        {
            ogs_sbi_nf_instance_t *nrf_instance = ogs_sbi_self()->nrf_instance;
            if (nrf_instance)
                ogs_fsm_dispatch(&nrf_instance->sm, (ogs_event_t *)e);
            break;
        }

    case DMF_EVENT_GNB_SYNC:
        if (e->gnb_sync.is_register) {
            dmf_context_add_gnb(e->gnb_sync.gnb_id);
        } else {
            dmf_context_remove_gnb(e->gnb_sync.gnb_id);
        }
        break;
        
    case DMF_EVENT_GNB_DEREG:
        dmf_context_remove_gnb(e->gnb_dereg.gnb_id);
        break;
        
    case DMF_EVENT_PRINT_GNB_LIST:
        {
            dmf_gnb_t *gnb;
            int count = 0;
            dmf_context_t *ctx = dmf_context_self();
            
            ogs_info("[DMF] ===== Current gNB List =====");
            ogs_list_for_each_entry(&ctx->gnb_list, gnb, lnode) {
                ogs_info("[DMF] gNB[%d]: %s", count++, gnb->id);
                if (gnb->ran_addr) {
                    char addr_str[OGS_ADDRSTRLEN];
                    ogs_inet_ntop(gnb->ran_addr, addr_str, sizeof(addr_str));
                    ogs_info("[DMF]   - RAN Address: %s", addr_str);
                }
                if (gnb->session_id[0] != '\0') {
                    ogs_info("[DMF]   - Session ID: %s", gnb->session_id);
                }
                if (gnb->teid > 0) {
                    ogs_info("[DMF]   - TEID: 0x%x", gnb->teid);
                }
            }
            if (count == 0) {
                ogs_info("[DMF] No gNBs registered");
            }
            ogs_info("[DMF] ============================");
        }
        break;
        
    default:
        ogs_error("Unknown event %s", dmf_event_get_name(e));
        break;
    }
}

