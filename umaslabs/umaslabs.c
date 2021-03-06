/*-
 * Copyright (c) 2017-2018, 2020 Mark Johnston <markj@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/uma.h>
#include <vm/uma_int.h>

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <kvm.h>

static struct nlist namelist[] = {
#define	X_UMA_KEGS	0
	{ .n_name = "_uma_kegs" },
#define	X_VM_PAGE_ARRAY 1
	{ .n_name = "_vm_page_array" },
#define X_VM_PAGE_ARRAY_SIZE 2
	{ .n_name = "_vm_page_array_size" },
	{ .n_name = "" },
};

static int
kread(kvm_t *kvm, void *ptr, void *addr, size_t size)
{
	ssize_t ret;

	ret = kvm_read(kvm, (u_long)(uintptr_t)ptr, addr, size);
	if (ret < 0)
		return (-1);
	if ((size_t)ret != size)
		return (-2);
	return (0);
}

static int
kread_string(kvm_t *kvm, const void *ptr, char *buf, size_t size)
{
	ssize_t ret;
	size_t i;

	for (i = 0; i < size; i++) {
		ret = kvm_read(kvm, (u_long)(uintptr_t)ptr + i, &buf[i],
		    sizeof(char));
		if (ret < 0)
			return (-1);
		if (ret != sizeof(char))
			return (-2);
		if (buf[i] == '\0')
			return (0);
	}
	buf[i - 1] = '\0';
	return (0);
}

static int
kread_symbol(kvm_t *kvm, int index, void *addr, size_t size)
{
	ssize_t ret;

	ret = kvm_read(kvm, namelist[index].n_value, addr, size);
	if (ret < 0)
		return (-1);
	if ((size_t)ret != size)
		return (-2);
	return (0);
}

static void
dump_slabs(kvm_t *kvm, int ppera, struct slabhead *list)
{
	struct uma_slab slab, *slabp;
	int ret;

	for (slabp = LIST_FIRST(list); slabp != NULL;
	    slabp = LIST_NEXT(&slab, us_link)) {
		ret = kread(kvm, slabp, &slab, sizeof(slab));
		if (ret != 0)
			errx(1, "kread: %s", kvm_geterr(kvm));
		printf("%#lx\n",
		    roundup2((uintptr_t)slabp, PAGE_SIZE) - ppera * PAGE_SIZE);
	}
}

static void
usage(void)
{

	errx(1, "usage: -m <zone name>");
}

int
main(int argc, char **argv)
{
	char errbuf[_POSIX2_LINE_MAX], name[128], *match;
	LIST_HEAD(, uma_keg) uma_kegs;
	struct uma_keg *keg, *kegkp;
	struct uma_zone zone, *zonep;
	kvm_t *kvm;
	size_t ksize, sz;
	int ch, count, i, ndomains, ppera, ret;

	match = NULL;
	while ((ch = getopt(argc, argv, "m:")) != -1)
		switch (ch) {
		case 'm':
			match = strdup(optarg);
			break;
		default:
			usage();
			break;
		}

	if (match == NULL)
		usage();

	sz = sizeof(ndomains);
	if (sysctlbyname("vm.ndomains", &ndomains, &sz, NULL, 0) != 0)
		err(1, "sysctl(vm.ndomains)");

	kvm = kvm_openfiles(NULL, NULL, NULL, 0, errbuf);
	if (kvm == NULL)
		errx(1, "kvm_openfiles: %s", errbuf);

	count = kvm_nlist(kvm, namelist);
	if (count == -1)
		errx(1, "kvm_nlist: %s", kvm_geterr(kvm));
	if (count > 0)
		errx(1, "kvm_nlist: failed to resolve symbols");

	ret = kread_symbol(kvm, X_UMA_KEGS, &uma_kegs, sizeof(uma_kegs));
	if (ret != 0)
		errx(1, "kread_symbol: %s", kvm_geterr(kvm));

	ksize = sizeof(*keg) + ndomains * sizeof(keg->uk_domain[0]);
	keg = malloc(ksize);
	if (keg == NULL)
		err(1, "malloc");

	for (kegkp = LIST_FIRST(&uma_kegs); kegkp != NULL;
	    kegkp = LIST_NEXT(keg, uk_link)) {
		ret = kread(kvm, kegkp, keg, ksize);
		if (ret != 0)
			errx(1, "kread: %s", kvm_geterr(kvm));

		for (zonep = LIST_FIRST(&keg->uk_zones); zonep != NULL;
		    zonep = LIST_NEXT(&zone, uz_link)) {
			ret = kread(kvm, zonep, &zone, sizeof(zone));
			if (ret != 0)
				errx(1, "kread: %s", kvm_geterr(kvm));
			ret = kread_string(kvm, (void *)zone.uz_name, name,
			    sizeof(name));
			if (ret != 0)
				errx(1, "kread_string: %s", kvm_geterr(kvm));
			if (strcmp(match, name) == 0)
				break;
		}
		if (zonep == NULL)
			continue;

		if ((keg->uk_flags & UMA_ZFLAG_VTOSLAB) != 0) {
			long size;
			ret = kread_symbol(kvm, X_VM_PAGE_ARRAY_SIZE, &size,
			    sizeof(size));
			if (ret != 0)
				errx(1, "kread_symbol: %s", kvm_geterr(kvm));
			vm_page_t a;
			ret = kread_symbol(kvm, X_VM_PAGE_ARRAY, &a, sizeof(a));
			if (ret != 0)
				errx(1, "kread_symbol: %s", kvm_geterr(kvm));
			vm_page_t arr = malloc(size * sizeof(struct vm_page));
			if (arr == NULL)
				err(1, "malloc");
			ret = kread(kvm, a, arr, size * sizeof(struct vm_page));
			if (ret != 0)
				errx(1, "kread: %s", kvm_geterr(kvm));

			printf("size is %ld\n", size);
			for (long j = 0; j < size; j++) {
				if (VPRC_WIRE_COUNT(arr[j].ref_count) == 1 &&
				    arr[j].plinks.uma.zone == zonep)
					printf("%#lx\n", arr[j].phys_addr);
			}
		} else if ((keg->uk_flags & UMA_ZFLAG_OFFPAGE) == 0) {
			ppera = keg->uk_ppera;
			for (i = 0; i < ndomains; i++)
				dump_slabs(kvm, ppera,
				    &keg->uk_domain[i].ud_part_slab);
			for (i = 0; i < ndomains; i++)
				dump_slabs(kvm, ppera,
				    &keg->uk_domain[i].ud_free_slab);
			for (i = 0; i < ndomains; i++)
				dump_slabs(kvm, ppera,
				    &keg->uk_domain[i].ud_full_slab);
		}
	}

	(void)kvm_close(kvm);
	free(keg);

	return (0);
}
