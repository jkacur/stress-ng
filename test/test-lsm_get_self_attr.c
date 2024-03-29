/*
 * Copyright (C) 2024      Colin Ian King.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#define  _GNU_SOURCE

#include <stdlib.h>
#include <linux/lsm.h>

int main(void)
{
	char buf[4096];
	struct lsm_ctx *ctx = (struct lsm_ctx *)buf;
	size_t size = sizeof(buf);

	return lsm_get_self_attr(LSM_ATTR_CURRENT, &ctx, &size, 0);
}
