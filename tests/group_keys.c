/* Focused tests for full-string keys and the parallel aggregation paths. */
#include "tpose_io.h"

char* prefixGlobal = "";
char* suffixGlobal = "";

void testEofFile(const char* mode, const char* path, char* aggregation);

static void check(int condition, const char* message) {
    if(!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void testTree(void) {
    enum { KEY_COUNT = 256 };
    char names[KEY_COUNT][64];
    char lookup[64];
    BTree* tree = btreeAlloc();
    BTreeKey key = {0};
    check(tree != NULL, "allocate tree");
    check(btreeSearch(tree, tree->root, "missing") == NULL, "empty tree");

    /* Insert a permutation to exercise splits and both comparison directions. */
    for(unsigned int i = 0; i < KEY_COUNT; ++i) {
        unsigned int index = (i * 73) % KEY_COUNT;
        snprintf(names[index], sizeof(names[index]), "group-%03u-abcdefghijklmnop", index);
        btreeSetKeyValue(&key, names[index], index, 0);
        check(btreeInsert(tree, &key) == 0, "insert full string");
    }
    check(tree->numKeys == KEY_COUNT, "distinct string count");
    for(unsigned int i = 0; i < KEY_COUNT; ++i) {
        /* Search from a different buffer: equality must compare contents. */
        snprintf(lookup, sizeof(lookup), "group-%03u-abcdefghijklmnop", i);
        BTreeKey* found = btreeSearch(tree, tree->root, lookup);
        check(found != NULL && found->dataOffset == i, "lookup after tree splits");
    }
    check(btreeSearch(tree, tree->root, "") == NULL, "missing key before first");
    check(btreeSearch(tree, tree->root, "group-128-abcdefghijklmnop_suffix") == NULL,
          "a shared prefix is not an equal key");
    check(btreeSearch(tree, tree->root, "zzz") == NULL, "missing key after last");
    btreeFree(&tree); /* borrowed names remain alive through tree destruction */
}

static void testParallel(const char* mode, char* inputPath, char* aggregation) {
    int withId = strcmp(mode, "id") == 0;
    check(withId || strcmp(mode, "group") == 0, "parallel mode");
    check(strcmp(aggregation, "sum") == 0 || strcmp(aggregation, "count") == 0 ||
          strcmp(aggregation, "avg") == 0, "aggregation type");
    TposeInputFile* input = tposeIOOpenInputFile(inputPath, '\t', TPOSE_IO_MODIFY_HEADER);
    check(input != NULL, "open fixture");
    TposeOutputFile* output = tposeIOOpenOutputFile("stdout", "w", '\t');
    check(output != NULL, "open output");
    TposeQuery* query = tposeIOQueryIndexedAlloc(input, output, withId ? 1 : -1, 2, 3, aggregation);
    check(query != NULL, "create parallel query");

    /* Supply two exact data partitions, split at ID 2. This invokes actual
       workers/reducers without a 1 GiB file or the separately reviewed splitter. */
    off_t length = input->fileSize - (input->dataAddr - input->fileAddr);
    off_t split = 0;
    for(off_t i = 1; i + 1 < length; ++i) {
        if(input->dataAddr[i - 1] == '\n' && input->dataAddr[i] == '2' &&
           input->dataAddr[i + 1] == '\t') {
            split = i;
            break;
        }
    }
    check(split > 0 && split < length, "fixture has two nonempty ID partitions");
    fileChunks = 2;
    partitions[0] = 0;
    partitions[1] = split;
    partitions[2] = length;
    btreeGlobal = btreeAlloc();
    check(btreeGlobal != NULL, "allocate shared tree");
    tposeIOUniqueGroupsParallel(query);
    if(withId)
        tposeIOTransposeGroupIdParallel(query);
    else
        tposeIOTransposeGroupParallel(query);

    btreeFree(&btreeGlobal);
    tposeIOCloseInputFile(input);
    tposeIOCloseOutputFile(output);
    tposeIOQueryFree(&query);
    check(fflush(stdout) == 0, "flush parallel output");
}

static void testPartitionId(const char* widthArg) {
    check(strcmp(widthArg, "4999") == 0 || strcmp(widthArg, "5000") == 0,
          "partition test width");
    size_t width = (size_t) strtoul(widthArg, NULL, 10);
    /* The partitioner starts its interior boundary at dataSize % chunkSize.
       Reserve virtual address space, touching only the rows at that boundary;
       the helper uses a 64 KiB chunk size, with no large file needed. */
    size_t tail = 16384;
    size_t length = (size_t) TPOSE_IO_CHUNK_SIZE + tail;
    char* data = mmap(NULL, length, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(data != MAP_FAILED, "map partition fixture");
    data[tail] = '\n';
    memset(data + tail + 1, '1', width);
    const char* rest = "\tA\t1\n2\tA\t1\n";
    memcpy(data + tail + 1 + width, rest, strlen(rest));
    TposeInputFile input = {0};
    input.fileAddr = input.dataAddr = data;
    input.fileSize = (off_t) length;
    input.fieldDelimiter = '\t';
    TposeQuery query = {0};
    query.inputFile = &input;
    query.id = 0;
    fileChunks = 0;
    check(tposeIOBuildPartitions(&query, TPOSE_IO_PARTITION_ID) == 0,
          "partition an ID at the width limit");
    check(fileChunks == 2 && partitions[1] > (off_t) tail &&
          partitions[1] < (off_t) (tail + width + strlen(rest) + 1),
          "partition boundary remains within test rows");
    check(munmap(data, length) == 0, "unmap partition fixture");
}

int main(int argc, char** argv) {
    if(argc == 1) {
        testTree();
        return EXIT_SUCCESS;
    }
    if(argc == 3 && strcmp(argv[1], "partition-id") == 0) {
        testPartitionId(argv[2]);
        return EXIT_SUCCESS;
    }
    if(argc == 5 && strcmp(argv[1], "eof") == 0) {
        testEofFile(argv[2], argv[3], argv[4]);
        return EXIT_SUCCESS;
    }
    check(argc == 4, "usage: group-keys-test [group|id input-file sum|count|avg] or partition-id width");
    testParallel(argv[1], argv[2], argv[3]);
    return EXIT_SUCCESS;
}
