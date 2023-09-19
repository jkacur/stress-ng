// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Colin Ian King
 *
 */

#if defined(__hppa) ||	\
    defined(__hppa__)
int main(void)
{
	__asm__ __volatile__("diag 0");

	return 0;
}
#else
#error not HPPA so no diag instruction
#endif
