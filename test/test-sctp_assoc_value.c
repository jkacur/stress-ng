// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Colin Ian King
 *
 */
#include <netinet/sctp.h>

int main(void)
{
	struct sctp_assoc_value s;

	(void)s;

	return sizeof(s);
}
