// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2013-2021 Canonical, Ltd.
 * Copyright (C) 2022-2023 Colin Ian King.
 *
 */
#include "stress-ng.h"
#include "core-builtin.h"
#include "core-mounts.h"

#if defined(HAVE_LINUX_FS_H)
#include <linux/fs.h>
#else
UNEXPECTED
#endif

#if defined(HAVE_SYS_SYSMACROS_H)
#include <sys/sysmacros.h>
#else
UNEXPECTED
#endif

#if defined(HAVE_SYS_STATFS_H)
#include <sys/statfs.h>
#else
UNEXPECTED
#endif

#if defined(HAVE_SYS_STATVFS_H)
#include <sys/statvfs.h>
#else
UNEXPECTED
#endif

static const stress_help_t help[] = {
	{ NULL,	"sysinfo N",	 "start N workers reading system information" },
	{ NULL,	"sysinfo-ops N", "stop after sysinfo bogo operations" },
	{ NULL,	NULL,		 NULL }
};

/*
 *  stress on system information
 *	stress system by rapid fetches of system information
 */
static int stress_sysinfo(const stress_args_t *args)
{
	int n_mounts;
	const int verify = !!(g_opt_flags & OPT_FLAGS_VERIFY);
	char *mnts[128];
#if defined(HAVE_SYS_SYSINFO_H) &&	\
    defined(HAVE_SYSINFO) &&		\
    defined(HAVE_SYS_STATFS_H)
	const int bad_fd = stress_get_bad_fd();
#endif

	(void)shim_memset(mnts, 0, sizeof(mnts));

	n_mounts = stress_mount_get(mnts, SIZEOF_ARRAY(mnts));
	if (n_mounts < 0) {
		pr_err("%s: failed to get mount points\n", args->name);
		return EXIT_FAILURE;
	}
	if (args->instance == 0)
		pr_dbg("%s: found %d mount points\n",
			args->name, n_mounts);

	stress_set_proc_state(args->name, STRESS_STATE_RUN);

	do {
		struct tms tms_buf;
		clock_t clk;
#if defined(HAVE_SYS_SYSINFO_H) &&	\
    defined(HAVE_SYSINFO) &&		\
    defined(HAVE_SYS_STATFS_H)
		{
			struct sysinfo sysinfo_buf;
			struct statfs statfs_buf;
			int i, ret;

			ret = sysinfo(&sysinfo_buf);
			if ((ret < 0) &&
			    (verify) &&
			    (errno != EPERM))
			 	pr_fail("%s: sysinfo failed, errno=%d (%s)\n",
					args->name, errno, strerror(errno));

			if (!stress_continue_flag())
				break;

			/* Linux statfs variant */
			for (i = 0; i < n_mounts; i++) {
				int fd;

				if (UNLIKELY(!stress_continue_flag()))
					break;

				if (UNLIKELY(!mnts[i]))
					continue;

				ret = statfs(mnts[i], &statfs_buf);
				/* Mount may have been removed, so purge it */
				if (UNLIKELY((ret < 0) && (errno == ENOENT))) {
					free(mnts[i]);
					mnts[i] = NULL;
					continue;
				}
				if (UNLIKELY((ret < 0) && (verify))) {
					if ((errno != ENOSYS) &&
					    (errno != EOVERFLOW) &&
					    (errno != EACCES) &&
					    (errno != ENOTCONN) &&
					    (errno != EPERM)) {
						pr_fail("%s: statfs on %s failed: errno=%d (%s)\n",
							args->name, mnts[i], errno,
							strerror(errno));
					}
				}

				/*
				 *  Exercise invalid mount points
				 */
				VOID_RET(int, statfs("/invalid_stress_ng", &statfs_buf));
				VOID_RET(int, statfs("", &statfs_buf));

				fd = open(mnts[i], O_RDONLY | O_DIRECTORY);
				if (UNLIKELY(fd < 0))
					continue;
#if defined(FS_IOC_GETFSLABEL) &&	\
    defined(FSLABEL_MAX)
				{
					char label[FSLABEL_MAX];

					VOID_RET(int, ioctl(fd, FS_IOC_GETFSLABEL, label));
				}
#else
				UNEXPECTED
#endif

				ret = fstatfs(fd, &statfs_buf);
				(void)close(fd);
				if (UNLIKELY((ret < 0) && (errno == ENOENT)))
					continue;
				if (UNLIKELY((ret < 0) && (verify))) {
					if ((errno != ENOSYS) &&
					    (errno != EOVERFLOW) &&
					    (errno != EACCES) &&
					    (errno != ENOTCONN) &&
					    (errno != EPERM)) {
						pr_fail("%s: fstatfs on %s failed: errno=%d (%s)\n",
							args->name, mnts[i], errno,
							strerror(errno));
					}
				}
				/*
				 *  Exercise invalid fd
				 */
				VOID_RET(int, fstatfs(bad_fd, &statfs_buf));
			}
		}
#endif

		{
			int i, ret;
			struct stat sbuf;
			struct shim_ustat ubuf;

			if (!stress_continue_flag())
				break;

			for (i = 0; i < n_mounts; i++) {
				if (!mnts[i])
					continue;

				ret = stat(mnts[i], &sbuf);
				if (UNLIKELY(ret < 0))
					continue;

				ret = shim_ustat(sbuf.st_dev, &ubuf);
				if (UNLIKELY((ret < 0) && (verify))) {
					if ((errno != EINVAL) &&
					    (errno != EFAULT) &&
					    (errno != ENOSYS) &&
					    (errno != ENOTCONN) &&
					    (errno != EPERM)) {
						pr_fail("%s: ustat on %s failed: errno=%d (%s)\n",
							args->name, mnts[i], errno,
							strerror(errno));
					}
				}
			}
#if defined(HAVE_SYS_SYSMACROS_H)
			/*
			 * Exercise invalid ustat, assuming that major ~0 is
			 * invalid
			 */
			sbuf.st_dev = makedev(~0, stress_mwc32());
			VOID_RET(int, shim_ustat(sbuf.st_dev, &ubuf));
#endif
		}

		if (UNLIKELY(!stress_continue_flag()))
			break;

#if defined(HAVE_SYS_STATVFS_H)
		{
			int i;

			struct statvfs statvfs_buf;
			/* POSIX.1-2001 statfs variant */
			for (i = 0; i < n_mounts; i++) {
				int ret;

				if (UNLIKELY(!stress_continue_flag()))
					break;

				if (UNLIKELY(!mnts[i]))
					continue;

				ret = statvfs(mnts[i], &statvfs_buf);
				if (UNLIKELY((ret < 0) && (verify))) {
					if ((errno != ENOSYS) &&
					    (errno != EOVERFLOW) &&
					    (errno != EACCES) &&
					    (errno != ENOTCONN) &&
					    (errno != EPERM)) {
						pr_fail("%s: statvfs on %s failed: errno=%d (%s)\n",
							args->name, mnts[i], errno,
							strerror(errno));
					}
				}
				/*
				 *  Exercise invalid mount point
				 */
				VOID_RET(int, statvfs("/invalid_stress_ng", &statvfs_buf));
			}
		}
#endif

		if (!stress_continue_flag())
			break;
		clk = times(&tms_buf);
		if ((clk == (clock_t)-1) && (verify)) {
			 pr_fail("%s: times failed, errno=%d (%s)\n",
				args->name, errno, strerror(errno));
		}
		stress_bogo_inc(args);
	} while (stress_continue(args));

	stress_set_proc_state(args->name, STRESS_STATE_DEINIT);

	stress_mount_free(mnts, n_mounts);

	return EXIT_SUCCESS;
}

stressor_info_t stress_sysinfo_info = {
	.stressor = stress_sysinfo,
	.class = CLASS_OS,
	.verify = VERIFY_OPTIONAL,
	.help = help
};
