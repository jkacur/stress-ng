/*
 * Copyright (C) 2013-2021 Canonical, Ltd.
 * Copyright (C) 2022-2023 Colin Ian King.
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
#include "stress-ng.h"
#include "core-builtin.h"
#include "core-pragma.h"
#include "core-put.h"
#include "core-target-clones.h"

#define MIN_MATRIX3D_SIZE	(16)
#define MAX_MATRIX3D_SIZE	(1024)
#define DEFAULT_MATRIX3D_SIZE	(128)

static const stress_help_t help[] = {
	{ NULL,	"matrix-3d N",		"start N workers exercising 3D matrix operations" },
	{ NULL,	"matrix-3d-method M",	"specify 3D matrix stress method M, default is all" },
	{ NULL,	"matrix-3d-ops N",	"stop after N 3D maxtrix bogo operations" },
	{ NULL,	"matrix-3d-size N",	"specify the size of the N x N x N matrix" },
	{ NULL,	"matrix-3d-zyx",	"matrix operation is z by y by x instead of x by y by z" },
	{ NULL,	NULL,			NULL }
};

#if defined(HAVE_VLA_ARG) &&	\
    !defined(HAVE_COMPILER_PCC)

typedef float 	stress_matrix_3d_type_t;

/*
 *  the matrix stress test has different classes of matrix stressor
 */
typedef void (*stress_matrix_3d_func_t)(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n]);

typedef struct {
	const char			*name;		/* human readable form of stressor */
	const stress_matrix_3d_func_t	func[2];	/* method functions, x by y by z, z by y by x */
} stress_matrix_3d_method_info_t;

static const char *current_method = NULL;		/* current matrix method */
static size_t method_all_index;				/* all method index */

static const stress_matrix_3d_method_info_t matrix_3d_methods[];

static int stress_set_matrix_3d_size(const char *opt)
{
	size_t matrix_3d_size;

	matrix_3d_size = stress_get_uint64(opt);
	stress_check_range("matrix-3d-size", matrix_3d_size,
		MIN_MATRIX3D_SIZE, MAX_MATRIX3D_SIZE);
	return stress_set_setting("matrix-3d-size", TYPE_ID_SIZE_T, &matrix_3d_size);
}

static int stress_set_matrix_3d_zyx(const char *opt)
{
	size_t matrix_3d_zyx = 1;

	(void)opt;

	return stress_set_setting("matrix-3d-zyx", TYPE_ID_SIZE_T, &matrix_3d_zyx);
}

/*
 *  stress_matrix_3d_xyz_add()
 *	matrix addition
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_add(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				r[i][j][k] = a[i][j][k] + b[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_zyx_add()
 *	matrix addition
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_add(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = a[i][j][k] + b[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_xyz_sub()
 *	matrix subtraction
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_sub(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				r[i][j][k] = a[i][j][k] - b[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_zyx_add()
 *	matrix subtraction
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_sub(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = a[i][j][k] + b[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_trans()
 *	matrix transpose
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_trans(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],	/* Ignored */
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	(void)b;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

			for (k = 0; k < n; k++) {
				r[i][j][k] = a[k][j][i];
			}
		}
	}
}

/*
 *  stress_matrix_3d_trans()
 *	matrix transpose
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_trans(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],	/* Ignored */
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	(void)b;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = a[k][j][i];
			}
		}
	}
}

