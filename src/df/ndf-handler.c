#include "context.h"
#include "ndf-handler.h"
#include "ndf-build.h"
#include "pfcp-path.h"
#include "ogs-core.h"

/* 暂时注释掉 URR 处理函数 */
/*
static int df_n4_handle_create_urr(df_sess_t *sess, ogs_pfcp_urr_t *urr)
{
    ogs_assert(sess);
    ogs_assert(urr);

    df_sess_urr_acc_timers_setup(sess, urr);

    return OGS_OK;
}
*/

void df_n4_handle_session_establishment_request(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_establishment_request_t *req)
{
    ogs_pfcp_pdr_t *pdr = NULL;
    ogs_pfcp_far_t *far = NULL;
    ogs_pfcp_pdr_t *created_pdr[OGS_MAX_NUM_OF_PDR];
    int num_of_created_pdr = 0;
    uint8_t cause_value = 0;
    uint8_t offending_ie_value = 0;
    int i;

    ogs_pfcp_sereq_flags_t sereq_flags;
    bool restoration_indication = false;

    // TODO: df_metrics_inst_global_inc(DF_METR_GLOB_CTR_SM_N4SESSIONESTABREQ);

    ogs_assert(xact);
    ogs_assert(req);

    ogs_debug("Session Establishment Request");

    cause_value = OGS_PFCP_CAUSE_REQUEST_ACCEPTED;

    if (!sess) {
        ogs_error("No Context");
        ogs_pfcp_send_error_message(xact, 0,
                OGS_PFCP_SESSION_ESTABLISHMENT_RESPONSE_TYPE,
                OGS_PFCP_CAUSE_MANDATORY_IE_MISSING, 0);
        // TODO: df_metrics_inst_by_cause_add(OGS_PFCP_CAUSE_MANDATORY_IE_MISSING,
        //         DF_METR_CTR_SM_N4SESSIONESTABFAIL, 1);
        return;
    }

    memset(&sereq_flags, 0, sizeof(sereq_flags));
    if (req->pfcpsereq_flags.presence == 1)
        sereq_flags.value = req->pfcpsereq_flags.u8;

    for (i = 0; i < OGS_MAX_NUM_OF_PDR; i++) {
        created_pdr[i] = ogs_pfcp_handle_create_pdr(&sess->pfcp,
                &req->create_pdr[i], &sereq_flags,
                &cause_value, &offending_ie_value);
        if (created_pdr[i] == NULL)
            break;
        
        // 注册 TEID 到哈希表
        if (created_pdr[i]->f_teid.teid) {
            ogs_hash_set(df_self()->teid_hash, &created_pdr[i]->f_teid.teid, 
                          sizeof(created_pdr[i]->f_teid.teid), sess);
            // 保存 RAN 地址和 TEID
            if (created_pdr[i]->f_teid.ipv4 || created_pdr[i]->f_teid.ipv6) {
                sess->ran_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
                if (created_pdr[i]->f_teid.ipv4) {
                    sess->ran_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
                    ogs_assert(sess->ran_addr);
                    sess->ran_addr->ogs_sa_family = AF_INET;
                    sess->ran_addr->sin.sin_addr.s_addr = created_pdr[i]->f_teid.ipv4;
                } else if (created_pdr[i]->f_teid.ipv6) {
                    sess->ran_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
                    ogs_assert(sess->ran_addr);
                    sess->ran_addr->ogs_sa_family = AF_INET6;
                    memcpy(&sess->ran_addr->sin6.sin6_addr, created_pdr[i]->f_teid.addr6, OGS_IPV6_LEN);
                }
                sess->ran_teid = created_pdr[i]->f_teid.teid;
                if (sess->ran_addr) {
                    char *ip = ogs_ipstrdup(sess->ran_addr);
                    ogs_info("[DF] Rcv RAN addr: addr=%s teid=0x%x", ip ? ip : "-", sess->ran_teid);
                    if (ip) ogs_free(ip);
                }
            }
        }
    }
    num_of_created_pdr = i;
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_FAR; i++) {
        if (ogs_pfcp_handle_create_far(&sess->pfcp, &req->create_far[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    /* 暂时注释掉 URR 处理 */
    /*
    df_n4_handle_create_urr(sess, &req->create_urr[0]);
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;
    */

    if (req->apn_dnn.presence) {
        char apn_dnn[OGS_MAX_DNN_LEN+1];

        if (ogs_fqdn_parse(apn_dnn, req->apn_dnn.data,
            ogs_min(req->apn_dnn.len, OGS_MAX_DNN_LEN)) <= 0) {
            ogs_error("Invalid APN");
            cause_value = OGS_PFCP_CAUSE_MANDATORY_IE_INCORRECT;
            goto cleanup;
        }

        if (sess->apn_dnn)
            ogs_free(sess->apn_dnn);
        sess->apn_dnn = ogs_strdup(apn_dnn);
        ogs_assert(sess->apn_dnn);
    }

    for (i = 0; i < OGS_MAX_NUM_OF_QER; i++) {
        if (ogs_pfcp_handle_create_qer(&sess->pfcp, &req->create_qer[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
        // TODO: df_metrics_inst_by_dnn_add(sess->apn_dnn,
        //         DF_METR_GAUGE_DF_QOSFLOWS, 1);
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    ogs_pfcp_handle_create_bar(&sess->pfcp, &req->create_bar,
                &cause_value, &offending_ie_value);
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    /* Setup GTP Node */
    ogs_list_for_each(&sess->pfcp.far_list, far) {
        if (OGS_ERROR == ogs_pfcp_setup_far_gtpu_node(far)) {
            ogs_fatal("CHECK CONFIGURATION: df.gtpu");
            ogs_fatal("ogs_pfcp_setup_far_gtpu_node() failed");
            goto cleanup;
        }
        if (far->gnode)
            ogs_pfcp_far_f_teid_hash_set(far);
    }

    /* PFCPSEReq-Flags */
    if (sereq_flags.restoration_indication == 1) {
        for (i = 0; i < num_of_created_pdr; i++) {
            pdr = created_pdr[i];
            ogs_assert(pdr);

            if (pdr->f_teid_len > 0 && pdr->f_teid.ch == false) {
                cause_value = ogs_pfcp_pdr_swap_teid(pdr);
                if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
                    goto cleanup;
            }
        }
        restoration_indication = true;
    }

    for (i = 0; i < num_of_created_pdr; i++) {
        pdr = created_pdr[i];
        ogs_assert(pdr);

        /* Setup UE IP address */
        if (pdr->ue_ip_addr_len) {
            if (req->pdn_type.presence == 1) {
                cause_value = df_sess_set_ue_ip(sess, req->pdn_type.u8, pdr);
                if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
                    goto cleanup;
            } else {
                ogs_error("No PDN Type");
            }
        }

        if (pdr->ipv4_framed_routes) {
            cause_value =
                df_sess_set_ue_ipv4_framed_routes(sess,
                        pdr->ipv4_framed_routes);
            if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
                goto cleanup;
        }

        if (pdr->ipv6_framed_routes) {
            cause_value =
                df_sess_set_ue_ipv6_framed_routes(sess,
                        pdr->ipv6_framed_routes);
            if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
                goto cleanup;
        }

        /* Setup DF-N3-TEID & QFI Hash */
        if (pdr->f_teid_len)
            ogs_pfcp_object_teid_hash_set(
                    OGS_PFCP_OBJ_SESS_TYPE, pdr, restoration_indication);
    }

    /* Send Buffered Packet to gNB/SGW */
    ogs_list_for_each(&sess->pfcp.pdr_list, pdr) {
        if (pdr->src_if == OGS_PFCP_INTERFACE_CORE) { /* Downlink */
            ogs_pfcp_send_buffered_packet(pdr);
        }
    }

    if (restoration_indication == true ||
        ogs_pfcp_self()->up_function_features.ftup == 0)
        ogs_assert(OGS_OK ==
            df_pfcp_send_session_establishment_response(
                xact, sess, NULL, 0));
    else
        ogs_assert(OGS_OK ==
            df_pfcp_send_session_establishment_response(
                xact, sess, created_pdr, num_of_created_pdr));

    return;

cleanup:
    // TODO: df_metrics_inst_by_cause_add(cause_value,
    //         DF_METR_CTR_SM_N4SESSIONESTABFAIL, 1);
    ogs_pfcp_sess_clear(&sess->pfcp);
    ogs_pfcp_send_error_message(xact, sess ? sess->dsmf_n4_f_seid.seid : 0,
            OGS_PFCP_SESSION_ESTABLISHMENT_RESPONSE_TYPE,
            cause_value, offending_ie_value);
}

void df_n4_handle_session_modification_request(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_modification_request_t *req)
{
    ogs_pfcp_pdr_t *pdr = NULL;
    ogs_pfcp_far_t *far = NULL;
    ogs_pfcp_pdr_t *created_pdr[OGS_MAX_NUM_OF_PDR];
    int num_of_created_pdr = 0;
    uint8_t cause_value = 0;
    uint8_t offending_ie_value = 0;
    int i;

    ogs_assert(xact);
    ogs_assert(req);

    ogs_debug("Session Modification Request");

    cause_value = OGS_PFCP_CAUSE_REQUEST_ACCEPTED;

    if (!sess) {
        ogs_error("No Context");
        ogs_pfcp_send_error_message(xact, 0,
                OGS_PFCP_SESSION_MODIFICATION_RESPONSE_TYPE,
                OGS_PFCP_CAUSE_SESSION_CONTEXT_NOT_FOUND, 0);
        return;
    }

    for (i = 0; i < OGS_MAX_NUM_OF_PDR; i++) {
        created_pdr[i] = ogs_pfcp_handle_create_pdr(&sess->pfcp,
                &req->create_pdr[i], NULL, &cause_value, &offending_ie_value);
        if (created_pdr[i] == NULL)
            break;
    }
    num_of_created_pdr = i;
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_PDR; i++) {
        if (ogs_pfcp_handle_update_pdr(&sess->pfcp, &req->update_pdr[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_PDR; i++) {
        if (ogs_pfcp_handle_remove_pdr(&sess->pfcp, &req->remove_pdr[i],
                &cause_value, &offending_ie_value) == false)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_FAR; i++) {
        if (ogs_pfcp_handle_create_far(&sess->pfcp, &req->create_far[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_FAR; i++) {
        if (ogs_pfcp_handle_update_far_flags(&sess->pfcp, &req->update_far[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    /* Send End Marker to gNB */
    ogs_list_for_each(&sess->pfcp.pdr_list, pdr) {
        if (pdr->src_if == OGS_PFCP_INTERFACE_CORE) { /* Downlink */
            far = pdr->far;
            if (far && far->smreq_flags.send_end_marker_packets)
                ogs_assert(OGS_ERROR != ogs_pfcp_send_end_marker(pdr));
        }
    }
    /* Clear PFCPSMReq-Flags */
    ogs_list_for_each(&sess->pfcp.far_list, far)
        far->smreq_flags.value = 0;

    for (i = 0; i < OGS_MAX_NUM_OF_FAR; i++) {
        if (ogs_pfcp_handle_update_far(&sess->pfcp, &req->update_far[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_FAR; i++) {
        if (ogs_pfcp_handle_remove_far(&sess->pfcp, &req->remove_far[i],
                &cause_value, &offending_ie_value) == false)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    /* 暂时注释掉 URR 处理 */
    /*
    df_n4_handle_create_urr(sess, &req->create_urr[0]);
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;
    */

    for (i = 0; i < OGS_MAX_NUM_OF_URR; i++) {
        if (ogs_pfcp_handle_update_urr(&sess->pfcp, &req->update_urr[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_URR; i++) {
        if (ogs_pfcp_handle_remove_urr(&sess->pfcp, &req->remove_urr[i],
                &cause_value, &offending_ie_value) == false)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_QER; i++) {
        if (ogs_pfcp_handle_create_qer(&sess->pfcp, &req->create_qer[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
        // TODO: df_metrics_inst_by_dnn_add(sess->apn_dnn,
        //         DF_METR_GAUGE_DF_QOSFLOWS, 1);
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_QER; i++) {
        if (ogs_pfcp_handle_update_qer(&sess->pfcp, &req->update_qer[i],
                    &cause_value, &offending_ie_value) == NULL)
            break;
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    for (i = 0; i < OGS_MAX_NUM_OF_QER; i++) {
        if (ogs_pfcp_handle_remove_qer(&sess->pfcp, &req->remove_qer[i],
                &cause_value, &offending_ie_value) == false)
            break;
        // TODO: df_metrics_inst_by_dnn_add(sess->apn_dnn,
        //         DF_METR_GAUGE_DF_QOSFLOWS, -1);
    }
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    ogs_pfcp_handle_create_bar(&sess->pfcp, &req->create_bar,
                &cause_value, &offending_ie_value);
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    ogs_pfcp_handle_remove_bar(&sess->pfcp, &req->remove_bar,
            &cause_value, &offending_ie_value);
    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED)
        goto cleanup;

    /* Setup GTP Node */
    ogs_list_for_each(&sess->pfcp.far_list, far) {
        if (OGS_ERROR == ogs_pfcp_setup_far_gtpu_node(far)) {
            ogs_fatal("CHECK CONFIGURATION: df.gtpu");
            ogs_fatal("ogs_pfcp_setup_far_gtpu_node() failed");
            goto cleanup;
        }
        if (far->gnode)
            ogs_pfcp_far_f_teid_hash_set(far);
    }

    for (i = 0; i < num_of_created_pdr; i++) {
        pdr = created_pdr[i];
        ogs_assert(pdr);

        /* Setup DF-N3-TEID & QFI Hash */
        if (pdr->f_teid_len)
            ogs_pfcp_object_teid_hash_set(OGS_PFCP_OBJ_SESS_TYPE, pdr, false);
    }

    /* Send Buffered Packet to gNB/SGW */
    ogs_list_for_each(&sess->pfcp.pdr_list, pdr) {
        if (pdr->src_if == OGS_PFCP_INTERFACE_CORE) { /* Downlink */
            ogs_pfcp_send_buffered_packet(pdr);
        }
    }

    if (ogs_pfcp_self()->up_function_features.ftup == 0)
        ogs_assert(OGS_OK ==
            df_pfcp_send_session_modification_response(
                xact, sess, NULL, 0));
    else
        ogs_assert(OGS_OK ==
            df_pfcp_send_session_modification_response(
                xact, sess, created_pdr, num_of_created_pdr));
    return;

cleanup:
    ogs_pfcp_sess_clear(&sess->pfcp);
    ogs_pfcp_send_error_message(xact, sess ? sess->dsmf_n4_f_seid.seid : 0,
            OGS_PFCP_SESSION_MODIFICATION_RESPONSE_TYPE,
            cause_value, offending_ie_value);
}

void df_n4_handle_session_deletion_request(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_deletion_request_t *req)
{
    ogs_assert(xact);
    ogs_assert(sess);
    ogs_assert(req);

    /* 暂时注释掉未使用的变量 */
    /*
    ogs_pfcp_pdr_t *pdr = NULL;
    ogs_pfcp_far_t *far = NULL;
    ogs_pfcp_urr_t *urr = NULL;
    ogs_pfcp_bar_t *bar = NULL;
    uint8_t cause_value = 0;
    uint8_t offending_ie_value = 0;
    */

    ogs_info("Session Deletion Request");

    /* 暂时注释掉 cause_value 相关代码 */
    /*
    cause_value = OGS_PFCP_CAUSE_REQUEST_ACCEPTED;
    */

    if (!sess) {
        df_pfcp_send_session_deletion_response(xact, sess);
        return;
    }

    /* 暂时注释掉 QER 处理 */
    /*
    ogs_list_for_each(&sess->pfcp.qer_list, qer) {
        ogs_pfcp_qer_remove(qer);
    }
    */

    // 清理 TEID 哈希表
    if (sess->ran_teid) {
        ogs_hash_set(df_self()->teid_hash, &sess->ran_teid, sizeof(sess->ran_teid), NULL);
    }

    df_sess_remove(sess);
}

void df_n4_handle_session_report_response(
        df_sess_t *sess, ogs_pfcp_xact_t *xact,
        ogs_pfcp_session_report_response_t *rsp)
{
    uint8_t cause_value = 0;

    ogs_assert(xact);
    ogs_assert(rsp);

    ogs_pfcp_xact_commit(xact);

    ogs_debug("Session Report Response");

    cause_value = OGS_PFCP_CAUSE_REQUEST_ACCEPTED;

    if (!sess) {
        ogs_warn("No Context");
        cause_value = OGS_PFCP_CAUSE_SESSION_CONTEXT_NOT_FOUND;
    }

    if (rsp->cause.presence) {
        if (rsp->cause.u8 != OGS_PFCP_CAUSE_REQUEST_ACCEPTED) {
            ogs_error("PFCP Cause[%d] : Not Accepted", rsp->cause.u8);
            cause_value = rsp->cause.u8;
        }
    } else {
        ogs_error("No Cause");
        cause_value = OGS_PFCP_CAUSE_MANDATORY_IE_MISSING;
    }

    if (cause_value != OGS_PFCP_CAUSE_REQUEST_ACCEPTED) {
        ogs_error("Cause request not accepted[%d]", cause_value);
        return;
    } else {
        // TODO: df_metrics_inst_global_inc(DF_METR_GLOB_CTR_SM_N4SESSIONREPORTSUCC);
    }
} 