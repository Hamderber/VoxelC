#include "core/core.h"
#include "main.h"
#include "app.h"
#include "testing/unit_tests.h"
#include <time.h>
// #include <s

int main(void)
{
    logs_create(PROGRAM_NAME);

    // Init the deterministic random with a random (if 0) seed
    rand_init(0);

#ifdef DEBUG
    // Run unit tests if this is a debug build
    ut_run();
#endif

    State_t state = {
        .config = cfg_init(),
    };

    app_init(&state);
    app_loop(&state);
    app_cleanup(&state);

    logs_destroy();
}