/*
 *  stress_matrix_3d_mult()
 *	matrix scalar multiply
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_mult(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;
	stress_matrix_3d_type_t v = b[0][0][0];

	(void)b;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				r[i][j][k] = v * a[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_mult()
 *	matrix scalar multiply
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_mult(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;
	stress_matrix_3d_type_t v = b[0][0][0];

	(void)b;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = v * a[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_div()
 *	matrix scalar divide
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_div(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;
	stress_matrix_3d_type_t v = b[0][0][0];

	(void)b;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				r[i][j][k] = a[i][j][k] / v;
			}
		}
	}
}

/*
 *  stress_matrix_3d_div()
 *	matrix scalar divide
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_div(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;
	stress_matrix_3d_type_t v = b[0][0][0];

	(void)b;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = a[i][j][k] / v;
			}
		}
	}
}

/*
 *  stress_matrix_3d_hadamard()
 *	matrix hadamard product
 *	(A o B)ij = AijBij
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_hadamard(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				r[i][j][k] = a[i][j][k] * b[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_hadamard()
 *	matrix hadamard product
 *	(A o B)ij = AijBij
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_hadamard(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = a[i][j][k] * b[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_frobenius()
 *	matrix frobenius product
 *	A : B = Sum(AijBij)
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_frobenius(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;
	stress_matrix_3d_type_t sum = 0.0;

	(void)r;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				sum += a[i][j][k] * b[i][j][k];
			}
		}
	}
	stress_float_put((float)sum);
}

/*
 *  stress_matrix_3d_frobenius()
 *	matrix frobenius product
 *	A : B = Sum(AijBij)
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_frobenius(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;
	stress_matrix_3d_type_t sum = 0.0;

	(void)r;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				sum += a[i][j][k] * b[i][j][k];
			}
		}
	}
	stress_float_put((float)sum);
}

/*
 *  stress_matrix_3d_copy()
 *	naive matrix copy, r = a
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_copy(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	(void)b;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

			/* unrolling does not help, the following turns into a memcpy */
			for (k = 0; k < n; k++) {
				r[i][j][k] = a[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_copy()
 *	naive matrix copy, r = a
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_copy(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	(void)b;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			/* unrolling does not help, the following turns into a memcpy */
			for (i = 0; i < n; i++) {
				r[i][j][k] = a[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_mean(void)
 *	arithmetic mean
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_mean(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				r[i][j][k] = (a[i][j][k] + b[i][j][k]) / (stress_matrix_3d_type_t)2.0;
			}
		}
	}
}

/*
 *  stress_matrix_3d_mean(void)
 *	arithmetic mean
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_mean(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = (a[i][j][k] + b[i][j][k]) / (stress_matrix_3d_type_t)2.0;
			}
		}
	}
}

/*
 *  stress_matrix_3d_zero()
 *	simply zero the result matrix
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_zero(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	(void)a;
	(void)b;

	for (i = 0; i < n; i++) {
		register size_t j;

PRAGMA_UNROLL_N(8)
		for (j = 0; j < n; j++) {
			register size_t k;

			/* unrolling does not help, the following turns into a memzero */
			for (k = 0; k < n; k++) {
				r[i][j][k] = 0.0;
			}
		}
	}
}

/*
 *  stress_matrix_3d_zero()
 *	simply zero the result matrix
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_zero(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	(void)a;
	(void)b;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = 0.0;
			}
		}
	}
}

/*
 *  stress_matrix_3d_negate()
 *	simply negate the matrix a and put result in r
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_negate(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	(void)a;
	(void)b;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

PRAGMA_UNROLL_N(8)
			for (k = 0; k < n; k++) {
				r[i][j][k] = -a[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_negate()
 *	simply negate the matrix a and put result in r
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_negate(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	(void)a;
	(void)b;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = -a[i][j][k];
			}
		}
	}
}

/*
 *  stress_matrix_3d_identity()
 *	set r to the identity matrix
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_xyz_identity(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t i;

	(void)a;
	(void)b;

	for (i = 0; i < n; i++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t k;

			/* loop unrolling does not improve the loop */
			for (k = 0; k < n; k++) {
				r[i][j][k] = ((i == j) && (j == k)) ? 1.0 : 0.0;
			}
		}
	}
}

/*
 *  stress_matrix_3d_identity()
 *	set r to the identity matrix
 */
static void OPTIMIZE3 TARGET_CLONES stress_matrix_3d_zyx_identity(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	register size_t k;

	(void)a;
	(void)b;

	for (k = 0; k < n; k++) {
		register size_t j;

		for (j = 0; j < n; j++) {
			register size_t i;

			for (i = 0; i < n; i++) {
				r[i][j][k] = ((i == j) && (j == k)) ? 1.0 : 0.0;
			}
		}
	}
}

static void stress_matrix_3d_xyz_all(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n]);

