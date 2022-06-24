#pragma once

#include <stddef.h>
#include <stdint.h>

void signChallenge(const char *challenge, size_t challenge_len, char **signature, size_t *signature_len);
