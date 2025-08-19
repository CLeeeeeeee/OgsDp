#include "context.h"

static dnf_context_t _dnf_self = {0};

dnf_context_t *dnf_context_self(void)
{
    return &_dnf_self;
} 