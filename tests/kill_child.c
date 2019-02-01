/*
 * Check for the corner case that previously lead to segfault
 * due to an attempt to access unitialised tcp->s_ent.
 *
 * 13994 ????( <unfinished ...>
 * ...
 * 13994 <... ???? resumed>) = ?
 *
 * Copyright (c) 2019 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tests.h"

#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define ITERS    10000
#define SC_ITERS 10000

int
main(void)
{
	for (unsigned int i = 0; i < ITERS; i++) {
		int pid = fork();

		if (pid < 0)
			perror_msg_and_fail("fork");

		if (!pid) {
			for (unsigned int j = 0; j < SC_ITERS; j++)
				getuid();

			pause();

			return 0;
		}

		kill(pid, SIGKILL);
		wait(NULL);
	}

	return 0;
}
