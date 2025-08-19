#include "ogs-app.h"
#include "context.h"

int app_initialize(const char *const argv[])
{
    int rv;
    rv = dnf_initialize();
    if (rv != OGS_OK) {
        ogs_error("Failed to initialize DNF");
        return rv;
    }
    ogs_info("DNF initialize...done");
    return OGS_OK;
}

void app_terminate(void)
{
    dnf_terminate();
    ogs_info("DNF terminate...done");
} 

/*
第一个方案是干脆不暴露这些函数了 
反正数据上来一律是清洗和压缩，具体的实现怎么做，就看附带的来源标签。
禁止UE采集数据，只允许使用固定位置的传感器。对于不同的传感器有不同的编号，不同的编号可以采取不同的函数。
这样就有足够的时间和数据去训练一个足够准确的模型，并且周期性地再次训练，每次训练好后只把模型的结果保留在DNF里面。
那我其实在DNF里面就是有x个函数，这些函数具体是干啥的我也不知道，反正当UE发起请求的时候只要知道UE的具体位置，把他周围的那些传感器拎出来套函数，然后返回结果。
*/