# Shell smoke tests

Run `make test` from the repository root, using the same `CC` setting as the
build (for example, `make test CC=gcc-16` on macOS).

The suite needs only a POSIX shell and standard Unix utilities (`mktemp`,
`diff`, `grep`, `sed`, `tr`, and `cmp` for the CI installation check). No Python
or test framework is required. Every run uses its own temporary directory,
which is removed on exit. A failed command, unexpected diagnostic, or output
mismatch fails the suite.

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

This is an initial smoke suite, not comprehensive correctness or memory-safety
coverage. The remaining hashing, parsing, memory, and file-handling defects remain
separate fixes. Add focused regression tests with those fixes; simple transpose,
help/error-help paths, malformed input, and actual parallel processing are not
covered yet. Small inputs with `-P` would only exercise the serial fallback.
Leak detection is explicitly disabled for this memory-access check because
allocation leaks remain a separate review item. UndefinedBehaviorSanitizer and
broader sanitizer coverage are deferred to the remaining fixes.
