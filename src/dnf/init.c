/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: init.c
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#include "ogs-core.h"
#include "ogs-app.h"
#include "context.h"
#include "event.h"
#include "sbi-path.h"
#include "gtpu-path.h"
#include "dnf-sm.h"

static ogs_thread_t *thread = NULL;
static int initialized = 0;
static void dnf_main(void *data);

int dnf_initialize(void)
{
    int rv;

    // 0. 读取本地配置文件（dnf.yaml）
    rv = ogs_app_parse_local_conf("dnf");
    if (rv != OGS_OK) return rv;

    // 1. 初始化 SBI 上下文，声明自己是 NF=DNF
    ogs_sbi_context_init(OpenAPI_nf_type_DNF);

    // 1.1 应用日志级别
    rv = ogs_log_config_domain(ogs_app()->logger.domain, ogs_app()->logger.level);
    if (rv != OGS_OK) return rv;

    // 2. 初始化事件池
    dnf_event_init();

    // 3. 初始化 DNF 上下文
    rv = dnf_context_init();
    if (rv != OGS_OK) {
        ogs_error("Failed to initialize DNF context");
        return rv;
    }

    // 4. 解析 DNF 配置
    rv = dnf_context_parse_config();
    if (rv != OGS_OK) {
        ogs_error("Failed to parse DNF configuration");
        return rv;
    }

    // 5. 解析 SBI 配置
    rv = ogs_sbi_context_parse_config("dnf", "nrf", "scp");
    if (rv != OGS_OK) {
        ogs_error("Failed to parse SBI configuration");
        return rv;
    }

    // 6. 初始化 SBI 路径 */
    rv = dnf_sbi_init();
    if (rv != OGS_OK) {
        ogs_error("Failed to initialize DNF SBI");
        return rv;
    }

    // 7. 初始化 GTP-U 路径 */
    rv = dnf_gtpu_init();
    if (rv != OGS_OK) {
        ogs_error("Failed to initialize DNF GTP-U");
        return rv;
    }

    // 8. 打开 SBI 服务器 */
    rv = dnf_sbi_open();
    if (rv != OGS_OK) {
        ogs_error("Failed to open DNF SBI server");
        return rv;
    }

    // 9. 打开 GTP-U 服务器 */
    rv = dnf_gtpu_open();
    if (rv != OGS_OK) {
        ogs_error("Failed to open DNF GTP-U server");
        return rv;
    }

    // 10. 创建工作线程，跑事件循环 */
    thread = ogs_thread_create(dnf_main, NULL);
    if (!thread) {
        ogs_error("Failed to create DNF main thread");
        return OGS_ERROR;
    }

    initialized = 1;
    ogs_info("DNF initialized successfully");
    return OGS_OK;
}

void dnf_terminate(void)
{
    if (!initialized) return;

    ogs_info("DNF terminating...");

    // 1. 销毁主线程
    if (thread) {
        ogs_thread_destroy(thread);
        thread = NULL;
    }

    // 2. 关闭接口
    dnf_gtpu_close();
    dnf_sbi_close();
    dnf_context_final();

    // 3. 清理事件池
    dnf_event_final();

    initialized = 0;
    ogs_info("DNF terminated");
}

static void dnf_main(void *data)
{
    ogs_fsm_t dnf_sm;
    dnf_event_t e;
    int rv;

    memset(&e, 0, sizeof(e));
    e.h.id = OGS_FSM_ENTRY_SIG;
    ogs_fsm_init(&dnf_sm, dnf_state_initial, dnf_state_final, &e);

    for ( ;; ) {
        ogs_pollset_poll(ogs_app()->pollset,
                ogs_timer_mgr_next(ogs_app()->timer_mgr));

        /*
         * After ogs_pollset_poll(), ogs_timer_mgr_expire() must be called.
         *
         * The reason is why ogs_timer_mgr_next() can get the current value
         * when ogs_timer_stop() is called internally in ogs_timer_mgr_expire().
         *
         * You should not use event-queue before ogs_timer_mgr_expire().
         * In this case, ogs_timer_mgr_expire() does not work
         * because 'if rv == OGS_DONE' statement is exiting and
         * not calling ogs_timer_mgr_expire().
         */
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);

        for ( ;; ) {
            dnf_event_t *e = NULL;

            rv = ogs_queue_trypop(ogs_app()->queue, (void**)&e);
            ogs_assert(rv != OGS_ERROR);

            if (rv == OGS_DONE)
                goto done;

            if (rv == OGS_RETRY)
                break;

            ogs_assert(e);
            ogs_fsm_dispatch(&dnf_sm, e);
            dnf_event_free(e);
        }
    }
done:

    ogs_fsm_fini(&dnf_sm, 0);
} 