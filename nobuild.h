#ifndef NOBUILD_H_
#define NOBUILD_H_


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
#	include <sys/types.h>
typedef pid_t Pid;
typedef int Fd;
#else
#	define WIN32_MEAN_AND_LEAN
#	include <windows.h>
typedef HANDLE Pid;
typedef HANDLE Fd;
#endif


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


////////////////////////////////////////////////////////////////////////////////


#ifndef NOBUILD__DEPRECATED
#	if defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER))
#		define NOBUILD__DEPRECATED(func) __attribute__ ((deprecated)) func
#	elif defined(_MSC_VER)
#		define NOBUILD__DEPRECATED(func) __declspec (deprecated) func
#	endif
#endif

#ifndef _WIN32
#	define PATH_SEP "/"
#else
#	define PATH_SEP "\\"
#endif


////////////////////////////////////////////////////////////////////////////////


#define PATH(...) JOIN(PATH_SEP, __VA_ARGS__)

Cstr path_no_ext(Cstr path);
#define NOEXT(path) path_no_ext(path)

Cstr path_dirname(Cstr path);
#define DIRNAME(path) path_dirname(path)

Cstr path_basename(Cstr path);
#define BASENAME(path) path_basename(path)

int path_is_dir(Cstr path);
#define IS_DIR(path) path_is_dir(path)

int path_is_file(Cstr path);
#define IS_FILE(path) path_is_file(path)

int path_exists(Cstr path);
#define PATH_EXISTS(path) path_exists(path)

NOBUILD__DEPRECATED(int is_path1_modified_after_path2(Cstr path1, Cstr path2));
int path_is_newer(Cstr path1, Cstr path2);
#define IS_NEWER(path1, path2) path_is_newer(path1, path2)

void path_mkdirs(Cstr_Array path);
#define MKDIRS(...)                                             \
    do {                                                        \
        Cstr_Array path = cstr_array_make(__VA_ARGS__, NULL);   \
        INFO("MKDIRS: %s", cstr_array_join(PATH_SEP, path));    \
        path_mkdirs(path);                                      \
    } while (0)

void path_rename(Cstr old_path, Cstr new_path);
#define RENAME(old_path, new_path)                    \
    do {                                              \
        INFO("RENAME: %s -> %s", old_path, new_path); \
        path_rename(old_path, new_path);              \
    } while (0)

void path_copy(Cstr old_path, Cstr new_path);
#define COPY(old_path, new_path)                    \
    do {                                            \
        INFO("COPY: %s -> %s", old_path, new_path); \
        path_copy(old_path, new_path);              \
    } while(0)

void path_rm(Cstr path);
#define RM(path)                                \
    do {                                        \
        INFO("RM: %s", path);                   \
        path_rm(path);                          \
    } while(0)

#define FOREACH_FILE_IN_DIR(file, dirpath, body)        \
    do {                                                \
        struct dirent *dp = NULL;                       \
        DIR *dir = opendir(dirpath);                    \
        if (dir == NULL) {                              \
            PANIC("could not open directory %s: %s",    \
                  dirpath, nobuild__strerror(errno));   \
        }                                               \
        errno = 0;                                      \
        while ((dp = readdir(dir))) {                   \
            const char *file = dp->d_name;              \
            body;                                       \
        }                                               \
                                                        \
        if (errno > 0) {                                \
            PANIC("could not read directory %s: %s",    \
                  dirpath, nobuild__strerror(errno));   \
        }                                               \
                                                        \
        closedir(dir);                                  \
    } while(0)


////////////////////////////////////////////////////////////////////////////////


