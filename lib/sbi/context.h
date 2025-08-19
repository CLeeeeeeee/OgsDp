
#if !defined(OGS_SBI_INSIDE) && !defined(OGS_SBI_COMPILATION)
#error "This header cannot be included directly."
#endif

#ifndef OGS_SBI_CONTEXT_H
#define OGS_SBI_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

    //最大 NF 类型的数量（最多支持8种类型，每种类型一个 info 结构）
#define OGS_MAX_NUM_OF_NF_INFO 8
#define OGS_MAX_NUM_OF_SCP_DOMAIN 8

typedef struct ogs_sbi_client_s ogs_sbi_client_t;
typedef struct ogs_sbi_smf_info_s ogs_sbi_smf_info_t;
typedef struct ogs_sbi_nf_instance_s ogs_sbi_nf_instance_t;

typedef enum {
    OGS_SBI_DISCOVERY_DELEGATED_AUTO = 0,
    OGS_SBI_DISCOVERY_DELEGATED_YES,
    OGS_SBI_DISCOVERY_DELEGATED_NO,
} ogs_sbi_discovery_delegated_mode;

typedef enum {
    OGS_SBI_CLIENT_DELEGATED_AUTO = 0,
    OGS_SBI_CLIENT_DELEGATED_YES,
    OGS_SBI_CLIENT_DELEGATED_NO,
} ogs_sbi_client_delegated_mode_e;

typedef struct ogs_sbi_discovery_config_s {
    ogs_sbi_discovery_delegated_mode delegated;
    bool no_service_names;
    bool prefer_requester_nf_instance_id;
} ogs_sbi_discovery_config_t;

typedef struct ogs_sbi_context_s {
    ogs_sbi_discovery_config_t discovery_config; /* SCP Discovery Delegated */

    //???????????hou gai??????
    struct {
        struct {
            ogs_sbi_client_delegated_mode_e nfm;
            ogs_sbi_client_delegated_mode_e disc;
        } nrf;
        struct {
            ogs_sbi_client_delegated_mode_e next;
        } scp;
    } client_delegated_config;

#define OGS_HOME_NETWORK_PKI_VALUE_MIN 1
#define OGS_HOME_NETWORK_PKI_VALUE_MAX 254

//如果在 DMF 中加入私钥认证、签名、或设备安全性校验，可能会使用这部分逻辑。
    struct {
        uint8_t avail;
        uint8_t scheme;
        uint8_t key[OGS_ECCKEY_LEN]; /* 32 bytes Private Key */
    } hnet[OGS_HOME_NETWORK_PKI_VALUE_MAX+1]; /* PKI Value : 1 ~ 254 */


    struct {
        struct {
            OpenAPI_uri_scheme_e scheme;

            const char *private_key;
            const char *cert;
            const char *sslkeylog;

            bool verify_client;   //双向验证
            const char *verify_client_cacert;
        } server;
        struct {
            OpenAPI_uri_scheme_e scheme;

            bool insecure_skip_verify;  //测试时可以开启
            const char *cacert;

            const char *private_key;
            const char *cert;
            const char *sslkeylog;
        } client;
    } tls;

    ogs_list_t server_list;
    ogs_list_t client_list;

    ogs_uuid_t uuid;  //NF的全局唯一标识，注册时发给NRF的ID

    ogs_list_t nf_instance_list;
    ogs_list_t subscription_spec_list;//自己想要订阅哪些 NF 的服务
    ogs_list_t subscription_data_list;//实际已注册的订阅数据

    ogs_sbi_nf_instance_t *nf_instance;     /* SELF NF Instance */
    ogs_sbi_nf_instance_t *nrf_instance;    /* NRF Instance */
    ogs_sbi_nf_instance_t *scp_instance;    /* SCP Instance */
    ogs_sbi_nf_instance_t *sepp_instance;   /* SEPP Instance */

    const char *content_encoding;

    int num_of_service_name;
    const char *service_name[OGS_SBI_MAX_NUM_OF_SERVICE_TYPE];
    const char *local_if;
} ogs_sbi_context_t;

