// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Colin Ian King
 *
 */

#include <linux/rtc.h>

int main(void)
{
	struct rtc_param param;

	return sizeof(param);
}
