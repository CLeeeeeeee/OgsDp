#include "context.h"
#include "event.h"
#include "pfcp-path.h"

void df_pfcp_state_initial(ogs_fsm_t *s, df_event_t *e)
{
    ogs_pfcp_node_t *node = NULL;

    ogs_assert(s);
    ogs_assert(e);

    df_sm_debug(e);

    node = e->pfcp_node;
    ogs_assert(node);

    ogs_assert(OGS_FSM_STATE(&node->sm));

    OGS_FSM_TRAN(s, &df_pfcp_state_will_associate);
}

void df_pfcp_state_final(ogs_fsm_t *s, df_event_t *e)
{
    ogs_pfcp_node_t *node = NULL;

    ogs_assert(s);
    ogs_assert(e);

    df_sm_debug(e);

    node = e->pfcp_node;
    ogs_assert(node);

    ogs_assert(OGS_FSM_STATE(&node->sm));
}

void df_pfcp_state_will_associate(ogs_fsm_t *s, df_event_t *e)
{
    ogs_pfcp_node_t *node = NULL;
    ogs_pfcp_xact_t *xact = NULL;
    ogs_pkbuf_t *recvbuf = NULL;
    ogs_pfcp_message_t *pfcp_message = NULL;
    int rv;

    ogs_assert(s);
    ogs_assert(e);

    df_sm_debug(e);

    node = e->pfcp_node;
    ogs_assert(node);

    ogs_assert(OGS_FSM_STATE(&node->sm));

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DF_EVT_PFCP_MESSAGE:
        ogs_assert(e);
        recvbuf = e->pkbuf;
        ogs_assert(recvbuf);
        pfcp_message = e->pfcp_message;
        ogs_assert(pfcp_message);
        ogs_assert(node);
        ogs_assert(OGS_FSM_STATE(&node->sm));

        rv = ogs_pfcp_xact_receive(node, &pfcp_message->h, &xact);
        if (rv != OGS_OK) {
            ogs_pkbuf_free(recvbuf);
            ogs_pfcp_message_free(pfcp_message);
            break;
        }

        e->pfcp_xact_id = xact ? xact->id : OGS_INVALID_POOL_ID;
        ogs_fsm_dispatch(&node->sm, e);
        if (OGS_FSM_CHECK(&node->sm, df_pfcp_state_exception)) {
            ogs_error("PFCP state machine exception");
        }

        ogs_pkbuf_free(recvbuf);
        ogs_pfcp_message_free(pfcp_message);
        break;

    default:
        ogs_error("Unknown event %s", df_event_get_name(e));
        break;
    }
}

void df_pfcp_state_associated(ogs_fsm_t *s, df_event_t *e)
{
    ogs_pfcp_node_t *node = NULL;
    ogs_pfcp_xact_t *xact = NULL;
    ogs_pkbuf_t *recvbuf = NULL;
    ogs_pfcp_message_t *pfcp_message = NULL;
    int rv;

    ogs_assert(s);
    ogs_assert(e);

    df_sm_debug(e);

    node = e->pfcp_node;
    ogs_assert(node);

    ogs_assert(OGS_FSM_STATE(&node->sm));

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DF_EVT_PFCP_MESSAGE:
        ogs_assert(e);
        recvbuf = e->pkbuf;
        ogs_assert(recvbuf);
        pfcp_message = e->pfcp_message;
        ogs_assert(pfcp_message);
        ogs_assert(node);
        ogs_assert(OGS_FSM_STATE(&node->sm));

        rv = ogs_pfcp_xact_receive(node, &pfcp_message->h, &xact);
        if (rv != OGS_OK) {
            ogs_pkbuf_free(recvbuf);
            ogs_pfcp_message_free(pfcp_message);
            break;
        }

        e->pfcp_xact_id = xact ? xact->id : OGS_INVALID_POOL_ID;
        ogs_fsm_dispatch(&node->sm, e);
        if (OGS_FSM_CHECK(&node->sm, df_pfcp_state_exception)) {
            ogs_error("PFCP state machine exception");
        }

        ogs_pkbuf_free(recvbuf);
        ogs_pfcp_message_free(pfcp_message);
        break;

    default:
        ogs_error("Unknown event %s", df_event_get_name(e));
        break;
    }
}

void df_pfcp_state_exception(ogs_fsm_t *s, df_event_t *e)
{
    ogs_pfcp_node_t *node = NULL;

    ogs_assert(s);
    ogs_assert(e);

    df_sm_debug(e);

    node = e->pfcp_node;
    ogs_assert(node);

    ogs_assert(OGS_FSM_STATE(&node->sm));

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
