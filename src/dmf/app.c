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

