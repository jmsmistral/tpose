# Shell smoke tests

Run `make test` from the repository root, using the same `CC` setting as the
build (for example, `make test CC=gcc-16` on macOS).

The suite uses GNU GCC, a POSIX shell, and standard Unix utilities (`mktemp`,
`awk`, `diff`, `grep`, `sed`, `tr`, `wc`, and `cmp` for the CI installation check). No Python
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
reduction, and both aggregation paths without creating a 1 GiB fixture. The
separate EOF tests below exercise automatic partition construction as well.

`make test-ubsan` runs the suite with GCC's UndefinedBehaviorSanitizer, configured
to fail immediately on a diagnostic. Use the same `CC` setting as for the other
targets. CI runs normal, AddressSanitizer, and UndefinedBehaviorSanitizer tests
on both platforms.

## Parser limit regression tests

The fixed-buffer write-limit finding is fixed. Buffers reserve their final byte
for the terminating zero: at most 4,999 bytes of field content are copied.
Capacity is checked before copying field contents in simple transpose, serial and
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
5,000-byte IDs. The test helper now uses 64 KiB chunks, so it only needs a small
anonymous mapping with test rows near the split. All limit tests run in the
normal, AddressSanitizer, and UndefinedBehaviorSanitizer suites.

## End-of-file regression tests

The EOF overread and dropped-final-record finding is fixed. Headers and data
records use a shared reader with an exclusive end pointer. It returns the last
record whether or not a newline terminates it, without inventing a record after
a trailing newline. Aggregation consumes each record once. Empty partitions do
not read input or emit an uninitialized ID. A header without data rows, or data
without any nonempty group values, fails with a diagnostic and status 1 instead
of asserting. Simple transpose accepts a single unterminated row or column.

`eof.sh` checks sum/count/average with and without a final newline, different
field orders, a new group or ID at EOF, repeated final IDs, an empty final field,
header-only and empty inputs, exact 4 KiB/16 KiB file sizes, and field-width
limits at EOF. `eof.c` places each fixture immediately before an inaccessible
memory page. This exposes overreads that ordinary mmap padding can hide, even
without a sanitizer. A byte-255 fixture also checks that the parallel output
reducer keeps `getc` results in an `int`, distinguishing every byte from EOF.

Make compiles only the C test helper with `TPOSE_IO_CHUNK_SIZE=65536`; the normal
CLI still uses 1 GiB chunks. Tests exercise actual partition construction and
worker/reducer execution using small fixtures with exact chunk lengths,
remainders, IDs spanning proposed boundaries, no later newline or distinct ID,
and guarded EOF. They assert that expected multi-partition cases really create
multiple partitions, and that all endpoints refer to data bytes within the
input. Automatically constructed empty/duplicate partitions are collapsed;
separate manual-partition tests cover empty workers at either end. Production
partition construction also rejects counts beyond the existing array capacities.

The shared field reader also fixes stale values in empty simple-transpose cells;
regressions cover interior and final empty cells. Indexed queries now initialize
absent selections to -1, as named queries already do,
so the shared aggregation path can reliably distinguish ID and group-only modes.

## Simple-transpose shape regression tests

The simple-transpose correctness finding is fixed. Every input row must have
the same number of fields as the first row. Explicitly empty fields count,
including one after a trailing delimiter; a blank line is one empty field.
The entire table is checked before any transposed data is written. A mismatch
exits with status 1 and reports the one-based row number and actual/expected
field counts. Output contains delimiters only between cells, preserving any
delimiter that is needed to represent a final empty cell.

`simple.sh` checks rectangular tables, empty cells and headers, tables of empty
cells, and blank rows in single-column input. Exact byte comparisons and
double-transpose round trips verify row/column counts and empty-cell positions.
Tests cover tabs and commas, both final-newline forms, and output files. Short,
wide, blank, and extra-empty-field rows must fail with an exact diagnostic and
no stdout output; tab cases also run against protected input memory. The prior
width-limit and EOF tests now expect the corrected output format.

## Output safety regression tests

The output-protection/error-status and predictable parallel-temporary-file
findings are fixed. The CLI validates field selections and option dependencies
before creating output. Named destinations are staged in a unique `.tpose-*`
file in the destination directory. Successful publication follows checked
writing, flushing, file synchronization, and closing. Existing results are
atomically replaced; new paths are published with a hard link so a concurrent
creator is not overwritten. Destination identity is checked again before
publication. Handled failures discard the staged file and preserve the old
result, or leave a previously absent destination absent.

Input/output aliases are checked by device and inode, including hard links and
input symlinks. Output symlinks (including dangling links), directories, and
special files are rejected. Named outputs require a writable destination
directory and a filesystem supporting sibling temporary files and rename/link.
Existing basic permission bits are retained, and new files honor the umask.
Atomic replacement creates a new inode: other hard links retain the old data,
and ownership, ACLs, and extended attributes are not copied. The identity check
is not a lock against concurrent in-place edits. Abrupt termination can leave
a private staged file; this is not a power-loss durability guarantee.

Stdout remains streaming. Write/flush/close failures return nonzero, including
broken pipes and file-size limits. Stdout aliases are rejected when detectable,
but a shell redirect such as `> input.tsv` can truncate the input before tpose
starts; use the explicit output argument for file protection. Parallel workers
use automatically cleaned `tmpfile()` streams, and reduction checks seek,
read, write, flush, and close failures rather than clearing error indicators.

`output.sh` checks existing and absent destinations after invalid queries,
parser failures, and write failures; successful contents and permission modes;
aliases and special paths; destination changes before publication; and staged
file cleanup. `output.c` uses a child with a four-byte file-size limit, a broken
pipe, and invalid descriptors to reproduce I/O failures portably. Diagnostics
are relayed through a pipe so the file-size limit cannot truncate them. Tests
also exercise input/output named `temp0.txt`, unrelated `temp1.txt`, failed
parallel writes, and concurrent parallel invocations in the same directory.

Supporting validation fixes reject zero, negative, and overflowing field
indexes and incomplete aggregation options. Help and error-help leave standard
streams to the registered exit handler, fixing the earlier double-close path.

This is an initial smoke suite, not comprehensive correctness or memory-safety
coverage. The remaining parsing, memory, and file-handling defects remain
separate fixes, including remaining header/numeric/CLI validation and ID ordering
semantics. Small inputs with the CLI's `-P`
only exercise its serial fallback, which is why the C helper tests parallel
execution directly with a smaller chunk threshold. These checks do not establish
full-scale performance, resource usage, or race freedom under all failure modes.
Leak detection is explicitly disabled for this memory-access check because
allocation leaks remain a separate review item. Broader sanitizer coverage
will accompany the remaining fixes.
