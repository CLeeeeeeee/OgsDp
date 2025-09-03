/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: app.c
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:06 CST
 */

#include "ogs-app.h"

int app_initialize(const char *const argv[])
{
    int rv;

    //TODO dmf_initialize();
    rv = dmf_initialize();
    if (rv != OGS_OK) {
        ogs_error("Failed to initialize DMF");
        return rv;
    }
    ogs_info("DMF initialize...done");

    return OGS_OK;
}

void app_terminate(void)
{
    //TODO dmf_terminate();
    dmf_terminate();
    ogs_info("DMF terminate...done");
}

