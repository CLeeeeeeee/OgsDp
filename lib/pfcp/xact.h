/*
 * Copyright (C) 2019-2023 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#if !defined(OGS_PFCP_INSIDE) && !defined(OGS_PFCP_COMPILATION)
#error "This header cannot be included directly."
#endif

#ifndef OGS_PFCP_XACT_H
#define OGS_PFCP_XACT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Transaction context
 */
typedef struct ogs_pfcp_xact_s {
    ogs_lnode_t     lnode;          /**< A node of list 链表节点 */
    ogs_lnode_t     tmpnode;        /**< A node of temp-list 临时链表节点 */

    ogs_pool_id_t   id;             /**< Transaction ID 事务ID */

    ogs_pool_id_t   index;          /**< Index 索引 */

#define OGS_PFCP_LOCAL_ORIGINATOR  0  /**< Local originator 本地发起方 */
#define OGS_PFCP_REMOTE_ORIGINATOR 1  /**< Remote originator 远程发起方 */
    uint8_t         org;            /**< Transaction' originator.发起方 本地或远程
                                         local or remote */

    uint32_t        xid;            /**< Transaction ID 事务ID */
    ogs_pfcp_node_t *node;          /**< Relevant PFCP node context 相关PFCP节点上下文 */

    /**< Local timer expiration handler & Data*/
    void (*cb)(ogs_pfcp_xact_t *, void *);  /**< Local timer expiration handler 本地定时器过期处理函数 */
    void            *data;                 /**< Data 数据 */

    int             step;           /**< Current step in the sequence. 当前步骤
                                         1 : Initial 初始
                                         2 : Triggered 触发
                                         3 : Triggered-Reply 触发-回复 */
    struct {
        uint8_t     type;           /**< Message type history 消息类型历史 */
        ogs_pkbuf_t *pkbuf;         /**< Packet history 包历史 */
    } seq[3];                       /**< history for the each step 每个步骤的历史 */

    ogs_timer_t     *tm_response;   /**< Timer waiting for next message 等待下一个消息的定时器 */
    uint8_t         response_rcount; /**< Response retry count 回复重试计数 */
    ogs_timer_t     *tm_holding;    /**< Timer waiting for holding message 等待保持消息的定时器 */
    uint8_t         holding_rcount; /**< Holding retry count 保持重试计数 */

    ogs_timer_t     *tm_delayed_commit; /**< Timer waiting for commit xact 等待提交事务的定时器 */

    uint64_t        local_seid;     /**< Local SEID, 本地SEID，预期在回复中 */

    ogs_pool_id_t   assoc_xact_id;  /**< Associated GTP transaction ID 关联GTP事务ID */
    ogs_pkbuf_t     *gtpbuf;        /**< GTP packet buffer GTP包缓冲区 */

    uint8_t         gtp_pti;        /**< GTP Procedure transaction identity GTP过程事务标识 */
    uint8_t         gtp_cause;      /**< GTP Cause Value GTP原因值 */

    ogs_pool_id_t   assoc_stream_id;  /**< Associated SBI session ID 关联SBI会话ID */

    bool            epc;            /**< EPC or 5GC EPC或5GC */

#define OGS_PFCP_CREATE_RESTORATION_INDICATION ((uint64_t)1<<0)
    uint64_t        create_flags;   /**< Create flags 创建标志 */

#define OGS_PFCP_MODIFY_SESSION ((uint64_t)1<<0)
#define OGS_PFCP_MODIFY_DL_ONLY ((uint64_t)1<<1)
#define OGS_PFCP_MODIFY_UL_ONLY ((uint64_t)1<<2)
#define OGS_PFCP_MODIFY_INDIRECT ((uint64_t)1<<3)
#define OGS_PFCP_MODIFY_UE_REQUESTED ((uint64_t)1<<4)
#define OGS_PFCP_MODIFY_NETWORK_REQUESTED ((uint64_t)1<<5)
#define OGS_PFCP_MODIFY_CREATE ((uint64_t)1<<6)
#define OGS_PFCP_MODIFY_REMOVE ((uint64_t)1<<7)
#define OGS_PFCP_MODIFY_EPC_TFT_UPDATE ((uint64_t)1<<8)
#define OGS_PFCP_MODIFY_EPC_QOS_UPDATE ((uint64_t)1<<9)
#define OGS_PFCP_MODIFY_TFT_NEW ((uint64_t)1<<10)
#define OGS_PFCP_MODIFY_TFT_ADD ((uint64_t)1<<11)
#define OGS_PFCP_MODIFY_TFT_REPLACE ((uint64_t)1<<12)
#define OGS_PFCP_MODIFY_TFT_DELETE ((uint64_t)1<<13)
#define OGS_PFCP_MODIFY_QOS_CREATE ((uint64_t)1<<14)
#define OGS_PFCP_MODIFY_QOS_MODIFY ((uint64_t)1<<15)
#define OGS_PFCP_MODIFY_QOS_DELETE ((uint64_t)1<<16)
#define OGS_PFCP_MODIFY_OUTER_HEADER_REMOVAL ((uint64_t)1<<17)
#define OGS_PFCP_MODIFY_ACTIVATE ((uint64_t)1<<18)
#define OGS_PFCP_MODIFY_DEACTIVATE ((uint64_t)1<<19)
#define OGS_PFCP_MODIFY_END_MARKER ((uint64_t)1<<20)
#define OGS_PFCP_MODIFY_ERROR_INDICATION ((uint64_t)1<<21)
#define OGS_PFCP_MODIFY_XN_HANDOVER ((uint64_t)1<<22)
#define OGS_PFCP_MODIFY_N2_HANDOVER ((uint64_t)1<<23)
#define OGS_PFCP_MODIFY_HANDOVER_CANCEL ((uint64_t)1<<24)
#define OGS_PFCP_MODIFY_HOME_ROUTED_ROAMING ((uint64_t)1<<25)
#define OGS_PFCP_MODIFY_URR  ((uint64_t)1<<26) /* type of trigger */
#define OGS_PFCP_MODIFY_URR_MEAS_METHOD ((uint64_t)1<<27)
#define OGS_PFCP_MODIFY_URR_REPORT_TRIGGER ((uint64_t)1<<28)
#define OGS_PFCP_MODIFY_URR_QUOTA_VALIDITY_TIME ((uint64_t)1<<29)
#define OGS_PFCP_MODIFY_URR_VOLUME_QUOTA ((uint64_t)1<<30)
#define OGS_PFCP_MODIFY_URR_TIME_QUOTA ((uint64_t)1<<31)
#define OGS_PFCP_MODIFY_URR_VOLUME_THRESH ((uint64_t)1<<32)
#define OGS_PFCP_MODIFY_URR_TIME_THRESH ((uint64_t)1<<33)
    uint64_t        modify_flags;   /**< Modify flags 修改标志 */

#define OGS_PFCP_DELETE_TRIGGER_LOCAL_INITIATED 1
#define OGS_PFCP_DELETE_TRIGGER_UE_REQUESTED 2
#define OGS_PFCP_DELETE_TRIGGER_PCF_INITIATED 3
#define OGS_PFCP_DELETE_TRIGGER_RAN_INITIATED 4
#define OGS_PFCP_DELETE_TRIGGER_SMF_INITIATED 5
#define OGS_PFCP_DELETE_TRIGGER_AMF_RELEASE_SM_CONTEXT 6
#define OGS_PFCP_DELETE_TRIGGER_AMF_UPDATE_SM_CONTEXT 7
    int             delete_trigger;   /**< Delete trigger 删除触发 */

    ogs_list_t      pdr_to_create_list; /**< PDR to create list 创建PDR列表 */
    ogs_list_t      bearer_to_modify_list; /**< Bearer to modify list 修改Bearer列表 */
} ogs_pfcp_xact_t;

