/*
 * Check tracing of orphaned process group.
 *
 * Copyright (c) 2019 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tests.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static siginfo_t sinfo;

static void
handler(const int no, siginfo_t *const si, void *const uc)
{
	memcpy(&sinfo, si, sizeof(sinfo));
}

int
main(void)
{
	/*
	 * Unblock all signals.
	 */
	static sigset_t mask;
	if (sigprocmask(SIG_SETMASK, &mask, NULL))
		perror_msg_and_fail("sigprocmask");

	/*
	 * Install a SIGCHLD signal handler to copy siginfo_t
	 * for printing expected output.
	 */
	static const struct sigaction sa = {
		.sa_sigaction = handler,
		.sa_flags = SA_SIGINFO
	};
	if (sigaction(SIGCHLD, &sa, NULL))
		perror_msg_and_fail("sigaction SIGCHLD");

	/*
	 * Reset the SIGHUP signal handler.
	 */
	static const struct sigaction sa_def = { .sa_handler = SIG_DFL };
	if (sigaction(SIGHUP, &sa_def, NULL))
		perror_msg_and_fail("sigaction SIGHUP");

	/* Create a new process group.  */
	if (setpgid(0, 0))
		perror_msg_and_fail("setpgid");

	/*
	 * When the main process terminates, the process group becomes orphaned.
	 * If any member of the orphaned process group is stopped, then
	 * a SIGHUP signal followed by a SIGCONT signal is sent to each process
	 * in the orphaned process group.
	 * Create a process in a stopped state to activate this behaviour.
	 */
	const pid_t stopped = fork();
	if (stopped < 0)
		perror_msg_and_fail("fork");
	if (!stopped) {
		raise(SIGSTOP);
		_exit(0);
	}

	/*
	 * Wait for the process to stop.
	 */
	int status;
	if (waitpid(stopped, &status, WUNTRACED) != stopped)
		perror_msg_and_fail("waitpid WUNTRACED");
	if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGSTOP)
                error_msg_and_fail("unexpected wait status %d", status);

	/*
	 * Print expected output.
	 */
	const pid_t leader = getpid();
	const unsigned int uid = geteuid();
	printf("%-5d --- %s {si_signo=%s, si_code=SI_TKILL, si_pid=%d, si_uid=%u} ---\n",
	       stopped, "SIGSTOP", "SIGSTOP", stopped, uid);
	printf("%-5d --- stopped by SIGSTOP ---\n", stopped);
	printf("%-5d --- %s {si_signo=%s, si_code=CLD_STOPPED, si_pid=%d"
               ", si_uid=%u, si_status=%s, si_utime=%u, si_stime=%u} ---\n",
	       leader, "SIGCHLD", "SIGCHLD", (int) sinfo.si_pid,
	       (unsigned int) sinfo.si_uid, "SIGSTOP",
               (unsigned int) sinfo.si_utime, (unsigned int) sinfo.si_stime);
	printf("%-5d +++ exited with 0 +++\n", leader);
	printf("%-5d --- %s {si_signo=%s, si_code=SI_KERNEL} ---\n",
	       stopped, "SIGHUP", "SIGHUP");
	printf("%-5d +++ killed by %s +++\n", stopped, "SIGHUP");

	/*
	 * Make the process group orphaned.
	 */
	return 0;
}
