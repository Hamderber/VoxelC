#include <stdint.h>
#include "main.h"
#include "app.h"
#include "core/types/state_t.h"
#include "core/logs.h"
#include "core/random.h"

int main(void)
{
    logs_create(PROGRAM_NAME);

    uint32_t seed = 8675309U;
    random_init(seed);

    State_t state = {0};
    config_init(&state);

    app_init(&state);
    app_loop(&state);
    app_cleanup(&state);

    logs_destroy();
}