typedef struct ogs_sbi_nf_instance_s {
    ogs_lnode_t lnode;

    ogs_fsm_t sm;                           /* A state machine 状态机结构 */
    ogs_timer_t *t_registration_interval;   /* timer to retry
                                               to register peer node */
    struct {
        int heartbeat_interval;//NRF 要求的心跳周期。
        int validity_duration;//注册信息的有效时长（如 60s、300s）。
    } time;

    ogs_timer_t *t_heartbeat_interval;      /* heartbeat interval */
    ogs_timer_t *t_no_heartbeat;            /* check heartbeat */
    ogs_timer_t *t_validity;                /* check validation 定时刷新注册或注销。 */

#define NF_INSTANCE_EXCLUDED_FROM_DISCOVERY(__nFInstance) \
    (!(__nFInstance) || !((__nFInstance)->id))

#define NF_INSTANCE_ID(__nFInstance) \
    ((__nFInstance) ? ((__nFInstance)->id) : NULL)
#define NF_INSTANCE_ID_IS_SELF(_iD) \
    (_iD) && ogs_sbi_self()->nf_instance && \
        strcmp((_iD), ogs_sbi_self()->nf_instance->id) == 0
#define NF_INSTANCE_ID_IS_OTHERS(_iD) \
    (_iD) && ogs_sbi_self()->nf_instance && \
        strcmp((_iD), ogs_sbi_self()->nf_instance->id) != 0
    char *id; //NF 实例的全局唯一 ID（UUID），用于注册与发现。

#define NF_INSTANCE_TYPE(__nFInstance) \
    ((__nFInstance) ? ((__nFInstance)->nf_type) : OpenAPI_nf_type_NULL)
#define NF_INSTANCE_TYPE_IS_NRF(__nFInstance) \
    (NF_INSTANCE_TYPE(__nFInstance) == OpenAPI_nf_type_NRF)
    OpenAPI_nf_type_e nf_type;//如 AMF、SMF
    OpenAPI_nf_status_e nf_status;//注册状态

    //表示该 NF 实例在哪些运营区域提供服务
    ogs_plmn_id_t plmn_id[OGS_MAX_NUM_OF_PLMN];
    int num_of_plmn_id;

    char *fqdn; //服务注册到 NRF 后生成的服务名称
    //SBI 接口地址 将来让 DMF 在注册时写入这些信息，其他模块就能用它的 SBI 地址访问它
#define OGS_SBI_MAX_NUM_OF_IP_ADDRESS 8
    int num_of_ipv4;
    ogs_sockaddr_t *ipv4[OGS_SBI_MAX_NUM_OF_IP_ADDRESS];
    int num_of_ipv6;
    ogs_sockaddr_t *ipv6[OGS_SBI_MAX_NUM_OF_IP_ADDRESS];

    //NF 访问权限控制 你可以让 DMF 只允许 AMF 发请求，其他 NF 都不行。
    int num_of_allowed_nf_type;
#define OGS_SBI_MAX_NUM_OF_NF_TYPE 128
    OpenAPI_nf_type_e allowed_nf_type[OGS_SBI_MAX_NUM_OF_NF_TYPE];

    //服务负载与优先级信息
#define OGS_SBI_DEFAULT_PRIORITY 0
#define OGS_SBI_DEFAULT_CAPACITY 100
#define OGS_SBI_DEFAULT_LOAD 0
    int priority;
    int capacity;
    int load;

    //服务与信息列表 这个 NF 提供的 SBI 服务 如 nsmf-pdusession，你未来可提供 ndmf-upload 等
    ogs_list_t nf_service_list;
    ogs_list_t nf_info_list;

    //客户端指针（仅用于 SBI client）
#define NF_INSTANCE_CLIENT(__nFInstance) \
    ((__nFInstance) ? ((__nFInstance)->client) : NULL)
    void *client;                       /* only used in CLIENT */
} ogs_sbi_nf_instance_t;

