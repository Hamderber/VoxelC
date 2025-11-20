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

/// @brief Returns true if *any* bit in MASK is set in FLAGS.
static inline bool flag64_hasAny(flags64_t flags, flags64_t mask) { return (flags & mask) != 0ULL; }

/// @brief Returns true if *all* bits in MASK are set in FLAGS.
/// Note: mask == 0 returns true
static inline bool flag64_hasAll(flags64_t flags, flags64_t mask) { return (flags & mask) == mask; }

/// @brief Returns FLAGS with all bits in MASK enabled (set to 1).
static inline flags64_t flag64_enableMask(flags64_t flags, flags64_t mask) { return flags | mask; }

/// @brief Returns FLAGS with all bits in MASK disabled (set to 0).
static inline flags64_t flag64_disableMask(flags64_t flags, flags64_t mask) { return flags & ~mask; }

/// @brief Returns FLAGS with all bits in MASK toggled (0->1, 1->0).
static inline flags64_t flag64_toggleMask(flags64_t flags, flags64_t mask) { return flags ^ mask; }

/// @brief Clears all bits in *pFlags (sets to 0). Safe for NULL.
static inline void flag64_clearAll(flags64_t *pFlags)
{
    if (!pFlags)
        return;
    *pFlags = 0ULL;
}

/// @brief Sets all bits in *pFlags (0..63). Safe for NULL.
static inline void flag64_setAll(flags64_t *pFlags)
{
    if (!pFlags)
        return;
    *pFlags = ~0ULL;
}

/// @brief Counts how many bits are set in FLAGS (population count).
static inline uint8_t flag64_popcount(flags64_t flags)
{
    uint8_t count = 0;
    while (flags)
    {
        // clear lowest set bit
        flags &= (flags - 1);
        ++count;
    }
    return count;
}

/// @brief Returns index of first set bit [0,63], or -1 if none.
static inline int flag64_findFirstSet(flags64_t flags)
{
    if (flags == 0ULL)
        return -1;

    for (int bit = 0; bit < 64; ++bit)
    {
        if ((flags >> bit) & 1ULL)
            return bit;
    }
    return -1;
}

/// @brief Returns index of first clear bit [0,63], or -1 if all bits set.
static inline int flag64_findFirstClear(flags64_t flags)
{
    for (int bit = 0; bit < 64; ++bit)
    {
        if (((flags >> bit) & 1ULL) == 0ULL)
            return bit;
    }
    return -1;
}

/// @brief Writes FLAGS as a 64-character binary string into OUT (must be >= 65 bytes).
/// Bit 63 is written at out[0], bit 0 at out[63]. Null-terminated.
static inline void flag64_toBinaryString(flags64_t flags, char out[65])
{
    if (!out)
        return;

    for (int bit = 63; bit >= 0; --bit)
    {
        int idx = 63 - bit; // bit63 -> index0
        out[idx] = ((flags >> bit) & 1ULL) ? '1' : '0';
    }
    out[64] = '\0';
}
#pragma endregion