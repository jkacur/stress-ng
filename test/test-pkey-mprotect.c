// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2013-2021 Canonical, Ltd.
 * Copyright (C) 2022-2023 Colin Ian King
 *
 */

#define _GNU_SOURCE

#include <sys/mman.h>
#include <unistd.h>
#include <features.h>

int main(void)
{
	return pkey_mprotect((void *)0, 4096, PROT_NONE, -1);
}
