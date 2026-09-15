/* Portable fault probes: no preload libraries or external test framework. */
#include "tpose_io.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>

static void check(int ok, const char* message) {
    if(!ok) {
        fprintf(stderr, "Test setup failed: %s\n", message);
        exit(99);
    }
}

static void runFailureChild(const char* mode, char** command) {
    int errors[2];
    check(pipe(errors) == 0, "create diagnostic pipe");
    pid_t child = fork();
    check(child >= 0, "fork fault probe");
    if(child == 0) {
        close(errors[0]);
        check(dup2(errors[1], STDERR_FILENO) >= 0, "redirect diagnostics");
        close(errors[1]);
        if(strcmp(mode, "limited") == 0) {
            /* Keep stderr on a pipe: the limit would truncate diagnostics
               redirected directly to a regular file as well. */
            struct rlimit limit = {4, 4};
            check(setrlimit(RLIMIT_FSIZE, &limit) == 0, "set exact file size limit");
            signal(SIGXFSZ, SIG_IGN);
        } else if(strcmp(mode, "broken-pipe") == 0) {
            int output[2];
            check(pipe(output) == 0, "create output pipe");
            close(output[0]);
            check(dup2(output[1], STDOUT_FILENO) >= 0, "redirect output");
            close(output[1]);
        } else {
            check(strcmp(mode, "closed-stdout") == 0, "fault mode");
            close(STDOUT_FILENO);
        }
        execv(command[0], command);
        _exit(99);
    }
    close(errors[1]);
    char buffer[1024];
    ssize_t count;
    while((count = read(errors[0], buffer, sizeof(buffer))) != 0) {
        if(count < 0 && errno == EINTR) continue;
        check(count > 0, "read child diagnostics");
        check(fwrite(buffer, 1, count, stderr) == (size_t) count, "forward diagnostics");
    }
    close(errors[0]);
    int status;
    while(waitpid(child, &status, 0) < 0) check(errno == EINTR, "wait for child");
    exit(WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status));
}

void testOutput(int argc, char** argv) {
    const char* mode = argv[2];
    if(strcmp(mode, "limited") == 0 || strcmp(mode, "broken-pipe") == 0 ||
       strcmp(mode, "closed-stdout") == 0) {
        check(argc >= 4, "fault command arguments");
        runFailureChild(mode, &argv[3]);
    } else if(strcmp(mode, "close") == 0) {
        FILE* stream = tmpfile();
        check(stream != NULL, "create close probe");
        TposeOutputFile* output = tposeIOOutputFileAlloc(stream, '\t');
        check(output != NULL, "allocate close probe");
        /* No buffered bytes: exercise fclose failure, not an earlier write. */
        check(close(fileno(stream)) == 0, "invalidate underlying descriptor");
        exit(tposeIOCloseOutputFile(output) == 0 ? 0 : 1);
    } else if(strcmp(mode, "parallel-read") == 0) {
        FILE* stream = fopen("read-fault.tmp", "w");
        check(stream != NULL, "create write-only worker stream");
        tempFileArray[0] = tposeIOOutputFileAlloc(stream, '\t');
        TposeQuery query = {0};
        query.outputFile = tposeIOOutputFileAlloc(stdout, '\t');
        check(query.outputFile && tempFileArray[0], "allocate read probe");
        fileChunks = 1;
        tposeIOTransposeGroupIdReduce(&query);
    } else if(strcmp(mode, "permissions") == 0) {
        check(argc == 5, "permissions arguments");
        struct stat st;
        check(stat(argv[3], &st) == 0, "stat output permissions");
        check((st.st_mode & 0777) == (mode_t) strtoul(argv[4], NULL, 8), "output mode matches");
    } else if(strcmp(mode, "parallel-destination") == 0) {
        check(argc == 5, "parallel destination arguments");
        TposeInputFile* input = tposeIOOpenInputFile(argv[3], '\t', 1);
        check(input != NULL, "open parallel input");
        char aggregation[] = "sum";
        TposeQuery* query = tposeIOQueryAlloc(input, NULL, "id", "group", "amount", aggregation);
        check(query != NULL, "create parallel query");
        TposeOutputFile* output = tposeIOOpenDestination(input, argv[4], '\t');
        check(output != NULL, "open parallel destination");
        query->outputFile = output;
        off_t length = input->fileSize - (input->dataAddr - input->fileAddr);
        const char* first = memchr(input->dataAddr, '\n', length);
        check(first != NULL && first + 1 < input->dataAddr + length, "two-row parallel fixture");
        fileChunks = 2;
        partitions[0] = 0;
        partitions[1] = first + 1 - input->dataAddr;
        partitions[2] = length;
        btreeGlobal = btreeAlloc();
        check(btreeGlobal != NULL, "allocate parallel tree");
        tposeIOUniqueGroupsParallel(query);
        tposeIOTransposeGroupIdParallel(query);
        btreeFree(&btreeGlobal);
        tposeIOQueryFree(&query);
        check(tposeIOCloseInputFile(input) == 0, "close parallel input");
        int result = tposeIOCommitOutput(output);
        tposeIOOutputFileFree(&output);
        exit(result == 0 ? 0 : 1);
    } else {
        check(argc == 5, "transaction probe arguments");
        check(strcmp(mode, "changed") == 0 || strcmp(mode, "appeared") == 0 ||
              strcmp(mode, "directory") == 0, "transaction probe mode");
        TposeInputFile* input = tposeIOOpenInputFile(argv[3], '\t', 0);
        check(input != NULL, "open transaction input");
        TposeOutputFile* output = tposeIOOpenDestination(input, argv[4], '\t');
        check(output != NULL, "open transaction output");
        check(fputs("replacement\n", output->fd) >= 0, "write staged output");
        if(strcmp(mode, "appeared") != 0)
            check(rename(argv[4], "saved-destination") == 0, "move original destination");
        if(strcmp(mode, "directory") == 0) {
            check(mkdir(argv[4], 0700) == 0, "create conflicting directory");
        } else {
            FILE* other = fopen(argv[4], "w");
            check(other != NULL, "simulate concurrent creator");
            check(fputs("concurrent\n", other) >= 0 && fclose(other) == 0, "write concurrent result");
        }
        int result = tposeIOCommitOutput(output);
        tposeIOOutputFileFree(&output);
        check(tposeIOCloseInputFile(input) == 0, "close transaction input");
        exit(result == 0 ? 0 : 1);
    }
}
