#include "../../unit_tests.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "collection/flags64_t.h"

static int fails = 0;

static bool test_flag64_basic_set_get(void)
{
    flags64_t f = 0;

    // Set bit 0
    f = flag64_set(f, 0, true);
    if (f != 1ULL)
        return false;
    if (!flag64_get(f, 0))
        return false;
    if (flag64_get(f, 1))
        return false;

    // Set bit 1
    f = flag64_set(f, 1, true);
    if (f != 0b11ULL)
        return false;
    if (!flag64_get(f, 1))
        return false;

    // Clear bit 0
    f = flag64_set(f, 0, false);
    if (f != (1ULL << 1))
        return false;
    if (flag64_get(f, 0))
        return false;
    if (!flag64_get(f, 1))
        return false;

    // Idempotent: setting already-set bit does not change value
    flags64_t old = f;
    f = flag64_set(f, 1, true);
    if (f != old)
        return false;

    return true;
}

static bool test_flag64_bounds_bits_0_and_63(void)
{
    flags64_t f = 0;

    // Set lowest bit
    f = flag64_set(f, 0, true);
    if (f != 1ULL)
        return false;
    if (!flag64_get(f, 0))
        return false;

    // Set highest bit (63)
    f = flag64_set(f, 63, true);
    flags64_t expected = 1ULL | (1ULL << 63);
    if (f != expected)
        return false;
    if (!flag64_get(f, 63))
        return false;
    if (!flag64_get(f, 0))
        return false;
    if (flag64_get(f, 62))
        return false;

    // Clear highest bit
    f = flag64_set(f, 63, false);
    if (f != 1ULL)
        return false;
    if (flag64_get(f, 63))
        return false;
    if (!flag64_get(f, 0))
        return false;

    return true;
}

static bool test_flag64_invalid_bits(void)
{
    // Start with a nontrivial pattern
    flags64_t f = 0xF0F0F0F0F0F0F0F0ULL;
    flags64_t original = f;

    // Invalid bits must not change flags
    flags64_t r1 = flag64_set(f, -1, true);
    flags64_t r2 = flag64_set(f, -1, false);
    flags64_t r3 = flag64_set(f, 64, true);
    flags64_t r4 = flag64_set(f, 64, false);

    if (r1 != original || r2 != original || r3 != original || r4 != original)
        return false;

    // Getting invalid bits should always return false
    if (flag64_get(f, -1) != false)
        return false;
    if (flag64_get(f, 64) != false)
        return false;
    if (flag64_get(f, 1000) != false)
        return false;

    return true;
}

static bool test_flag64_setInline_basic(void)
{
    flags64_t f = 0;

    flag64_setInline(&f, 0, true);
    if (f != 1ULL)
        return false;
    if (!flag64_get(f, 0))
        return false;

    flag64_setInline(&f, 1, true);
    if (f != 0b11ULL)
        return false;

    // Clear bit 0
    flag64_setInline(&f, 0, false);
    if (f != (1ULL << 1))
        return false;
    if (flag64_get(f, 0))
        return false;
    if (!flag64_get(f, 1))
        return false;

    return true;
}

static bool test_flag64_setInline_invalid_and_null(void)
{
    // Null pointer: should be a no-op (just ensure no crash)
    flag64_setInline(NULL, 0, true);

    // Negative / out-of-range bits: should not change the flags
    flags64_t f = 0xAAAAAAAAAAAAAAAAULL;
    flags64_t original = f;

    flag64_setInline(&f, -1, true);
    if (f != original)
        return false;

    flag64_setInline(&f, -1, false);
    if (f != original)
        return false;

    flag64_setInline(&f, 64, true);
    if (f != original)
        return false;

    flag64_setInline(&f, 64, false);
    if (f != original)
        return false;

    return true;
}

