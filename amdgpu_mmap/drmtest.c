#include <sys/mman.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libdrm/drm.h>
#include <libdrm/amdgpu_drm.h>

int
main(int argc, char **argv)
{
	union drm_amdgpu_gem_create create;
	union drm_amdgpu_gem_mmap arg;
	drm_version_t version;
	void *addr;
	char name[16];
	int fd, handle;

	fd = open("/dev/dri/card0", O_RDWR);
	if (fd < 0)
		err(1, "open(/dev/dri/card0)");

	memset(&version, 0, sizeof(version));
	name[15] = '\0';
	version.name = name;
	version.name_len = sizeof(name) - 1;
	if (ioctl(fd, DRM_IOCTL_VERSION, &version) != 0)
		err(1, "ioctl(DRM_IOCTL_VERSION)");

	if (strcmp(name, "amdgpu") != 0)
		errx(1, "unknown driver: %s", name);

	memset(&create, 0, sizeof(create));
	create.in.bo_size = 4096;
	if (ioctl(fd, DRM_IOCTL_AMDGPU_GEM_CREATE, &create) != 0)
		err(1, "ioctl(DRM_IOCTL_AMDGPU_GEM_CREATE)");
	handle = create.out.handle;

	memset(&arg, 0, sizeof(arg));
	arg.in.handle = handle;
	if (ioctl(fd, DRM_IOCTL_AMDGPU_GEM_MMAP, &arg) != 0)
		err(1, "ioctl(DRM_IOCTL_AMDGPU_GEM_MMAP)");

	addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
	    arg.out.addr_ptr);
	if (addr == MAP_FAILED)
		err(1, "mmap");

	memset(addr, 0, 4096);

#if 0
	if (munmap(addr, 4096) != 0)
		err(1, "munmap");
#endif
	if (close(fd) != 0)
		err(1, "close");

	return (0);
}
