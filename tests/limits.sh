# Sourced by run.sh, inside its private temporary directory.
limit_output() {
    name=$1
    expected=$2
    shift 2
    "$@" > actual 2> stderr || fail "$name (exit status)"
    [ ! -s stderr ] || fail "$name (unexpected diagnostic)"
    diff -u "$expected" actual || fail "$name (output)"
    pass "$name"
}

limit_error() {
    name=$1
    diagnostic=$2
    shift 2
    if "$@" > actual 2> stderr; then
        fail "$name (invalid input accepted)"
    else
        status=$?
        [ "$status" -eq 1 ] || fail "$name (exit status)"
    fi
    # Exact diagnostics also ensure a sanitizer abort cannot count as success.
    printf '%s\n' "$diagnostic" > expected-error
    diff -u expected-error stderr || fail "$name (diagnostic)"
    pass "$name"
}

# Generate boundary values instead of keeping large repetitive fixtures in Git.
for width in 4999 5000 6000; do
    for field in simple header group numeric id; do
        awk -v width="$width" -v field="$field" 'BEGIN {
            for (i = 1; i <= width; i++) value = value "x"
            if (field == "simple" || field == "header") {
                first = field == "header" ? value : "left"
                second = field == "simple" ? value : "value"
                printf "%s\tright\n%s\tv\n", first, second > "limit-input.tsv"
                printf "%s\t%s\nright\tv\n", first, second > "limit-expected.tsv"
            } else {
                group = field == "group" ? value : "A"
                id = field == "id" ? value : "1"
                number = "2"
                if (field == "numeric") {
                    number = ""
                    for (i = 1; i < width; i++) number = number "0"
                    number = number "2"
                }
                # Only one worker gets invalid input, so diagnostics are deterministic.
                secondGroup = field == "group" && width >= 5000 ? "A" : group
                printf "id\tgroup\tamount\n%s\t%s\t%s\n2\t%s\t3\n", id, group, number, secondGroup > "limit-input.tsv"
                printf "%s\n5.00\n", group > "limit-group.tsv"
                printf "id\t%s\n%s\t2.00\n2\t3.00\n", group, id > "limit-id.tsv"
            }
        }'

        case "$field" in
            simple|header) label=Field ;;
            group) label='Group field' ;;
            numeric) label='Numeric field' ;;
            id) label='ID field' ;;
        esac
        diagnostic="Error: $label exceeds the maximum field width of 4999 bytes"
        if [ "$field" = simple ] || [ "$field" = header ]; then
            if [ "$width" -eq 4999 ]; then
                limit_output "$field width $width" limit-expected.tsv "$binary" limit-input.tsv
            else
                limit_error "$field width $width" "$diagnostic" "$binary" limit-input.tsv
            fi
            continue
        fi

        for mode in group id; do
            # ID values are only copied when aggregating by ID.
            [ "$field" != id ] || [ "$mode" = id ] || continue
            case "$mode" in group) id_option= ;; id) id_option=-I1 ;; esac
            if [ "$width" -eq 4999 ]; then
                limit_output "$mode $field width $width" "limit-$mode.tsv" \
                    "$binary" limit-input.tsv -i $id_option -G2 -N3
                limit_output "parallel $mode $field width $width" "limit-$mode.tsv" \
                    "$key_binary" "$mode" limit-input.tsv sum
            else
                limit_error "$mode $field width $width" "$diagnostic" \
                    "$binary" limit-input.tsv -i $id_option -G2 -N3
                limit_error "parallel $mode $field width $width" "$diagnostic" \
                    "$key_binary" "$mode" limit-input.tsv sum
            fi
        done
    done
done

for count in 5000 5001; do
    for layout in worker reduce; do
        # worker: all distinct groups in partition 1; reduce: each partition
        # fits on its own. A repeated g1 after reaching capacity must be valid.
        awk -v count="$count" -v layout="$layout" 'BEGIN {
            print "id\tgroup\tamount" > "limit-input.tsv"
            for (i = 1; i <= count; i++) {
                id = layout == "reduce" && i > 2500 ? 2 : 1
                printf "%d\tg%d\t1\n", id, i > "limit-input.tsv"
                printf "%sg%d", (i == 1 ? "" : "\t"), i > "limit-expected.tsv"
            }
            if (layout == "worker") print "1\tg1\t2" > "limit-input.tsv"
            print "2\tg1\t2" > "limit-input.tsv"
            print "" > "limit-expected.tsv"
            for (i = 1; i <= count; i++)
                printf "%s%.2f", (i == 1 ? "" : "\t"), (i == 1 ? (layout == "worker" ? 5 : 3) : 1) > "limit-expected.tsv"
            print "" > "limit-expected.tsv"
        }'
        if [ "$count" -eq 5000 ]; then
            limit_output "$layout $count groups" limit-expected.tsv \
                "$binary" limit-input.tsv -i -G2 -N3
            limit_output "parallel $layout $count groups" limit-expected.tsv \
                "$key_binary" group limit-input.tsv sum
        else
            diagnostic='Error: Input exceeds the maximum of 5000 distinct groups'
            limit_error "$layout $count groups" "$diagnostic" \
                "$binary" limit-input.tsv -i -G2 -N3
            limit_error "parallel $layout $count groups" "$diagnostic" \
                "$key_binary" group limit-input.tsv sum
        fi
    done
done

# Exercise the real ID partition scanner using a small virtual mapping in C.
: > empty
limit_output 'partition ID width 4999' empty "$key_binary" partition-id 4999
limit_error 'partition ID width 5000' \
    'Error: ID field exceeds the maximum field width of 4999 bytes' \
    "$key_binary" partition-id 5000
