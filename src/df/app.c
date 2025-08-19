#include "ogs-app.h"
#include "context.h"

int app_initialize(const char *const argv[])
{
    int rv;
    rv = df_initialize();
    if (rv != OGS_OK) {
        ogs_error("Failed to initialize DF");
        return rv;
    }
    ogs_info("DF initialize...done");
    return OGS_OK;
}

void app_terminate(void)
{
    df_terminate();
    ogs_info("DF terminate...done");
} 