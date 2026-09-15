# Destination preservation and portable I/O failure regressions.
output_error() {
    name=$1
    diagnostic=$2
    shift 2
    if "$@" > actual 2> stderr; then fail "$name (unexpected success)"; else
        status=$?
        [ "$status" -eq 1 ] || fail "$name (exit status)"
    fi
    grep -F "$diagnostic" stderr > /dev/null || fail "$name (diagnostic)"
    if grep -E 'AddressSanitizer|runtime error:|Test setup failed' stderr > /dev/null; then
        fail "$name (unexpected runtime failure)"
    fi
    pass "$name"
}

no_staged_output() {
    for stage in .tpose-* 'output directory'/.tpose-*; do
        [ ! -e "$stage" ] || fail "staged output left behind: $stage"
    done
}

mkdir 'output directory'
printf 'a\tb\n1\t2\n3\t4\n' > output-input.tsv
cp output-input.tsv output-input-copy.tsv
printf 'a\t1\t3\nb\t2\t4\n' > output-expected.tsv
printf 'keep this result\n' > output-sentinel
printf 'id\tgroup\tamount\n1\tA\t2\n2\tA\t4\n' > output-group.tsv

for invalid in unknown zero negative overflow dependency aggregate; do
    cp output-sentinel protected.tsv
    case "$invalid" in
        unknown) set -- -Gmissing -Namount; diagnostic='do not match input fields' ;;
        zero) set -- -i -I0 -G2 -N3; diagnostic='requires a positive integer' ;;
        negative) set -- -i -I-2 -G2 -N3; diagnostic='requires a positive integer' ;;
        overflow) set -- -i -I4294967297 -G2 -N3; diagnostic='requires a positive integer' ;;
        dependency) set -- -Namount; diagnostic='GROUP and NUMERIC fields need to be specified' ;;
        aggregate) set -- -Ggroup -Namount -abad; diagnostic='requires either' ;;
    esac
    output_error "invalid query preserves output: $invalid" "$diagnostic" \
        "$binary" output-group.tsv protected.tsv "$@"
    cmp output-sentinel protected.tsv || fail 'invalid query changed existing output'
    output_error "invalid query creates no output: $invalid" "$diagnostic" \
        "$binary" output-group.tsv nonexistent.tsv "$@"
    [ ! -e nonexistent.tsv ] || fail 'invalid query created output'
    [ ! -s actual ] || fail 'invalid query wrote to stdout'
    no_staged_output
done

ln output-input.tsv input-hardlink.tsv
ln -s output-input.tsv input-symlink.tsv
for alias in output-input.tsv input-hardlink.tsv input-symlink.tsv; do
    output_error "reject input alias $alias" 'Error:' "$binary" output-input.tsv "$alias"
    cmp output-input-copy.tsv output-input.tsv || fail 'alias damaged input'
    [ ! -s actual ] || fail 'alias wrote to stdout'
    no_staged_output
done
output_error 'input symlink to output' 'same file' "$binary" input-symlink.tsv output-input.tsv
cmp output-input-copy.tsv output-input.tsv || fail 'input symlink damaged input'
output_error 'stdout append alias rejected' 'same file' \
    sh -c 'exec "$1" "$2" >> "$2"' sh "$binary" output-input.tsv
cmp output-input-copy.tsv output-input.tsv || fail 'stdout alias damaged input'
output_error 'missing destination directory' 'Cannot create temporary' \
    "$binary" output-input.tsv missing-parent/result.tsv
no_staged_output

cp output-sentinel protected.tsv
ln -s protected.tsv output-symlink.tsv
ln -s missing-target.tsv dangling-output.tsv
mkdir directory-output
mkfifo fifo-output
for special in output-symlink.tsv dangling-output.tsv directory-output fifo-output; do
    output_error "reject special destination $special" 'Output must be a regular file' \
        "$binary" output-input.tsv "$special"
    no_staged_output
done
cmp output-sentinel protected.tsv || fail 'output symlink target changed'
[ -L dangling-output.tsv ] && [ ! -e missing-target.tsv ] || fail 'dangling symlink changed'
[ -d directory-output ] && [ -p fifo-output ] || fail 'special destination changed'

# Failure after opening a transaction, including after some cells were written.
printf 'a\tb\n1\t2\n3' > invalid-shape.tsv
awk 'BEGIN { print "a\tb"; printf "1\t"; for(i=0;i<5000;i++) printf "x"; print "" }' > invalid-width.tsv
for invalid in shape width; do
    cp output-sentinel 'output directory/protected result.tsv'
    output_error "runtime $invalid preserves output" 'Error:' \
        "$binary" "invalid-$invalid.tsv" 'output directory/protected result.tsv'
    cmp output-sentinel 'output directory/protected result.tsv' || fail 'runtime error changed existing output'
    output_error "runtime $invalid creates no output" 'Error:' \
        "$binary" "invalid-$invalid.tsv" 'output directory/new result.tsv'
    [ ! -e 'output directory/new result.tsv' ] || fail 'runtime error published new output'
    no_staged_output
done

# Atomic replacement updates only the chosen directory entry, retaining modes.
cp output-sentinel 'output directory/protected result.tsv'
chmod 640 'output directory/protected result.tsv'
ln 'output directory/protected result.tsv' original-result-link
limit_output 'replace existing result' empty "$binary" output-input.tsv 'output directory/protected result.tsv'
cmp output-expected.tsv 'output directory/protected result.tsv' || fail 'replacement contents'
cmp output-sentinel original-result-link || fail 'replacement modified other hard link'
limit_output 'replacement permissions' empty "$key_binary" output permissions 'output directory/protected result.tsv' 640
(
    umask 027
    "$binary" output-input.tsv 'output directory/new result.tsv'
) > actual 2> stderr || fail 'create result with umask'
cmp output-expected.tsv 'output directory/new result.tsv' || fail 'new result contents'
[ ! -s actual ] && [ ! -s stderr ] || fail 'unexpected output creating result'
limit_output 'new result permissions' empty "$key_binary" output permissions 'output directory/new result.tsv' 640
no_staged_output