#define FOREACH_ARRAY(type, elem, array, body)                                  \
    for (size_t elem_##index = 0; elem_##index < array.count; ++elem_##index) { \
        type *elem = &array.elems[elem_##index];                                \
        body;                                                                   \
    }

#ifndef REBUILD_URSELF
#	if _WIN32
#		if defined(__GNUC__)
#			define REBUILD_URSELF(binary_path, source_path) CMD("gcc", "-o", binary_path, source_path)
#		elif defined(__clang__)
#			define REBUILD_URSELF(binary_path, source_path) CMD("clang", "-o", binary_path, source_path)
#		elif defined(_MSC_VER)
#			define REBUILD_URSELF(binary_path, source_path) CMD("cl.exe", source_path)
#		endif
#	else
#		define REBUILD_URSELF(binary_path, source_path) CMD("cc", "-o", binary_path, source_path)
#	endif
#endif

// Go Rebuild Urself™ Technology
//
//   How to use it:
//     int main(int argc, char** argv) {
//         GO_REBUILD_URSELF(argc, argv);
//         // actual work
//         return 0;
//     }
//
//   After your added this macro every time you run ./nobuild it will detect
//   that you modified its original source code and will try to rebuild itself
//   before doing any actual work. So you only need to bootstrap your build system
//   once.
//
//   The modification is detected by comparing the last modified times of the executable
//   and its source code. The same way the make utility usually does it.
//
//   The rebuilding is done by using the REBUILD_URSELF macro which you can redefine
//   if you need a special way of bootstraping your build system. (which I personally
//   do not recommend since the whole idea of nobuild is to keep the process of bootstrapping
//   as simple as possible and doing all of the actual work inside of the nobuild)
//
#define GO_REBUILD_URSELF(argc, argv)                                  \
    do {                                                               \
        const char *source_path = __FILE__;                            \
        assert(argc >= 1);                                             \
        const char *binary_path = argv[0];                             \
                                                                       \
        if (IS_NEWER(source_path, binary_path)) { \
            RENAME(binary_path, CONCAT(binary_path, ".old"));          \
            REBUILD_URSELF(binary_path, source_path);                  \
            Cmd cmd = {                                                \
                .line = {                                              \
                    .elems = (Cstr*) argv,                             \
                    .count = argc,                                     \
                },                                                     \
            };                                                         \
            INFO("CMD: %s", cmd_show(cmd));                            \
            cmd_run_sync(cmd);                                         \
            exit(0);                                                   \
        }                                                              \
    } while(0)

char *shift_args(int *argc, char ***argv);

void file_to_c_array(Cstr path, Cstr out_path, Cstr array_type,  Cstr array_name, int null_term);
#define FILE_TO_C_ARRAY(path, out_path, array_name) file_to_c_array(path, out_path, "unsigned char", array_name, 1)

#endif  // NOBUILD_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef NOBUILD_IMPLEMENTATION


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



////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


////////////////////////////////////////////////////////////////////////////////


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



////////////////////////////////////////////////////////////////////////////////


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>


////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////


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



////////////////////////////////////////////////////////////////////////////////


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////


#ifndef _WIN32
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <dirent.h>
#else
#	define WIN32_MEAN_AND_LEAN
#	include <windows.h>
#	include <direct.h>
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================
//
// minirent — 0.0.1 — A subset of dirent interface for Windows.
//
// https://github.com/tsoding/minirent
//
// ============================================================
//
// ChangeLog (https://semver.org/ is implied)
//
//    0.0.1 First Official Release


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct dirent
{
    char d_name[MAX_PATH+1];
};

typedef struct DIR DIR;

DIR *opendir(const char *dirpath);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);


////////////////////////////////////////////////////////////////////////////////


struct DIR
{
    HANDLE hFind;
    WIN32_FIND_DATA data;
    struct dirent *dirent;
};

DIR *opendir(const char *dirpath)
{
    assert(dirpath);

    char buffer[MAX_PATH];
    snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

    DIR *dir = (DIR*)calloc(1, sizeof(DIR));

    dir->hFind = FindFirstFile(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }

    return dir;

fail:
    if (dir) {
        free(dir);
    }

    return NULL;
}

struct dirent *readdir(DIR *dirp)
{
    assert(dirp);

    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent*)calloc(1, sizeof(struct dirent));
    } else {
        if(!FindNextFile(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }

            return NULL;
        }
    }

    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

    strncpy(
        dirp->dirent->d_name,
        dirp->data.cFileName,
        sizeof(dirp->dirent->d_name) - 1);

    return dirp->dirent;
}

