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
#include <fcntl.h>
#include <gelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	DOF_ID_SIZE	16

struct dof_hdr {
	uint8_t dofh_ident[DOF_ID_SIZE];
	uint32_t dofh_flags;
	uint32_t dofh_hdrsize;
	uint32_t dofh_secsize;
	uint32_t dofh_secnum;
	uint64_t dofh_secoff;
	uint64_t dofh_loadsz;
	uint64_t dofh_filesz;
	uint64_t dofh_pad;
};

struct dof_sec {
	uint32_t dofs_type;
	uint32_t dofs_align;
	uint32_t dofs_flags;
	uint32_t dofs_entsize;
	uint64_t dofs_offset;
	uint64_t dofs_size;
};

static void
usage(void)
{

	fprintf(stderr, "usage: %s <file>\n", getprogname());
	exit(1);
}

const char *
modelstr(uint32_t b)
{

	return (b == 0 ? "NONE" : b == 1 ? "ILP32" : b == 2 ? "LP64" : "??");
}

const char *
encstr(uint32_t b)
{

	return (b == 0 ? "NONE" : b == 1 ? "LSB" : b == 2 ? "MSB" : "??");
}

int
main(int argc, char **argv)
{
	Elf *e;
	int fd, i;
	Elf_Scn *scn;
	Elf_Data *data;
	char *name;
	size_t shstrndx;
	GElf_Shdr shdr;
	struct dof_hdr *hdr;
	struct dof_sec *dsec;

	if (argc != 2)
		usage();

	if (elf_version(EV_CURRENT) == EV_NONE)
		errx(1, "ELF library initialization failed: %s",
		    elf_errmsg(-1));

	if ((fd = open(argv[1], O_RDONLY)) < 0)
		err(1, "opening %s", argv[1]);

	if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
		errx(1, "elf_begin() failed: %s", elf_errmsg(-1));

	if (elf_kind(e) != ELF_K_ELF)
		errx(1, "%s is not an ELF object", argv[1]);

	if (elf_getshdrstrndx(e, &shstrndx) != 0)
		errx(1, "elf_getshdrstrndx failed: %s", elf_errmsg(-1));

	scn = NULL;
	while ((scn = elf_nextscn(e, scn)) != NULL) {
		if (gelf_getshdr(scn, &shdr) != &shdr)
			errx(1, "gelf_getshdr failed: %s", elf_errmsg(-1));
		if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL)
			errx(1, "elf_strptr() failed: %s", elf_errmsg(-1));

		if (shdr.sh_type == SHT_SUNW_dof &&
		    strcmp(name, ".SUNW_dof") == 0)
			break;
	}

	if (scn == NULL)
		errx(1, "no DOF section found in %s", argv[1]);

	if ((data = elf_getdata(scn, NULL)) == NULL)
		errx(1, "couldn't obtain data descriptor for .SUNW_dof: %s",
		    elf_errmsg(-1));

	hdr = (struct dof_hdr *)data->d_buf;
	if (data->d_size < sizeof(*hdr))
		errx(1, "data buffer is smaller than DOF header");
	else if (memcmp(hdr->dofh_ident, "\177DOF", 4) != 0)
		errx(1, "DOF header is invalid or corrupt");

	/* Dump the DOF header. */
	printf("DOF Header:\n");
	printf("    DOF data model: %s\n", modelstr(hdr->dofh_ident[4]));
	printf("    DOF encoding: %s\n", encstr(hdr->dofh_ident[5]));
	printf("    DOF format version: %u\n", hdr->dofh_ident[6]);
	printf("    DIF instruction set version: %u\n", hdr->dofh_ident[7]);
	printf("    DIF integer register count: %u\n", hdr->dofh_ident[8]);
	printf("    DIF tuple register count: %u\n", hdr->dofh_ident[9]);
	printf("\n");

	/* Iterate over the section headers. */
	for (i = 0; i < hdr->dofh_secnum; i++) {
		dsec = (struct dof_sec *)((uint8_t *)data->d_buf +
		    hdr->dofh_hdrsize + i * hdr->dofh_secsize);

		printf("DOF Section:\n");
		printf("    Section type: %u\n", dsec->dofs_type);
		printf("    Section data size: %lu\n", dsec->dofs_size);
	}

	(void)elf_end(e);
	(void)close(fd);

	return (0);
}
