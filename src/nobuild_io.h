#ifndef NOBUILD_IO_H_
#define NOBUILD_IO_H_

#ifndef _WIN32
#    include <sys/types.h>
typedef pid_t Pid;
typedef int Fd;
#else
#    define WIN32_MEAN_AND_LEAN
#    include <windows.h>
typedef HANDLE Pid;
typedef HANDLE Fd;
#endif

#ifndef NOBUILD_PRINTF_FORMAT
#	if defined(__GNUC__) || defined(__clang__)
#		// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#		define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))
#	else
#		define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#	endif
#endif

typedef struct {
    Fd read;
    Fd write;
} Pipe;

Pipe pipe_make(void);

Fd fd_open_for_read(const char *path);
Fd fd_open_for_write(const char *path);
size_t fd_read(Fd fd, void *buf, unsigned long count);
size_t fd_write(Fd fd, void *buf, unsigned long count);
int fd_printf(Fd fd, const char *fmt, ...) NOBUILD_PRINTF_FORMAT(2, 3);
void fd_close(Fd fd);

void pid_wait(Pid pid);

#endif  // NOBUILD_IO_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef NOBUILD_IO_IMPLEMENTATION
#ifndef NOBUILD_IO_I_
#define NOBUILD_IO_I_

#ifndef _WIN32
#	include <sys/wait.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <fcntl.h>

// Avoid requiring the user to define `_POSIX_C_SOURCE` as `200809L`
char *strsignal(int sig);
#else
#	include <assert.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define NOBUILD_LOG_IMPLEMENTATION
#include "nobuild_log.h"

// Multiple modules could define this function, so add a guard around it to prevent redefinition
#if defined(_WIN32) && !defined(NOBUILD__GETLASTERROR)
#define NOBUILD__GETLASTERROR
LPSTR nobuild__GetLastErrorAsString(void)
{
    // https://stackoverflow.com/q/1387064/21582981
    DWORD errorMessageId = GetLastError();
    assert(errorMessageId != 0);

    LPSTR messageBuffer = NULL;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // DWORD   dwFlags,
        NULL, // LPCVOID lpSource,
        errorMessageId, // DWORD   dwMessageId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // DWORD   dwLanguageId,
        (LPSTR) &messageBuffer, // LPTSTR  lpBuffer,
        0, // DWORD   nSize,
        NULL // va_list *Arguments
    );

    return messageBuffer;
}
#endif // NOBUILD__GETLASTERROR

Pipe pipe_make(void)
{
    Pipe pip = {0};

#ifndef _WIN32
    Fd pipefd[2];
    if (pipe(pipefd) < 0) {
        PANIC("Could not create pipe: %s", strerror(errno));
    }

    pip.read = pipefd[0];
    pip.write = pipefd[1];
#else
    // https://docs.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output

    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    if (!CreatePipe(&pip.read, &pip.write, &saAttr, 0)) {
        PANIC("Could not create pipe: %s", nobuild__GetLastErrorAsString());
    }
#endif // _WIN32

    return pip;
}

Fd fd_open_for_read(const char *path)
{
#ifndef _WIN32
    Fd result = open(path, O_RDONLY);
    if (result < 0) {
        PANIC("Could not open file %s: %s", path, strerror(errno));
    }
    return result;
#else
    // https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    Fd result = CreateFile(
                    path,
                    GENERIC_READ,
                    0,
                    &saAttr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_READONLY,
                    NULL);

    if (result == INVALID_HANDLE_VALUE) {
        PANIC("Could not open file %s", path);
    }

    return result;
#endif // _WIN32
}

Fd fd_open_for_write(const char *path)
{
#ifndef _WIN32
    Fd result = open(path,
                     O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (result < 0) {
        PANIC("Could not open file %s: %s", path, strerror(errno));
    }
    return result;
#else
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    Fd result = CreateFile(
                    path,                  // name of the write
                    GENERIC_WRITE,         // open for writing
                    0,                     // do not share
                    &saAttr,               // default security
                    CREATE_NEW,            // create new file only
                    FILE_ATTRIBUTE_NORMAL, // normal file
                    NULL                   // no attr. template
                );

    if (result == INVALID_HANDLE_VALUE) {
        PANIC("Could not open file %s: %s", path, nobuild__GetLastErrorAsString());
    }

    return result;
#endif // _WIN32
}

size_t fd_read(Fd fd, void *buf, unsigned long count)
{
#ifndef _WIN32
    ssize_t bytes = read(fd, buf, count);
    if (bytes == -1) {
        ERRO("Read error: %s", strerror(errno));
        return 0;
    }
#else
    DWORD bytes;
    if (!ReadFile(fd, buf, count, &bytes, NULL)) {
        ERRO("Read error: %s", nobuild__GetLastErrorAsString());
        return 0;
    }
#endif

    return (size_t) bytes;
}

size_t fd_write(Fd fd, void *buf, unsigned long count)
{
#ifndef _WIN32
    ssize_t bytes = read(fd, buf, (size_t) count);
    if (bytes == -1) {
        ERRO("Write error: %s", strerror(errno));
        return 0;
    }
#else
    DWORD bytes;
    if (!WriteFile(fd, buf, count, &bytes, NULL)) {
        ERRO("Write error: %s", nobuild__GetLastErrorAsString());
        return 0;
    }
#endif

    return (size_t) bytes;
}

int fd_printf(Fd fd, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0) {
        return len;
    }

    // MSVC does not support variable length arrays
#ifndef _WIN32
     char buffer[len + 1];
#else
     char *buffer = malloc(sizeof *buffer * (len + 1));
#endif

    va_start(args, fmt);
    int result = vsnprintf(buffer, (size_t)(len + 1), fmt, args);
    va_end(args);
    if (result < 0) {
        return result;
    }

    fd_write(fd, buffer, (unsigned long) result);

#ifdef _WIN32
    free(buffer);
#endif

    return result;
}

void fd_close(Fd fd)
{
#ifndef _WIN32
    close(fd);
#else
    CloseHandle(fd);
#endif // _WIN32
}

void pid_wait(Pid pid)
{
#ifndef _WIN32
    for (;;) {
        int wstatus = 0;
        if (waitpid(pid, &wstatus, 0) < 0) {
            PANIC("Could not wait on command (pid %d): %s", pid, strerror(errno));
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                PANIC("Command exited with exit code %d", exit_status);
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            PANIC("Command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
        }
    }
#else
    DWORD result = WaitForSingleObject(
                       pid,     // HANDLE hHandle,
                       INFINITE // DWORD  dwMilliseconds
                   );

    if (result == WAIT_FAILED) {
        PANIC("Could not wait on child process: %s", nobuild__GetLastErrorAsString());
    }

    DWORD exit_status;
    if (GetExitCodeProcess(pid, &exit_status) == 0) {
        PANIC("Could not get process exit code: %lu", GetLastError());
    }

    if (exit_status != 0) {
        PANIC("Command exited with exit code %lu", exit_status);
    }

    CloseHandle(pid);
#endif // _WIN32
}

#endif // NOBUILD_IO_I_
#endif // NOBUILD_IO_IMPLEMENTATION