int closedir(DIR *dirp)
{
    assert(dirp);

    if(!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }

    if (dirp->dirent) {
        free(dirp->dirent);
    }
    free(dirp);

    return 0;
}

#endif // _WIN32

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

int nobuild__mkdir(const char *pathname, unsigned int mode)
{
#ifndef _WIN32
    return mkdir(pathname, mode);
#else
    _Pragma("unused(mode)");
    return _mkdir(pathname);
#endif
}

int nobuild__rmdir(const char *pathname)
{
#ifndef _WIN32
    return rmdir(pathname);
#else
    return _rmdir(pathname);
#endif
}

int nobuild__unlink(const char *pathname)
{
#ifndef _WIN32
    return unlink(pathname);
#else
    return _unlink(pathname);
#endif
}

Cstr path_no_ext(Cstr path)
{
    size_t n = strlen(path);
    while (n > 0 && path[n - 1] != '.') {
        n -= 1;
    }

    if (n > 0) {
        char *result = malloc(n);
        memcpy(result, path, n);
        result[n - 1] = '\0';

        return result;
    } else {
        return path;
    }
}

Cstr path_dirname(Cstr path)
{
    char path_sep = *PATH_SEP;
    size_t prefix_len = 0;

    // Get length of directory prefix
    for (size_t i = 1; i < strlen(path); ++i) {
        if (path[i] != path_sep && path[i-1] == path_sep) {
            prefix_len = i;
        }
    }

    if (prefix_len == 0) {
        return *path == path_sep ? PATH_SEP : ".";
    }

    // Strip trailing slashes
    while (prefix_len > 1 && path[prefix_len-1] == path_sep) {
        --prefix_len;
    }

    // copy prefix
    size_t len = prefix_len;
    char* dirname = malloc(len+1);
    if (dirname == NULL) {
        PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
    }

    return dirname[len] = '\0', memcpy(dirname, path, len);
}

Cstr path_basename(Cstr path)
{
    char path_sep = *PATH_SEP;
    Cstr last_sep = strrchr(path, path_sep);
    if (last_sep == NULL) {
        return path;
    }

    // Last character is not a separator
    if (*(last_sep + 1) != '\0') {
        size_t len = strlen(last_sep + 1);
        char* basename = malloc(len + 1);
        if (basename == NULL) {
            PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
        }

        return basename[len] = '\0', memcpy(basename, last_sep + 1, len);
    }

    // Skip consecutive seprators
    while (last_sep > path && *(last_sep - 1) == path_sep) {
        --last_sep;
    }

    if (last_sep == path) {
        return PATH_SEP;
    }

    // Find the start of the basename
    Cstr start = last_sep;
    while (start > path && *(start - 1) != path_sep) {
        --start;
    }
    assert(last_sep >= start && "last_sep must never be less than start");

    size_t len = (size_t)(last_sep - start);
    char *basename = malloc(len + 1);
    if (basename == NULL) {
        PANIC("Could not allocate memory: %s", nobuild__strerror(errno));
    }

    return basename[len] = '\0', memcpy(basename, start, len);
}

int path_is_dir(Cstr path)
{
#ifndef _WIN32
    struct stat statbuf = {0};
    if (stat(path, &statbuf) < 0) {
        if (errno == ENOENT) {
            errno = 0;
            return 0;
        }

        PANIC("could not retrieve information about file %s: %s",
              path, nobuild__strerror(errno));
    }

    return S_ISDIR(statbuf.st_mode);
#else
    DWORD dwAttrib = GetFileAttributes(path);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif // _WIN32
}

int path_is_file(Cstr path)
{
#ifndef _WIN32
    struct stat statbuf = {0};
    if (stat(path, &statbuf) < 0) {
        if (errno == ENOENT) {
            errno = 0;
            return 0;
        }

        PANIC("Could not retrieve information about file %s: %s",
              path, nobuild__strerror(errno));
    }

    return S_ISREG(statbuf.st_mode);
#else
    DWORD dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif // _WIN32
}

