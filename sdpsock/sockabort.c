#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
usage()
{

	errx(1, "usage: sockabort <-c <addr> | -s> <port>");
}

int
main(int argc, char **argv)
{
	struct sockaddr_in sin;
	socklen_t slen;
	in_addr_t addr;
	int one, sd, csd, ch, client, server, flags;
	short port;

	client = server = 0;
	memset(&sin, 0, sizeof(sin));
	while ((ch = getopt(argc, argv, "c:s")) != -1) {
		switch (ch) {
		case 'c':
			addr = inet_addr(optarg);
			client = 1;
			break;
		case 's':
			server = 1;
			break;
		default:
			usage();
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0 || (client ^ server) == 0)
		usage();

	port = (short)strtol(argv[0], NULL, 10);

	slen = sizeof(sin);
	sin.sin_family = AF_INET;

	if (client) {
		while (1) {
			sd = socket(AF_INET_SDP, SOCK_STREAM, 0);
			if (sd < 0)
				err(1, "socket");

			one = 1;
			if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one,
			    sizeof(one)) < 0)
				err(1, "setsockopt");

			if ((flags = fcntl(sd, F_GETFL, 0)) < 0)
				err(1, "fcntl(F_GETFL)");
			flags |= O_NONBLOCK;
			if (fcntl(sd, F_SETFL, flags) < 0)
				err(1, "fcntl(F_SETFL)");

			sin.sin_addr.s_addr = INADDR_ANY;
			sin.sin_port = 0;
			if (bind(sd, (struct sockaddr *)&sin, slen) != 0)
				err(1, "bind");

			sin.sin_addr.s_addr = addr;
			sin.sin_port = htons(port);
			if (connect(sd, (struct sockaddr *)&sin, slen) != 0)
				if (errno != EINPROGRESS)
					err(1, "connect");
			if (close(sd) != 0)
				err(1, "close");
		}
	} else /* server */ {
		sd = socket(AF_INET_SDP, SOCK_STREAM, 0);
		if (sd < 0)
			err(1, "socket");

		one = 1;
		if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one,
		    sizeof(one)) < 0)
			err(1, "setsockopt");

		sin.sin_port = htons(port);
		sin.sin_addr.s_addr = INADDR_ANY;
		if (bind(sd, (struct sockaddr *)&sin, slen) != 0)
			err(1, "bind");

		if (listen(sd, 10) != 0)
			err(1, "listen");

		while ((csd = accept(sd, NULL, NULL)) >= 0) {
			printf("Accepting a connection.\n");
			shutdown(csd, SHUT_RDWR);
			close(csd);
		}
		err(1, "accept");
	}
	return (0);
}
