#ifndef NOBUILD_H_
#define NOBUILD_H_

#include "nobuild_log.h"
#include "nobuild_cstr.h"
#include "nobuild_io.h"
#include "nobuild_cmd.h"
#include "nobuild_path.h"

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

#define NOBUILD_LOG_IMPLEMENTATION
#include "nobuild_log.h"

#define NOBUILD_CSTR_IMPLEMENTATION
#include "nobuild_cstr.h"

#define NOBUILD_IO_IMPLEMENTATION
#include "nobuild_io.h"

#define NOBUILD_CMD_IMPLEMENTATION
#include "nobuild_cmd.h"

#define NOBUILD_PATH_IMPLEMENTATION
#include "nobuild_path.h"

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
