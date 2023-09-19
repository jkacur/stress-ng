// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C)      2023 Colin Ian King
 *
 */
#include <immintrin.h>
#include <string.h>
#include <stdint.h>

void rndset(unsigned char *ptr, const size_t len)
{
	size_t i;
	uintptr_t addr = (uintptr_t)rndset;

	for (i = 0; i < len; i++, addr += 37)
		ptr[i] = (unsigned char)((addr >> 3) & 0xff);
}

int __attribute__ ((target("avx512bw"))) main(int argc, char **argv)
{
	__m512i r;
	unsigned char a[256];

	(void)rndset((unsigned char *)&a, sizeof(a));
	r = _mm512_loadu_si512((void *)a);

	return *(int *)&r;
}
