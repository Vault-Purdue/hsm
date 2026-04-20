#include <stdint.h>
#include <stddef.h>
#include "ext/handy.h"

void cf_curve25519_mul(uint8_t *q, const uint8_t *n, const uint8_t *p);
void cf_curve25519_mul_base(uint8_t *q, const uint8_t *n);