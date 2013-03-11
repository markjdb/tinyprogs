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

#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <libutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *prefixes[] = { "B", "KB", "MB", "GB", "TB" };
const char *progname;

void	usage(void);

void __dead2
usage()
{

	fprintf(stderr,
	    "Usage: %s [-p] mem-size\n\n"
	    "Arguments:\n"
	    "  -p\t\tTreat mem-size as a page count (i.e. multiply by 4096).\n",
	    basename(progname));
	exit(1);
}

int
main(int argc, char **argv)
{
	unsigned long in;
	char *endptr;
	char buf[32];
	int i, numprfx;

	progname = argv[0];

	if (argc != 2 && argc != 3)
		usage();

	in = strtoul(argv[argc - 1], &endptr, 10);
	if (argv[argc - 1][0] == '\0' || *endptr != '\0')
		usage();

	if (argc == 3) {
		if (strcmp(argv[1], "-p") == 0) {
			in *= 4096;
		} else {
			usage();
		}
	}

	numprfx = sizeof(prefixes) / sizeof(*prefixes);
	for (i = 0; i < numprfx; i++) {
		if (in < 1024 || i == numprfx - 1) {
			snprintf(buf, sizeof(buf), "%lu%s", in, prefixes[i]);
			break;
		}
		in /= 1024;
	}

	printf("%s\n", buf);

	return (0);
}
