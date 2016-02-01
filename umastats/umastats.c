#include <sys/types.h>

#include <err.h>
#include <memstat.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int wflag = 0;

static void
log_stats(void)
{
	struct memory_type_list *mtlp;
	struct memory_type *mtp;
	char name[MEMTYPE_MAXNAME + 1];
	uint64_t pcpuhits, pcpumisses, pcpufree;
	int i;

	mtlp = memstat_mtl_alloc();
	if (mtlp == NULL)
		err(1, "memstat_mtl_alloc");
	if (memstat_sysctl_uma(mtlp, 0) < 0)
		err(1, "memstat_sysctl_uma");

	for (mtp = memstat_mtl_first(mtlp); mtp != NULL;
	    mtp = memstat_mtl_next(mtp)) {
		strlcpy(name, memstat_get_name(mtp), MEMTYPE_MAXNAME);
		pcpuhits = pcpumisses = pcpufree = 0;
		for (i = 0; i < 24; i++) {
			pcpuhits += memstat_get_percpu_hits(mtp, i);
			pcpumisses += memstat_get_percpu_misses(mtp, i);
			pcpufree += memstat_get_percpu_free(mtp, i);
		}
		printf("%s: %ju %ju %ju %ju/%ju %ju/%ju\n", name,
		    memstat_get_numallocs(mtp) - memstat_get_numfrees(mtp),
		    pcpufree,
		    memstat_get_zonefree(mtp),
		    pcpuhits, pcpumisses,
		    memstat_get_zonehits(mtp), memstat_get_zonemisses(mtp));

	}
	memstat_mtl_free(mtlp);
	fflush(stdout);
}

static void
usage(void)
{

	errx(1, "usage: %s [-w]", getprogname());
}

int
main(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "w")) != -1) {
		switch (ch) {
		case 'w':
			wflag = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}

	if (wflag) {
		while (true) {
			log_stats();
			sleep(1);
		}
	} else {
		log_stats();
	}

	return (0);
}
