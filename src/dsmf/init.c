/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: init.c
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:11 CST
 */

#include "context.h"
#include "pfcp-path.h"
#include "sbi-path.h"
#include "dsmf-sm.h"
#include "ogs-core.h"

static ogs_thread_t *thread = NULL;
static int initialized = 0;

static void dsmf_main(void *data);

int dsmf_initialize(void)
{
    int rv;

    /* 1. 读取本地配置文件（dsmf.yaml） */
    rv = ogs_app_parse_local_conf("dsmf");
    if (rv != OGS_OK) return rv;

    /* 2. 初始化 PFCP 上下文与事务池（参考 SMF） */
    ogs_pfcp_context_init();
    rv = ogs_pfcp_xact_init();
    if (rv != OGS_OK) return rv;

    /* 3. 初始化 SBI 全局上下文，声明自己是 NF=DSMF */
    ogs_sbi_context_init(OpenAPI_nf_type_DSMF);

    /* 4. 初始化 DSMF 专用上下文 */
    dsmf_context_init();

    /* 5. 应用日志级别 */
    rv = ogs_log_config_domain(
            ogs_app()->logger.domain, ogs_app()->logger.level);
    if (rv != OGS_OK) return rv;

    /* 6. 解析 YAML 中的「sbi / nrf / scp」段落 */
    rv = ogs_sbi_context_parse_config("dsmf", "nrf", "scp");
    if (rv != OGS_OK) return rv;

    /* 7. 解析 PFCP 配置（DSMF 作为 PFCP 服务器+客户端） */
    rv = ogs_pfcp_context_parse_config("dsmf", "df");
    if (rv != OGS_OK) return rv;

    /* 8. 打开 PFCP 接口 */
    rv = dsmf_pfcp_open();
    if (rv != OGS_OK) return rv;

    /* 9. 打开 SBI 接口 */
    ogs_info("[DSMF] Opening SBI interface...");
    rv = dsmf_sbi_open();
    if (rv != OGS_OK) {
        ogs_error("[DSMF] Failed to open SBI interface");
        return rv;
    }
    ogs_info("[DSMF] SBI interface opened successfully");

    /* 10. 创建工作线程，跑事件循环 */
    thread = ogs_thread_create(dsmf_main, NULL);
    if (!thread) return OGS_ERROR;

    initialized = 1;

    return OGS_OK;
}

void dsmf_terminate(void)
{
    if (!initialized) return;

    /* 停止工作线程 */
    ogs_thread_destroy(thread);

    /* 关闭接口 */
    dsmf_pfcp_close();
    dsmf_sbi_close();

    /* 清理上下文 */
    dsmf_context_final();
    ogs_pfcp_context_final();
    ogs_pfcp_xact_final();
}

static void dsmf_main(void *data)
{
    ogs_fsm_t dsmf_sm;
    int rv;

    ogs_fsm_init(&dsmf_sm, dsmf_state_initial, dsmf_state_final, 0);

    for ( ;; ) {
        ogs_pollset_poll(ogs_app()->pollset, ogs_timer_mgr_next(ogs_app()->timer_mgr));//获取下一个到期定时器
        //After ogs_pollset_poll(), ogs_timer_mgr_expire() must be called.
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);//触发到期定时器

        for ( ;; ) {
            dsmf_event_t *e = NULL;

            rv = ogs_queue_trypop(ogs_app()->queue, (void**)&e);
            ogs_assert(rv != OGS_ERROR);

            if (rv == OGS_DONE)
                goto done;

            if (rv == OGS_RETRY)
                break;

            ogs_assert(e);
            ogs_fsm_dispatch(&dsmf_sm, e);
            dsmf_event_free(e);
        }
    }
done:

    ogs_fsm_fini(&dsmf_sm, 0);
}