int path_exists(Cstr path)
{
#ifndef _WIN32
    struct stat statbuf = {0};
    if (stat(path, &statbuf) < 0) {
        if (errno == ENOENT) {
            errno = 0;
            return 0;
        }

        PANIC("could not retrieve information about file %s: %s",
              path, nobuild__strerror(errno));
    }

    return 1;
#else
    DWORD dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
#endif
}

int is_path1_modified_after_path2(Cstr path1, Cstr path2)
{
    WARN("This function is deprecated. Use `path_is_newer()` instead.");
    return path_is_newer(path1, path2);
}

long long nobuild__get_modification_time(Cstr path) {
    if (IS_DIR(path)) {
        long long mod_time = -1;
        FOREACH_FILE_IN_DIR(file, path, {
            if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0) {
                continue;
            }

            long long path_mod_time = nobuild__get_modification_time(PATH(path, file));
            mod_time = path_mod_time > mod_time ? path_mod_time : mod_time;
        });
        return mod_time;
    } else {
#ifndef _WIN32
        struct stat statbuf = {0};

        if (stat(path, &statbuf) < 0) {
            PANIC("Could not stat %s: %s\n", path, nobuild__strerror(errno));
        }
        return (long long) statbuf.st_mtime;
#else
        FILETIME path_time;
        Fd path_fd = fd_open_for_read(path);
        if (!GetFileTime(path_fd, NULL, NULL, &path_time)) {
            PANIC("could not get time of %s: %s", path, nobuild__GetLastErrorAsString());
        }
        fd_close(path_fd);
        return ((long long) path_time.dwHighDateTime) << 32 | path_time.dwLowDateTime;
#endif
    }
}

int path_is_newer(Cstr path1, Cstr path2)
{
    // Warn the user that the path is missing
    if (!PATH_EXISTS(path1)) {
        WARN("File %s does not exist", path2);
        return 0;
    }

    if (!PATH_EXISTS(path2)) {
        return 1;
    }

    return nobuild__get_modification_time(path1) > nobuild__get_modification_time(path2);
}

void path_mkdirs(Cstr_Array path)
{
    if (path.count == 0) {
        return;
    }

    size_t len = 0;
    for (size_t i = 0; i < path.count; ++i) {
        len += strlen(path.elems[i]);
    }

    size_t seps_count = path.count - 1;
    const size_t sep_len = strlen(PATH_SEP);

    char *result = malloc(len + seps_count * sep_len + 1);

    len = 0;
    for (size_t i = 0; i < path.count; ++i) {
        size_t n = strlen(path.elems[i]);
        memcpy(result + len, path.elems[i], n);
        len += n;

        if (seps_count > 0) {
            memcpy(result + len, PATH_SEP, sep_len);
            len += sep_len;
            seps_count -= 1;
        }

        result[len] = '\0';

        if (nobuild__mkdir(result, 0755) < 0) {
            if (errno == EEXIST) {
                errno = 0;
                WARN("directory %s already exists", result);
            } else {
                PANIC("could not create directory %s: %s", result, nobuild__strerror(errno));
            }
        }
    }
}

void path_rename(Cstr old_path, Cstr new_path)
{
#ifndef _WIN32
    if (rename(old_path, new_path) < 0) {
        PANIC("could not rename %s to %s: %s", old_path, new_path,
              nobuild__strerror(errno));
    }
#else
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        PANIC("could not rename %s to %s: %s", old_path, new_path,
              nobuild__GetLastErrorAsString());
    }
#endif // _WIN32
}

