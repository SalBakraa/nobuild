#include <ctype.h>

#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"

#define CFLAGS "-Wall", "-Wextra", "-std=c99", "-pedantic"

void build_tool(const char *tool)
{
    Cstr tool_path = PATH("tools", tool);
#ifndef _WIN32
    CMD("cc", CFLAGS, "-o", NOEXT(tool_path), tool_path);
#else
    CMD("cl.exe", "/Fe.\\tools\\", tool_path);
#endif
}

void build_tools(void)
{
    FOREACH_FILE_IN_DIR(tool, "tools", {
        if (ENDS_WITH(tool, ".c")) {
            build_tool(tool);
        }
    });
}

void run_example(const char *example)
{
    Cstr example_path = PATH("examples", example);
#ifndef _WIN32
    CMD("cc", CFLAGS, "-o", NOEXT(example_path), example_path);
#else
    CMD("cl.exe", "/Fe.\\examples\\", example_path);
#endif
    CMD(NOEXT(example_path));
}

void run_examples(void)
{
    FOREACH_FILE_IN_DIR(example, "examples", {
        if (ENDS_WITH(example, ".c")) {
            run_example(example);
        }
    });
}


void print_chain(const Chain *chain)
{
    INFO("input: %s", chain->input_filepath);
    INFO("output: %s", chain->output_filepath);
    FOREACH_ARRAY(Cmd, cmd, chain->cmds, {
        INFO("cmd: %s", cmd_show(*cmd));
    });
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

    build_tools();
    run_examples();

    Cstr_Array args = {
        .elems = (Cstr *) argv,
        .count = argc,
    };

    if (!cstr_array_contains(args, "--rebuild")) {
        return 0;
    }

    // Build PCPP
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        PANIC("Could not retrieve current working directory.");
    }

    Cstr dir = PATH("tools", "pcpp");
    if (chdir(dir) < 0) {
        PANIC("Could not change current directory to '%s': %s", dir, nobuild__strerror(errno));
    }

    CMD("cc", CFLAGS, "-o", "nobuild", "nobuild.c");
    CMD(PATH(".", "nobuild"));

    if (chdir(cwd) < 0) {
        PANIC("Could not change current directory to '%s': %s", cwd, nobuild__strerror(errno));
    }

    // Build standalone versions of each nobuild header
    if (!path_exists("standalone")) {
        MKDIRS("standalone");
    }

    Cstr_Array header_guards = CSTR_ARRAY_MAKE(
        "NOBUILD_LOG_H_", "NOBUILD_CSTR_H_", "NOBUILD_PATH_H_",
        "NOBUILD_CMD_H_", "NOBUILD_IO_H_", "MINIRENT_H_"
    );
    Cstr_Array impl_flags = CSTR_ARRAY_MAKE(
        "NOBUILD_LOG_IMPLEMENTATION", "NOBUILD_CSTR_IMPLEMENTATION", "NOBUILD_PATH_IMPLEMENTATION",
        "NOBUILD_CMD_IMPLEMENTATION", "NOBUILD_IO_IMPLEMENTATION", "MINIRENT_IMPLEMENTATION"
    );
    Cstr_Array impl_guards = CSTR_ARRAY_MAKE(
        "NOBUILD_LOG_I_", "NOBUILD_CSTR_I_", "NOBUILD_PATH_I_",
        "NOBUILD_CMD_I_", "NOBUILD_IO_I_", "MINIRENT_I_"
    );

    FOREACH_FILE_IN_DIR(header, "src", {
        if (!STARTS_WITH(header, "nobuild_")) {
            continue;
        }

        char *lib_name = (char *)NOEXT(header);
        for (int i = 0; lib_name[i] != '\0'; i++) {
            lib_name[i] = toupper(lib_name[i]);
        }

        header_guards = cstr_array_remove(header_guards, CONCAT(lib_name, "_H_"));
        impl_guards = cstr_array_remove(impl_guards, CONCAT(lib_name, "_I_"));
        impl_flags = cstr_array_remove(impl_flags, CONCAT(lib_name, "_IMPLEMENTATION"));

        Cstr macros = JOIN(",", cstr_array_join(",", header_guards), cstr_array_join(",", impl_guards), cstr_array_join(",", impl_flags));
        CMD(PATH("tools", "pcpp", "build", "pcpp"), "--implicitly-undef", "--only", macros, "--include-all", "-o", PATH("standalone", header), PATH("src", header));

        header_guards = cstr_array_append(header_guards, CONCAT(lib_name, "_H_"));
        impl_guards = cstr_array_append(impl_guards, CONCAT(lib_name, "_I_"));
        impl_flags = cstr_array_append(impl_flags, CONCAT(lib_name, "_IMPLEMENTATION"));
    });

    Cstr macros = JOIN(",", cstr_array_join(",", header_guards), cstr_array_join(",", impl_guards), cstr_array_join(",", impl_flags));
    CMD(PATH("tools", "pcpp", "build", "pcpp"), "--implicitly-undef",  "--only", macros, "--include-all", "-o", "nobuild.h", PATH("src", "nobuild.h"));
    return 0;
}
