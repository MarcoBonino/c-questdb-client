#pragma once

#include <stddef.h>
#include <stdint.h>

int sign_bin_file(const char *ec_name, const char *ec_sig_name, const char *hash_algorithm,
                  const uint8_t *private_key, uint8_t private_key_len,
                  const uint8_t *challenge, size_t challenge_len,
                  uint8_t *sig, size_t *sig_len);

void signChallenge(const char *challenge, size_t challenge_len, char **signature, size_t *signature_len);
