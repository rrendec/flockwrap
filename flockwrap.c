#define _GNU_SOURCE

#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>

static void __attribute__((format(printf, 1, 2))) dbg(const char *fmt, ...)
{
	static int enabled;

	va_list(ap);

	if (!enabled) {
		char *env = getenv("FLOCKWRAP_DEBUG");
		enabled = env && strspn(env, "1yt") > 0 ? 'y' : 'n';
	}

	if (enabled != 'y')
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static int wrap_open_call(int fd, int flags)
{
	char *prefix;
	char link[40];
	char path[PATH_MAX];
	int plen, xlen;
	struct stat sb;

	if (fd < 0)
		return fd;

	prefix = getenv("FLOCKWRAP_PREFIX");
	if (!prefix)
		return fd;

	if (fstat(fd, &sb) == 0 && (sb.st_mode & S_IFMT) != S_IFREG)
		return fd;

	plen = snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
	if (plen >= sizeof(link))
		return fd;

	plen = readlink(link, path, sizeof(path));
	if (plen < 0 || plen >= sizeof(path))
		return fd;

	xlen = strlen(prefix);
	if (plen >= xlen && strncmp(path, prefix, xlen) == 0) {
		flock(fd, flags & O_WRONLY || flags & O_RDWR ?
			LOCK_EX : LOCK_SH);
		dbg("# wrap(lock) %.*s\n", plen, path);
	} else
		dbg("# wrap(pass) %.*s\n", plen, path);

	return fd;
}

#define REAL(name) __real_##name

#define DEFINE_WRAPPED_SYMBOL(name)						\
	static typeof(&name) REAL(name)

#define INIT_WRAPPED_SYMBOL(name) 						\
	do {									\
		if (!REAL(name)) {						\
			REAL(name) = dlsym(RTLD_NEXT, #name);			\
			assert(REAL(name));					\
		}								\
	} while (0)

int open(const char *path, int flags, ...)
{
	DEFINE_WRAPPED_SYMBOL(open);

	mode_t mode;
	va_list ap;
	int fd;

	INIT_WRAPPED_SYMBOL(open);

	va_start(ap, flags);
	if (__OPEN_NEEDS_MODE(flags)) {
		mode = va_arg(ap, mode_t);
		fd = REAL(open)(path, flags, mode);
	} else
		fd = REAL(open)(path, flags);
	va_end(ap);

	return wrap_open_call(fd, flags);
}

int openat(int dirfd, const char *path, int flags, ...)
{
	DEFINE_WRAPPED_SYMBOL(openat);

	mode_t mode;
	va_list ap;
	int fd;

	INIT_WRAPPED_SYMBOL(openat);

	va_start(ap, flags);
	if (__OPEN_NEEDS_MODE(flags)) {
		mode = va_arg(ap, mode_t);
		fd = REAL(openat)(dirfd, path, flags, mode);
	} else
		fd = REAL(openat)(dirfd, path, flags);
	va_end(ap);

	return wrap_open_call(fd, flags);
}

/*
 * For fortified code (i.e. -D_FORTIFY_SOURCE=1 or larger), /usr/include/fcntl.h
 * includes <bits/fcntl2.h> at the end. In that case, open() and openat() become
 * static inline functions that call a different variant of the glibc function.
 *
 * These variants must be wrapped as well to intercept the corresponding library
 * calls for fortified code.
 *
 * Note: _FORTIFY_SOURCE is "translated" into __USE_FORTIFY_LEVEL in
 *       /usr/include/features.h.
 */

int __open_2(const char *path, int flags)
{
	DEFINE_WRAPPED_SYMBOL(__open_2);
	INIT_WRAPPED_SYMBOL(__open_2);
	return wrap_open_call(REAL(__open_2)(path, flags), flags);
}

int __openat_2(int dirfd, const char *path, int flags)
{
	DEFINE_WRAPPED_SYMBOL(__openat_2);
	INIT_WRAPPED_SYMBOL(__openat_2);
	return wrap_open_call(REAL(__openat_2)(dirfd, path, flags), flags);
}
