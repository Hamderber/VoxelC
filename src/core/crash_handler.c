#include "main.h"
#include "logs.h"
#include <stdlib.h>

_Analysis_noreturn_ void crashHandler_crash_graceful(const char *pLOCATION, const char *pCONTEXT)
{
    logs_log(LOG_CRASH, "----------------------------------- CRASH ERPORT -----------------------------------");
    logs_log(LOG_CRASH, PROGRAM_NAME " has crashed! Context:");
    logs_log(LOG_CRASH, pCONTEXT);
    logs_log(LOG_CRASH, "At %s", pLOCATION);
    logs_log(LOG_CRASH, "------------------------------------------------------------------------------------");

    abort();
}