int ogs_pfcp_xact_init(void);         /**< Initialize PFCP transaction 初始化PFCP事务 */
void ogs_pfcp_xact_final(void);       /**< Finalize PFCP transaction 最终化PFCP事务 */

ogs_pfcp_xact_t *ogs_pfcp_xact_local_create(ogs_pfcp_node_t *node,
        void (*cb)(ogs_pfcp_xact_t *xact, void *data), void *data); /**< Create local PFCP transaction 创建本地PFCP事务 */
void ogs_pfcp_xact_delete_all(ogs_pfcp_node_t *node); /**< Delete all PFCP transactions 删除所有PFCP事务 */
ogs_pfcp_xact_t *ogs_pfcp_xact_find_by_id(ogs_pool_id_t id); /**< Find PFCP transaction by ID 按ID查找PFCP事务 */

int ogs_pfcp_xact_update_tx(ogs_pfcp_xact_t *xact,
        ogs_pfcp_header_t *hdesc, ogs_pkbuf_t *pkbuf); /**< Update PFCP transaction 更新PFCP事务 */

int ogs_pfcp_xact_commit(ogs_pfcp_xact_t *xact); /**< Commit PFCP transaction 提交PFCP事务 */
void ogs_pfcp_xact_delayed_commit(ogs_pfcp_xact_t *xact, ogs_time_t duration); /**< Delay commit PFCP transaction 延迟提交PFCP事务 */

int ogs_pfcp_xact_delete(ogs_pfcp_xact_t *xact); /**< Delete PFCP transaction 删除PFCP事务 */

int ogs_pfcp_xact_receive(ogs_pfcp_node_t *node,
        ogs_pfcp_header_t *h, ogs_pfcp_xact_t **xact); /**< Receive PFCP message 接收PFCP消息 */

#ifdef __cplusplus
}
#endif

#endif /* OGS_PFCP_XACT_H */