static void stress_matrix_3d_zyx_all(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n]);

/*
 * Table of matrix_3 stress methods, ordered x by y by z and z by y by x
 */
static const stress_matrix_3d_method_info_t matrix_3d_methods[] = {
	{ "all",		{ stress_matrix_3d_xyz_all,		stress_matrix_3d_zyx_all } },/* Special "all" test */

	{ "add",		{ stress_matrix_3d_xyz_add,		stress_matrix_3d_zyx_add } },
	{ "copy",		{ stress_matrix_3d_xyz_copy,		stress_matrix_3d_zyx_copy } },
	{ "div",		{ stress_matrix_3d_xyz_div,		stress_matrix_3d_zyx_div } },
	{ "frobenius",		{ stress_matrix_3d_xyz_frobenius,	stress_matrix_3d_zyx_frobenius } },
	{ "hadamard",		{ stress_matrix_3d_xyz_hadamard,	stress_matrix_3d_zyx_hadamard } },
	{ "identity",		{ stress_matrix_3d_xyz_identity,	stress_matrix_3d_zyx_identity } },
	{ "mean",		{ stress_matrix_3d_xyz_mean,		stress_matrix_3d_zyx_mean } },
	{ "mult",		{ stress_matrix_3d_xyz_mult,		stress_matrix_3d_zyx_mult } },
	{ "negate",		{ stress_matrix_3d_xyz_negate,		stress_matrix_3d_zyx_negate } },
	{ "sub",		{ stress_matrix_3d_xyz_sub,		stress_matrix_3d_zyx_sub } },
	{ "trans",		{ stress_matrix_3d_xyz_trans,		stress_matrix_3d_zyx_trans } },
	{ "zero",		{ stress_matrix_3d_xyz_zero,		stress_matrix_3d_zyx_zero } },
};

static stress_metrics_t matrix_3d_metrics[SIZEOF_ARRAY(matrix_3d_methods)];

/*
 *  stress_matrix_3d_all()
 *	iterate over all matrix_3 stressors
 */
static void OPTIMIZE3 stress_matrix_3d_xyz_all(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	double t;

	current_method = matrix_3d_methods[method_all_index].name;

	t = stress_time_now();
	matrix_3d_methods[method_all_index].func[0](n, a, b, r);
	matrix_3d_metrics[method_all_index].duration += stress_time_now() - t;
	matrix_3d_metrics[method_all_index].count += 1.0;
}

/*
 *  stress_matrix_3d_all()
 *	iterate over all matrix_3d stressors
 */
static void OPTIMIZE3 stress_matrix_3d_zyx_all(
	const size_t n,
	stress_matrix_3d_type_t a[RESTRICT n][n][n],
	stress_matrix_3d_type_t b[RESTRICT n][n][n],
	stress_matrix_3d_type_t r[RESTRICT n][n][n])
{
	double t;

	current_method = matrix_3d_methods[method_all_index].name;

	t = stress_time_now();
	matrix_3d_methods[method_all_index].func[1](n, a, b, r);
	matrix_3d_metrics[method_all_index].duration += stress_time_now() - t;
	matrix_3d_metrics[method_all_index].count += 1.0;
}

/*
 *  stress_set_matrix_3d_method()
 *	get the default matrix stress method
 */
static int stress_set_matrix_3d_method(const char *name)
{
	size_t matrix_3d_method;

	for (matrix_3d_method = 0; matrix_3d_method < SIZEOF_ARRAY(matrix_3d_methods); matrix_3d_method++) {
		if (!strcmp(matrix_3d_methods[matrix_3d_method].name, name)) {
			stress_set_setting("matrix-3d-method", TYPE_ID_SIZE_T, &matrix_3d_method);
			return 0;
		}
	}

	(void)fprintf(stderr, "matrix-3d-method must be one of:");
	for (matrix_3d_method = 0; matrix_3d_method < SIZEOF_ARRAY(matrix_3d_methods); matrix_3d_method++)
		(void)fprintf(stderr, " %s", matrix_3d_methods[matrix_3d_method].name);
	(void)fprintf(stderr, "\n");

	return -1;
}

