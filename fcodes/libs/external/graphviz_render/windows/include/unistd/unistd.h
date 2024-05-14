#ifndef _UNISTD_H
#define _UNISTD_H    1

/* This file intended to serve as a drop-in replacement for 
 * unistd.h on Windows
 */

#include <stdlib.h>
#include <io.h>
#include <process.h> /* for getpid() and the exec..() family */
#include <direct.h> /* for _getcwd() */
#include <BaseTsd.h>
#include <sys/stat.h>

/* Values for the second argument to access.
   These may be OR'd together.  */
#define R_OK    4       /* Test for read permission.  */
#define W_OK    2       /* Test for write permission.  */
#define X_OK    R_OK    /* execute permission - unsupported in windows*/
#define F_OK    0       /* Test for existence.  */

#define S_ISDIR(mode) (((mode) & _S_IFDIR) == _S_IFDIR)

#define access _access
#define ftruncate _chsize
#define fileno _fileno
#define getcwd _getcwd
#define isatty _isatty
#define lseek _lseek
/* read, write, and close are NOT being #defined here, because while there are file handle specific versions for Windows, they probably don't work for sockets. You need to look at your app and consider whether to call e.g. closesocket(). */

#define ssize_t SSIZE_T

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#endif /* unistd.h  */
