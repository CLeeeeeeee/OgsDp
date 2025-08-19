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