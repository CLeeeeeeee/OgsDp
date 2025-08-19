#include "context.h"
#include "dn3-path.h"
#include "pfcp-path.h"
#include "event.h"

static ogs_thread_t *thread;
static int initialized = 0;
static void df_main(void *data);

int df_initialize(void)
{
    int rv;

    ogs_assert(!initialized);

    // 1. 初始化 PFCP 上下文
    ogs_pfcp_context_init();

    // 2. 解析 PFCP 配置
    rv = ogs_pfcp_context_parse_config("df", "dsmf");
    if (rv != OGS_OK) {
        ogs_error("Failed to parse PFCP configuration");
        return rv;
    }

    // 3. 初始化 DF 上下文
    df_context_init();

    // 4. 解析 DF 配置
    rv = df_context_parse_config();
    if (rv != OGS_OK) {
        ogs_error("Failed to parse DF configuration");
        return rv;
    }

    // 5. 打开DN3接口
    rv = df_dn3_open();
    if (rv != OGS_OK) {
        ogs_error("Failed to open DN3 interface");
        return rv;
    }

    // 6. 打开PFCP接口
    rv = df_pfcp_open();
    if (rv != OGS_OK) {
        ogs_error("Failed to open PFCP interface");
        return rv;
    }

    // 7. 创建主线程
    thread = ogs_thread_create(df_main, NULL);
    if (!thread) {
        ogs_error("Failed to create DF main thread");
        return OGS_ERROR;
    }

    initialized = 1;
    ogs_info("DF initialized");
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
    df_dn3_close();
    df_pfcp_close();

    // 3. 清理上下文
    df_context_final();

    initialized = 0;
    ogs_info("DF terminated");
}

static void df_main(void *data)
{
    df_event_t *e = NULL;
    int rv;

    // ogs_assert(data); // 移除这个断言，因为我们可以传递 NULL

    for (;;) {
        /* 1. poll 所有 fd */
        ogs_pollset_poll(ogs_app()->pollset,
                         ogs_timer_mgr_next(ogs_app()->timer_mgr));

        /* 2. 处理到期定时器 */
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);

        /* 3. 取队列事件，处理事件 */
        for (;;) {
            rv = ogs_queue_trypop(ogs_app()->queue, (void **)&e);

            if (rv == OGS_DONE)
                goto done;                 /* 全部事件已处理且 app 结束 */

            if (rv == OGS_RETRY)
                break;                     /* 队列暂空，去 poll */

            ogs_assert(e);

            switch (e->id) {
            case DF_EVT_PFCP_MESSAGE:
                df_pfcp_handle_message(e->pfcp_node, e->pfcp_message);
                break;
            default:
                ogs_warn("Unknown event %d", e->id);
                break;
            }

            df_event_free(e);
        }
    }
    done:
    ogs_info("DF main loop terminated");
} 