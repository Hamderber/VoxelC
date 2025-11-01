#include "main.h"
#include "app.h"
#include "core/types/state_t.h"
#include "core/logs.h"
#include "threading/threading.h"

int main(void)
{
    threading_thread_thisIsMain();

    logs_create(PROGRAM_NAME);

    State_t state = {0};

    app_init(&state);
    app_loop_main(&state);
    app_cleanup(&state);

    logs_destroy();
}
