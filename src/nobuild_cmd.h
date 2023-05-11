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

#include "nobuild_cstr.h"

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

#define NOBUILD_CSTR_IMPLEMENTATION
#include "nobuild_cstr.h"

#define NOBUILD_LOG_IMPLEMENTATION
#include "nobuild_log.h"

#define NOBUILD_IO_IMPLEMENTATION
#include "nobuild_io.h"

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
