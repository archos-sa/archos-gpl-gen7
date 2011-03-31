#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/*
	Warning:
	This wrapper is really smbd specific. The smbd-process allways uses complete
	paths for it mkdir/rmdir/unlink/open calls. Therefor it generates useable information
	for us. If it would use relative path components we had nothing to work with.
*/

static void *handle = NULL;
static int(*mkdir_f)(const char*, mode_t) = NULL;
static int(*rmdir_f)(const char*);
static int(*unlink_f)(const char*);
static int(*open_f)(const char *, int, ...);
static int(*creat_f)(const char *, mode_t);
static int(*rename_f)(const char *, const char *);
static int log_fd = -1;
static FILE *flog = NULL;
static char *buf = NULL;

void logdir(FILE* flog, const char*pathname, int ret)
{
	// a share seems to be chrooted the path strings i see
	// start all as relative path inside the share.
	if ( pathname[0] == '/' || ret == -1 || flog == NULL ) {
		return;
	}

	int path_len = strlen(pathname);
	buf = malloc(path_len + 1 + 1);
	if ( !buf ) {
		return;
	}

	buf[0] = '/';
	strncpy(buf + 1, pathname, path_len);
	path_len++;

	// directories can have an optional / at the end. strip it
	if ( buf[path_len - 1] == '/' ) {
		buf[path_len - 1] = '\0';
		path_len--;
	}

	// strip the last path component. we are only interested
	// in the parent hosting a modification.
	char* cursor = buf + path_len - 1;
	while ( *cursor != '/' ) {
		cursor--;
	}
	*(cursor + 1) = '\0';
	fprintf(flog, "%s\n", buf);
	free(buf);
	fflush(flog);
}

static int prepare(void)
{
	if ( !handle ) {
		handle = dlopen("/lib/libc.so.0", RTLD_LAZY);
	}

	if ( !handle ) {
		printf("dlopen failed\n");
		// set errno?
		return -1;
	}

	// prevent an endless loop
	if ( !open_f ) {
		dlerror();
		open_f = dlsym(handle, "open");
		char *error;
		if ((error = dlerror()) != NULL) {
			printf("dlsym failed %s\n", error);
			// set errno?
			return -1;
        	}
	}

	if ( log_fd == -1 ) {
		log_fd = open_f("/tmp/samba_write.log", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP );
		if ( log_fd == -1 ) {
			printf("opening /tmp/samba_write.log failed!\n");
		}
		else {
			flog = fdopen(log_fd, "a");
		}
	}
	return 0;
}

int mkdir(const char *pathname, mode_t mode)
{
	if ( prepare() ) {
		return -1;
	}

	if ( !mkdir_f ) {
		dlerror();
		mkdir_f = dlsym(handle, "mkdir");
		char *error;
		if ((error = dlerror()) != NULL)  {
			printf("dlsym failed %s\n", error);
			// set errno?
			return -1;
        	}
	}

	int ret = mkdir_f(pathname, mode);
	logdir(flog, pathname, ret);
	return ret;
}

int rename(const char *oldpath, const char *newpath)
{
	if ( prepare() ) {
		return -1;
	}

	if ( !rename_f ) {
		dlerror();
		rename_f = dlsym(handle, "rename");
		char *error;
		if ((error = dlerror()) != NULL)  {
			printf("dlsym failed %s\n", error);
			// set errno?
			return -1;
        	}
	}

	int ret = rename_f(oldpath, newpath);
	logdir(flog, oldpath, ret);
	logdir(flog, newpath, ret);
	return ret;
}

int rmdir(const char *pathname)
{
	if ( prepare() ) {
		return -1;
	}

	if ( !rmdir_f ) {
		dlerror();
		rmdir_f = dlsym(handle, "rmdir");
		char *error;
		if ((error = dlerror()) != NULL)  {
			printf("dlsym failed %s\n", error);
			// set errno?
			return -1;
        	}
	}

	int ret = rmdir_f(pathname);
	logdir(flog, pathname, ret);
	return ret;
}

int unlink(const char *pathname)
{
	if ( prepare() ) {
		return -1;
	}

	if ( !unlink_f ) {
		dlerror();
		unlink_f = dlsym(handle, "unlink");
		char *error;
		if ((error = dlerror()) != NULL)  {
			printf("dlsym failed %s\n", error);
			// set errno?
			return -1;
        	}
	}

	int ret = unlink_f(pathname);
	logdir(flog, pathname, ret);
	return ret;
}

int open64(const char *pathname, int flags, ...)
{
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode_t mode = va_arg(ap, mode_t);
		va_end(ap);
		return open(pathname, flags, mode);
	}
	else {
		return open(pathname, flags);
	}
}

int open(const char *pathname, int flags, ...)
{
	if ( prepare() ) {
		return -1;
	}

	if ( !open_f ) {
		dlerror();
		open_f = dlsym(handle, "open");
		char *error;
		if ((error = dlerror()) != NULL)  {
			printf("dlsym failed %s\n", error);
			// set errno?
			return -1;
        	}
	}

	int ret;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode_t mode = va_arg(ap, mode_t);
		va_end(ap);
		ret = open_f(pathname, flags, mode);
	}
	else {
		ret = open_f(pathname, flags);
	}

	int modify = (flags & O_RDWR) || (flags & O_WRONLY);

	if ( modify ) {
		logdir(flog, pathname, ret);
	}
	return ret;
}

int creat(const char *pathname, mode_t mode)
{
	if ( prepare() ) {
		return -1;
	}

	if ( !creat_f ) {
		dlerror();
		creat_f = dlsym(handle, "creat");
		char *error;
		if ((error = dlerror()) != NULL)  {
			printf("dlsym failed %s\n", error);
			// set errno?
			return -1;
        	}
	}

	int ret = creat_f(pathname, mode);
	logdir(flog, pathname, ret);
	return ret;
}