typedef enum {
    OGS_SBI_OBJ_BASE = 0,
    OGS_SBI_OBJ_UE_TYPE,
    OGS_SBI_OBJ_SESS_TYPE,
    OGS_SBI_OBJ_TOP,
} ogs_sbi_obj_type_e;

//用于维护特定类型服务发现与交互上下文
typedef struct ogs_sbi_object_s {
    ogs_lnode_t lnode;

    ogs_sbi_obj_type_e type;

    struct {
        ogs_sbi_nf_instance_t *nf_instance;
        char *nf_instance_id;
        /*
         * Search.Result stored in nf_instance->time.validity_duration;
         *
         * validity_timeout = nf_instance->validity->timeout =
         *     ogs_get_monotonic_time() + nf_instance->time.validity_duration;
         *
         * if no validityPeriod in SearchResult, validity_timeout is 0.
         */
        ogs_time_t validity_timeout;
    } nf_type_array[OGS_SBI_MAX_NUM_OF_NF_TYPE],
      service_type_array[OGS_SBI_MAX_NUM_OF_SERVICE_TYPE];

    ogs_list_t xact_list;

} ogs_sbi_object_t;

typedef ogs_sbi_request_t *(*ogs_sbi_build_f)(
        void *context, void *data);


typedef struct ogs_sbi_xact_s {
    ogs_lnode_t lnode;

    ogs_pool_id_t id;

    ogs_sbi_service_type_e service_type;
    OpenAPI_nf_type_e requester_nf_type;//发起该事务的 NF 类型
    ogs_sbi_discovery_option_t *discovery_option;

    ogs_sbi_request_t *request;
    ogs_timer_t *t_response;

    ogs_pool_id_t assoc_stream_id;

    int state;
    char *target_apiroot;//目标 NF 的 API 根路径 在发送请求时会构造完整的 URL：target_apiroot + URI path

    ogs_sbi_object_t *sbi_object;
    ogs_pool_id_t sbi_object_id;
} ogs_sbi_xact_t;

#define OGS_SBI_XACT_LOG(x)

typedef struct ogs_sbi_nf_service_s {
    ogs_lnode_t lnode;

    char *id;
    char *name;
    OpenAPI_uri_scheme_e scheme;

    OpenAPI_nf_service_status_e status;

#define OGS_SBI_MAX_NUM_OF_SERVICE_VERSION 8
    int num_of_version;
    struct {
        char *in_uri;
        char *full;
        char *expiry;
    } version[OGS_SBI_MAX_NUM_OF_SERVICE_VERSION];

    char *fqdn;
    int num_of_addr;
    struct {
        ogs_sockaddr_t *ipv4;
        ogs_sockaddr_t *ipv6;
        bool is_port;
        int port;
    } addr[OGS_SBI_MAX_NUM_OF_IP_ADDRESS];

    int num_of_allowed_nf_type;
    OpenAPI_nf_type_e allowed_nf_type[OGS_SBI_MAX_NUM_OF_NF_TYPE];

    int priority;
    int capacity;
    int load;

    /* Related Context */
    ogs_sbi_nf_instance_t *nf_instance;//哪个 NF 实例提供了这个服务
    void *client;
} ogs_sbi_nf_service_t;

//订阅意图说明（spec），表示“我想订阅哪类服务”
typedef struct ogs_sbi_subscription_spec_s {
    ogs_lnode_t lnode;

    struct {
        OpenAPI_nf_type_e nf_type;          /* nfType希望订阅的目标 NF 类型 */
        char *service_name;                 /* ServiceName目标 NF 提供的服务名 */
        char *nf_instance_id;
    } subscr_cond;//我希望订阅谁的什么

} ogs_sbi_subscription_spec_t;//本地意愿，不一定代表已经注册成功的订阅

