#include "context.h"
#include "event.h"
#include "dsmf-sm.h"
#include "ogs-core.h"
#include "ogs-pfcp.h"

static dsmf_context_t _dsmf_self;

OGS_POOL(dsmf_sess_pool, dsmf_sess_t);
OGS_POOL(dsmf_dn4_seid_pool, ogs_pool_id_t);

dsmf_context_t *dsmf_self(void)
{
    return &_dsmf_self;
}

void dsmf_context_init(void)
{
    memset(&_dsmf_self, 0, sizeof(_dsmf_self));

    ogs_list_init(&_dsmf_self.sess_list);

    ogs_pool_init(&dsmf_sess_pool, ogs_app()->pool.sess);
    ogs_pool_init(&dsmf_dn4_seid_pool, ogs_app()->pool.sess);

    _dsmf_self.dsmf_dn4_seid_hash = ogs_hash_make();
    _dsmf_self.df_dn4_seid_hash = ogs_hash_make();
    _dsmf_self.df_dn4_f_seid_hash = ogs_hash_make();
    _dsmf_self.session_ref_hash = ogs_hash_make();
    _dsmf_self.gnb_session_hash = ogs_hash_make();
}

void dsmf_context_final(void)
{
    dsmf_sess_t *sess = NULL, *next_sess = NULL;

    ogs_list_for_each_safe(&_dsmf_self.sess_list, next_sess, sess)
        dsmf_sess_remove(sess);

    if (_dsmf_self.dsmf_dn4_seid_hash)
        ogs_hash_destroy(_dsmf_self.dsmf_dn4_seid_hash);
    if (_dsmf_self.df_dn4_seid_hash)
        ogs_hash_destroy(_dsmf_self.df_dn4_seid_hash);
    if (_dsmf_self.df_dn4_f_seid_hash)
        ogs_hash_destroy(_dsmf_self.df_dn4_f_seid_hash);
    if (_dsmf_self.session_ref_hash)
        ogs_hash_destroy(_dsmf_self.session_ref_hash);
    if (_dsmf_self.gnb_session_hash)
        ogs_hash_destroy(_dsmf_self.gnb_session_hash);

    ogs_pool_final(&dsmf_sess_pool);
    ogs_pool_final(&dsmf_dn4_seid_pool);
}

dsmf_sess_t *dsmf_sess_add(ogs_pfcp_f_seid_t *cp_f_seid)
{
    dsmf_event_t e;
    dsmf_sess_t *sess = NULL;

    ogs_assert(cp_f_seid);

    ogs_pool_alloc(&dsmf_sess_pool, &sess);
    ogs_assert(sess);
    memset(sess, 0, sizeof *sess);

    ogs_pool_alloc(&dsmf_dn4_seid_pool, &sess->dsmf_dn4_seid_node);
    ogs_assert(sess->dsmf_dn4_seid_node);

    sess->dsmf_dn4_seid = *(sess->dsmf_dn4_seid_node);

    ogs_hash_set(_dsmf_self.dsmf_dn4_seid_hash, &sess->dsmf_dn4_seid,
            sizeof(sess->dsmf_dn4_seid), sess);

    /* Set F-SEID */
    sess->df_dn4_f_seid.seid = cp_f_seid->seid;
    sess->df_dn4_f_seid.ip.addr = cp_f_seid->ipv4;
    sess->df_dn4_f_seid.ip.len = 4;

    ogs_hash_set(_dsmf_self.df_dn4_f_seid_hash, &sess->df_dn4_f_seid,
            sizeof(sess->df_dn4_f_seid), sess);

    /* Initialize PFCP session */
    memset(&sess->pfcp, 0, sizeof(sess->pfcp));

    /* Initialize state machine */
    memset(&e, 0, sizeof(e));
    e.h.id = DSMF_EVT_SESSION_ESTABLISHMENT_REQUEST;
    ogs_fsm_init(&sess->sm, dsmf_session_state_initial, dsmf_session_state_final, &e);

    ogs_list_add(&_dsmf_self.sess_list, sess);

    return sess;
}

