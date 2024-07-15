# Spandex
for length in 32 64 128 256 512 1024
do
    echo ""
    echo "SORT: Spandex; Length=${length}"
    echo ""
    ./sort-spx/sort-spx-${loglen}.exe
done