static inline size_t round_up(size_t page_size, size_t n)
{
	page_size = (page_size == 0) ? 4096 : page_size;

	return (n + page_size - 1) & (~(page_size -1));
}

/*
 *  stress_matrix_data()
 *`	generate some random data scaled by v
 */
static inline stress_matrix_3d_type_t stress_matrix_data(const stress_matrix_3d_type_t v)
{
	const uint64_t r = stress_mwc64();

	return v * (stress_matrix_3d_type_t)r;
}

static inline int stress_matrix_3d_exercise(
	const stress_args_t *args,
	const size_t matrix_3d_method,
	const size_t matrix_3d_zyx,
	const size_t n)
{
	int ret = EXIT_NO_RESOURCE;
	typedef stress_matrix_3d_type_t (*matrix_3d_ptr_t)[n][n];
	size_t matrix_3d_size = sizeof(stress_matrix_3d_type_t) * n * n * n;
	size_t matrix_3d_mmap_size = round_up(args->page_size, matrix_3d_size);
	const size_t num_matrix_3d_methods = SIZEOF_ARRAY(matrix_3d_methods);
	const stress_matrix_3d_func_t func = matrix_3d_methods[matrix_3d_method].func[matrix_3d_zyx];
	const bool verify = !!(g_opt_flags & OPT_FLAGS_VERIFY);

	matrix_3d_ptr_t a, b = NULL, r = NULL, s = NULL;
	register size_t i, j;
	const stress_matrix_3d_type_t v = 65535 / (stress_matrix_3d_type_t)((uint64_t)~0);
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#if defined(MAP_POPULATE)
	flags |= MAP_POPULATE;
#endif

	method_all_index = 1;
	current_method = matrix_3d_methods[matrix_3d_method].name;

	for (i = 0; i < num_matrix_3d_methods; i++) {
		matrix_3d_metrics[i].duration = 0.0;
		matrix_3d_metrics[i].count = 0.0;
	}

	a = (matrix_3d_ptr_t)mmap(NULL, matrix_3d_mmap_size,
		PROT_READ | PROT_WRITE, flags, -1, 0);
	if (a == MAP_FAILED) {
		pr_fail("%s: matrix allocation failed, out of memory\n", args->name);
		goto tidy_ret;
	}
	b = (matrix_3d_ptr_t)mmap(NULL, matrix_3d_mmap_size,
		PROT_READ | PROT_WRITE, flags, -1, 0);
	if (b == MAP_FAILED) {
		pr_fail("%s: matrix allocation failed, out of memory\n", args->name);
		goto tidy_a;
	}
	r = (matrix_3d_ptr_t)mmap(NULL, matrix_3d_mmap_size,
		PROT_READ | PROT_WRITE, flags, -1, 0);
	if (r == MAP_FAILED) {
		pr_fail("%s: matrix allocation failed, out of memory\n", args->name);
		goto tidy_b;
	}
	if (verify) {
		s = (matrix_3d_ptr_t)mmap(NULL, matrix_3d_mmap_size,
			PROT_READ | PROT_WRITE, flags, -1, 0);
		if (s == MAP_FAILED) {
			pr_fail("%s: matrix allocation failed, out of memory\n", args->name);
			goto tidy_r;
		}
	}

	/*
	 *  Initialise matrices
	 */
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			register size_t k;

			for (k = 0; k < n; k++) {
				a[i][j][k] = stress_matrix_data(v);
				b[i][j][k] = stress_matrix_data(v);
				r[i][j][k] = 0.0;
			}
		}
	}

	/*
	 * Normal use case, 100% load, simple spinning on CPU
	 */
	do {
		double t;

		t = stress_time_now();
		(void)func(n, a, b, r);
		matrix_3d_metrics[matrix_3d_method].duration += stress_time_now() - t;
		matrix_3d_metrics[matrix_3d_method].count += 1.0;
		stress_bogo_inc(args);

		if (verify) {
			t = stress_time_now();
			(void)func(n, a, b, s);
			matrix_3d_metrics[matrix_3d_method].duration += stress_time_now() - t;
			matrix_3d_metrics[matrix_3d_method].count += 1.0;
			stress_bogo_inc(args);

			if (shim_memcmp(r, s, matrix_3d_size)) {
				pr_fail("%s: %s: data difference between identical matrix-3d computations\n",
					args->name, current_method);
			}
		}
		if (matrix_3d_method == 0) {
			method_all_index++;
			if (method_all_index >= SIZEOF_ARRAY(matrix_3d_methods))
				method_all_index = 1;
		}
	} while (stress_continue(args));

	/* Dump metrics except for 'all' method */
	for (i = 1, j = 0; i < num_matrix_3d_methods; i++) {
		if (matrix_3d_metrics[i].duration > 0.0) {
			char msg[64];
			const double rate = matrix_3d_metrics[i].count / matrix_3d_metrics[i].duration;

			(void)snprintf(msg, sizeof(msg), "%s matrix-3d ops per sec", matrix_3d_methods[i].name);
			stress_metrics_set(args, j, msg,
				rate, STRESS_HARMONIC_MEAN);
			j++;
		}
	}

	ret = EXIT_SUCCESS;
	if (verify)
		(void)munmap((void *)s, matrix_3d_mmap_size);