typedef struct ogs_sbi_subscription_data_s {//实际已经注册到 NRF 的订阅项
    ogs_lnode_t lnode;

    ogs_time_t validity_duration;           /* valditiyTime(unit: usec)有效期 */
    ogs_timer_t *t_validity;                /* check validation定期检查是否订阅已经过期的定时器 */
    ogs_timer_t *t_patch;                   /* for sending PATCH周期性 PATCH 订阅状态或延长订阅有效期的定时器 */

    char *id;                               /* SubscriptionId */
    char *req_nf_instance_id;               /* reqNfInstanceId 本地 NF 的实例 ID*/
    OpenAPI_nf_type_e req_nf_type;          /* reqNfType 本地 NF 的类型*/
    OpenAPI_nf_status_e nf_status;
    char *notification_uri;//当 NRF 检测到被订阅的目标 NF 有状态变更时，会向该 URI 发送通知消息 DMF实现时需监听该URI
    char *resource_uri;

    struct {
        OpenAPI_nf_type_e nf_type;          /* nfType */
        char *service_name;                 /* ServiceName */
        char *nf_instance_id;
    } subscr_cond;

    uint64_t requester_features;
    uint64_t nrf_supported_features;

    void *client;
} ogs_sbi_subscription_data_t;//“我已经订阅成功了”的记录

typedef struct ogs_sbi_smf_info_s {
    int num_of_slice;
    struct {
        ogs_s_nssai_t s_nssai;

        int num_of_dnn;
        char *dnn[OGS_MAX_NUM_OF_DNN];
    } slice[OGS_MAX_NUM_OF_SLICE];

    int num_of_nr_tai;
    ogs_5gs_tai_t nr_tai[OGS_MAX_NUM_OF_TAI];

    int num_of_nr_tai_range;
    struct {
        ogs_plmn_id_t plmn_id;
        /*
         * TS29.510 6.1.6.2.28 Type: TacRange
         *
         * Either the start and end attributes, or
         * the pattern attribute, shall be present.
         */
        int num_of_tac_range;
        ogs_uint24_t start[OGS_MAX_NUM_OF_TAI], end[OGS_MAX_NUM_OF_TAI];
    } nr_tai_range[OGS_MAX_NUM_OF_TAI];
} ogs_sbi_smf_info_t;

typedef struct ogs_sbi_scp_info_s {
    ogs_port_t http, https;

    int num_of_domain;
    struct {
        char *name;
        char *fqdn;
        ogs_port_t http, https;
    } domain[OGS_MAX_NUM_OF_SCP_DOMAIN];

} ogs_sbi_scp_info_t;

typedef struct ogs_sbi_sepp_info_s {
    ogs_port_t http, https;
} ogs_sbi_sepp_info_t;

typedef struct ogs_sbi_dmf_info_s {
    // DMF特定的信息结构
    // 可以根据需要添加DMF特有的字段
    int num_of_gnb;
    char *gnb_list[OGS_MAX_NUM_OF_TAI]; // 简化的gNB列表
} ogs_sbi_dmf_info_t;

typedef struct ogs_sbi_amf_info_s {
    uint8_t amf_set_id;//所属 AMF Set 的 ID（用于 UE 寻址和重定位）
    uint16_t amf_region_id;//所在的 AMF Region 区域 ID，用于位置选择

    //AMF 支持的 GUAMI 列表（全球唯一 AMF 标识，组合了 PLMN 和 AMF ID）
    int num_of_guami;
    ogs_guami_t guami[OGS_MAX_NUM_OF_SERVED_GUAMI];

    //支持的 TAI（5G Tracking Area Identity），用于区域服务能力表示。
    int num_of_nr_tai;
    ogs_5gs_tai_t nr_tai[OGS_MAX_NUM_OF_TAI];

    //NR TAI 范围数量。
    int num_of_nr_tai_range;

    //与 smf_info 一样，每个范围对应的运营商 PLMN。
    struct {
        ogs_plmn_id_t plmn_id;
        /*
         * TS29.510 6.1.6.2.28 Type: TacRange
         *
         * Either the start and end attributes, or
         * the pattern attribute, shall be present.
         */
        int num_of_tac_range;
        ogs_uint24_t start[OGS_MAX_NUM_OF_TAI], end[OGS_MAX_NUM_OF_TAI];
    } nr_tai_range[OGS_MAX_NUM_OF_TAI];
} ogs_sbi_amf_info_t;

