/* Transactional named output. Worker scratch files use the separate I/O API.
   SPDX-License-Identifier: GPL-3.0-or-later */
#include "tpose_io.h"

typedef struct TposeOutputTransaction {
    char* destination;
    char* temporary;
    int existed;
    struct stat original;
    mode_t mode;
} TposeOutputTransaction;

/* The CLI has one destination. Fatal parser/worker errors call exit(), so
   register its temporary path for cleanup independently of the success path. */
static TposeOutputTransaction* pending;
static int cleanupRegistered;

static void cleanupOutput(void) {
    if(pending && pending->temporary) unlink(pending->temporary);
}

static int outputError(const char* action, const char* path) {
    int error = errno ? errno : EIO;
    fprintf(stderr, "Error: Cannot %s output '%s': %s\n", action, path, strerror(error));
    return -1;
}

static int sameFile(const struct stat* a, const struct stat* b) {
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

void tposeIODiscardOutput(TposeOutputFile* output) {
    TposeOutputTransaction* transaction = output->transaction;
    if(!transaction) return;
    if(output->fd) fclose(output->fd);
    output->fd = NULL;
    if(transaction->temporary) unlink(transaction->temporary);
    if(pending == transaction) pending = NULL;
    free(transaction->temporary);
    free(transaction->destination);
    free(transaction);
    output->transaction = NULL;
}

TposeOutputFile* tposeIOOpenDestination(TposeInputFile* input, const char* path,
                                       unsigned char delimiter) {
    struct stat inputStat, destinationStat;
    if(fstat(input->fd, &inputStat) != 0) {
        outputError("check input identity for", path);
        return NULL;
    }
    if(strcmp(path, "stdout") == 0) {
        if(fstat(fileno(stdout), &destinationStat) != 0) {
            outputError("inspect", "stdout");
            return NULL;
        }
        if(sameFile(&inputStat, &destinationStat)) {
            fprintf(stderr, "Error: Input and output refer to the same file\n");
            return NULL;
        }
        return tposeIOOutputFileAlloc(stdout, delimiter);
    }
    int existed = lstat(path, &destinationStat) == 0;
    if(!existed && errno != ENOENT) {
        outputError("inspect", path);
        return NULL;
    }
    if(existed) {
        /* Do not follow an output symlink or replace devices/FIFOs/directories. */
        if(!S_ISREG(destinationStat.st_mode)) {
            fprintf(stderr, "Error: Output must be a regular file, not a symlink or special file\n");
            return NULL;
        }
        if(sameFile(&inputStat, &destinationStat)) {
            fprintf(stderr, "Error: Input and output refer to the same file\n");
            return NULL;
        }
        if(access(path, W_OK) != 0) {
            outputError("write", path);
            return NULL;
        }
    }
    if(pending) {
        fprintf(stderr, "Error: An output transaction is already active\n");
        return NULL;
    }
    if(!cleanupRegistered) {
        if(atexit(cleanupOutput) != 0) {
            fprintf(stderr, "Error: Cannot register output cleanup\n");
            return NULL;
        }
        cleanupRegistered = 1;
    }
    TposeOutputFile* output = tposeIOOutputFileAlloc(stdout, delimiter);
    if(!output) return NULL;
    output->fd = NULL;
    output->transaction = calloc(1, sizeof(TposeOutputTransaction));
    if(!output->transaction) {
        outputError("allocate", path);
        tposeIOOutputFileFree(&output);
        return NULL;
    }
    TposeOutputTransaction* transaction = output->transaction;
    transaction->existed = existed;
    if(existed) transaction->original = destinationStat;
    mode_t mask = umask(0);
    umask(mask); /* This runs before worker threads are started. */
    transaction->mode = existed ? destinationStat.st_mode & 0777 : 0666 & ~mask;
    transaction->destination = strdup(path);
    const char* slash = strrchr(path, '/');
    size_t directoryLength = slash ? (size_t) (slash - path + 1) : 0;
    transaction->temporary = malloc(directoryLength + sizeof(".tpose-XXXXXX"));
    if(!transaction->destination || !transaction->temporary) {
        /* Do not let discard treat an uninitialized buffer as a pathname. */
        free(transaction->temporary);
        transaction->temporary = NULL;
        outputError("allocate", path);
        tposeIOOutputFileFree(&output);
        return NULL;
    }
    memcpy(transaction->temporary, path, directoryLength);
    strcpy(transaction->temporary + directoryLength, ".tpose-XXXXXX");
    int fd = mkstemp(transaction->temporary);
    if(fd < 0) {
        /* mkstemp did not give us ownership of any file. */
        free(transaction->temporary);
        transaction->temporary = NULL;
        outputError("create temporary", path);
        tposeIOOutputFileFree(&output);
        return NULL;
    }
    pending = transaction;
    output->fd = fdopen(fd, "w");
    if(!output->fd) {
        int error = errno;
        close(fd);
        errno = error;
        outputError("open temporary", path);
        tposeIOOutputFileFree(&output);
        return NULL;
    }
    return output;
}

int tposeIOCommitOutput(TposeOutputFile* output) {
    TposeOutputTransaction* transaction = output->transaction;
    if(!transaction) {
        tposeIOFlushOutput(output->fd);
        return 0;
    }
    int error = ferror(output->fd) ? EIO : 0;
    if(fflush(output->fd) != 0 && !error) error = errno;
    if(!error && fchmod(fileno(output->fd), transaction->mode) != 0) error = errno;
    if(!error && fsync(fileno(output->fd)) != 0) error = errno;
    if(fclose(output->fd) != 0 && !error) error = errno;
    output->fd = NULL;
    if(error) {
        errno = error;
        return outputError("finish", transaction->destination);
    }
    struct stat current;
    int exists = lstat(transaction->destination, &current) == 0;
    if(!exists && errno != ENOENT) return outputError("inspect", transaction->destination);
    if(exists != transaction->existed ||
       (exists && (!S_ISREG(current.st_mode) || !sameFile(&current, &transaction->original)))) {
        fprintf(stderr, "Error: Output destination changed while processing\n");
        return -1;
    }
    if(transaction->existed) {
        if(rename(transaction->temporary, transaction->destination) != 0)
            return outputError("replace", transaction->destination);
    } else {
        /* link creates a new destination without overwriting a concurrent creator. */
        if(link(transaction->temporary, transaction->destination) != 0)
            return outputError("publish", transaction->destination);
    }
    tposeIODiscardOutput(output);
    return 0;
}
