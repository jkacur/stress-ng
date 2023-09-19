// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2013-2021 Canonical, Ltd.
 * Copyright (C) 2022-2023 Colin Ian King.
 *
 */

#define _GNU_SOURCE

#include <time.h>

int main(void)
{
	timer_t timerid = 0;
	struct itimerspec new = { 0 };
	struct itimerspec old = { 0 };

	return timer_settime(&timerid, 0, &new, &old);
}
