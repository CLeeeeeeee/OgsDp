/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: init.c
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:07 CST
 */


/*
 *  src/dmf/init.c
 *  ----------------
 *  DMF 进程入口：解析 YAML → 初始化上下文 → 打开 SBI → 启动主循环
 */
#include "sbi-path.h"         /* ogs_sbi_* API */
// #include "metrics.h"          /* 如需统计指标可保留 */
#include "context.h"          /* dmf_context_* */
#include "dmf-sm.h"           /* 状态机头，可先留空壳 */
#include "ogs-core.h"

static ogs_thread_t *thread       = NULL;
static ogs_timer_t  *t_term_hold  = NULL;
static int           initialized  = 0;
static ogs_timer_t *t_gnb_count  = NULL;

static void print_gnb_count(void *data) {
    dmf_event_t *e = dmf_event_print_gnb_list_new();
    if (e) {
        ogs_queue_push(ogs_app()->queue, e);
        ogs_pollset_notify(ogs_app()->pollset);
    }
    // 重新启动定时器，每分钟执行一次
    ogs_timer_start(t_gnb_count, ogs_time_from_sec(60));
}

/* ---------- 前向声明 ---------- */
static void dmf_main(void *data);
static void event_termination(void);

/*=====================================================================
 *  初始化
 *===================================================================*/
int dmf_initialize(void)
{
    int rv;

    /* 1. 读取本地配置文件（dmf.yaml） */
    rv = ogs_app_parse_local_conf("dmf");
    if (rv != OGS_OK) return rv;

    /* 2. (可选) 初始化 metrics */
    // dmf_metrics_init();

    /* 3. 初始化 SBI 全局上下文，声明自己是 NF=DMF */
    ogs_sbi_context_init(OpenAPI_nf_type_DMF);

    /* 4. 初始化 DMF 专用上下文（链表 / pool / hash …） */
    dmf_context_init();

    /* 5. 应用日志级别 */
    rv = ogs_log_config_domain(
            ogs_app()->logger.domain, ogs_app()->logger.level);
    if (rv != OGS_OK) return rv;

    /* 6. 解析 YAML 中的「sbi / nrf / scp」段落。
       参数分别为：本地 section 名、NRF section 名、SCP section 名 (可无)。 */
    rv = ogs_sbi_context_parse_config("dmf", "nrf", "scp");
    if (rv != OGS_OK) return rv;

    /* 7. 解析 DMF 自己的 YAML (nodeB 列表之类) */
    rv = dmf_context_parse_config();
    if (rv != OGS_OK) return rv;

    /* 若 DMF 需要把 NF-Info 写进 Profile，可在这里填充 */
    rv = dmf_context_nf_info();          /* 留空返回 OGS_OK 亦可 */
    if (rv != OGS_OK) return rv;

    /* 8. 打开 metrics (Prometheus) 端口 */
    // ogs_metrics_context_open(ogs_metrics_self());

    /* 9. 打开 SBI Server / 建立到 NRF 的客户端，并启动 NF-FSM 注册 */
    rv = dmf_sbi_open();
    if (rv != OGS_OK) return rv;

    /* ---------- 如果 DMF 还有其它接口 (如 QUIC) 在此打开 ---------- */
    /* rv = dmf_quic_open(); */

    /* 10. 创建工作线程，跑事件循环 */
    thread = ogs_thread_create(dmf_main, NULL);
    if (!thread) return OGS_ERROR;

    initialized = 1;

    // 启动定时打印gNB数量的定时器
    t_gnb_count = ogs_timer_add(ogs_app()->timer_mgr, print_gnb_count, NULL);
    ogs_assert(t_gnb_count);
    ogs_timer_start(t_gnb_count, ogs_time_from_sec(60));
    return OGS_OK;
}

/*=====================================================================
 *  终止
 *===================================================================*/
void dmf_terminate(void)
{
    if (!initialized) return;

    // 删除定时器
    if (t_gnb_count) {
        ogs_timer_delete(t_gnb_count);
        t_gnb_count = NULL;
    }

    /* 1. 通知主循环退出 */
    event_termination();
    ogs_thread_destroy(thread);

    /* 2. 关闭 SBI / 释放上下文 */
    dmf_sbi_close();
    dmf_context_final();
    ogs_sbi_context_final();
}

/*=====================================================================
 *  私有：状态机 / 主循环
 *===================================================================*/
static void event_termination(void)
{
    /* 让所有 NF-FSM 进入终止态，然后启动一个 300 ms 的保持计时器 */
    ogs_sbi_nf_instance_t *nf = NULL;
    ogs_list_for_each(&ogs_sbi_self()->nf_instance_list, nf)
    ogs_sbi_nf_fsm_fini(nf);

    t_term_hold = ogs_timer_add(ogs_app()->timer_mgr, NULL, NULL);
    ogs_assert(t_term_hold);
    ogs_timer_start(t_term_hold, ogs_time_from_msec(300));

    ogs_queue_term(ogs_app()->queue);
    ogs_pollset_notify(ogs_app()->pollset);
}

static void dmf_main(void *data)
{
    /* ——— DMF 全局状态机；可以暂时让它什么都不做 ——— */
    ogs_fsm_t dmf_sm;
    ogs_fsm_init(&dmf_sm, dmf_state_initial, dmf_state_final, NULL);

    for (;;) {
        /* 1. poll 所有 fd；如果没有 fd 丰富，可用空 poll */
        ogs_pollset_poll(ogs_app()->pollset,
                         ogs_timer_mgr_next(ogs_app()->timer_mgr));

        /* 2. 处理到期定时器 */
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);

        /* 3. 取队列事件，丢给状态机 */
        for (;;) {
            dmf_event_t *e = NULL;
            int rv = ogs_queue_trypop(ogs_app()->queue, (void **)&e);

            if (rv == OGS_DONE)
                goto done;                 /* 全部事件已处理且 app 结束 */

            if (rv == OGS_RETRY)
                break;                     /* 队列暂空，去 poll */

            ogs_assert(e);
            ogs_fsm_dispatch(&dmf_sm, e);
            ogs_event_free(e);
        }
    }
    done:
    ogs_fsm_fini(&dmf_sm, NULL);
}

/*=====================================================================
 *  统计辅助（可做可不做）
 *===================================================================*/
// static int num_of_nodeb = 0;

// static void stats_add_nodeb(void)
// {
//     /* dmf_metrics_inst_global_inc(DMF_METR_GLOB_NODEB); */
//     num_of_nodeb++;
//     ogs_info("[Added] Number of NodeBs is now %d", num_of_nodeb);
// }
// static void stats_remove_nodeb(void)
// {
//     /* dmf_metrics_inst_global_dec(DMF_METR_GLOB_NODEB); */
//     num_of_nodeb--;
//     ogs_info("[Removed] Number of NodeBs is now %d", num_of_nodeb);
// }

/* Session 计数器如无需可先注释 */
//static void stats_add_dmf_session(void)   { /* ... */ }
//static void stats_remove_dmf_session(void){ /* ... */ }

