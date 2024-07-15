echo "Running Linux tests under Spandex"
# Sort
# ASI
echo "Running Linux apps under Spandex-FCS with ASI"
# Sort
for length in 32 64 128 256 512 1024
do
    # Spandex
    ./asi-spandex/sort_stratus-test-asi-spandex.exe ${length}
done