typedef struct ogs_sbi_nf_info_s {
    ogs_lnode_t lnode;

    OpenAPI_nf_type_e nf_type; //这个字段用来选择 union 中具体哪一块数据有效。
    union {
        ogs_sbi_smf_info_t smf;
        ogs_sbi_amf_info_t amf;
        ogs_sbi_scp_info_t scp;
        ogs_sbi_sepp_info_t sepp;
        ogs_sbi_dmf_info_t dmf;
    };//指明本结构体描述的是哪种 NF 类型

} ogs_sbi_nf_info_t;

void ogs_sbi_context_init(OpenAPI_nf_type_e nf_type);
void ogs_sbi_context_final(void);
ogs_sbi_context_t *ogs_sbi_self(void);
int ogs_sbi_context_parse_config(
        const char *local, const char *nrf, const char *scp);
int ogs_sbi_context_parse_hnet_config(ogs_yaml_iter_t *root_iter);
int ogs_sbi_context_parse_server_config(
        ogs_yaml_iter_t *parent, const char *interface);
ogs_sbi_client_t *ogs_sbi_context_parse_client_config(
        ogs_yaml_iter_t *iter);

bool ogs_sbi_nf_service_is_available(const char *name);

ogs_sbi_nf_instance_t *ogs_sbi_nf_instance_add(void);
void ogs_sbi_nf_instance_set_id(ogs_sbi_nf_instance_t *nf_instance, char *id);
void ogs_sbi_nf_instance_set_type(
        ogs_sbi_nf_instance_t *nf_instance, OpenAPI_nf_type_e nf_type);
void ogs_sbi_nf_instance_set_status(
        ogs_sbi_nf_instance_t *nf_instance, OpenAPI_nf_status_e nf_status);
void ogs_sbi_nf_instance_add_allowed_nf_type(
        ogs_sbi_nf_instance_t *nf_instance, OpenAPI_nf_type_e allowed_nf_type);
bool ogs_sbi_nf_instance_is_allowed_nf_type(
        ogs_sbi_nf_instance_t *nf_instance, OpenAPI_nf_type_e allowed_nf_type);
void ogs_sbi_nf_instance_clear(ogs_sbi_nf_instance_t *nf_instance);
void ogs_sbi_nf_instance_remove(ogs_sbi_nf_instance_t *nf_instance);
void ogs_sbi_nf_instance_remove_all(void);
ogs_sbi_nf_instance_t *ogs_sbi_nf_instance_find(char *id);
ogs_sbi_nf_instance_t *ogs_sbi_nf_instance_find_by_discovery_param(
        OpenAPI_nf_type_e nf_type,
        OpenAPI_nf_type_e requester_nf_type,
        ogs_sbi_discovery_option_t *discovery_option);
ogs_sbi_nf_instance_t *ogs_sbi_nf_instance_find_by_service_type(
        ogs_sbi_service_type_e service_type,
        OpenAPI_nf_type_e requester_nf_type);
bool ogs_sbi_nf_instance_maximum_number_is_reached(void);

ogs_sbi_nf_service_t *ogs_sbi_nf_service_add(
        ogs_sbi_nf_instance_t *nf_instance,
        char *id, const char *name, OpenAPI_uri_scheme_e scheme);
void ogs_sbi_nf_service_add_version(
        ogs_sbi_nf_service_t *nf_service,
        const char *in_uri, const char *full, const char *expiry);
void ogs_sbi_nf_service_add_allowed_nf_type(
        ogs_sbi_nf_service_t *nf_service, OpenAPI_nf_type_e allowed_nf_type);
bool ogs_sbi_nf_service_is_allowed_nf_type(
        ogs_sbi_nf_service_t *nf_service, OpenAPI_nf_type_e allowed_nf_type);
