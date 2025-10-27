#include "logs.h"
#include <stdlib.h>

_Analysis_noreturn_ void crashHandler_crash_graceful(const char *pCONTEXT)
{
    logs_log(LOG_ERROR, "Forcing a crash! Context: %s", pCONTEXT);
    abort();
}