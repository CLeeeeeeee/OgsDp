/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: event.h
 * 组件: dnf
 * 添加时间: 2025年 09月 1日 星期一 14:00:00 CST
 */

#ifndef DNF_EVENT_H
#define DNF_EVENT_H

#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dnf_sess_s dnf_sess_t;

typedef enum {
    DNF_EVT_BASE = OGS_MAX_NUM_OF_PROTO_EVENT,

    DNF_EVT_NRF_REGISTER,
    DNF_EVT_NRF_DEREGISTER,
    DNF_EVT_DATA_FORWARD,
    DNF_EVT_SESSION_CREATE,
    DNF_EVT_SESSION_REMOVE,

    DNF_EVT_TOP,

} dnf_event_e;

typedef struct dnf_event_s {
    ogs_event_t h;

    int timer_id;

    union {
        struct {
            dnf_sess_t *sess;
            ogs_pkbuf_t *pkbuf;
        } data_forward;
        struct {
            uint32_t teid;
            ogs_sockaddr_t *ran_addr;
        } session_create;
        struct {
            uint32_t teid;
        } session_remove;
    };
} dnf_event_t;

OGS_STATIC_ASSERT(OGS_EVENT_SIZE >= sizeof(dnf_event_t));

void dnf_event_init(void);
void dnf_event_term(void);
void dnf_event_final(void);

dnf_event_t *dnf_event_new(dnf_event_e id);
void dnf_event_free(dnf_event_t *e);

const char *dnf_event_get_name(dnf_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* DNF_EVENT_H */
