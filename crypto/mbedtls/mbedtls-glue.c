// SPDX-License-Identifier: GPL-2.0-only
/*
 * Glue between barebox and Mbed TLS. This TU deliberately includes both barebox
 * and mbedtls headers to exercise (and rely on) the barebox <stdint.h> shim
 * that lets the two header worlds coexist; see crypto/mbedtls/include/stdint.h.
 */

#include <common.h>
#include <init.h>
#include <linux/errno.h>
#include <string.h>

#include <psa/crypto.h>

/*
 * The library is built with MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG, so the PSA core
 * expects the platform to supply randomness. Verifying a PKCS#1 v1.5 signature
 * never consumes any, so this exists only to satisfy the linker and PSA init.
 * If an RNG-consuming operation is ever enabled, replace this with a real
 * entropy source.
 */
psa_status_t mbedtls_psa_external_get_random(mbedtls_psa_external_random_context_t *context,
					     uint8_t *output, size_t output_size,
					     size_t *output_length)
{
	memset(output, 0, output_size);
	*output_length = output_size;
	return PSA_SUCCESS;
}

/*
 * The library is built with MBEDTLS_PLATFORM_ZEROIZE_ALT so it does not pull in
 * glibc's explicit_bzero(). This is mbedtls' own fallback: a volatile pointer
 * defeats dead-store elimination without needing a compiler barrier.
 */
void mbedtls_platform_zeroize(void *buf, size_t len)
{
	volatile unsigned char *p = buf;

	while (len--)
		*p++ = 0;
}

/*
 * barebox exposes its string helper as _strchr(), with a "#define strchr
 * _strchr" macro in <linux/string.h>. mbedtls references the plain strchr
 * symbol, which barebox does not otherwise export, so provide it here. Undef the
 * macro first so this defines strchr, not _strchr. C semantics: c == 0 returns
 * the terminator.
 */
#undef strchr
char *strchr(const char *s, int c);
char *strchr(const char *s, int c)
{
	for (; *s; s++)
		if (*s == (char)c)
			return (char *)s;

	return c == 0 ? (char *)s : NULL;
}

static int mbedtls_init(void)
{
	psa_status_t st;

	st = psa_crypto_init();
	if (st != PSA_SUCCESS) {
		pr_err("mbedtls: psa_crypto_init failed: %d\n", (int)st);
		return -EIO;
	}

	return 0;
}
core_initcall(mbedtls_init);
