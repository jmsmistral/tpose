#!/bin/sh
# Small CLI smoke suite using only the shell and standard Unix utilities.
set -eu
LC_ALL=C
export LC_ALL

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
binary=${1:-./tpose}
key_binary=${2:-./tests/group-keys-test}
case "$binary" in
    /*) ;;
    *) binary="$(pwd)/$binary" ;;
esac
case "$key_binary" in
    /*) ;;
    *) key_binary="$(pwd)/$key_binary" ;;
esac
if [ ! -x "$binary" ]; then
    printf 'Executable not found: %s\nRun make first.\n' "$binary" >&2
    exit 1
fi
if [ ! -x "$key_binary" ]; then
    printf 'Test executable not found: %s\nRun make test first.\n' "$key_binary" >&2
    exit 1
fi

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/tpose-tests.XXXXXX")
trap 'rm -rf "$work_dir"' 0
trap 'exit 1' HUP INT TERM
cd "$work_dir"
fixtures="$test_dir/fixtures"
passed=0

pass() {
    passed=$((passed + 1))
    printf 'ok %s - %s\n' "$passed" "$1"
}

fail() {
    printf 'FAIL: %s\n' "$1" >&2
    cat stderr >&2
    exit 1
}

check_output() {
    name=$1
    expected=$2
    shift 2
    "$binary" "$@" > actual 2> stderr || fail "$name (exit status)"
    [ ! -s stderr ] || fail "$name (unexpected diagnostic)"
    diff -u "$expected" actual || fail "$name (output)"
    pass "$name"
}

"$binary" --version > actual 2> stderr || fail 'version (exit status)'
grep -q '^tpose version ' actual || fail 'version (output)'
[ ! -s stderr ] || fail 'version (unexpected diagnostic)'
pass 'version'

if "$binary" "$work_dir/missing.tsv" > actual 2> stderr; then
    fail 'missing input file accepted'
else
    status=$?
    [ "$status" -eq 1 ] || fail 'missing input file (exit status)'
fi
[ ! -s actual ] || fail 'missing input file wrote to stdout'
grep -q 'Can not open input file' stderr || fail 'missing input file (diagnostic)'
pass 'missing input file'

check_output 'group sum' "$fixtures/group-sum.tsv" \
    "$fixtures/input.tsv" -Grevenue_group -Namount
check_output 'ID and group sum' "$fixtures/id-sum.tsv" \
    "$fixtures/input.tsv" -Icustomer_id -Grevenue_group -Namount
check_output 'indexed fields' "$fixtures/id-sum.tsv" \
    "$fixtures/input.tsv" -i -I1 -G2 -N3
check_output 'count' "$fixtures/id-count.tsv" \
    "$fixtures/input.tsv" -i -I1 -G2 -N3 -acount

# The sign of a printed NaN differs between C libraries.
"$binary" "$fixtures/input.tsv" -i -I1 -G2 -N3 -aavg > actual 2> stderr || fail 'average (exit status)'
[ ! -s stderr ] || fail 'average (unexpected diagnostic)'
sed 's/-nan/nan/g' actual > normalized
diff -u "$fixtures/id-avg.tsv" normalized || fail 'average (output)'
pass 'average'

tr '\t' ',' < "$fixtures/input.tsv" > input.csv
tr '\t' ',' < "$fixtures/id-sum.tsv" > expected.csv
check_output 'comma delimiter' expected.csv input.csv -d, -i -I1 -G2 -N3
check_output 'prefix and suffix' "$fixtures/id-affixes.tsv" \
    "$fixtures/input.tsv" -i -I1 -G2 -N3 -ppre_ -s_post

# Longer header allocations and short group names exercise string termination
# and cleanup of a header with many unused slots (checked by test-asan).
check_output 'header and group string storage' "$fixtures/string-storage-expected.tsv" \
    "$fixtures/string-storage.tsv" -Icustomer_identifier -Gproduct_category -Ntransaction_amount

"$binary" "$fixtures/input.tsv" result.tsv -i -I1 -G2 -N3 > actual 2> stderr || fail 'output file (exit status)'
[ ! -s actual ] || fail 'output file wrote to stdout'
[ ! -s stderr ] || fail 'output file (unexpected diagnostic)'
diff -u "$fixtures/id-sum.tsv" result.tsv || fail 'output file (contents)'
pass 'output file'

"$key_binary" > actual 2> stderr || fail 'B-tree string keys'
[ ! -s actual ] && [ ! -s stderr ] || fail 'B-tree string keys (unexpected output)'
pass 'B-tree string keys across splits'

for mode in group id; do
    case "$mode" in
        group) expected_prefix=group-keys; id_option= ;;
        id) expected_prefix=id-group-keys; id_option=-I1 ;;
    esac
    for aggregation in sum count avg; do
        expected="$fixtures/$expected_prefix-$aggregation.tsv"
        # id_option is either empty or one fixed option, so splitting is intentional.
        "$binary" "$fixtures/group-keys.tsv" -i $id_option -G2 -N3 "-a$aggregation" > actual 2> stderr || fail "$mode keys $aggregation (exit status)"
        [ ! -s stderr ] || fail "$mode keys $aggregation (unexpected diagnostic)"
        sed 's/-nan/nan/g' actual > normalized
        diff -u "$expected" normalized || fail "$mode keys $aggregation (output)"
        pass "$mode keys $aggregation"

        "$key_binary" "$mode" "$fixtures/group-keys.tsv" "$aggregation" > actual 2> stderr || fail "parallel $mode keys $aggregation (exit status)"
        [ ! -s stderr ] || fail "parallel $mode keys $aggregation (unexpected diagnostic)"
        sed 's/-nan/nan/g' actual > normalized
        diff -u "$expected" normalized || fail "parallel $mode keys $aggregation (output)"
        pass "parallel $mode keys $aggregation"
    done
done

printf 'Passed %s tests.\n' "$passed"