void path_copy(Cstr old_path, Cstr new_path) {
    if (IS_DIR(old_path)) {
        path_mkdirs(cstr_array_make(new_path, NULL));
        FOREACH_FILE_IN_DIR(file, old_path, {
            if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0) {
                continue;
            }

            path_copy(PATH(old_path, file), PATH(new_path, file));
        });
    } else {
        Fd f1 = fd_open_for_read(old_path);
        Fd f2 = fd_open_for_write(new_path);

        unsigned char buffer[4096];
        while (1) {
#ifndef _WIN32
            ssize_t bytes = read(f1, buffer, sizeof buffer);
            if (bytes == -1) {
                ERRO("Could not copy %s to %s due to read error: %s", old_path, new_path, nobuild__strerror(errno));
                break;
            }

            if (bytes == 0) {
                break;
            }

            bytes = write(f2, buffer, (size_t)bytes);
            if (bytes == -1) {
                ERRO("Could not copy %s to %s due to write error: %s", old_path, new_path, nobuild__strerror(errno));
                break;
            }

            if (bytes == 0) {
                break;
            }
#else
            DWORD bytes;
            if (!ReadFile(f1, buffer, sizeof buffer, &bytes, NULL)) {
                ERRO("Could not copy %s to %s due to read error: %s", old_path, new_path, nobuild__GetLastErrorAsString());
                break;

            }

            if (bytes == 0) {
                break;
            }

            if (!WriteFile(f2, buffer, bytes, &bytes, NULL)) {
                ERRO("Could not copy %s to %s due to write error: %s", old_path, new_path, nobuild__GetLastErrorAsString());
                break;
            }

            if (bytes == 0) {
                break;
            }
#endif
        }

        fd_close(f1);
        fd_close(f2);
    }
}

void path_rm(Cstr path)
{
    if (IS_DIR(path)) {
        FOREACH_FILE_IN_DIR(file, path, {
            if (strcmp(file, ".") != 0 && strcmp(file, "..") != 0)
            {
                path_rm(PATH(path, file));
            }
        });

        if (nobuild__rmdir(path) < 0) {
            if (errno == ENOENT) {
                errno = 0;
                WARN("Directory %s does not exist", path);
            } else {
                PANIC("Could not remove directory %s: %s", path, nobuild__strerror(errno));
            }
        }
    } else {
        if (nobuild__unlink(path) < 0) {
            if (errno == ENOENT) {
                errno = 0;
                WARN("File %s does not exist", path);
            } else {
                PANIC("Could not remove file %s: %s", path, nobuild__strerror(errno));
            }
        }
    }
}


char *shift_args(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argc -= 1;
    *argv += 1;
    return result;
}

void file_to_c_array(Cstr path, Cstr out_path, Cstr array_type, Cstr array_name, int null_term) {
    Fd file = fd_open_for_read(path);
    Fd output_file = fd_open_for_write(out_path);
    fd_printf(output_file, "%s %s[] = {\n", array_type, array_name);

    unsigned char buffer[4096];
    unsigned long total_bytes_read = 0;
    do {
#ifndef _WIN32
        ssize_t bytes = read(file, buffer, sizeof(buffer));
        if (bytes == -1) {
            ERRO("Could not read file %s: %s", path, strerror(errno));
            fd_close(file);
            fd_close(output_file);
            break;
        }

        if (bytes == 0) {
            break;
        }
#else
        DWORD bytes;
        if (!ReadFile(file, buffer, sizeof buffer, &bytes, NULL)) {
            ERRO("Could not read file %s: %s", path, nobuild__GetLastErrorAsString());
            break;
        }
#endif
        int bytes_read = (int) bytes;

        if (bytes_read == 0) {
            break;
        }

        for (int i = 0; i < bytes_read; i+=16) {
            fd_printf(output_file, "\t");
            for (int j = i; j < i+16; j++) {
                if (j >= bytes_read) {
                    break;
                }
                fd_printf(output_file, "0x%02x, ", buffer[j]);
            }
            fd_printf(output_file, "\n");
        }
        total_bytes_read += (unsigned long) bytes_read;
    } while (1);

    if (null_term) {
        fd_printf(output_file, "\t0x00 /* Terminate with null */\n");
        total_bytes_read++;
    }
    fd_printf(output_file, "};\n");
    fd_printf(output_file, "unsigned long %s_len = %lu;\n", array_name, total_bytes_read);

    fd_close(file);
    fd_close(output_file);
}

#endif // NOBUILD_IMPLEMENTATION
