/*
 * Copyright (C) 2013-2014 Canonical, Ltd.
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
 * This code is a complete clean re-write of the stress tool by
 * Colin Ian King <colin.king@canonical.com> and attempts to be
 * backwardly compatible with the stress tool by Amos Waterland
 * <apw@rossby.metr.ou.edu> but has more stress tests and more
 * functionality.
 *
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "stress-ng.h"

/*
 *  stress_link_unlink()
 *	remove all links
 */
static void stress_link_unlink(const char *funcname, const uint64_t n)
{
	uint64_t i;
	const pid_t pid = getpid();

	for (i = 0; i < n; i++) {
		char path[PATH_MAX];

		snprintf(path, sizeof(path), "stress-%s-%i-%"
			PRIu64 ".lnk", funcname, pid, i);
		(void)unlink(path);
	}
	sync();
}

/*
 *  stress_link_generic
 *	stress links, generic case
 */
static int stress_link_generic(
	uint64_t *const counter,
	const uint32_t instance,
	const uint64_t max_ops,
	const char *name,
	int (*linkfunc)(const char *oldpath, const char *newpath),
	const char *funcname)
{
	const pid_t pid = getpid();
	int fd;
	char oldpath[PATH_MAX];

	(void)instance;

	snprintf(oldpath, sizeof(oldpath), "stress-%s-%i", funcname, pid);
	if ((fd = open(oldpath, O_CREAT | O_RDWR, 0666)) < 0) {
		pr_failed_err(name, "open");
		return EXIT_FAILURE;
	}
	(void)close(fd);

	do {
		uint64_t i, n = DEFAULT_LINKS;

		for (i = 0; i < n; i++) {
			char newpath[PATH_MAX];

			snprintf(newpath, sizeof(newpath), "stress-%s-%i-%"
				PRIu64 ".lnk", funcname, pid, i);

			if (linkfunc(oldpath, newpath) < 0) {
				pr_failed_err(name, funcname);
				n = i;
				break;
			}

			if (!opt_do_run ||
			    (max_ops && *counter >= max_ops))
				goto abort;

			(*counter)++;
		}
		stress_link_unlink(funcname, n);
		if (!opt_do_run)
			break;
	} while (opt_do_run && (!max_ops || *counter < max_ops));

abort:
	/* force unlink of all files */
	pr_dbg(stdout, "%s: removing %" PRIu32" entries\n", name, DEFAULT_LINKS);
	stress_link_unlink(funcname, DEFAULT_LINKS);
	(void)unlink(oldpath);

	return EXIT_SUCCESS;
}

/*
 *  stress_link
 *	stress hard links
 */
int stress_link(
	uint64_t *const counter,
	const uint32_t instance,
	const uint64_t max_ops,
	const char *name)
{
	return stress_link_generic(counter, instance,
		max_ops, name, link, "link");
}

/*
 *  stress_symlink
 *	stress symbolic links
 */
int stress_symlink(
	uint64_t *const counter,
	const uint32_t instance,
	const uint64_t max_ops,
	const char *name)
{
	return stress_link_generic(counter, instance,
		max_ops, name, symlink, "symlink");
}