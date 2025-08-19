#ifndef DF_SM_H
#define DF_SM_H

#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

void df_state_initial(ogs_fsm_t *s, df_event_t *e);
void df_state_final(ogs_fsm_t *s, df_event_t *e);
void df_state_operational(ogs_fsm_t *s, df_event_t *e);
void df_state_exception(ogs_fsm_t *s, df_event_t *e);

void df_pfcp_state_initial(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_final(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_will_associate(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_associated(ogs_fsm_t *s, df_event_t *e);
void df_pfcp_state_exception(ogs_fsm_t *s, df_event_t *e);

#define df_sm_debug(__pe) \
    ogs_debug("%s(): %s", __func__, df_event_get_name(__pe))

#ifdef __cplusplus
}
#endif

#endif /* DF_SM_H */
