#ifndef NOBUILD_PATH_H_
#define NOBUILD_PATH_H_

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

#include "nobuild_cstr.h"

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

#endif // NOBUILD_PATH_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef NOBUILD_PATH_IMPLEMENTATION
#ifndef NOBUILD_PATH_I_
#define NOBUILD_PATH_I_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define NOBUILD_LOG_IMPLEMENTATION
#include "nobuild_log.h"

#define NOBUILD_CSTR_IMPLEMENTATION
#include "nobuild_cstr.h"

#define NOBUILD_IO_IMPLEMENTATION
#include "nobuild_io.h"

#ifndef _WIN32
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <dirent.h>
#else
#	define WIN32_MEAN_AND_LEAN
#	include <windows.h>
#	include <direct.h>
#	define MINIRENT_IMPLEMENTATION
#	include "minirent.h"
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

#endif // NOBUILD_PATH_I_
#endif // NOBUILD_PATH_IMPLEMENTATION
