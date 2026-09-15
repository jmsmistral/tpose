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

This is an initial smoke suite, not comprehensive correctness or memory-safety
coverage. The known hashing, parsing, memory, and file-handling defects remain
separate fixes. Add focused regression tests with those fixes; simple transpose,
help/error-help paths, malformed input, and actual parallel processing are not
covered yet. Small inputs with `-P` would only exercise the serial fallback.
Sanitizer checks will be added after the known memory defects are addressed.
