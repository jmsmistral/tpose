# Exact simple-transpose output, using the shared shell test helpers.
for ending in newline eof; do
    for shape in rectangular empty-cells empty-header all-empty blank-row; do
        case "$shape" in
            rectangular)
                printf 'a\tb\tc\n1\t2\t3' > simple-input.tsv
                printf 'a\t1\nb\t2\nc\t3\n' > simple-expected.tsv ;;
            empty-cells)
                printf 'a\tb\tc\n\t2\t\n3\t\t4' > simple-input.tsv
                printf 'a\t\t3\nb\t2\t\nc\t\t4\n' > simple-expected.tsv ;;
            empty-header)
                printf '\tb\t\n1\t\t3' > simple-input.tsv
                printf '\t1\nb\t\n\t3\n' > simple-expected.tsv ;;
            all-empty)
                printf '\t\n\t' > simple-input.tsv
                printf '\t\n\t\n' > simple-expected.tsv ;;
            blank-row)
                printf 'a\n\nb' > simple-input.tsv
                printf 'a\t\tb\n' > simple-expected.tsv ;;
        esac
        # Round-trip expectation always has the canonical final newline.
        cat simple-input.tsv > simple-roundtrip.tsv
        printf '\n' >> simple-roundtrip.tsv
        [ "$ending" != newline ] || printf '\n' >> simple-input.tsv
        limit_output "simple $shape $ending" simple-expected.tsv "$binary" simple-input.tsv
        limit_output "guarded simple $shape $ending" simple-expected.tsv \
            "$key_binary" eof simple simple-input.tsv sum
        limit_output "simple round trip $shape $ending" simple-roundtrip.tsv \
            "$binary" simple-expected.tsv

        tr '\t' ',' < simple-input.tsv > simple-input.csv
        tr '\t' ',' < simple-expected.tsv > simple-expected.csv
        limit_output "simple comma $shape $ending" simple-expected.csv "$binary" simple-input.csv -d,
    done
done

printf '\n' > simple-input.tsv
printf '\n' > simple-expected.tsv
limit_output 'one empty cell' simple-expected.tsv "$binary" simple-input.tsv
limit_output 'guarded one empty cell' simple-expected.tsv "$key_binary" eof simple simple-input.tsv sum

for ending in newline eof; do
    for shape in short wide extra-empty blank-row single-column; do
        case "$shape" in
            short)
                printf 'a\tb\n1\t2\n3' > simple-input.tsv
                diagnostic='Error: Row 3 has 1 field; expected 2' ;;
            wide)
                printf 'a\tb\n1\t2\n3\t4\t5' > simple-input.tsv
                diagnostic='Error: Row 3 has 3 fields; expected 2' ;;
            extra-empty)
                printf 'a\tb\n1\t2\t' > simple-input.tsv
                diagnostic='Error: Row 2 has 3 fields; expected 2' ;;
            blank-row)
                printf 'a\tb\n\n1\t2' > simple-input.tsv
                diagnostic='Error: Row 2 has 1 field; expected 2' ;;
            single-column)
                printf 'a\n1\t2' > simple-input.tsv
                diagnostic='Error: Row 2 has 2 fields; expected 1' ;;
        esac
        [ "$ending" != newline ] || printf '\n' >> simple-input.tsv
        limit_error "simple rejects $shape $ending" "$diagnostic" "$binary" simple-input.tsv
        [ ! -s actual ] || fail 'invalid shape wrote transposed data'
        limit_error "guarded simple rejects $shape $ending" "$diagnostic" \
            "$key_binary" eof simple simple-input.tsv sum
        [ ! -s actual ] || fail 'guarded invalid shape wrote transposed data'
        tr '\t' ',' < simple-input.tsv > simple-input.csv
        limit_error "simple comma rejects $shape $ending" "$diagnostic" "$binary" simple-input.csv -d,
        [ ! -s actual ] || fail 'comma invalid shape wrote transposed data'
    done
done

# The file-output path must produce exactly the same table as stdout.
printf 'a\tb\n1\t2\n3\t' > simple-input.tsv
printf 'a\t1\t3\nb\t2\t\n' > simple-expected.tsv
: > empty
limit_output 'simple output file stdout is empty' empty "$binary" simple-input.tsv simple-result.tsv
diff -u simple-expected.tsv simple-result.tsv || fail 'simple output file contents'
pass 'simple output file contents'
