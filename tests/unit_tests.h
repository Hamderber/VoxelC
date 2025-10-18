#pragma once
#include <stdio.h>

static inline int ut_assert(int condition, const char *testName)
{
    if (!condition)
    {
        printf("[FAIL] %s\n", testName);
    }

    return !condition;
}

static inline void ut_section(const char *name)
{
    printf("== %s ==\n", name);
}

int unitTests_run(void);