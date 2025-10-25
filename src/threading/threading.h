#pragma once

#include <stdbool.h>

void threading_thread_thisIsMain(void);

bool threading_thread_isMain(void);

bool threading_thread_errorIfNotMain(void);
