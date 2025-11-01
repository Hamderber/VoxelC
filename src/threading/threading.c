#include "compat/intellisense_shims.h"
#include <stdatomic.h>
#include <threads.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include "core/logs.h"

static thrd_t s_mainThread;
static bool s_mainThreadCached;

void threading_thread_thisIsMain()
{
    static bool functionLocked = false;
    if (!functionLocked)
    {
        s_mainThread = thrd_current();
        functionLocked = true;
        s_mainThreadCached = true;
    }
}

bool threading_thread_isMain(void)
{
    thrd_t thread = thrd_current();
    if (!s_mainThreadCached)
    {
        logs_log(LOG_ERROR, "Attempted to check if thread [%" PRIu32 "] is main, \
            but the main thread id hasn't been cached yet!",
                 thread._Tid);
        return false;
    }

    return thrd_equal(thread, s_mainThread) == true;
}

bool threading_thread_errorIfNotMain(void)
{
    bool isMain = threading_thread_isMain();
    logs_logIfError(isMain != true, "Current thread [%" PRIu32 "] isn't main [%" PRIu32 "] (it should be)!",
                    thrd_current()._Tid, s_mainThread._Tid);

    return isMain;
}