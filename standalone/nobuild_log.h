#ifndef NOBUILD_LOG_H_
#define NOBUILD_LOG_H_

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

#endif  // NOBUILD_LOG_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef NOBUILD_LOG_IMPLEMENTATION
#ifndef NOBUILD_LOG_I_
#define NOBUILD_LOG_I_

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

#endif // NOBUILD_LOG_I_
#endif // NOBUILD_IMPLEMENTATION