tidy_r:
	(void)munmap((void *)r, matrix_3d_mmap_size);
tidy_b:
	(void)munmap((void *)b, matrix_3d_mmap_size);
tidy_a:
	(void)munmap((void *)a, matrix_3d_mmap_size);
tidy_ret:
	return ret;
}

/*
 *  stress_matrix_3d()
 *	stress CPU by doing floating point math ops
 */
static int stress_matrix_3d(const stress_args_t *args)
{
	size_t matrix_3d_method = 0; 	/* All method */
	size_t matrix_3d_size = DEFAULT_MATRIX3D_SIZE;
	size_t matrix_3d_zyx = 0;
	int rc;

	stress_catch_sigill();

	(void)stress_get_setting("matrix-3d-method", &matrix_3d_method);
	(void)stress_get_setting("matrix-3d-zyx", &matrix_3d_zyx);

	if (args->instance == 0)
		pr_dbg("%s: using method '%s' (%s)\n", args->name, matrix_3d_methods[matrix_3d_method].name,
			matrix_3d_zyx ? "z by y by x" : "x by y by z");

	if (!stress_get_setting("matrix-3d-size", &matrix_3d_size)) {
		if (g_opt_flags & OPT_FLAGS_MAXIMIZE)
			matrix_3d_size = MAX_MATRIX3D_SIZE;
		if (g_opt_flags & OPT_FLAGS_MINIMIZE)
			matrix_3d_size = MIN_MATRIX3D_SIZE;
	}

	stress_set_proc_state(args->name, STRESS_STATE_RUN);

	rc = stress_matrix_3d_exercise(args, matrix_3d_method, matrix_3d_zyx, matrix_3d_size);

	stress_set_proc_state(args->name, STRESS_STATE_DEINIT);

	return rc;
}

static void stress_matrix_3d_set_default(void)
{
	stress_set_matrix_3d_method("all");
}

static const stress_opt_set_func_t opt_set_funcs[] = {
	{ OPT_matrix_3d_method,	stress_set_matrix_3d_method },
	{ OPT_matrix_3d_size,	stress_set_matrix_3d_size },
	{ OPT_matrix_3d_zyx,	stress_set_matrix_3d_zyx },
	{ 0,			NULL }
};

stressor_info_t stress_matrix_3d_info = {
	.stressor = stress_matrix_3d,
	.set_default = stress_matrix_3d_set_default,
	.class = CLASS_CPU | CLASS_CPU_CACHE | CLASS_MEMORY,
	.opt_set_funcs = opt_set_funcs,
	.verify = VERIFY_OPTIONAL,
	.help = help
};
#else
stressor_info_t stress_matrix_3d_info = {
	.stressor = stress_unimplemented,
	.class = CLASS_CPU | CLASS_CPU_CACHE | CLASS_MEMORY,
	.verify = VERIFY_OPTIONAL,
	.help = help,
	.unimplemented_reason = "compiler does not support variable length array function arguments"
};
#endif
