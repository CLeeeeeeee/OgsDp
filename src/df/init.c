#include "context.h"
#include "gtp-path.h"
#include "pfcp-path.h"

static ogs_thread_t *thread;
static void df_main(void *data);

static int initialized = 0;

int df_initialize(void)
{
    int rv;

#define APP_NAME "df"
    rv = ogs_app_parse_local_conf(APP_NAME);
    if (rv != OGS_OK) return rv;

    ogs_gtp_context_init(OGS_MAX_NUM_OF_GTPU_RESOURCE); 
    //TODO 上面这个函数似乎是独属于UPF的，可能需要自己修改
    ogs_pfcp_context_init();

    df_context_init();
    df_event_init();
    df_gtp_init();
    //TODO 上面三个函数似乎仍未实现 需要挨个验证

    rv = ogs_pfcp_xact_init();   //后续改QUIC这函数要改
    if (rv != OGS_OK) return rv;

    rv = ogs_log_config_domain(
            ogs_app()->logger.domain, ogs_app()->logger.level);
    if (rv != OGS_OK) return rv;

    rv = ogs_gtp_context_parse_config(APP_NAME, "dsmf")
    if (rv != OGS_OK) return rv;

    rv = ogs_pfcp_context_parse_config(APP_NAME, "dsmf");
    if (rv != OGS_OK) return rv;

    rv = df_context_parse_config();
    if (rv != OGS_OK) return rv;

    rv = df_pfcp_open();
    if (rv != OGS_OK) return rv;

    rv = df_gtp_open();
    if (rv != OGS_OK) return rv;

    thread = ogs_thread_create(df_main, NULL);
    if (!thread) return OGS_ERROR;

    initialized = 1;
    
    return OGS_OK;
}

void df_terminate(void)
{
    if (!initialized) return;

    // 1. 销毁主线程
    if (thread) {
        ogs_thread_destroy(thread);
        thread = NULL;
    }

    // 2. 关闭接口
    df_sbi_close();
    df_dn3_close();
    df_pfcp_close();

    // 3. 清理上下文
    df_context_final();
    ogs_pfcp_context_final();
    ogs_pfcp_xact_final();

    // 4. 释放事件池
    df_event_final();

    initialized = 0;
    ogs_info("DF terminated");
}

static void df_main(void *data)
{
    ogs_fsm_t df_sm;
    df_event_t e;
    df_event_t *event = NULL;
    int rv;

    // 初始化状态机
    memset(&e, 0, sizeof(e));
    e.h.id = OGS_FSM_ENTRY_SIG;
    ogs_fsm_init(&df_sm, df_state_initial, df_state_final, &e);

    for (;;) {
        /* 1. poll 所有 fd */
        ogs_pollset_poll(ogs_app()->pollset,
                         ogs_timer_mgr_next(ogs_app()->timer_mgr));

        /* 2. 处理到期定时器 */
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);

        /* 3. 取队列事件，处理事件 */
        for (;;) {
            rv = ogs_queue_trypop(ogs_app()->queue, (void **)&event);

            if (rv == OGS_DONE)
                goto done;                 /* 全部事件已处理且 app 结束 */

            if (rv == OGS_RETRY)
                break;                     /* 队列暂空，去 poll */

            ogs_assert(event);

            /* 使用状态机处理事件 */
            ogs_fsm_dispatch(&df_sm, event);

            df_event_free(event);
        }
    }
    done:
    ogs_fsm_fini(&df_sm, &e);
    ogs_info("DF main loop terminated");
} 