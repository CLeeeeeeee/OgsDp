#include "df-sm.h"
#include "context.h"
#include "event.h"

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

    switch (e->id) {
    case DF_EVT_PFCP_MESSAGE:
        break;
    default:
        ogs_error("Unknown event %s", df_event_get_name(e));
        break;
    }
}

void df_state_exception(ogs_fsm_t *s, df_event_t *e)
{
    df_sm_debug(e);

    ogs_assert(s);

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        break;
    case OGS_FSM_EXIT_SIG:
        break;
    default:
        ogs_error("Unknown event %s", df_event_get_name(e));
        break;
    }
}
