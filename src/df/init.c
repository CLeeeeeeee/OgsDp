/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: init.c
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:04 CST
 */

#include "context.h"
#include "dn3-path.h"
#include "pfcp-path.h"
#include "sbi-path.h"
#include "event.h"
#include "timer.h"
#include "ogs-core.h"

static ogs_thread_t *thread;
static int initialized = 0;
static void df_main(void *data);

int df_initialize(void)
{
    int rv;

    ogs_assert(!initialized);

    // 0. 读取本地配置文件（df.yaml）
    rv = ogs_app_parse_local_conf("df");
    if (rv != OGS_OK) return rv;

    // 1. 初始化 PFCP 上下文与事务池（参考 UPF）
    ogs_pfcp_context_init();
    rv = ogs_pfcp_xact_init();
    if (rv != OGS_OK) return rv;

    // 2. 解析 PFCP 配置（DF 作为 PFCP 服务器）
    rv = ogs_pfcp_context_parse_config("df", NULL);
    if (rv != OGS_OK) {
        ogs_error("Failed to parse PFCP configuration");
        return rv;
    }

    // 3. 初始化 SBI 上下文，声明自己是 NF=DF
    ogs_sbi_context_init(OpenAPI_nf_type_DF);

    // 3.1 应用日志级别
    rv = ogs_log_config_domain(ogs_app()->logger.domain, ogs_app()->logger.level);
    if (rv != OGS_OK) return rv;

    // 4. 初始化 DF 上下文
    df_context_init();

    // 4.1 初始化事件池（供 PFCP/SBI 回调派发）
    df_event_init();

    // 5. 解析 SBI 配置
    rv = ogs_sbi_context_parse_config("df", "nrf", "scp");
    if (rv != OGS_OK) {
        ogs_error("Failed to parse SBI configuration");
        return rv;
    }

    // 6. 解析 DF 配置
    rv = df_context_parse_config();
    if (rv != OGS_OK) {
        ogs_error("Failed to parse DF configuration");
        return rv;
    }

    // 7. 打开 SBI 接口
    rv = df_sbi_open();
    if (rv != OGS_OK) {
        ogs_error("Failed to open SBI interface");
        return rv;
    }

    // 8. 打开DN3接口
    rv = df_dn3_open();
    if (rv != OGS_OK) {
        ogs_error("Failed to open DN3 interface");
        return rv;
    }

    // 8.1 将 DN3 socket 添加到事件循环
    if (df_dn3_sock()) {
        ogs_pollset_add(ogs_app()->pollset, OGS_POLLIN, df_dn3_sock()->fd, df_dn3_recv_cb, NULL);
        ogs_info("DN3 socket added to event loop");
    }

    // 8.5 生成 PFCP UE 池（与SMF/UPF一致）
    rv = ogs_pfcp_ue_pool_generate();
    if (rv != OGS_OK) return rv;

    // 9. 打开PFCP接口
    rv = df_pfcp_open();
    if (rv != OGS_OK) {
        ogs_error("Failed to open PFCP interface");
        return rv;
    }

    // 10. 发现 DNF 服务
    rv = df_discover_dnf();
    if (rv != OGS_OK) {
        ogs_warn("Failed to discover DNF, using default configuration");
    }

    // 11. 心跳定时器由 Open5GS 标准机制自动管理，无需手动创建
    ogs_info("DF NRF heartbeat will be managed by Open5GS standard mechanism");

    // 11. 创建主线程
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