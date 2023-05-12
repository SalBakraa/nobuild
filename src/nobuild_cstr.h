#ifndef NOBUILD_CSTR_H_
#define NOBUILD_CSTR_H_

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

#endif  // NOBUILD_CSTR_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef NOBUILD_CSTR_IMPLEMENTATION
#ifndef NOBUILD_CSTR_I_
#define NOBUILD_CSTR_I_

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define NOBUILD_LOG_IMPLEMENTATION
#include "nobuild_log.h"

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

#endif // NOBUILD_CSTR_I_
#endif // NOBUILD_IMPLEMENTATION
