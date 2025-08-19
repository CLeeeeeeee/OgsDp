//
// Created by admin on 25-6-21.
//

#ifndef DMF_SM_H
#define DMF_SM_H

#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

void dmf_state_initial(ogs_fsm_t *s, dmf_event_t *e);
void dmf_state_final(ogs_fsm_t *s, dmf_event_t *e);
void dmf_state_operational(ogs_fsm_t *s, dmf_event_t *e);

#define dmf_sm_debug(__pe) \
    ogs_debug("%s(): %s", __func__, dmf_event_get_name(__pe))

#ifdef __cplusplus
}
#endif

#endif /* DMF_SM_H */