void ogs_sbi_nf_service_clear(ogs_sbi_nf_service_t *nf_service);
void ogs_sbi_nf_service_remove(ogs_sbi_nf_service_t *nf_service);
void ogs_sbi_nf_service_remove_all(ogs_sbi_nf_instance_t *nf_instance);
ogs_sbi_nf_service_t *ogs_sbi_nf_service_find_by_id(
        ogs_sbi_nf_instance_t *nf_instance, char *id);
ogs_sbi_nf_service_t *ogs_sbi_nf_service_find_by_name(
        ogs_sbi_nf_instance_t *nf_instance, char *name);

ogs_sbi_nf_info_t *ogs_sbi_nf_info_add(
        ogs_list_t *list, OpenAPI_nf_type_e nf_type);
void ogs_sbi_nf_info_remove(ogs_list_t *list, ogs_sbi_nf_info_t *nf_info);
void ogs_sbi_nf_info_remove_all(ogs_list_t *list);
ogs_sbi_nf_info_t *ogs_sbi_nf_info_find(
        ogs_list_t *list, OpenAPI_nf_type_e nf_type);

bool ogs_sbi_check_amf_info_guami(
        ogs_sbi_amf_info_t *amf_info, ogs_guami_t *guami);
bool ogs_sbi_check_smf_info_slice(
        ogs_sbi_smf_info_t *smf_info, ogs_s_nssai_t *s_nssai, char *dnn);
bool ogs_sbi_check_smf_info_tai(
        ogs_sbi_smf_info_t *smf_info, ogs_5gs_tai_t *tai);

void ogs_sbi_nf_instance_build_default(ogs_sbi_nf_instance_t *nf_instance);
ogs_sbi_nf_service_t *ogs_sbi_nf_service_build_default(
        ogs_sbi_nf_instance_t *nf_instance, const char *name);

ogs_sbi_client_t *ogs_sbi_client_find_by_service_name(
        ogs_sbi_nf_instance_t *nf_instance, char *name, char *version);
ogs_sbi_client_t *ogs_sbi_client_find_by_service_type(
        ogs_sbi_nf_instance_t *nf_instance,
        ogs_sbi_service_type_e service_type);

void ogs_sbi_client_associate(ogs_sbi_nf_instance_t *nf_instance);

int ogs_sbi_default_client_port(OpenAPI_uri_scheme_e scheme);

#define OGS_SBI_SETUP_NF_INSTANCE(__cTX, __nFInstance) \
    do { \
        ogs_assert(__nFInstance); \
        ogs_assert((__nFInstance)->id); \
        ogs_assert((__nFInstance)->t_validity); \
        \
        if ((__cTX).nf_instance) { \
            ogs_warn("[%s] NF Instance updated [type:%s validity:%ds]", \
                    ((__cTX).nf_instance)->id, \
                    OpenAPI_nf_type_ToString(((__cTX).nf_instance)->nf_type), \
                    ((__cTX).nf_instance)->time.validity_duration); \
        } \
        \
        ((__cTX).nf_instance) = __nFInstance; \
        if ((__nFInstance)->time.validity_duration) { \
            ((__cTX).validity_timeout) = (__nFInstance)->t_validity->timeout; \
        } else { \
            ((__cTX).validity_timeout) = 0; \
        } \
        ogs_info("[%s] NF Instance setup [type:%s validity:%ds]", \
                (__nFInstance)->id, \
                OpenAPI_nf_type_ToString((__nFInstance)->nf_type), \
                (__nFInstance)->time.validity_duration); \
    } while(0)

/*
 * Search.Result stored in nf_instance->time.validity_duration;
 *
 * validity_timeout = nf_instance->validity->timeout =
 *     ogs_get_monotonic_time() + nf_instance->time.validity_duration;
 *
 * if no validityPeriod in SearchResult, validity_timeout is 0.
 */