static bool test_flag64_all_bits_set_and_clear(void)
{
    flags64_t f = 0;

    // Set all bits 0..63
    for (int i = 0; i < 64; ++i)
        f = flag64_set(f, i, true);

    if (f != ~0ULL)
        return false;

    // All bits should read as true
    for (int i = 0; i < 64; ++i)
        if (!flag64_get(f, i))
            return false;

    // Clear all bits back to 0
    for (int i = 0; i < 64; ++i)
        f = flag64_set(f, i, false);

    if (f != 0ULL)
        return false;

    // All bits should read as false
    for (int i = 0; i < 64; ++i)
        if (flag64_get(f, i))
            return false;

    return true;
}

static bool test_flag64_combined_mask_patterns(void)
{
    flags64_t f = 0;

    // Set an alternating pattern: bits 0,2,4,...,62
    for (int i = 0; i < 64; i += 2)
        f = flag64_set(f, i, true);

    // All even bits should be true, odd bits false
    for (int i = 0; i < 64; ++i)
    {
        bool bit = flag64_get(f, i);
        if (i % 2 == 0)
        {
            if (!bit)
                return false;
        }
        else
        {
            if (bit)
                return false;
        }
    }

    // Flip pattern: clear even bits, set odd bits
    for (int i = 0; i < 64; ++i)
    {
        bool value = (i % 2 != 0);
        f = flag64_set(f, i, value);
    }

    // Now all odd bits should be true, even bits false
    for (int i = 0; i < 64; ++i)
    {
        bool bit = flag64_get(f, i);
        if (i % 2 != 0)
        {
            if (!bit)
                return false;
        }
        else
        {
            if (bit)
                return false;
        }
    }

    return true;
}

static bool test_flag64_hasAny_hasAll(void)
{
    // flags: bits 0 and 2 set -> 0b0101
    flags64_t f = 0;
    f = flag64_set(f, 0, true);
    f = flag64_set(f, 2, true);

    // only bit 2
    flags64_t maskA = (1ULL << 2);
    // bits 2 and 3
    flags64_t maskB = (1ULL << 2) | (1ULL << 3);
    // only bit 1
    flags64_t maskC = (1ULL << 1);
    flags64_t mask0 = 0ULL;

    // maskA: bit2 is set, so any+all both true
    if (!flag64_hasAny(f, maskA))
        return false;
    if (!flag64_hasAll(f, maskA))
        return false;

    // maskB: bit2 set, bit3 not set
    if (!flag64_hasAny(f, maskB))
        return false;
    if (flag64_hasAll(f, maskB))
        return false;

    // maskC: no overlap
    if (flag64_hasAny(f, maskC))
        return false;
    if (flag64_hasAll(f, maskC))
        return false;

    // mask0: by definition hasAny -> false, hasAll -> true
    if (flag64_hasAny(f, mask0))
        return false;
    if (!flag64_hasAll(f, mask0))
        return false;

    return true;
}

static bool test_flag64_mask_ops(void)
{
    flags64_t f = 0;
    const flags64_t mask = (1ULL << 1) | (1ULL << 3); // bits 1 and 3

    // enableMask
    f = flag64_enableMask(f, mask);
    if (!flag64_get(f, 1) || !flag64_get(f, 3))
        return false;

    // disableMask (clear bit1, leave bit3)
    f = flag64_disableMask(f, 1ULL << 1);
    if (flag64_get(f, 1))
        return false;
    if (!flag64_get(f, 3))
        return false;

    // toggle: flip bits 0 and 3
    f = flag64_toggleMask(f, (1ULL << 0) | (1ULL << 3));
    // bit3 should clear, bit0 set
    if (flag64_get(f, 3))
        return false;
    if (!flag64_get(f, 0))
        return false;

    return true;
}

