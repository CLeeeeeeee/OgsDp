#include "df-sm.h"
#include "context.h"
#include "event.h"
#include "pfcp-path.h"

void df_state_initial(ogs_fsm_t *s, df_event_t *e)
{
    df_sm_debug(e);

    ogs_assert(s);

    OGS_FSM_TRAN(s, &df_state_operational);
}

void df_state_final(ogs_fsm_t *s, df_event_t *e)
{
    df_sm_debug(e);

    ogs_assert(s);
}

void df_state_operational(ogs_fsm_t *s, df_event_t *e)
{
    df_sm_debug(e);

    ogs_assert(s);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        /* 进入操作状态时的处理 */
        break;
    case OGS_FSM_EXIT_SIG:
        /* 退出操作状态时的处理 */
        break;
    case DF_EVT_PFCP_MESSAGE:
        df_pfcp_handle_message(e->pfcp_node, e->pfcp_message);
        break;

    case OGS_EVENT_SBI_CLIENT:
        /* 交由通用 SBI 层处理；DF 无需拦截客户端响应 */
        break;

    case OGS_EVENT_SBI_TIMER: {
        ogs_info("[DF] OGS_EVENT_SBI_TIMER -> dispatch to NRF FSM");
        ogs_sbi_nf_instance_t *nrf_instance = ogs_sbi_self()->nrf_instance;
        if (nrf_instance)
            ogs_fsm_dispatch(&nrf_instance->sm, (ogs_event_t *)e);
        break;
    }
    default:
        ogs_error("Unknown event %s", df_event_get_name(e));
        break;
    }
}

void df_state_exception(ogs_fsm_t *s, df_event_t *e)
{
    df_sm_debug(e);

    ogs_assert(s);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        break;
    case OGS_FSM_EXIT_SIG:
        break;
    default:
        ogs_error("Unknown event %s", df_event_get_name(e));
        break;
    }
}
