/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: sbi-path.c
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:04 CST
 */

#include "sbi-path.h"
#include "context.h"

int df_sbi_open(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_nf_service_t *service = NULL;

    /* Initialize SELF NF instance */
    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_sbi_nf_fsm_init(nf_instance);

    /* Build NF instance information. It will be transmitted to NRF. */
    ogs_sbi_nf_instance_build_default(nf_instance);
    ogs_sbi_nf_instance_add_allowed_nf_type(nf_instance, OpenAPI_nf_type_DSMF);

    /* Build NF service information. It will be transmitted to NRF. */
    service = ogs_sbi_nf_service_build_default(
                nf_instance, OGS_SBI_SERVICE_NAME_NDF_PDUSESSION);
    ogs_assert(service);
    ogs_sbi_nf_service_add_version(
                service, OGS_SBI_API_V1, OGS_SBI_API_V1_0_0, NULL);
    ogs_sbi_nf_service_add_allowed_nf_type(service, OpenAPI_nf_type_DSMF);

    /* Initialize NRF NF Instance */
    nf_instance = ogs_sbi_self()->nrf_instance;
    if (nf_instance)
        ogs_sbi_nf_fsm_init(nf_instance);

    /* Setup Subscription-Data */
    ogs_sbi_subscription_spec_add(OpenAPI_nf_type_DSMF, NULL);

    if (ogs_sbi_server_start_all(ogs_sbi_server_handler) != OGS_OK)
        return OGS_ERROR;

    ogs_info("[DF] SBI interface opened");
    return OGS_OK;
}

void df_sbi_close(void)
{
    ogs_sbi_client_stop_all();
    ogs_sbi_server_stop_all();
}
