# Sourced after limits.sh, reusing its exact-output and error helpers.
eof_group_case() {
    case_name=$1
    input=$2
    mode=$3
    expected=$4
    aggregation=$5
    case "$mode" in *id) id_option=-Iid ;; *) id_option= ;; esac
    limit_output "$case_name CLI $mode $aggregation" "$expected" \
        "$binary" "$input" $id_option -Ggroup -Namount "-a$aggregation"
    limit_output "$case_name guarded $mode $aggregation" "$expected" \
        "$key_binary" eof "$mode" "$input" "$aggregation"
}

for ending in newline eof; do
    for layout in numeric-last group-last id-last; do
        case "$layout" in
            numeric-last) printf 'id\tgroup\tamount\n1\tA\t2\n2\tA\t4' > eof-input.tsv ;;
            group-last) printf 'id\tamount\tgroup\n1\t2\tA\n2\t4\tA' > eof-input.tsv ;;
            id-last) printf 'group\tamount\tid\nA\t2\t1\nA\t4\t2' > eof-input.tsv ;;
        esac
        [ "$ending" != newline ] || printf '\n' >> eof-input.tsv
        for aggregation in sum count avg; do
            case "$aggregation" in
                sum) printf 'A\n6.00\n' > eof-group.tsv; printf 'id\tA\n1\t2.00\n2\t4.00\n' > eof-id.tsv ;;
                count) printf 'A\n2\n' > eof-group.tsv; printf 'id\tA\n1\t1\n2\t1\n' > eof-id.tsv ;;
                avg) printf 'A\n3.00\n' > eof-group.tsv; printf 'id\tA\n1\t2.00\n2\t4.00\n' > eof-id.tsv ;;
            esac
            for mode in group id parallel-group parallel-id; do
                case "$mode" in *id) expected=eof-id.tsv ;; *) expected=eof-group.tsv ;; esac
                eof_group_case "$ending $layout" eof-input.tsv "$mode" "$expected" "$aggregation"
            done
        done
    done
done

# Empty partitions at both ends must neither read data nor print a phantom ID.
printf 'id\tgroup\tamount\n1\tA\t2\n1\tA\t4' > eof-input.tsv
printf 'A\n6.00\n' > eof-group.tsv
printf 'id\tA\n1\t6.00\n' > eof-id.tsv
eof_group_case 'last record shares ID' eof-input.tsv empty-id eof-id.tsv sum
eof_group_case 'empty partitions' eof-input.tsv empty-group eof-group.tsv sum

# Final groups must be discovered, even in column 1 or in the final field.
printf 'group\tamount\nA\t2\nB\t4' > eof-input.tsv
printf 'A\tB\n2.00\t4.00\n' > eof-expected.tsv
eof_group_case 'group first and new group at EOF' eof-input.tsv group eof-expected.tsv sum
printf 'id\tamount\tgroup\n1\t2\tA\n2\t4\tB' > eof-input.tsv
eof_group_case 'new final-field group at EOF' eof-input.tsv parallel-group eof-expected.tsv sum

# EOF and an explicitly empty final field have the same missing-value policy.
printf 'id\tgroup\tamount\n1\tA\t2\n2\tA\t' > eof-input.tsv
printf 'A\n2.00\n' > eof-expected.tsv
eof_group_case 'empty final numeric field' eof-input.tsv group eof-expected.tsv sum

# A byte with value 255 is data, not stdio EOF, in the parallel output reducer.
printf 'id\tgroup\tamount\n1\t\377\t2\n2\t\377\t4' > eof-input.tsv
printf 'id\t\377\n1\t2.00\n2\t4.00\n' > eof-expected.tsv
eof_group_case 'byte 255 in output' eof-input.tsv parallel-id eof-expected.tsv sum

for contents in one-byte one-row one-column records empty-cell interior-empty; do
    case "$contents" in
        one-byte) printf x > eof-input.tsv; printf 'x\t\n' > eof-expected.tsv ;;
        one-row) printf 'a\tb' > eof-input.tsv; printf 'a\t\nb\t\n' > eof-expected.tsv ;;
        one-column) printf 'a\n1' > eof-input.tsv; printf 'a\t1\t\n' > eof-expected.tsv ;;
        records) printf 'a\tb\n1\t2' > eof-input.tsv; printf 'a\t1\t\nb\t2\t\n' > eof-expected.tsv ;;
        empty-cell) printf 'a\tb\n1\t' > eof-input.tsv; printf 'a\t1\t\nb\t\t\n' > eof-expected.tsv ;;
        interior-empty) printf 'a\tb\n1\t\n\t4' > eof-input.tsv; printf 'a\t1\t\t\nb\t\t4\t\n' > eof-expected.tsv ;;
    esac
    limit_output "simple EOF $contents" eof-expected.tsv "$binary" eof-input.tsv
    limit_output "guarded simple EOF $contents" eof-expected.tsv "$key_binary" eof simple eof-input.tsv sum
done

for ending in newline eof; do
    printf 'id\tgroup\tamount' > eof-input.tsv
    [ "$ending" != newline ] || printf '\n' >> eof-input.tsv
    for mode in group id; do
        case "$mode" in id) id_option=-Iid ;; *) id_option= ;; esac
        diagnostic='Error: Input contains a header but no data rows'
        limit_error "header-only $ending $mode" "$diagnostic" \
            "$binary" eof-input.tsv $id_option -Ggroup -Namount
        limit_error "guarded header-only $ending $mode" "$diagnostic" \
            "$key_binary" eof "$mode" eof-input.tsv sum
    done
