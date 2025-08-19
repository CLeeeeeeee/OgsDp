#include "event.h"
#include "ogs-core.h"

const char *dsmf_event_get_name(dsmf_event_t *e)
{
    if (e == NULL)
        return OGS_FSM_NAME_INIT_SIG;

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        return OGS_FSM_NAME_ENTRY_SIG;
    case OGS_FSM_EXIT_SIG:
        return OGS_FSM_NAME_EXIT_SIG;

    case DSMF_EVT_DN4_MESSAGE:
        return "DSMF_EVT_DN4_MESSAGE";
    case DSMF_EVT_DN4_TIMER:
        return "DSMF_EVT_DN4_TIMER";
    case DSMF_EVT_DN4_NO_HEARTBEAT:
        return "DSMF_EVT_DN4_NO_HEARTBEAT";

    case DSMF_EVT_SBI_MESSAGE:
        return "DSMF_EVT_SBI_MESSAGE";
    case DSMF_EVT_SBI_TIMER:
        return "DSMF_EVT_SBI_TIMER";

    case DSMF_EVT_SESSION_ESTABLISHMENT_REQUEST:
        return "DSMF_EVT_SESSION_ESTABLISHMENT_REQUEST";
    case DSMF_EVT_SESSION_MODIFICATION_REQUEST:
        return "DSMF_EVT_SESSION_MODIFICATION_REQUEST";
    case DSMF_EVT_SESSION_DELETION_REQUEST:
        return "DSMF_EVT_SESSION_DELETION_REQUEST";
    case DSMF_EVT_SESSION_REPORT_REQUEST:
        return "DSMF_EVT_SESSION_REPORT_REQUEST";

    case DSMF_EVT_RAN_SYNC_REQUEST:
        return "DSMF_EVT_RAN_SYNC_REQUEST";
    case DSMF_EVT_RAN_SYNC_RESPONSE:
        return "DSMF_EVT_RAN_SYNC_RESPONSE";

    case OGS_EVENT_SBI_CLIENT:
        return "OGS_EVENT_SBI_CLIENT";
    case OGS_EVENT_SBI_TIMER:
        return "OGS_EVENT_SBI_TIMER";

    default:
        break;
    }

    return "UNKNOWN_EVENT";
}

dsmf_event_t *dsmf_event_new(int id)
{
    dsmf_event_t *e = NULL;

    e = ogs_calloc(1, sizeof(*e));
    ogs_assert(e);

    e->h.id = id;
    e->h.timer_id = 0;  /* 使用 0 代替 OGS_TIMER_NONE */

    return e;
}

void dsmf_event_free(dsmf_event_t *e)
{
    ogs_assert(e);
    ogs_free(e);
}