dsmf_sess_t *dsmf_sess_add_by_ran_sync(const char *gnb_id, const char *session_id,
                                       const char *ran_addr_str, uint16_t ran_port, uint32_t teid)
{
    dsmf_sess_t *sess = NULL;
    ogs_pfcp_f_seid_t cp_f_seid;
    char key[256];

    ogs_assert(gnb_id);
    ogs_assert(session_id);
    ogs_assert(ran_addr_str);

    /* 检查会话是否已存在 */
    snprintf(key, sizeof(key), "%s-%s", gnb_id, session_id);
    sess = ogs_hash_get(_dsmf_self.gnb_session_hash, key, strlen(key));
    if (sess) {
        ogs_warn("Session already exists: %s", key);
        return sess;
    }

    /* 创建新的会话 */
    memset(&cp_f_seid, 0, sizeof(cp_f_seid));
    /* 先分配一个 SEID 节点 */
    ogs_pool_id_t *seid_node = NULL;
    ogs_pool_alloc(&dsmf_dn4_seid_pool, &seid_node);
    ogs_assert(seid_node);
    cp_f_seid.seid = *seid_node;
    cp_f_seid.ipv4 = ogs_pfcp_self()->pfcp_addr->sin.sin_addr.s_addr;

    sess = dsmf_sess_add(&cp_f_seid);
    ogs_assert(sess);

    /* 选择 DF PFCP 节点并绑定到会话 */
    if (!sess->pfcp_node) {
        ogs_pfcp_node_t *df_node = NULL;
        ogs_list_for_each(&ogs_pfcp_self()->pfcp_peer_list, df_node) {
            sess->pfcp_node = df_node;
            break;
        }
        if (!sess->pfcp_node) {
            ogs_error("[DSMF] No PFCP DF peer node available");
        }
    }

    /* 设置 RAN 信息 */
    sess->gnb_id = ogs_strdup(gnb_id);
    sess->session_id = ogs_strdup(session_id);
    sess->ran_teid = teid;

    /* 解析 RAN 地址 */
    if (ran_addr_str) {
        sess->ran_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
        ogs_assert(sess->ran_addr);
        
        /* 使用正确的 ogs_getaddrinfo 函数调用 */
        if (ogs_getaddrinfo(&sess->ran_addr, AF_INET, ran_addr_str, htons(ran_port), 0) != OGS_OK) {
            ogs_error("Invalid RAN address: %s:%d", ran_addr_str, ran_port);
            ogs_free(sess->ran_addr);
            sess->ran_addr = NULL;
        }
    }

    /* 生成会话引用 */
    sess->session_ref = ogs_msprintf("dsmf-%s-%s", gnb_id, session_id);
    ogs_assert(sess->session_ref);

    /* 添加到哈希表 */
    ogs_hash_set(_dsmf_self.gnb_session_hash, key, strlen(key), sess);
    ogs_hash_set(_dsmf_self.session_ref_hash, sess->session_ref, 
                 strlen(sess->session_ref), sess);

    ogs_info("Created session: %s (gNB: %s, Session: %s, RAN: %s:%d, TEID: 0x%x)",
             sess->session_ref, gnb_id, session_id, ran_addr_str, ran_port, teid);

    return sess;
}

int dsmf_sess_remove(dsmf_sess_t *sess)
{
    ogs_assert(sess);

    /* 清理定时器 */
    if (sess->t_establishment) {
        ogs_timer_delete(sess->t_establishment);
        sess->t_establishment = NULL;
    }
    if (sess->t_deletion) {
        ogs_timer_delete(sess->t_deletion);
        sess->t_deletion = NULL;
    }

    ogs_list_remove(&_dsmf_self.sess_list, sess);

    ogs_hash_set(_dsmf_self.dsmf_dn4_seid_hash, &sess->dsmf_dn4_seid,
            sizeof(sess->dsmf_dn4_seid), NULL);
    ogs_hash_set(_dsmf_self.df_dn4_f_seid_hash, &sess->df_dn4_f_seid,
            sizeof(sess->df_dn4_f_seid), NULL);

    if (sess->session_ref) {
        ogs_hash_set(_dsmf_self.session_ref_hash, sess->session_ref,
                     strlen(sess->session_ref), NULL);
    }

    if (sess->gnb_id && sess->session_id) {
        char key[256];
        snprintf(key, sizeof(key), "%s-%s", sess->gnb_id, sess->session_id);
        ogs_hash_set(_dsmf_self.gnb_session_hash, key, strlen(key), NULL);
    }

    if (sess->gnb_id)
        ogs_free(sess->gnb_id);
    if (sess->session_id)
        ogs_free(sess->session_id);
    if (sess->ran_addr)
        ogs_free(sess->ran_addr);
    if (sess->session_ref)
        ogs_free(sess->session_ref);

    ogs_pool_free(&dsmf_dn4_seid_pool, sess->dsmf_dn4_seid_node);
    ogs_pool_free(&dsmf_sess_pool, sess);

    return OGS_OK;
}

dsmf_sess_t *dsmf_sess_find_by_id(ogs_pool_id_t id)
{
    return ogs_pool_find(&dsmf_sess_pool, id);
}

dsmf_sess_t *dsmf_sess_find_by_dsmf_dn4_seid(uint64_t seid)
{
    return ogs_hash_get(_dsmf_self.dsmf_dn4_seid_hash, &seid, sizeof(seid));
}

dsmf_sess_t *dsmf_sess_find_by_df_dn4_seid(uint64_t seid)
{
    return ogs_hash_get(_dsmf_self.df_dn4_seid_hash, &seid, sizeof(seid));
}

