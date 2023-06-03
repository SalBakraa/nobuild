#ifndef NOBUILD_CMD_H_
#define NOBUILD_CMD_H_

#ifndef _WIN32
#	include <sys/types.h>
typedef pid_t Pid;
typedef int Fd;
#else
#	define WIN32_MEAN_AND_LEAN
#	include <windows.h>
typedef HANDLE Pid;
typedef HANDLE Fd;
#endif


#include <stddef.h>

#ifndef NOBUILD__DEPRECATED
#	if defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER))
#		define NOBUILD__DEPRECATED(func) __attribute__ ((deprecated)) func
#	elif defined(_MSC_VER)
#		define NOBUILD__DEPRECATED(func) __declspec (deprecated) func
#	endif
#endif

typedef const char * Cstr;

int cstr_ends_with(Cstr cstr, Cstr postfix);
#define ENDS_WITH(cstr, postfix) cstr_ends_with(cstr, postfix)

int cstr_starts_with(Cstr cstr, Cstr prefix);
#define STARTS_WITH(cstr, prefix) cstr_starts_with(cstr, prefix)

typedef struct {
    Cstr *elems;
    size_t count;
    size_t capacity;
} Cstr_Array;

Cstr_Array cstr_array_make(Cstr first, ...);
#define CSTR_ARRAY_MAKE(first, ...) cstr_array_make(first, ##__VA_ARGS__, NULL)

Cstr_Array cstr_array_append(Cstr_Array cstrs, Cstr cstr);

Cstr_Array cstr_array_remove(Cstr_Array cstrs, Cstr cstr);

Cstr_Array cstr_array_concat(Cstr_Array cstrs_a, Cstr_Array cstrs_b);

int cstr_array_contains(Cstr_Array cstrs, Cstr cstr);

Cstr_Array cstr_array_from_cstr(Cstr cstr, Cstr delim);
#define SPLIT(cstr, delim) cstr_array_from_cstr(cstr, delim)

Cstr cstr_array_join(Cstr sep, Cstr_Array cstrs);
#define JOIN(sep, ...) cstr_array_join(sep, cstr_array_make(__VA_ARGS__, NULL))
#define CONCAT(...) JOIN("", __VA_ARGS__)


////////////////////////////////////////////////////////////////////////////////


typedef struct {
    Cstr_Array line;
} Cmd;

Cstr cmd_show(Cmd cmd);
Pid cmd_run_async(Cmd cmd, Fd *fdin, Fd *fdout);
void cmd_run_sync(Cmd cmd);

// TODO(#1): no way to disable echo in nobuild scripts
// TODO(#2): no way to ignore fails
#define CMD(...)                                        \
    do {                                                \
        Cmd cmd = {                                     \
            .line = cstr_array_make(__VA_ARGS__, NULL)  \
        };                                              \
        INFO("CMD: %s", cmd_show(cmd));                 \
        cmd_run_sync(cmd);                              \
    } while (0)

typedef struct {
    Cmd *elems;
    size_t count;
} Cmd_Array;

typedef enum {
    CHAIN_TOKEN_END = 0,
    CHAIN_TOKEN_IN,
    CHAIN_TOKEN_OUT,
    CHAIN_TOKEN_CMD
} Chain_Token_Type;

// A single token for the CHAIN(...) DSL syntax
typedef struct {
    Chain_Token_Type type;
    Cstr_Array args;
} Chain_Token;

#define CHAIN_IN(path)                      \
    (Chain_Token) {                         \
        .type = CHAIN_TOKEN_IN,             \
        .args = cstr_array_make(path, NULL) \
    }

#define CHAIN_OUT(path)                     \
    (Chain_Token) {                         \
        .type = CHAIN_TOKEN_OUT,            \
        .args = cstr_array_make(path, NULL) \
    }

#define CHAIN_CMD(...)                             \
    (Chain_Token) {                                \
        .type = CHAIN_TOKEN_CMD,                   \
        .args = cstr_array_make(__VA_ARGS__, NULL) \
    }

// TODO(#20): pipes do not allow redirecting stderr
typedef struct {
    Cstr input_filepath;
    Cmd_Array cmds;
    Cstr output_filepath;
} Chain;

Chain chain_build_from_tokens(Chain_Token first, ...);
void chain_run_sync(Chain chain);
void chain_echo(Chain chain);

