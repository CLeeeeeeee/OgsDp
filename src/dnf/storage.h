#ifndef DNF_STORAGE_H
#define DNF_STORAGE_H

// 预留：将数据交给DOMF网元，由DOMF负责与DR（MongoDB）交互
void dnf_send_to_domf(const void *data, int len);
 
#endif // DNF_STORAGE_H 