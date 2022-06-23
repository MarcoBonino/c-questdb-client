
#include "authentication.h"
#include "base64.h"

#include "../libecc/src/lib_ecc_types.h"
#include "../libecc/src/sig/ec_key.h"
#include "../libecc/src/libsig.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void signChallenge(const char *challenge, size_t challenge_len, char **signature, size_t *signature_len)
{
    const char *ec_name = "SECP256R1";
    const char *ec_sig_name = "ECDSA";
    const char *hash_algo = "SHA256";
    const char* SECP256R1_ECDSA_private_key_base64url = "5UjEMuA0Pj5pjK8a-fa24dyIf-Es5mYny3oE_Wmus48=";

    size_t private_key_len = 0;
    const uint8_t* SECP256R1_ECDSA_private_key = base64_decode(SECP256R1_ECDSA_private_key_base64url, strlen(SECP256R1_ECDSA_private_key_base64url), &private_key_len, true);
    assert(private_key_len == 32);

    size_t signature_bin_len = 0;
    uint8_t signature_bin[EC_MAX_SIGLEN];

    sign_bin_file(ec_name, ec_sig_name, hash_algo,
                  SECP256R1_ECDSA_private_key, (uint8_t)private_key_len,
                  (const uint8_t*)challenge, challenge_len,
                  signature_bin, &signature_bin_len);

    size_t sig_len = 0;
    *signature = base64_encode(signature_bin, signature_bin_len, &sig_len, false);
    *signature_len = (uint8_t)sig_len;
}

int string_to_params(const char *ec_name, const char *ec_sig_name,
                     ec_alg_type *sig_type,
                     const ec_str_params **ec_str_p,
                     const char *hash_name, hash_alg_type *hash_type)
{
    const ec_str_params *curve_params;
    const ec_sig_mapping *sm;
    const hash_mapping *hm;
    int ret;

    if (sig_type != NULL)
    {
        /* Get sig type from signature alg name */
        ret = get_sig_by_name(ec_sig_name, &sm);
        if ((ret) || (!sm))
        {
            printf("Error: signature type %s is unknown!\n", ec_sig_name);
            return -1;
        }
        *sig_type = sm->type;
    }

    if (ec_str_p != NULL)
    {
        /* Get curve params from curve name */
        size_t curve_name_len = strlen(ec_name);
        ret = ec_get_curve_params_by_name((const u8*)ec_name, (uint8_t)curve_name_len + 1, &curve_params);
        if ((ret) || (!curve_params))
        {
            printf("Error: EC curve %s is unknown!\n", ec_name);
            return -1;
        }
        *ec_str_p = curve_params;
    }

    if (hash_type != NULL)
    {
        /* Get hash type from hash alg name */
        ret = get_hash_by_name(hash_name, &hm);
        if ((ret) || (!hm))
        {
            printf("Error: hash function %s is unknown!\n", hash_name);
            return -1;
        }
        *hash_type = hm->type;
    }

    return 0;
}

int sign_bin_file(const char *ec_name, const char *ec_sig_name, const char *hash_algorithm,
                  const uint8_t *private_key, uint8_t private_key_len,
                  const uint8_t *challenge, size_t challenge_len,
                  uint8_t *sig, size_t *sig_len)
{
    ec_key_pair key_pair;
    const ec_str_params *ec_str_p;
    ec_params params;
    ec_alg_type sig_type;
    hash_alg_type hash_type;
    struct ec_sign_context sig_ctx;

    /* Get parameters from pretty names */
    if (   string_to_params(ec_name, ec_sig_name, &sig_type, &ec_str_p, hash_algorithm, &hash_type)
        || import_params(&params, ec_str_p))
    {
        printf("Invalid signing parameters.\n");
        return -1;
    }

    if (ec_key_pair_import_from_priv_key_buf(&key_pair, &params, private_key, private_key_len, sig_type))
    {
        printf("Error when importing private key.\n");
        return -1;
    }

    uint8_t siglen = 0;
    if (ec_get_sig_len(&params, sig_type, hash_type, &siglen))
    {
        printf("Error getting effective signature length from %s\n", (const char *)(ec_str_p->name->buf));
        return -1;
    }

    if (   ec_sign_init(&sig_ctx, &key_pair, sig_type, hash_type, NULL, 0)
        || ec_sign_update(&sig_ctx, challenge, challenge_len)
        || ec_sign_finalize(&sig_ctx, sig, siglen))
    {
        printf("Error: error when signing\n");
        return -1;
    }
    *sig_len = siglen;

    return 0;
}
