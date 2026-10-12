// cxxrt.cc -- the C++ run-time pieces a -fno-exceptions -fno-rtti program
// needs on CMS (M5f): operator new/delete on malloc, the libstdc++ throw
// helpers (they abort: no exceptions), guards for function statics and
// __cxa_atexit (static destructors do not run at exit).
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

void *operator new(size_t n) { void *p = malloc(n ? n : 1); if (!p) { fputs("C++: out of storage\n", stderr); abort(); } return p; }
void *operator new[](size_t n) { return operator new(n); }
void *operator new(size_t n, const std::nothrow_t &) noexcept { return malloc(n ? n : 1); }
void *operator new[](size_t n, const std::nothrow_t &) noexcept { return malloc(n ? n : 1); }
void operator delete(void *p) noexcept { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete[](void *p, size_t) noexcept { free(p); }
void *operator new(size_t n, std::align_val_t) { return operator new(n); }
void operator delete(void *p, std::align_val_t) noexcept { free(p); }
void operator delete(void *p, size_t, std::align_val_t) noexcept { free(p); }

static void die(const char *w) { fprintf(stderr, "C++: %s\n", w); abort(); }
namespace std {
void __throw_bad_alloc() { die("bad_alloc"); }
void __throw_bad_array_new_length() { die("bad_array_new_length"); }
void __throw_length_error(const char *s) { die(s); }
void __throw_logic_error(const char *s) { die(s); }
void __throw_out_of_range(const char *s) { die(s); }
void __throw_out_of_range_fmt(const char *s, ...) { die(s); }
void __throw_invalid_argument(const char *s) { die(s); }
void __throw_bad_function_call() { die("bad_function_call"); }
void __throw_runtime_error(const char *s) { die(s); }
void __throw_overflow_error(const char *s) { die(s); }
void __throw_bad_cast() { die("bad_cast"); }
}
extern "C" {
void *__dso_handle = 0;
int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
void __cxa_pure_virtual() { die("pure virtual call"); }
int __cxa_guard_acquire(long long *g) { return !*(char *)g; }
void __cxa_guard_release(long long *g) { *(char *)g = 1; }
void __cxa_guard_abort(long long *) {}
}

// glibc names the 32-bit libstdc++.a refers to
extern "C" {
char *secure_getenv(const char *n) { return getenv(n); }
unsigned long __isoc23_strtoul(const char *s, char **e, int b) { return strtoul(s, e, b); }
long __isoc23_strtol(const char *s, char **e, int b) { return strtol(s, e, b); }
}