done
: > eof-empty.tsv
limit_error 'empty file' 'Error: No data found in input file eof-empty.tsv' "$binary" eof-empty.tsv

printf 'id\tgroup\tamount\n\n' > eof-input.tsv
limit_error 'blank data rows' 'Error: Input contains no nonempty group values' \
    "$binary" eof-input.tsv -Ggroup -Namount
limit_error 'guarded blank data rows' 'Error: Input contains no nonempty group values' \
    "$key_binary" eof empty-group eof-input.tsv sum

for width in 4999 5000; do
    awk -v width="$width" 'BEGIN {
        printf "id\tamount\tgroup\n1\t2\t" > "eof-input.tsv"
        for(i = 0; i < width; i++) {
            printf "x" > "eof-input.tsv"
            printf "x" > "eof-expected.tsv"
        }
        printf "\n2.00\n" > "eof-expected.tsv"
    }'
    if [ "$width" -eq 4999 ]; then
        eof_group_case 'maximum-width final group' eof-input.tsv group eof-expected.tsv sum
    else
        limit_error 'oversized final group' 'Error: Group field exceeds the maximum field width of 4999 bytes' \
            "$binary" eof-input.tsv -Ggroup -Namount
        limit_error 'guarded oversized final group' 'Error: Group field exceeds the maximum field width of 4999 bytes' \
            "$key_binary" eof group eof-input.tsv sum
    fi
done

# Exact 4 KiB and 16 KiB file lengths catch reads hidden by mmap page padding.
for bytes in 4096 16384; do
    for ending in newline eof; do
        awk -v bytes="$bytes" -v ending="$ending" 'BEGIN {
            printf "id\tgroup\tamount\n" > "eof-input.tsv"
            rows = (bytes - 16) / 8
            for(i = 1; i <= rows; i++) {
                if(i == rows && ending == "eof") printf "1\tA\t0002" > "eof-input.tsv"
                else printf "1\tA\t002\n" > "eof-input.tsv"
            }
            printf "A\n%.2f\n", rows * 2 > "eof-group.tsv"
            printf "id\tA\n1\t%.2f\n", rows * 2 > "eof-id.tsv"
        }'
        [ "$(wc -c < eof-input.tsv)" -eq "$bytes" ] || fail 'page-sized fixture length'
        eof_group_case "page $bytes $ending" eof-input.tsv group eof-group.tsv sum
        eof_group_case "page $bytes $ending" eof-input.tsv id eof-id.tsv sum
    done
done

# Test helper uses 64 KiB chunks; the CLI retains its 1 GiB threshold.
# Data is exactly one/two chunks, or has a remainder; headers are extra bytes.
for rows in 8192 16384 20000; do
    for ending in newline eof; do
        awk -v rows="$rows" -v ending="$ending" 'BEGIN {
            printf "id\tgroup\tamount\n" > "eof-input.tsv"
            for(i = 1; i <= rows; i++) {
                id = int((i - 1) / 4096) + 1
                if(i == rows && ending == "eof") printf "%d\tA\t0002", id > "eof-input.tsv"
                else printf "%d\tA\t002\n", id > "eof-input.tsv"
                totals[id] += 2
            }
            printf "A\n%.2f\n", rows * 2 > "eof-group.tsv"
            printf "id\tA\n" > "eof-id.tsv"
            for(i = 1; i <= id; i++) printf "%d\t%.2f\n", i, totals[i] > "eof-id.tsv"
        }'
        if [ "$rows" -eq 8192 ]; then partition_mode=auto-single; else partition_mode=auto-multi; fi
        eof_group_case "automatic partition $rows rows $ending" eof-input.tsv "$partition_mode-group" eof-group.tsv sum
        eof_group_case "automatic partition $rows rows $ending" eof-input.tsv "$partition_mode-id" eof-id.tsv sum
    done
done

# No later newline/ID exists: an attempted split must stop at the true EOF.
# Padding is in an unused column and never enters a fixed-size field buffer.
for shape in final-record repeated-id; do
    awk -v shape="$shape" 'BEGIN {
        printf "id\tgroup\tamount\tunused\n" > "eof-input.tsv"
        if(shape == "final-record") {
            printf "1\tA\t2\t" > "eof-input.tsv"
            for(i = 0; i < 70000; i++) printf "x" > "eof-input.tsv"
            total = 2
        } else {
            for(i = 0; i < 10000; i++) printf "1\tA\t2\tx\n" > "eof-input.tsv"
            printf "1\tA\t4\tx" > "eof-input.tsv"
            total = 20004
        }
        printf "A\n%.2f\n", total > "eof-group.tsv"
        printf "id\tA\n1\t%.2f\n", total > "eof-id.tsv"
    }'
    if [ "$shape" = final-record ]; then partition_mode=auto-single; else partition_mode=auto-multi; fi
    eof_group_case "no later boundary $shape" eof-input.tsv "$partition_mode-group" eof-group.tsv sum
    eof_group_case "no later boundary $shape" eof-input.tsv auto-single-id eof-id.tsv sum
done