static bool test_flag64_bulk_clear_set_all(void)
{
    flags64_t f = 0x123456789ABCDEF0ULL;
    flags64_t original = f;

    // clearAll
    flag64_clearAll(&f);
    if (f != 0ULL)
        return false;

    // setAll
    flag64_setAll(&f);
    if (f != ~0ULL)
        return false;

    // NULL should be safe no-op
    flag64_clearAll(NULL);
    flag64_setAll(NULL);

    // ensure didn't accidentally alias
    (void)original;

    return true;
}

static bool test_flag64_popcount_and_scan(void)
{
    // 0
    flags64_t f = 0ULL;
    if (flag64_popcount(f) != 0)
        return false;
    if (flag64_findFirstSet(f) != -1)
        return false;
    if (flag64_findFirstClear(f) != 0) // first clear is bit0
        return false;

    // single bit at 5
    f = 1ULL << 5;
    if (flag64_popcount(f) != 1)
        return false;
    if (flag64_findFirstSet(f) != 5)
        return false;
    if (flag64_findFirstClear(f) != 0) // bit0 is clear
        return false;

    // pattern: bits 2,4,5 set -> 0b00110100 => pop=3, first set=2, first clear=0
    f = 0;
    f = flag64_set(f, 2, true);
    f = flag64_set(f, 4, true);
    f = flag64_set(f, 5, true);
    if (flag64_popcount(f) != 3)
        return false;
    if (flag64_findFirstSet(f) != 2)
        return false;
    if (flag64_findFirstClear(f) != 0)
        return false;

    // all bits set
    f = ~0ULL;
    if (flag64_popcount(f) != 64)
        return false;
    if (flag64_findFirstSet(f) != 0)
        return false;
    if (flag64_findFirstClear(f) != -1)
        return false;

    return true;
}

static bool test_flag64_toBinaryString(void)
{
    flags64_t f = 0;

    // Set bit63 and bit0
    f = flag64_set(f, 63, true);
    f = flag64_set(f, 0, true);

    char buf[65] = {0};
    flag64_toBinaryString(f, buf);

    // Must be length 64, plus null terminator
    if (strlen(buf) != 64)
        return false;
    if (buf[64] != '\0')
        return false;

    // bit63 -> buf[0] = '1'
    if (buf[0] != '1')
        return false;
    // bit0 -> buf[63] = '1'
    if (buf[63] != '1')
        return false;

    // all middle bits should be '0'
    for (int i = 1; i < 63; ++i)
    {
        if (buf[i] != '0')
            return false;
    }

    // NULL pointer should be safe (no crash)
    flag64_toBinaryString(f, NULL);

    return true;
}

int flags64_tests_run(void)
{
    fails += ut_assert(test_flag64_basic_set_get() == true,
                       "flags64 basic set/get");
    fails += ut_assert(test_flag64_bounds_bits_0_and_63() == true,
                       "flags64 bounds bits 0 and 63");
    fails += ut_assert(test_flag64_invalid_bits() == true,
                       "flags64 invalid bit indices");
    fails += ut_assert(test_flag64_setInline_basic() == true,
                       "flags64 setInline basic");
    fails += ut_assert(test_flag64_setInline_invalid_and_null() == true,
                       "flags64 setInline invalid bits & NULL");
    fails += ut_assert(test_flag64_all_bits_set_and_clear() == true,
                       "flags64 set/clear all bits basic");
    fails += ut_assert(test_flag64_combined_mask_patterns() == true,
                       "flags64 combined mask patterns");
    fails += ut_assert(test_flag64_hasAny_hasAll() == true,
                       "flags64 hasAny/hasAll");
    fails += ut_assert(test_flag64_mask_ops() == true,
                       "flags64 enable/disable/toggle mask");
    fails += ut_assert(test_flag64_bulk_clear_set_all() == true,
                       "flags64 bulk clear/set all helpers");
    fails += ut_assert(test_flag64_popcount_and_scan() == true,
                       "flags64 popcount and scan");
    fails += ut_assert(test_flag64_toBinaryString() == true,
                       "flags64 toBinaryString");

    return fails;
}