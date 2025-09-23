// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires

#include <base64.h>
#include <common.h>
#include <device-mapper.h>
#include <dps.h>
#include <fs.h>
#include <jsmn.h>
#include <libfile.h>
#include <malloc.h>
#include <string.h>

#include <crypto/sha.h>
#include <crypto/public_key.h>
#include <crypto/rsa.h>

#include <linux/err.h>

struct dm_verity_dps_sig {
	char *rh;

	uint8_t *p7s;
	size_t p7slen;

	uint8_t certfp[SHA256_DIGEST_SIZE];
};

static struct dm_verity_dps_sig *dm_verity_parse_dps_sig(const char *json,
							 size_t jsonlen)
{
	struct dm_verity_dps_sig *sig = NULL;
	const jsmntok_t *tcert, *trh, *tsig;
	int b64len, binlen;
	jsmntok_t *toks;

	toks = jsmn_parse_alloc(json, jsonlen, NULL);
	if (!toks)
		return NULL;

	sig = xzalloc(sizeof(*sig));

	tcert = jsmn_find_value("certificateFingerprint", json, toks);
	trh = jsmn_find_value("rootHash", json, toks);
	tsig = jsmn_find_value("signature", json, toks);
	if (!(tcert && trh && tsig))
		goto err;

	if (jsmn_token_size(tcert) != SHA256_DIGEST_SIZE * 2)
		goto err;

	hex2bin(sig->certfp, json + tcert->start, sizeof(sig->certfp));

	sig->rh = xstrndup(json + trh->start, jsmn_token_size(trh));

	binlen = jsmn_token_size(tsig);
	if (binlen & 3)
		goto err;

	binlen = (binlen >> 2) * 3;

	sig->p7s = malloc(binlen);
	if (!sig->p7s)
		goto err;

	b64len = decode_base64(sig->p7s, binlen, json + tsig->start);
	if (!(b64len == binlen || b64len == binlen - 1 || b64len == binlen - 2))
		goto err_free_sig;

	sig->p7slen = b64len;
	return sig;

err_free_sig:
	free(sig->p7s);
err:
	free(sig);
	free(toks);
	return NULL;
}

static void dm_verity_dps_sig_free(struct dm_verity_dps_sig *sig)
{
	free(sig->p7s);
	free(sig->rh);
	free(sig);
}

static char *dm_verity_root_hash_from_sig(const char *json, size_t jsonlen,
					  char **errstr)
{
	struct dm_verity_dps_sig *sig;
	const struct public_key *key;
	u8 hash[SHA256_DIGEST_SIZE];
	struct digest *d = NULL;
	char *rh;
	int err;

	sig = dm_verity_parse_dps_sig(json, jsonlen);
	if (!sig) {
		*errstr = xasprintf("Unable to parse signature JSON object");
		return NULL;
	}

	key = public_key_get_by_fingerprint(HASH_ALGO_SHA256, sig->certfp,
					    sizeof(sig->certfp));
	if (!key) {
		*errstr = xasprintf("Found no key matching fingerprint %*phN",
				    (int)sizeof(sig->certfp), sig->certfp);
		goto err;
	}
	if (key->type != PUBLIC_KEY_TYPE_RSA) {
		*errstr = xasprintf("Only RSA keys are currently supported");
		goto err;
	}

	d = digest_alloc_by_algo(HASH_ALGO_SHA256);
	if (!d ||
	    digest_init(d) ||
	    digest_update(d, sig->rh, strlen(sig->rh)) ||
	    digest_final(d, hash)) {
		*errstr = xasprintf("Unable to generate hash of root-hash");
		goto err;
	}

	digest_free(d);

	err = public_key_verify(key, &sig->p7s[sig->p7slen - (rsa_key_bits(key->rsa) >> 3)],
				rsa_key_bits(key->rsa) >> 3, hash, HASH_ALGO_SHA256);
	if (err) {
		*errstr = xasprintf("Signature validation failed: %pe", ERR_PTR(err));
		goto err;
	}

	rh = xstrdup(sig->rh);
	dm_verity_dps_sig_free(sig);

	return rh;

err:
	if (d)
		digest_free(d);

	dm_verity_dps_sig_free(sig);
	return NULL;
}

static char *__dm_verity_config_from_dps(const char *data_dev, const char *hash_dev,
					 const char *sig_dev, char **errmsg)
{
	char *config, *json, *root_hash;
	size_t jsonlen;

	json = read_file(sig_dev, &jsonlen);
	if (!json) {
		*errmsg = xasprintf("Unable to read signature from %s", sig_dev);
		return ERR_PTR(-EIO);
	}

	root_hash = dm_verity_root_hash_from_sig(json, jsonlen, errmsg);
	if (!root_hash)
		return ERR_PTR(-EINVAL);

	config = dm_verity_config_from_sb(data_dev, hash_dev, root_hash);
	if (IS_ERR(config))
		*errmsg = xasprintf("Invalid or missing superblock: %pe", config);

	return config;
}

char *dm_verity_config_from_dps(const char *data_dev, const char *hash_dev,
				const char *sig_dev, char **errmsg)
{
	char *config = ERR_PTR(-ENODEV), *hash_part = NULL, *sig_part = NULL;
	struct cdev *data, *meta;

	if (hash_dev && sig_dev)
		return __dm_verity_config_from_dps(data_dev, hash_dev, sig_dev, errmsg);

	data = cdev_open_by_path_name(data_dev, O_RDONLY);
	if (!data) {
		*errmsg = xasprintf("Could not open %s", data_dev);
		goto out;
	}

	data = cdev_readlink(data);

	if (!hash_dev) {
		meta = dps_get_part_from_data(data, DPS_TYPE_VERITY);
		if (IS_ERR_OR_NULL(meta)) {
			*errmsg = xasprintf("Could not locate verity partition");
			goto out;
		}

		hash_part = xasprintf("/dev/%s", cdev_name(meta));
	}

	if (!sig_dev) {
		meta = dps_get_part_from_data(data, DPS_TYPE_VERITY_SIG);
		if (IS_ERR_OR_NULL(meta)) {
			*errmsg = xasprintf("Could not locate verity signature partition");
			goto out;
		}

		sig_part = xasprintf("/dev/%s", cdev_name(meta));
	}

	config = __dm_verity_config_from_dps(data_dev, hash_dev ? : hash_part,
					     sig_dev ? : sig_part, errmsg);

out:
	free(sig_part);
	free(hash_part);
	return config;
}
EXPORT_SYMBOL(dm_verity_config_from_dps);