for mode in simple group id; do
    case "$mode" in
        simple) set -- output-input.tsv ;;
        group) set -- output-group.tsv -Ggroup -Namount ;;
        id) set -- output-group.tsv -Iid -Ggroup -Namount ;;
    esac
    input=$1
    shift
    cp output-sentinel protected.tsv
    output_error "limited $mode preserves output" 'Error: Cannot' \
        "$key_binary" output limited "$binary" "$input" protected.tsv "$@"
    cmp output-sentinel protected.tsv || fail 'failed write changed existing output'
    output_error "limited $mode creates no output" 'Error: Cannot' \
        "$key_binary" output limited "$binary" "$input" nonexistent.tsv "$@"
    [ ! -e nonexistent.tsv ] || fail 'failed write published new output'
    output_error "limited $mode stdout fails" 'Error: Cannot' \
        "$key_binary" output limited "$binary" "$input" "$@"
    output_error "broken pipe $mode fails" 'Error: Cannot' \
        "$key_binary" output broken-pipe "$binary" "$input" "$@"
    no_staged_output
done
output_error 'closed stdout fails' 'Error:' "$key_binary" output closed-stdout "$binary" output-input.tsv
output_error 'fclose failure propagates' 'Cannot flush or close output stream' "$key_binary" output close
output_error 'parallel read failure propagates' 'Cannot read parallel output stream' "$key_binary" output parallel-read

for conflict in changed appeared directory; do
    if [ "$conflict" != appeared ]; then cp output-sentinel "conflict-$conflict.tsv"; fi
    output_error "destination $conflict during processing" 'Output destination changed' \
        "$key_binary" output "$conflict" output-input.tsv "conflict-$conflict.tsv"
    if [ "$conflict" = directory ]; then
        [ -d "conflict-$conflict.tsv" ] || fail 'conflicting directory replaced'
    else
        printf 'concurrent\n' > concurrent-expected
        cmp concurrent-expected "conflict-$conflict.tsv" || fail 'concurrent result replaced'
    fi
    if [ "$conflict" != appeared ]; then cmp output-sentinel saved-destination || fail 'original destination changed'; fi
    no_staged_output
done

# Help no longer closes a standard stream twice (also exercised by invalid queries).
"$binary" --help > actual 2> stderr || fail 'help exit status'
[ ! -s stderr ] || fail 'help diagnostic'
grep -q 'Usage:' actual || fail 'help contents'
pass 'help stream ownership'

# Scratch streams must not touch the old predictable names, even when those
# names are the input or the destination itself.
printf 'id\tA\n1\t2.00\n2\t4.00\n' > parallel-expected.tsv
cp output-group.tsv temp0.txt
cp output-sentinel temp1.txt
limit_output 'parallel input named temp0.txt' empty \
    "$key_binary" output parallel-destination temp0.txt parallel-result.tsv
cmp output-group.tsv temp0.txt || fail 'worker damaged input named temp0.txt'
cmp output-sentinel temp1.txt || fail 'worker damaged unrelated temp1.txt'
cmp parallel-expected.tsv parallel-result.tsv || fail 'parallel result contents'
limit_output 'parallel destination named temp0.txt' empty \
    "$key_binary" output parallel-destination output-group.tsv temp0.txt
cmp parallel-expected.tsv temp0.txt || fail 'parallel destination contents'
cp output-sentinel temp0.txt
output_error 'parallel failure preserves temp0.txt destination' 'Error: Cannot' \
    "$key_binary" output limited "$key_binary" output parallel-destination output-group.tsv temp0.txt
cmp output-sentinel temp0.txt || fail 'parallel failure damaged destination'
cmp output-sentinel temp1.txt || fail 'parallel failure damaged unrelated file'
no_staged_output

"$key_binary" output parallel-destination output-group.tsv concurrent-one.tsv > first-stdout 2> first-stderr &
first_pid=$!
"$key_binary" output parallel-destination output-group.tsv concurrent-two.tsv > second-stdout 2> second-stderr &
second_pid=$!
wait "$first_pid" || fail 'first concurrent transpose'
wait "$second_pid" || fail 'second concurrent transpose'
cmp parallel-expected.tsv concurrent-one.tsv || fail 'first concurrent result'
cmp parallel-expected.tsv concurrent-two.tsv || fail 'second concurrent result'
[ ! -s first-stdout ] && [ ! -s first-stderr ] && [ ! -s second-stdout ] && [ ! -s second-stderr ] || fail 'concurrent diagnostics'
cmp output-sentinel temp0.txt || fail 'concurrent run changed temp0.txt'
cmp output-sentinel temp1.txt || fail 'concurrent run changed temp1.txt'
no_staged_output
pass 'concurrent parallel invocations keep scratch storage separate'

# Isolate a worker write failure from the rest of the suite.
mkdir worker-fault
cp output-group.tsv worker-fault/input.tsv
(
    cd worker-fault
    output_error 'parallel worker write failure propagates' 'Error: Cannot' \
        "$key_binary" output limited "$key_binary" eof parallel-id input.tsv sum
)
# The subshell has its own counter, so record the successful case here.
passed=$((passed + 1))
