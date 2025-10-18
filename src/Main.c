#include "main.h"
#include "app.h"
#include "core/logs.h"
#include "core/types/state_t.h"
#include <time.h>
#include "core/random.h"

int main(void)
{
    logs_create(PROGRAM_NAME);

    // Init the deterministic random with a random (if 0) seed
    rand_init(0);

    cfg_init();

    State_t state = {
        .config = cfg_appInit(),
    };

    app_init(&state);
    app_loop(&state);
    app_cleanup(&state);

    logs_destroy();
}