#define OGS_SBI_GET_NF_INSTANCE(__cTX) \
    ((__cTX).validity_timeout == 0 || \
     (__cTX).validity_timeout > ogs_get_monotonic_time() ? \
        ((__cTX).nf_instance) : NULL)

#define OGS_SBI_NF_INSTANCE_VALID(__nFInstance) \
    (((__nFInstance) && ((__nFInstance)->t_validity) && \
     ((__nFInstance)->time.validity_duration == 0 || \
      (__nFInstance)->t_validity->timeout > ogs_get_monotonic_time())) ? \
         true : false)

bool ogs_sbi_discovery_param_is_matched(
        ogs_sbi_nf_instance_t *nf_instance,
        OpenAPI_nf_type_e target_nf_type,
        OpenAPI_nf_type_e requester_nf_type,
        ogs_sbi_discovery_option_t *discovery_option);

bool ogs_sbi_discovery_param_serving_plmn_list_is_matched(
        ogs_sbi_nf_instance_t *nf_instance);

bool ogs_sbi_discovery_option_is_matched(
        ogs_sbi_nf_instance_t *nf_instance,
        OpenAPI_nf_type_e requester_nf_type,
        ogs_sbi_discovery_option_t *discovery_option);
bool ogs_sbi_discovery_option_service_names_is_matched(
        ogs_sbi_nf_instance_t *nf_instance,
        OpenAPI_nf_type_e requester_nf_type,
        ogs_sbi_discovery_option_t *discovery_option);
bool ogs_sbi_discovery_option_requester_plmn_list_is_matched(
        ogs_sbi_nf_instance_t *nf_instance,
        ogs_sbi_discovery_option_t *discovery_option);
bool ogs_sbi_discovery_option_target_plmn_list_is_matched(
        ogs_sbi_nf_instance_t *nf_instance,
        ogs_sbi_discovery_option_t *discovery_option);

void ogs_sbi_object_free(ogs_sbi_object_t *sbi_object);

ogs_sbi_xact_t *ogs_sbi_xact_add(
        ogs_pool_id_t sbi_object_id,
        ogs_sbi_object_t *sbi_object,
        ogs_sbi_service_type_e service_type,
        ogs_sbi_discovery_option_t *discovery_option,
        ogs_sbi_build_f build, void *context, void *data);
void ogs_sbi_xact_remove(ogs_sbi_xact_t *xact);
void ogs_sbi_xact_remove_all(ogs_sbi_object_t *sbi_object);
ogs_sbi_xact_t *ogs_sbi_xact_find_by_id(ogs_pool_id_t id);

ogs_sbi_subscription_spec_t *ogs_sbi_subscription_spec_add(
        OpenAPI_nf_type_e nf_type, const char *service_name);
void ogs_sbi_subscription_spec_remove(
        ogs_sbi_subscription_spec_t *subscription_spec);
void ogs_sbi_subscription_spec_remove_all(void);

ogs_sbi_subscription_data_t *ogs_sbi_subscription_data_add(void);
void ogs_sbi_subscription_data_set_resource_uri(
        ogs_sbi_subscription_data_t *subscription_data, char *resource_uri);
void ogs_sbi_subscription_data_set_id(
        ogs_sbi_subscription_data_t *subscription_data, char *id);
void ogs_sbi_subscription_data_remove(
        ogs_sbi_subscription_data_t *subscription_data);
void ogs_sbi_subscription_data_remove_all_by_nf_instance_id(
        char *nf_instance_id);
void ogs_sbi_subscription_data_remove_all(void);
ogs_sbi_subscription_data_t *ogs_sbi_subscription_data_find(char *id);

bool ogs_sbi_supi_in_vplmn(char *supi);
bool ogs_sbi_plmn_id_in_vplmn(ogs_plmn_id_t *plmn_id);
bool ogs_sbi_fqdn_in_vplmn(char *fqdn);

/* OpenSSL Key Log Callback */
void ogs_sbi_keylog_callback(const SSL *ssl, const char *line);

#ifdef __cplusplus
}
#endif

#endif /* OGS_SBI_CONTEXT_H */
