# Sourced by run.sh in its private temporary directory.
id_order_error() {
    case_name=$1
    repeated_id=$2
    repeated_row=$3
    shift 3
    limit_error "$case_name" "Error: ID '$repeated_id' reappears on row $repeated_row; rows for each ID must be consecutive (group or sort input by ID)" "$@"
    [ ! -s actual ] || fail "$case_name (wrote output before validation)"
    no_staged_output
}

# Rejection must work both across partitions and within a single worker's range.
for layout in across within; do
    case "$layout" in
        across) rows='1 2 1'; repeated=1; row=4 ;;
        within) rows='1 2 3 2'; repeated=2; row=5 ;;
    esac
    for ending in newline eof; do
        awk -v ids="$rows" -v ending="$ending" 'BEGIN {
            print "id\tgroup\tamount"
            n = split(ids, a, " ")
            for(i = 1; i <= n; i++) printf "%s\tA\t2%s", a[i], (i == n && ending == "eof" ? "" : "\n")
        }' > id-order.tsv
        for aggregation in sum count avg; do
            id_order_error "serial ID order $layout $ending $aggregation" "$repeated" "$row" \
                "$binary" id-order.tsv -Iid -Ggroup -Namount "-a$aggregation"
            id_order_error "parallel ID order $layout $ending $aggregation" "$repeated" "$row" \
                "$key_binary" id id-order.tsv "$aggregation"
        done
    done
done

# Valid blocks need not be sorted. Preserve their input order and aggregation.
printf 'id\tgroup\tamount\n3\tA\t2\n3\tA\t4\n2\tA\t2\n2\tA\t4\n1\tA\t2\n1\tA\t4\n' > id-order.tsv
for aggregation in sum count avg; do
    case "$aggregation" in sum) value=6.00 ;; count) value=2 ;; avg) value=3.00 ;; esac
    printf 'id\tA\n3\t%s\n2\t%s\n1\t%s\n' "$value" "$value" "$value" > id-order-expected
    limit_output "serial descending IDs $aggregation" id-order-expected \
        "$binary" id-order.tsv -Iid -Ggroup -Namount "-a$aggregation"
    limit_output "parallel descending IDs $aggregation" id-order-expected \
        "$key_binary" id id-order.tsv "$aggregation"
done

# Full string identity: case, numeric spellings, prefixes, and old hash collisions.
printf 'id\tgroup\tamount\nAa\tA\t2\nB<\tA\t2\na\tA\t2\naa\tA\t2\n1\tA\t2\n01\tA\t2\n2\tA\t2\n' > id-order.tsv
awk 'NR == 1 { print "id\tA"; next } { printf "%s\t2.00\n", $1 }' id-order.tsv > id-order-expected
limit_output 'serial exact ID identity' id-order-expected "$binary" id-order.tsv -Iid -Ggroup -Namount
limit_output 'parallel exact ID identity' id-order-expected "$key_binary" id id-order.tsv sum
printf 'Aa\tA\t2\n' >> id-order.tsv
id_order_error 'full string repeat' Aa 9 "$binary" id-order.tsv -Iid -Ggroup -Namount

# Report the earliest returning ID, not whichever sorts first in the index.
printf 'id\tgroup\tamount\nz\tA\t2\na\tA\t2\nz\tA\t2\na\tA\t2\n' > id-order.tsv
id_order_error 'earliest repeat' z 4 "$binary" id-order.tsv -Iid -Ggroup -Namount

# Missing numeric/group values do not exempt a nonempty ID from the contract.
for skipped in group numeric; do
    case "$skipped" in group) middle='2\t\t2' ;; numeric) middle='2\tA\t' ;; esac
    printf 'id\tgroup\tamount\n1\tA\t2\n%b\n1\tA\t2\n' "$middle" > id-order.tsv
    id_order_error "skipped $skipped still ends ID run" 1 4 "$binary" id-order.tsv -Iid -Ggroup -Namount
    id_order_error "parallel skipped $skipped still ends ID run" 1 4 "$key_binary" id id-order.tsv sum
done
printf 'id\tgroup\tamount\n1\tA\t2\n\tA\t2\n\n1\tA\t2\n' > id-order.tsv
printf 'id\tA\n1\t4.00\n' > id-order-expected
limit_output 'empty IDs do not end run' id-order-expected "$binary" id-order.tsv -Iid -Ggroup -Namount
limit_output 'parallel empty IDs do not end run' id-order-expected "$key_binary" eof empty-id id-order.tsv sum

# Named/indexed fields, reordered columns, custom delimiters, and output rollback.
printf 'amount,id,group\n2,1,A\n2,2,A\n2,1,A\n' > id-order.csv
id_order_error 'indexed comma ID order' 1 4 "$binary" id-order.csv -d, -i -I2 -G3 -N1
cp output-sentinel id-order-protected.tsv
id_order_error 'ID order preserves destination' 1 4 \
    "$binary" id-order.csv id-order-protected.tsv -d, -Iid -Ggroup -Namount
cmp output-sentinel id-order-protected.tsv || fail 'ID order changed destination'
id_order_error 'ID order creates no destination' 1 4 \
    "$binary" id-order.csv id-order-absent.tsv -d, -Iid -Ggroup -Namount
[ ! -e id-order-absent.tsv ] || fail 'ID order created destination'
printf 'A\n6.00\n' > id-order-expected
limit_output 'group-only ignores ID order' id-order-expected "$binary" id-order.csv -Ggroup -Namount -d,

# Exercise index growth beyond the unrelated 5,000-column limit.
awk 'BEGIN {
    print "id\tgroup\tamount" > "id-order.tsv"
    print "id\tA" > "id-order-expected"
    for(i = 6000; i >= 1; i--) {
        printf "%d\tA\t2\n", i > "id-order.tsv"
        printf "%d\t2.00\n", i > "id-order-expected"
    }
}'
limit_output '6000 distinct IDs' id-order-expected "$binary" id-order.tsv -Iid -Ggroup -Namount
limit_output 'parallel 6000 distinct IDs' id-order-expected "$key_binary" id id-order.tsv sum
printf '6000\tA\t2\n' >> id-order.tsv
id_order_error 'repeat after index growth' 6000 6002 "$binary" id-order.tsv -Iid -Ggroup -Namount

# Real automatic partitioning with guarded EOF; each ID block is 32 KiB.
for layout in valid repeated; do
    awk -v layout="$layout" 'BEGIN {
        print "id\tgroup\tamount"
        for(block = 3; block >= 1; block--)
            for(i = 1; i <= 4096; i++) printf "%d\tA\t002\n", (layout == "repeated" && block == 1 ? 3 : block)
    }' > id-order.tsv
    if [ "$layout" = valid ]; then
        printf 'id\tA\n3\t8192.00\n2\t8192.00\n1\t8192.00\n' > id-order-expected
        limit_output 'automatic parallel descending IDs' id-order-expected \
            "$key_binary" eof auto-multi-id id-order.tsv sum
    else
        id_order_error 'automatic parallel repeated ID' 3 8194 \
            "$key_binary" eof auto-multi-id id-order.tsv sum
        cp output-sentinel id-order-protected.tsv
        id_order_error 'parallel ID order preserves destination' 3 8194 \
            "$key_binary" output parallel-destination id-order.tsv id-order-protected.tsv
        cmp output-sentinel id-order-protected.tsv || fail 'parallel ID order changed destination'
    fi
done
