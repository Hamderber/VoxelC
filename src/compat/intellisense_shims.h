/* Only active for IntelliSense (VS Code C/C++ extension defines __INTELLISENSE__) */
#if defined(__INTELLISENSE__) && defined(_MSC_VER) && !defined(__clang__)
/* If MSVC's IntelliSense doesn't see the experimental flags, pretend the headers exist. */
#ifndef __has_include
#define __has_include(x) 0
#endif

/* ---- C11 atomics (very small, non-thread-safe placebo just to silence squiggles) ---- */
/* Make _Atomic(T) parse as T so declarations stop erroring in IntelliSense. */
#ifndef _Atomic
#define _Atomic(T) T

/* Define only what you actually use. Add more as needed. */
typedef int atomic_int;
typedef unsigned int atomic_uint;

typedef enum
{
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;

/* No-op stand-ins just for IntelliSense; DO NOT rely on these at runtime. */
static inline int atomic_load(const atomic_int *p) { return *(const int *)p; }
static inline void atomic_store(atomic_int *p, int v) { *(int *)p = v; }
static inline int atomic_exchange(atomic_int *p, int v)
{
    int o = *p;
    *p = v;
    return o;
}
static inline int atomic_fetch_add(atomic_int *p, int v)
{
    int o = *p;
    *p += v;
    return o;
}
static inline int atomic_fetch_sub(atomic_int *p, int v)
{
    int o = *p;
    *p -= v;
    return o;
}
static inline void atomic_thread_fence(memory_order order) { (void)order; }
#endif /* !__has_include(<stdatomic.h>) */

/* ---- C11 threads (MSVC doesn't ship <threads.h>) ---- */
#if !__has_include(<threads.h>)
typedef void *thrd_t;
typedef void *mtx_t;
enum
{
    thrd_success = 0,
    thrd_error = -1
};
static inline int thrd_create(thrd_t *t, int (*fn)(void *), void *arg)
{
    (void)t;
    (void)fn;
    (void)arg;
    return thrd_success;
}
static inline void thrd_yield(void) {}
static inline int thrd_join(thrd_t thr, int *res)
{
    (void)thr;
    if (res)
        *res = 0;
    return thrd_success;
}
static inline int mtx_init(mtx_t *m, int type)
{
    (void)m;
    (void)type;
    return thrd_success;
}
static inline void mtx_destroy(mtx_t *m) { (void)m; }
static inline int mtx_lock(mtx_t *m)
{
    (void)m;
    return thrd_success;
}
static inline int mtx_unlock(mtx_t *m)
{
    (void)m;
    return thrd_success;
}
#endif /* !__has_include(<threads.h>) */

#endif /* __INTELLISENSE__ && _MSC_VER && !__clang__ */
