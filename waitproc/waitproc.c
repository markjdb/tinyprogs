/*-
 * Copyright (c) 2013 Mark Johnston <markj@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer,
 * without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *progname;

void usage(void);

void
usage()
{

	fprintf(stderr, "usage: %s <PID 1> [ <PID 2> <PID 3> ... ]\n",
	    progname);
	exit(1);
}

int
main(int argc, char * const *argv)
{
	struct kevent *evlist, *chlist;
	unsigned long pid, status;
	int kq, i, j, nev, count;
	char *endptr;

	progname = basename(argv[0]);

	if (argc < 2)
		usage();

	argc--;
	argv++;

	kq = kqueue();
	if (kq < 0)
		err(1, "kqueue()");

	evlist = malloc(argc * sizeof(*evlist));
	chlist = malloc(argc * sizeof(*chlist));
	if (evlist == NULL || chlist == NULL)
		err(1, "malloc()");

	for (count = i = 0; i < argc; i++) {
		errno = 0;
		pid = strtoul(argv[i], &endptr, 10);
		if ((argv[i][0] == '\0' || *endptr != '\0') || errno != 0) {
			warnx("couldn't convert argument '%s' to a PID",
			    argv[i]);
			continue;
		}

		EV_SET(&chlist[i], (pid_t)pid, EVFILT_PROC, EV_ADD | EV_ONESHOT,
		    NOTE_EXIT, i, NULL);
		count++;
	}

	while (count > 0) {
		nev = kevent(kq, chlist, argc, evlist, argc, NULL);
		if (nev < 0)
			err(1, "kevent()");
		else if (nev == 0) {
			warnx("kevent() unexepectedly returned 0");
			continue;
		}

		for (i = 0; i < nev; i++) {
			pid = evlist[i].ident;
			status = evlist[i].data;
			if (evlist[i].flags & EV_ERROR) {
				warnx("EV_ERROR on %lu: %s", pid,
				    strerror(evlist[i].data));
			} else if (WIFEXITED(status)) {
				warnx("PID %lu exited with status %lu",
				    pid, WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				warnx("PID %lu was terminated by signal %lu%s",
				    pid, WTERMSIG(status),
				    WCOREDUMP(status) ? " (core dumped)" : "");
			} else {
				warnx("PID %lu returned %lu", pid, status);
			}
			count--;

			for (j = 0; j < argc; j++) {
				if (pid == chlist[j].ident) {
					EV_SET(&chlist[j], 1, EVFILT_PROC,
					    EV_ADD | EV_DISABLE, 0, 0, 0);
					break;
				}
			}
		}
	}

	free(evlist);
	free(chlist);

	return (0);
}
