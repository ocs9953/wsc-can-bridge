#include <errno.h>
#include <sys/types.h>

int _close(int fd)                        { (void)fd; errno = EBADF; return -1; }
int _read(int fd, char *p, int n)         { (void)fd; (void)p; (void)n; errno = EBADF; return -1; }
int _write(int fd, const char *p, int n)  { (void)fd; (void)p; (void)n; errno = EBADF; return -1; }
off_t _lseek(int fd, off_t o, int w)      { (void)fd; (void)o; (void)w; errno = EBADF; return -1; }
