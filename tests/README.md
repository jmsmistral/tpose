# Shell smoke tests

Run `make test` from the repository root, using the same `CC` setting as the
build (for example, `make test CC=gcc-16` on macOS).

The suite uses GNU GCC, a POSIX shell, and standard Unix utilities (`mktemp`,
`awk`, `diff`, `grep`, `sed`, `tr`, and `cmp` for the CI installation check). No Python
or test framework is required. Every run uses its own temporary directory,
which is removed on exit. A failed command, unexpected diagnostic, or output
mismatch fails the suite. Make also builds a small C test executable for direct
B-tree and parallel-worker checks; it uses the same compiler and no framework.

The fixtures reproduce the README's aggregation example. Checks cover version
output, a missing input file, group and ID aggregation, indexed fields, sum,
count, average, comma delimiters, prefixes/suffixes, and writing an output file.
Comparisons preserve tabs, empty fields, and final newlines. Only the optional
minus sign on `nan` is normalized for differences between C libraries.

`make test-asan` runs the same suite against a separate GCC AddressSanitizer
binary, `tpose-asan` (use `CC=gcc-16` on macOS as above). GCC's AddressSanitizer
runtime must be available. Both CI jobs run this check. It detects invalid
memory access even when ordinary output comparisons would pass.

The string-storage fixture exercises longer header names and short group names,
leaving most of the output header's allocated pointer slots unused. Together
with AddressSanitizer, it guards against the reviewed off-by-one heap string
allocations and invalid frees of unused header slots. Those two findings are
fixed; the header tokenizer also now uses a properly terminated delimiter
string.

## Full group-name regression tests

The B-tree now compares complete, case-sensitive group names, and borrows the
stable strings owned by the output headers. Columns still follow first
appearance in the input. The former hash-collision and signed-hash-overflow
findings are fixed: no hash arithmetic remains in group discovery or lookup.

`group-keys.tsv` includes two pairs that collided under the old hash (`Aa` / `B<`
and `Az` / `BU`), repeated names, case differences, and long names sharing a
prefix. Expected outputs check sum, count, and average, both with and without
IDs. `tests/group_keys.c` also checks 256 keys inserted in a permutation, forcing
B-tree splits and looking up strings from different buffers.

The C executable supplies two exact partitions to the production parallel
coordinators, then the shell compares their results with the same expected
outputs as the serial commands. One collision pair spans the two partitions;
others occur within each partition. This exercises worker discovery, header
reduction, and both aggregation paths without creating a 1 GiB fixture. It does
not validate general automatic file partitioning or its boundary handling.

`make test-ubsan` runs the suite with GCC's UndefinedBehaviorSanitizer, configured
to fail immediately on a diagnostic. Use the same `CC` setting as for the other
targets. CI runs normal, AddressSanitizer, and UndefinedBehaviorSanitizer tests
on both platforms.

## Parser limit regression tests

The fixed-buffer write-limit finding is fixed. Buffers reserve their final byte
for the terminating zero: at most 4,999 bytes of field content are copied.
Capacity is checked before appending a byte in simple transpose, serial and
parallel aggregation, group discovery/reduction, and ID partition scanning.
Group discovery checks capacity before inserting a new group, both within each
worker and when combining workers, allowing at most 5,000 distinct groups.
Existing groups remain usable when capacity is full.

`limits.sh` generates fixtures with standard `awk` in the suite's temporary
directory. It checks exact output at 4,999 bytes and rejection at 5,000 and 6,000
bytes for simple cells/headers and selected group, numeric, and ID values.
Numeric boundary values use leading zeros to keep the numeric value small.
Grouped header names are dynamically allocated; unused aggregation columns do
not enter fixed field buffers and are not subject to this width check.

Group-count tests check correct totals and first-seen column order at 5,000
groups, including duplicates, and rejection at 5,001. Parallel cases cover both
a worker exceeding capacity and the union of individually valid workers
exceeding capacity. Error tests require exit status 1 and the exact diagnostic,
so sanitizer failures cannot count as successful rejection.

A focused C probe also exercises the real ID partition scanner with 4,999- and
5,000-byte IDs. It reserves sparse anonymous virtual memory and touches only a
few test rows near the split; it does not create or scan a 1 GiB file. This checks
the ID buffer limit, not the partitioner's broader correctness. All limit
tests run in the normal, AddressSanitizer, and UndefinedBehaviorSanitizer suites.

This is an initial smoke suite, not comprehensive correctness or memory-safety
coverage. The remaining parsing, memory, and file-handling defects remain
separate fixes. End-of-file handling, empty cells/ragged rows, help/error-help,
and general automatic partition construction are not covered yet. Limit errors
may leave partial output; atomic output and temporary-file cleanup remain separate
review items. Small inputs with the CLI's `-P` would only exercise the serial
fallback, which is why the parallel tests call the coordinators directly.
Leak detection is explicitly disabled for this memory-access check because
allocation leaks remain a separate review item. Broader sanitizer coverage
will accompany the remaining fixes.
