#pragma once
#include <stdio.h>

static inline int ut_assert(int condition, const char *testName)
{
    if (condition)
    {
        printf("[PASS] %s\n", testName);
        return 0;
    }
    else
    {
        printf("[FAIL] %s\n", testName);
        return 1;
    }
}

static inline int ut_section(const char *name)
{
    printf("\n== %s ==\n", name);
    return 0;
}

int unitTests_run(void);