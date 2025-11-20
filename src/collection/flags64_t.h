// https://www.geeksforgeeks.org/c/c-bitmasking/
#pragma region Includes
#pragma once
#include <stdbool.h>
#include <stdint.h>
#pragma endregion
#pragma region Definitions
typedef uint64_t flags64_t;
#pragma endregion
#pragma region Flag Operations
/// @brief Sets flag bit [0, 63] to VALUE [T/F]. Accepts any ENUM type (positive) (implicit int cast)
static inline flags64_t flag64_set(flags64_t flags, const int BIT, const bool VALUE)
{
    if (BIT < 0 || BIT >= 64)
        return flags;

    const uint64_t MASK = 1ULL << (unsigned)BIT;
    return VALUE ? (flags | MASK) : (flags & ~MASK);
}

/// @brief Checks if flag bit [0, 63] is set (true) or not (false). Accepts any ENUM type (positive) (implicit int cast)
static inline bool flag64_get(const flags64_t FLAGS, const int BIT)
{
    if (BIT < 0 || BIT >= 64)
        return false;

    return (FLAGS >> (unsigned)BIT) & 1ULL;
}

/// @brief Sets flag bit [0, 63] of *pFlags to VALUE [T/F]. Accepts any ENUM type (positive) (implicit int cast)
static inline void flag64_setInline(flags64_t *pFlags, const int BIT, const bool VALUE)
{
    if (!pFlags || BIT >= 64)
        return;

    *pFlags = flag64_set(*pFlags, BIT, VALUE);
}
#pragma endregion