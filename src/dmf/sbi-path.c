#include "sbi-path.h"
#include "context.h"
#include "handler.h"
#include "ogs-sbi.h"

int dmf_sbi_open(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;
    ogs_sbi_nf_service_t *service = NULL;

    /* Initialize SELF NF instance */
    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_sbi_nf_fsm_init(nf_instance);

    /* Build NF instance information. It will be transmitted to NRF. */
    ogs_sbi_nf_instance_build_default(nf_instance);
    ogs_sbi_nf_instance_add_allowed_nf_type(nf_instance, OpenAPI_nf_type_AMF);
    ogs_sbi_nf_instance_add_allowed_nf_type(nf_instance, OpenAPI_nf_type_DSMF);

    /* Build NF service information. It will be transmitted to NRF. */
    service = ogs_sbi_nf_service_build_default(
                nf_instance, OGS_SBI_SERVICE_NAME_NDMF_GNB_SYNC);
    ogs_assert(service);
    ogs_sbi_nf_service_add_version(
                service, OGS_SBI_API_V1, OGS_SBI_API_V1_0_0, NULL);
    ogs_sbi_nf_service_add_allowed_nf_type(service, OpenAPI_nf_type_AMF);
    ogs_sbi_nf_service_add_allowed_nf_type(service, OpenAPI_nf_type_DSMF);

    /* Initialize NRF NF Instance */
    nf_instance = ogs_sbi_self()->nrf_instance;
    if (nf_instance)
        ogs_sbi_nf_fsm_init(nf_instance);

    /* Setup Subscription-Data */
    ogs_sbi_subscription_spec_add(OpenAPI_nf_type_AMF, NULL);

    if (ogs_sbi_server_start_all(ogs_sbi_server_handler) != OGS_OK)
        return OGS_ERROR;

    ogs_info("[DMF] SBI interface opened");
    return OGS_OK;
}

void dmf_sbi_close(void)
{
    ogs_sbi_client_stop_all();
    ogs_sbi_server_stop_all();
    ogs_info("[DMF] SBI interface closed");
} 