dsmf_sess_t *dsmf_sess_find_by_df_dn4_f_seid(ogs_pfcp_f_seid_t *f_seid)
{
    ogs_assert(f_seid);
    return ogs_hash_get(_dsmf_self.df_dn4_f_seid_hash, f_seid, sizeof(*f_seid));
}

dsmf_sess_t *dsmf_sess_find_by_session_ref(const char *session_ref)
{
    ogs_assert(session_ref);
    return ogs_hash_get(_dsmf_self.session_ref_hash, session_ref, strlen(session_ref));
}

dsmf_sess_t *dsmf_sess_find_by_gnb_session(const char *gnb_id, const char *session_id)
{
    char key[256];
    ogs_assert(gnb_id);
    ogs_assert(session_id);
    
    snprintf(key, sizeof(key), "%s-%s", gnb_id, session_id);
    return ogs_hash_get(_dsmf_self.gnb_session_hash, key, strlen(key));
}

int dsmf_sess_establish(dsmf_sess_t *sess)
{
    dsmf_event_t *e = NULL;
    
    ogs_assert(sess);
    
    e = dsmf_event_new(DSMF_EVT_SESSION_ESTABLISHMENT_REQUEST);
    ogs_assert(e);
    e->sess_id = sess->id;
    
    ogs_fsm_dispatch(&sess->sm, e);
    dsmf_event_free(e);
    
    return OGS_OK;
}

int dsmf_sess_modify(dsmf_sess_t *sess)
{
    dsmf_event_t *e = NULL;
    
    ogs_assert(sess);
    
    e = dsmf_event_new(DSMF_EVT_SESSION_MODIFICATION_REQUEST);
    ogs_assert(e);
    e->sess_id = sess->id;
    
    ogs_fsm_dispatch(&sess->sm, e);
    dsmf_event_free(e);
    
    return OGS_OK;
}

int dsmf_sess_delete(dsmf_sess_t *sess)
{
    dsmf_event_t *e = NULL;
    
    ogs_assert(sess);
    
    e = dsmf_event_new(DSMF_EVT_SESSION_DELETION_REQUEST);
    ogs_assert(e);
    e->sess_id = sess->id;
    
    ogs_fsm_dispatch(&sess->sm, e);
    dsmf_event_free(e);
    
    return OGS_OK;
}

void dsmf_context_add_gnb(const char *gnb_id)
{
    ogs_assert(gnb_id);
    ogs_info("[DSMF] Added gNB: %s", gnb_id);
}

void dsmf_context_remove_gnb(const char *gnb_id)
{
    ogs_assert(gnb_id);
    ogs_info("[DSMF] Removed gNB: %s", gnb_id);
}

void dsmf_context_add_gnb_with_ran_info(const char *gnb_id, const char *session_id,
                                       const char *ran_addr_str, uint16_t ran_port, uint32_t teid)
{
    dsmf_sess_t *sess = NULL;
    dsmf_event_t *e = NULL;
    
    ogs_assert(gnb_id);
    ogs_assert(session_id);
    ogs_assert(ran_addr_str);
    
    /* 创建或更新会话 */
    sess = dsmf_sess_add_by_ran_sync(gnb_id, session_id, ran_addr_str, ran_port, teid);
    if (sess) {
        sess->ran_sync_completed = true;
        ogs_info("[DSMF] Added gNB with RAN info: %s (session=%s, addr=%s:%d, teid=0x%x)", 
                 gnb_id, session_id, ran_addr_str, ran_port, teid);
        
        /* 触发会话状态机处理 RAN 同步事件 */
        e = dsmf_event_new(DSMF_EVT_RAN_SYNC_REQUEST);
        ogs_assert(e);
        e->sess_id = sess->id;
        e->ran_sync.gnb_id = gnb_id;
        e->ran_sync.session_id = session_id;
        e->ran_sync.ran_addr_str = ran_addr_str;
        e->ran_sync.ran_port = ran_port;
        e->ran_sync.teid = teid;
        
        ogs_fsm_dispatch(&sess->sm, e);
        dsmf_event_free(e);
    }
}

void dsmf_forward_ran_info_to_df(const char *gnb_id, const char *session_id,
                                 const char *ran_addr_str, uint16_t ran_port, uint32_t teid)
{
    dsmf_sess_t *sess = NULL;
    
    ogs_assert(gnb_id);
    ogs_assert(session_id);
    ogs_assert(ran_addr_str);
    
    /* 查找会话 */
    sess = dsmf_sess_find_by_gnb_session(gnb_id, session_id);
    if (sess) {
        /* TODO: 通过 PFCP 向 DF 转发 RAN 信息 */
        ogs_info("[DSMF] Forward RAN info to DF: %s (session=%s, addr=%s:%d, teid=0x%x)", 
                 gnb_id, session_id, ran_addr_str, ran_port, teid);
    } else {
        ogs_warn("[DSMF] Session not found for RAN info forwarding: %s-%s", gnb_id, session_id);
    }
}