// TODO(#15): PIPE does not report where exactly a syntactic error has happened
#define CHAIN(...)                                                             \
    do {                                                                       \
        Chain chain = chain_build_from_tokens(__VA_ARGS__, (Chain_Token) {0}); \
        chain_echo(chain);                                                     \
        chain_run_sync(chain);                                                 \
    } while(0)

#endif  // NOBUILD_CMD_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef NOBUILD_CMD_IMPLEMENTATION
#ifndef NOBUILD_CMD_I_
#define NOBUILD_CMD_I_

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>


////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


#include <stdio.h>
#include <stdarg.h>

#ifndef NOBUILD_PRINTF_FORMAT
#	if defined(__GNUC__) || defined(__clang__)
#		// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#		define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))
#	else
#		define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#	endif
#endif

#ifndef NOBUILD__DEPRECATED
#	if defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER))
#		define NOBUILD__DEPRECATED(func) __attribute__ ((deprecated)) func
#	elif defined(_MSC_VER)
#		define NOBUILD__DEPRECATED(func) __declspec (deprecated) func
#	endif
#endif

NOBUILD__DEPRECATED(void VLOG(FILE *stream, const char *tag, const char *fmt, va_list args));

void info(const char *fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
#define INFO(fmt, ...) info("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

void warn(const char *fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
#define WARN(fmt, ...) warn("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

void erro(const char *fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
#define ERRO(fmt, ...) erro("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

void panic(const char *fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
#define PANIC(fmt, ...) panic("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

void todo(const char *fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
#define TODO(fmt, ...) todo("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

void todo_safe(const char *fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
#define TODO_SAFE(fmt, ...) todo_safe("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)


////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>

void nobuild__vlog(FILE *stream, const char *tag, const char *fmt, va_list args)
{
    fprintf(stream, "[%s] ", tag);
    vfprintf(stream, fmt, args);
    fprintf(stream, "\n");
}

void VLOG(FILE *stream, const char *tag, const char *fmt, va_list args)
{
    WARN("This function is deprecated.");
    nobuild__vlog(stream, tag, fmt, args);
}

void info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nobuild__vlog(stderr, "INFO", fmt, args);
    va_end(args);
}

void warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nobuild__vlog(stderr, "WARN", fmt, args);
    va_end(args);
}

void erro(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nobuild__vlog(stderr, "ERRO", fmt, args);
    va_end(args);
}

void panic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nobuild__vlog(stderr, "ERRO", fmt, args);
    va_end(args);
    exit(1);
}

void todo(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nobuild__vlog(stderr, "TODO", fmt, args);
    va_end(args);
    exit(1);
}

void todo_safe(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nobuild__vlog(stderr, "TODO", fmt, args);
    va_end(args);
}


// Multiple modules could define this function, so add a guard around it to prevent redefinition
#ifndef NOBUILD__STRERROR
#define NOBUILD__STRERROR
Cstr nobuild__strerror(int errnum)
{
#ifndef _WIN32
    return strerror(errnum);
#else
    static char buffer[1024];
    strerror_s(buffer, 1024, errnum);
    return buffer;
#endif
}
#endif // NOBUILD__STRERROR

int cstr_ends_with(Cstr cstr, Cstr postfix)
{
    const size_t cstr_len = strlen(cstr);
    const size_t postfix_len = strlen(postfix);
    return postfix_len <= cstr_len
           && strcmp(cstr + cstr_len - postfix_len, postfix) == 0;
}

int cstr_starts_with(Cstr cstr, Cstr prefix)
{
    const size_t cstr_len = strlen(cstr);
    const size_t prefix_len = strlen(prefix);
    return prefix_len <= cstr_len && strncmp(cstr, prefix, prefix_len) == 0;
}

Cstr_Array cstr_array_make(Cstr first, ...)
{
    Cstr_Array result = {0};

    if (first == NULL) {
        return result;
    }
    result.count += 1;

    va_list args;
    va_start(args, first);
    for (Cstr next = va_arg(args, Cstr);
            next != NULL;
            next = va_arg(args, Cstr)) {
        result.count += 1;
    }
    va_end(args);

    result.elems = malloc(sizeof *result.elems * result.count);
    if (result.elems == NULL) {
        PANIC("could not allocate memory: %s", nobuild__strerror(errno));
    }

    result.count = 0;
    result.elems[result.count++] = first;

    va_start(args, first);
    for (Cstr next = va_arg(args, Cstr);
            next != NULL;
            next = va_arg(args, Cstr)) {
        result.elems[result.count++] = next;
    }
    va_end(args);

    return result;
}

Cstr_Array cstr_array_append(Cstr_Array cstrs, Cstr cstr)
{
    if (cstrs.capacity < 1) {
        cstrs.elems = realloc(cstrs.elems, sizeof *cstrs.elems * (cstrs.count + 10));
        cstrs.capacity += 10;
        if (cstrs.elems == NULL) {
            PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
        }
    }

    cstrs.elems[cstrs.count++] = cstr;
    cstrs.capacity--;
    return cstrs;
}


Cstr_Array cstr_array_remove(Cstr_Array cstrs, Cstr cstr)
{
    if (cstrs.count == 0) {
        return cstrs;
    }

    if (cstr == NULL) {
        cstrs.elems[--cstrs.count];
        cstrs.capacity++;
        return cstrs;
    }

    // Find the index of the element to be removed
    const size_t cstr_len = strlen(cstr);
    for (size_t i = 0; i < cstrs.count; i++) {
        const size_t elem_len = strlen(cstrs.elems[i]);
        if (elem_len != cstr_len || strcmp(cstrs.elems[i], cstr) != 0) {
            continue;
        }

        // Shift elements left if found the cstr
        for (size_t j = i; j < cstrs.count - 1; j++) {
            cstrs.elems[j] = cstrs.elems[j + 1];
        }
        cstrs.count--;
        cstrs.capacity++;

        // TODO: Might want to realloc array if capacity is too high
        return cstrs;
    }

    // The string was not found
    return cstrs;
}

Cstr_Array cstr_array_concat(Cstr_Array cstrs_a, Cstr_Array cstrs_b)
{
    if (cstrs_a.capacity < cstrs_b.count) {
        cstrs_a.elems = realloc(cstrs_a.elems, sizeof *cstrs_a.elems * (cstrs_a.count + cstrs_b.count));
        cstrs_a.capacity += cstrs_b.count;
        if (cstrs_a.elems == NULL) {
            PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
        }
    }

    memcpy(cstrs_a.elems + cstrs_a.count, cstrs_b.elems, sizeof *cstrs_a.elems * cstrs_b.count);
    cstrs_a.count += cstrs_b.count;
    cstrs_a.capacity -= cstrs_b.count;
    return cstrs_a;
}

int cstr_array_contains(Cstr_Array cstrs, Cstr cstr) {
    for (size_t i = 0; i < cstrs.count; ++i) {
        if (strcmp(cstr, cstrs.elems[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

Cstr_Array cstr_array_from_cstr(Cstr cstr, Cstr delim)
{
    size_t len = strlen(cstr);
    size_t d_len = strlen(delim);
    size_t substr_count = 1;
    for (size_t i = 0; i < len; ++i) {
        if ((len - i) < d_len) {
            break;
        }

        size_t delim_found = 0;
        for (size_t j = 0; j < d_len; ++j) {
            if (cstr[i+j] != delim[j]) {
                delim_found = 0;
                break;
            }
            delim_found = 1;
        }

        if (delim_found) {
            substr_count++;
            i += d_len - 1;
        }
    }

    // if dlen == 0 or was never found
    if (substr_count == 1) {
        // TODO: differentiate between delim == null and delim == "" and delim not found
        //       Split the string into an array of strings, where each string is a single character
        return cstr_array_make(cstr);
    }

    Cstr_Array ret = { .count = substr_count };
    ret.elems = malloc(sizeof(Cstr) * ret.count);
    if (ret.elems == NULL) {
        PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
    }

    size_t substr_start = 0;
    size_t substr_index = 0;
    for (size_t i = 0; i < len; ++i) {
        if ((len - i) < d_len) {
            break;
        }

        size_t delim_found = 0;
        for (size_t j = 0; j < d_len; ++j) {
            if (cstr[i+j] != delim[j]) {
                delim_found = 0;
                break;
            }
            delim_found = 1;
        }

        if (!delim_found) {
            continue;
        }

        size_t substr_len = i - substr_start;
        char *substr = calloc(substr_len + 1, sizeof(unsigned char));
        if (substr == NULL) {
            PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
        }

        ret.elems[substr_index++] = memcpy(substr, (cstr+substr_start), substr_len * sizeof(unsigned char));
        i += d_len - 1;
        substr_start = i + 1;
    }

    // Add the last substring
    size_t substr_len = len - substr_start;
    char *substr = malloc(substr_len * sizeof(unsigned char));
    if (substr == NULL) {
        PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
    }

    ret.elems[substr_index++] = memcpy(substr, (cstr+substr_start), substr_len * sizeof(unsigned char));
    return ret;
}

Cstr cstr_array_join(Cstr sep, Cstr_Array cstrs)
{
    if (cstrs.count == 0) {
        return "";
    }

    const size_t sep_len = strlen(sep);
    size_t len = 0;
    for (size_t i = 0; i < cstrs.count; ++i) {
        len += strlen(cstrs.elems[i]);
    }

    const size_t result_len = (cstrs.count - 1) * sep_len + len + 1;
    char *result = malloc(sizeof(char) * result_len);
    if (result == NULL) {
        PANIC("could not allocate memory: %s", nobuild__strerror(errno));
    }

    len = 0;
    for (size_t i = 0; i < cstrs.count; ++i) {
        if (i > 0) {
            memcpy(result + len, sep, sep_len);
            len += sep_len;
        }

        size_t elem_len = strlen(cstrs.elems[i]);
        memcpy(result + len, cstrs.elems[i], elem_len);
        len += elem_len;
    }
    result[len] = '\0';

    return result;
}



////////////////////////////////////////////////////////////////////////////////



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


////////////////////////////////////////////////////////////////////////////////


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


////////////////////////////////////////////////////////////////////////////////


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
                    CREATE_ALWAYS,         // Same as `O_CREAT | O_TRUNC`
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


// Multiple modules could define this function, so add a guard around it to prevent redefinition
#ifndef NOBUILD__STRERROR
#define NOBUILD__STRERROR
Cstr nobuild__strerror(int errnum)
{
#ifndef _WIN32
    return nobuild__strerror(errnum);
#else
    static char buffer[1024];
    strerror_s(buffer, 1024, errnum);
    return buffer;
#endif
}
#endif // NOBUILD__STRERROR

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

Cstr cmd_show(Cmd cmd)
{
    // TODO(#31): cmd_show does not render the command line properly
    // - No string literals when arguments contains space
    // - No escaping of special characters
    // - Etc.
    return cstr_array_join(" ", cmd.line);
}

Pid cmd_run_async(Cmd cmd, Fd *fdin, Fd *fdout)
{
#ifndef _WIN32
    pid_t cpid = fork();
    if (cpid < 0) {
        PANIC("Could not fork child process: %s: %s",
              cmd_show(cmd), nobuild__strerror(errno));
    }

    if (cpid == 0) {
        Cstr_Array args = {
            .count = cmd.line.count
        };
        args.elems = malloc(sizeof(Cstr) * args.count+1);
        memcpy(args.elems, cmd.line.elems, args.count * sizeof(Cstr));
        args.elems[args.count++] = NULL;

        if (fdin) {
            if (dup2(*fdin, STDIN_FILENO) < 0) {
                PANIC("Could not setup stdin for child process: %s", nobuild__strerror(errno));
            }
        }

        if (fdout) {
            if (dup2(*fdout, STDOUT_FILENO) < 0) {
                PANIC("Could not setup stdout for child process: %s", nobuild__strerror(errno));
            }
        }

        if (execvp(args.elems[0], (char * const*) args.elems) < 0) {
            PANIC("Could not exec child process: %s: %s",
                  cmd_show(cmd), nobuild__strerror(errno));
        }
    }

    return cpid;
#else
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    // TODO(#32): check for errors in GetStdHandle
    siStartInfo.hStdOutput = fdout ? *fdout : GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = fdin ? *fdin : GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    BOOL bSuccess =
        CreateProcess(
            NULL,
            // TODO(#33): cmd_run_async on Windows does not render command line properly
            // It may require wrapping some arguments with double-quotes if they contains spaces, etc.
            (LPSTR) cstr_array_join(" ", cmd.line),
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &siStartInfo,
            &piProcInfo
        );

    if (!bSuccess) {
        PANIC("Could not create child process %s: %s\n",
              cmd_show(cmd), nobuild__GetLastErrorAsString());
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#endif // _WIN32
}

void cmd_run_sync(Cmd cmd)
{
    pid_wait(cmd_run_async(cmd, NULL, NULL));
}

static void chain_set_input_output_files_or_count_cmds(Chain *chain, Chain_Token token)
{
    switch (token.type) {
    case CHAIN_TOKEN_CMD: {
        chain->cmds.count += 1;
    }
    break;

    case CHAIN_TOKEN_IN: {
        if (chain->input_filepath) {
            PANIC("Input file path was already set");
        }

        chain->input_filepath = token.args.elems[0];
    }
    break;

    case CHAIN_TOKEN_OUT: {
        if (chain->output_filepath) {
            PANIC("Output file path was already set");
        }

        chain->output_filepath = token.args.elems[0];
    }
    break;

    case CHAIN_TOKEN_END:
    default: {
        assert(0 && "unreachable");
        exit(1);
    }
    }
}

static void chain_push_cmd(Chain *chain, Chain_Token token)
{
    if (token.type == CHAIN_TOKEN_CMD) {
        chain->cmds.elems[chain->cmds.count++] = (Cmd) {
            .line = token.args
        };
    }
}

Chain chain_build_from_tokens(Chain_Token first, ...)
{
    Chain result = {0};

    chain_set_input_output_files_or_count_cmds(&result, first);
    va_list args;
    va_start(args, first);
    Chain_Token next = va_arg(args, Chain_Token);
    while (next.type != CHAIN_TOKEN_END) {
        chain_set_input_output_files_or_count_cmds(&result, next);
        next = va_arg(args, Chain_Token);
    }
    va_end(args);

    result.cmds.elems = malloc(sizeof(result.cmds.elems[0]) * result.cmds.count);
    if (result.cmds.elems == NULL) {
        PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
    }
    result.cmds.count = 0;

    chain_push_cmd(&result, first);

    va_start(args, first);
    next = va_arg(args, Chain_Token);
    while (next.type != CHAIN_TOKEN_END) {
        chain_push_cmd(&result, next);
        next = va_arg(args, Chain_Token);
    }
    va_end(args);

    return result;
}

void chain_run_sync(Chain chain)
{
    if (chain.cmds.count == 0) {
        return;
    }

    Pid *cpids = malloc(sizeof(Pid) * chain.cmds.count);

    Pipe pip = {0};
    Fd fdin = 0;
    Fd *fdprev = NULL;

    if (chain.input_filepath) {
        fdin = fd_open_for_read(chain.input_filepath);
        fdprev = &fdin;
    }

    for (size_t i = 0; i < chain.cmds.count - 1; ++i) {
        pip = pipe_make();

        cpids[i] = cmd_run_async(
                       chain.cmds.elems[i],
                       fdprev,
                       &pip.write);

        if (fdprev) fd_close(*fdprev);
        fd_close(pip.write);
        fdprev = &fdin;
        fdin = pip.read;
    }

    {
        Fd fdout = 0;
        Fd *fdnext = NULL;

        if (chain.output_filepath) {
            fdout = fd_open_for_write(chain.output_filepath);
            fdnext = &fdout;
        }

        const size_t last = chain.cmds.count - 1;
        cpids[last] =
            cmd_run_async(
                chain.cmds.elems[last],
                fdprev,
                fdnext);

        if (fdprev) fd_close(*fdprev);
        if (fdnext) fd_close(*fdnext);
    }

    for (size_t i = 0; i < chain.cmds.count; ++i) {
        pid_wait(cpids[i]);
    }
}

void chain_echo(Chain chain)
{
    printf("[INFO] CHAIN:");
    if (chain.input_filepath) {
        printf(" %s", chain.input_filepath);
    }

    for (size_t cmd_index = 0; cmd_index < chain.cmds.count; ++cmd_index) {
        Cmd *cmd = &chain.cmds.elems[cmd_index];
        printf(" |> %s", cmd_show(*cmd));
    }

    if (chain.output_filepath) {
        printf(" |> %s", chain.output_filepath);
    }

    printf("\n");
}

#endif // NOBUILD_CMD_I_
#endif // NOBUILD_CMD_IMPLEMENTATION
