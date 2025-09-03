/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: sbi-path.c
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#include "context.h"
#include "sbi-path.h"

static ogs_sbi_server_t *sbi_server = NULL;

int dnf_sbi_init(void)
{
    ogs_info("DNF SBI initialized");
    return OGS_OK;
}

int dnf_sbi_open(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_nf_service_t *service = NULL;

    /* Initialize SELF NF instance */
    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_sbi_nf_fsm_init(nf_instance);

    /* Build NF instance information. It will be transmitted to NRF. */
    ogs_sbi_nf_instance_build_default(nf_instance);
    ogs_sbi_nf_instance_add_allowed_nf_type(nf_instance, OpenAPI_nf_type_DF);

    /* Build NF service information. It will be transmitted to NRF. */
    service = ogs_sbi_nf_service_build_default(
                nf_instance, OGS_SBI_SERVICE_NAME_NDNF_DATA_FORWARDING);
    ogs_assert(service);
    ogs_sbi_nf_service_add_version(
                service, OGS_SBI_API_V1, OGS_SBI_API_V1_0_0, NULL);
    ogs_sbi_nf_service_add_allowed_nf_type(service, OpenAPI_nf_type_DF);

    /* Initialize NRF NF Instance */
    nf_instance = ogs_sbi_self()->nrf_instance;
    if (nf_instance)
        ogs_sbi_nf_fsm_init(nf_instance);

    /* 启动所有 SBI 服务器 */
    if (ogs_sbi_server_start_all(dnf_sbi_server_handler) != OGS_OK)
        return OGS_ERROR;
    ogs_info("DNF SBI server started");

    return OGS_OK;
}

void dnf_sbi_close(void)
{
    ogs_sbi_server_stop_all();
    ogs_sbi_server_remove_all();
    sbi_server = NULL;
    ogs_info("DNF SBI server closed");
}

int dnf_sbi_server_handler(ogs_sbi_request_t *request, void *data)
{
    ogs_sbi_stream_t *stream = ogs_sbi_stream_find_by_id((ogs_pool_id_t)(uintptr_t)data);
    ogs_sbi_response_t *response = NULL;
    int rv;

    ogs_assert(request);
    ogs_assert(stream);

    ogs_info("DNF SBI request: %s %s", 
            request->h.method,
            request->h.uri);

    /* 处理数据转发请求 */
    if (strcmp(request->h.uri, "/ndnf-data-forwarding/v1/forward") == 0) {
        if (request->h.method && strcmp(request->h.method, OGS_SBI_HTTP_METHOD_POST) == 0) {
            /* 处理数据转发请求 */
            response = ogs_sbi_response_new();
            ogs_assert(response);

            /* 这里可以添加数据处理逻辑 */
            ogs_info("DNF received data forwarding request");

            response->status = OGS_SBI_HTTP_STATUS_OK;
        } else {
            response = ogs_sbi_response_new();
            ogs_assert(response);
            response->status = OGS_SBI_HTTP_STATUS_METHOD_NOT_ALLOWED;
        }
    } else {
        response = ogs_sbi_response_new();
        ogs_assert(response);
        response->status = OGS_SBI_HTTP_STATUS_NOT_FOUND;
    }

    /* 发送响应 */
    rv = ogs_sbi_server_send_response(stream, response);
    if (rv != OGS_OK) {
        ogs_error("Failed to send DNF SBI response");
        return OGS_ERROR;
    }

    return OGS_OK;
}
