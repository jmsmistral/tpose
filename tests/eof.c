/* Run production readers against an input ending at an inaccessible page. */
#include "tpose_io.h"

static void check(int condition, const char* message) {
    if(!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

void testEofFile(const char* mode, const char* path, char* aggregation) {
    int simple = strcmp(mode, "simple") == 0;
    int withId = strcmp(mode, "id") == 0 || strstr(mode, "-id") != NULL;
    int automatic = strncmp(mode, "auto-", 5) == 0;
    int empty = strncmp(mode, "empty-", 6) == 0;
    int parallel = automatic || empty || strncmp(mode, "parallel-", 9) == 0;
    check(simple || withId || strcmp(mode, "group") == 0 || parallel, "EOF test mode");
    FILE* source = fopen(path, "rb");
    check(source != NULL, "open EOF fixture");
    check(fseek(source, 0, SEEK_END) == 0, "seek EOF fixture");
    long size = ftell(source);
    check(size > 0 && size <= 1024 * 1024, "EOF fixture size");
    rewind(source);
    long page = sysconf(_SC_PAGESIZE);
    check(page > 0, "system page size");
    size_t span = ((size_t) size + page - 1) / page * page;
    size_t mappedSize = span + 2 * page;
    char* mapped = mmap(NULL, mappedSize, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(mapped != MAP_FAILED, "map guarded fixture");
    check(mprotect(mapped, page, PROT_NONE) == 0 &&
          mprotect(mapped + page + span, page, PROT_NONE) == 0, "protect guard pages");
    char* data = mapped + page + span - size;
    check(fread(data, 1, size, source) == (size_t) size, "read EOF fixture");
    check(fclose(source) == 0, "close EOF fixture");

    TposeInputFile input = {0};
    input.fileAddr = data;
    input.fileSize = size;
    input.fieldDelimiter = '\t';
    input.fileHeader = tposeIOReadInputHeader(&input, !simple);
    check(input.fileHeader != NULL, "read guarded header");
    TposeOutputFile* output = tposeIOOutputFileAlloc(stdout, '\t');
    check(output != NULL, "allocate EOF output");
    TposeQuery* query = tposeIOQueryAlloc(&input, output, withId ? "id" : NULL,
                                         simple ? NULL : "group", simple ? NULL : "amount", aggregation);
    check(query != NULL, "create EOF query");
    if(simple) {
        tposeIOTransposeSimple(query);
    } else if(parallel) {
        off_t length = input.fileSize - (input.dataAddr - input.fileAddr);
        if(automatic) {
            check(tposeIOBuildPartitions(query, withId ? TPOSE_IO_PARTITION_ID : TPOSE_IO_PARTITION_GROUP) == 0,
                  "build real partitions");
            check(partitions[0] == 0 && partitions[fileChunks] == length, "data-relative endpoints");
            if(strncmp(mode, "auto-multi-", 11) == 0)
                check(fileChunks > 1, "fixture actually creates multiple partitions");
            if(strncmp(mode, "auto-single-", 12) == 0)
                check(fileChunks == 1, "no viable interior boundary means one partition");
            for(unsigned int i = 1; i <= fileChunks; ++i)
                check(partitions[i] > partitions[i - 1] && partitions[i] <= length, "bounded nonempty partitions");
        } else if(empty) {
            fileChunks = 3;
            partitions[0] = partitions[1] = 0;
            partitions[2] = partitions[3] = length;
        } else {
            /* A record boundary halfway through the data; ID fixtures keep
               each ID in one record or use one ID on each side of this split. */
            off_t split = 0;
            for(off_t i = length / 2; i < length; ++i) {
                if(input.dataAddr[i] == '\n') { split = i + 1; break; }
            }
            fileChunks = 2;
            partitions[0] = 0;
            partitions[1] = split;
            partitions[2] = length;
        }
        btreeGlobal = btreeAlloc();
        check(btreeGlobal != NULL, "allocate parallel tree");
        tposeIOUniqueGroupsParallel(query);
        if(withId) tposeIOTransposeGroupIdParallel(query);
        else tposeIOTransposeGroupParallel(query);
        btreeFree(&btreeGlobal);
    } else {
        BTree* tree = btreeAlloc();
        check(tree != NULL, "allocate EOF tree");
        tposeIOUniqueGroups(query, tree);
        if(withId) tposeIOTransposeGroupId(query, tree);
        else tposeIOTransposeGroup(query, tree);
        btreeFree(&tree);
    }
    tposeIOQueryFree(&query);
    tposeIOHeaderFree(&input.fileHeader);
    tposeIOOutputFileFree(&output);
    check(munmap(mapped, mappedSize) == 0, "unmap guarded fixture");
    check(fflush(stdout) == 0, "flush EOF output");
}
