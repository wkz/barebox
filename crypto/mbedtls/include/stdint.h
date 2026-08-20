/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * <stdint.h> for barebox, consistent with <linux/types.h>.
 *
 * Third-party code such as Mbed TLS (which we do not patch) includes
 * <stdint.h>. The toolchain's own <stdint.h> typedefs [u]int64_t as
 * [unsigned] long, whereas barebox's <linux/types.h> uses [unsigned] long long.
 * Both are 64-bit, but they are distinct types, so a TU that pulls in both
 * headers hits a conflicting-typedef error.
 *
 * Resolve it here: take the fixed-width typedefs from barebox, then chain to the
 * real <stdint.h> for everything else (INTx_MAX, SIZE_MAX, the UINTx_C()/INTx_C()
 * literal macros, the least/fast/max/ptr-sized types, ...) with only its
 * conflicting 64-bit typedefs suppressed. Undefining __INT64_TYPE__ /
 * __UINT64_TYPE__ drops just those two typedefs; the value macros use separate
 * builtins and are left intact.
 *
 * This shim is placed ahead of the toolchain include dir on the search path, so
 * <stdint.h> resolves here first and #include_next reaches the toolchain copy.
 */
#ifndef _BAREBOX_MBEDTLS_STDINT_H
#define _BAREBOX_MBEDTLS_STDINT_H

#include <linux/types.h>

#undef __INT64_TYPE__
#undef __UINT64_TYPE__

#include_next <stdint.h>

#endif /* _BAREBOX_MBEDTLS_STDINT_H */
