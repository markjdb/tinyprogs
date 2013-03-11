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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <stdio.h>
#include <fetch.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

FILE	*inputfile(const char *);
void	 usage(int);

void
usage(int code)
{

	fprintf(stderr,
	    "usage: %s [ -u <user> ] [ -p <passwd> ] <local file> <dest>\n\n"
	    "Arguments:\n"
	    "  dest:\t\tan FTP URL\n", basename(getprogname()));
	exit(code);
}

FILE *
inputfile(const char *fname)
{
	struct stat sb;
	FILE *in;

	if (stat(fname, &sb))
		err(1, "%s", fname);
	in = fopen(fname, "r");
	if (in == NULL)
		err(1, "%s", fname);

	return (in);
}

int
main(int argc, char **argv)
{
	struct termios tios;
	tcflag_t saved;
	struct url *url;
	FILE *in, *out;
	const size_t bufsize = 1024;
	char buf[bufsize], *fullpath, *base;
	char *passwd = NULL, *p, *user = NULL, *u;
	size_t n;
	int c;

	if (argc != 3)
		usage(1);

	while ((c = getopt(argc, argv, "hp:u:")) != -1)
		switch (c) {
		case 'h':
			usage(0);
			break;
		case 'p':
			passwd = optarg;
			break;
		case 'u':
			user = optarg;
			break;
		default:
			usage(1);
			break;
		}

	argc -= optind;
	argv += optind;

	in = inputfile(argv[0]);

	fullpath = malloc(strlen(argv[0]) + strlen(argv[1]) + 2);	
	if (fullpath == NULL)
		err(1, "malloc");
	strcpy(fullpath, argv[1]);
	base = fullpath + strlen(argv[1]);
	*base++ = '/';
	strcpy(base, argv[0]);

	fetchTimeout = 0;

	url = fetchParseURL(fullpath);
	if (url == NULL)
		errx(1, "parsing URL: %s", fetchLastErrString);
	
	if (user == NULL) {
		fprintf(stderr, "Username: ");
		if (fgets(url->user, sizeof(url->user), stdin) == NULL)
			err(1, "reading username");
		u = (char *)url->user;
		strsep(&u, "\n\r");
	} else {
		strlcpy(url->user, user, sizeof(url->user));
	}

	if (passwd == NULL) {
		fprintf(stderr, "Password: ");
		if (tcgetattr(STDIN_FILENO, &tios) == 0) {
			saved = tios.c_lflag;
			tios.c_lflag &= ~ECHO;
			tios.c_lflag |= ECHONL | ICANON;
			tcsetattr(STDIN_FILENO, TCSAFLUSH | TCSASOFT, &tios);
			if (fgets(url->pwd, sizeof(url->pwd), stdin) == NULL)
				err(1, "reading password");
			tios.c_lflag = saved;
			tcsetattr(STDIN_FILENO, TCSAFLUSH | TCSASOFT, &tios);
		} else {
			if (fgets(url->pwd, sizeof(url->pwd), stdin) == NULL)
				err(1, "reading password");
		}
		p = (char *)url->pwd;
		strsep(&p, "\n\r");
	} else {
		strlcpy(url->pwd, passwd, sizeof(url->pwd));
	}

	out = fetchPutFTP(url, "");
	if (out == NULL) {
		printf("user: %s, pwd: %s\n", url->user, url->pwd);
		errx(1, "couldn't open a connection to %s: %s", fullpath,
		    fetchLastErrString);
	}

	while ((n = fread(buf, 1, bufsize, in)) > 0) {
		if (fwrite(buf, n, 1, out) != 1)
			err(1, "writing to %s", fullpath);
	}

	fclose(in);
	fclose(out);
	fetchFreeURL(url);

	return (0);
}
