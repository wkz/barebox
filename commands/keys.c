#include <command.h>
#include <stdio.h>
#include <crypto/ecdsa.h>
#include <crypto/public_key.h>
#include <crypto/rsa.h>

static void print_key(const struct public_key *key)
{
	struct x509_fingerprint *fp;
	struct digest *d;

	if (key->key_name_hint)
		printf("Hint: %s\n", key->key_name_hint);

	switch (key->type) {
	case PUBLIC_KEY_TYPE_RSA:
		printf("Type: RSA (%u bits)\n", rsa_key_bits(key->rsa));
		break;
	case PUBLIC_KEY_TYPE_ECDSA:
		printf("Type: ECDSA (%s)\n", key->ecdsa->curve_name);
		break;
	}

	printf("Hash: %*phN\n", key->hashlen, key->hash);

	if (!key->fingerprints || !key->fingerprints->data)
		return;

	puts("X509 Fingerprints:\n");
	for (fp = key->fingerprints; fp->data; fp++) {
		d = digest_alloc_by_algo(fp->algo);

		printf("  %*phN (%s)\n", digest_length(d), fp->data,
		       digest_name(d) ? : "UNKNOWN-DIGEST");

		digest_free(d);
	}
}

static int do_keys(int argc, char *argv[])
{
	const struct public_key *key;
	int i = 0;

	for_each_public_key(key) {
		if (i++)
			putchar('\n');

		print_key(key);
	}

	return 0;
}

BAREBOX_CMD_HELP_START(keys)
BAREBOX_CMD_HELP_TEXT("Print informations about public keys")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(keys)
        .cmd            = do_keys,
        BAREBOX_CMD_DESC("Print informations about public keys")
        BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
        BAREBOX_CMD_HELP(cmd_keys_help)
BAREBOX_CMD_END
