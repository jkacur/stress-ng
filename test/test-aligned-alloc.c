// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Colin Ian King
 *
 */

#define _GNU_SOURCE

#include <stdlib.h>

int main(void)
{
	void *alloc_buf;

	alloc_buf = aligned_alloc(64, (size_t)1024);
	if (alloc_buf)
		free(alloc_buf);

	return 0;
}
