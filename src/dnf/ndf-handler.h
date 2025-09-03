/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: ndf-handler.h
 * 组件: dnf
 * 添加时间: 2025年 08月 20日 星期三 11:16:13 CST
 */

#ifndef DNF_HANDLER_H
#define DNF_HANDLER_H

// 预留：接收DF网元数据的接口
void dnf_receive_from_df(const void *data, int len);
 
#endif // DNF_HANDLER_H 