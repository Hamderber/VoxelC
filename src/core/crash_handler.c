#include "logs.h"
#include <stdlib.h>

void crashHandler_crash_graceful(const char *pCONTEXT)
{
    logs_log(LOG_ERROR, "Forcing a crash! Context: %s", pCONTEXT);
    abort();
}