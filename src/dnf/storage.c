/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: storage.c
 * 组件: dnf
 * 添加时间: 2025年 08月 20日 星期三 11:16:13 CST
 */

#include "storage.h"
 
void dnf_send_to_domf(const void *data, int len)
{
    // 预留：将数据交给DOMF网元，由DOMF负责与DR（MongoDB）交互，后续实现
} 