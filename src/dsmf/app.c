/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: app.c
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:10 CST
 */

#include "ogs-app.h"

int app_initialize(const char *const argv[])
{
    int rv;

    rv = dsmf_initialize();
    if (rv != OGS_OK) {
        ogs_error("Failed to initialize DSMF");
        return rv;
    }
    ogs_info("DSMF initialize...done");

    return OGS_OK;
}

void app_terminate(void)
{
    dsmf_terminate();
    ogs_info("DSMF terminate...done");
} 