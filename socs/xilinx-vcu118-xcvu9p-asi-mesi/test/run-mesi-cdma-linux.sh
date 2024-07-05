echo "Running Linux tests under MESI and Coherent DMA"
# ASI
echo "Running Linux apps under MESI and CDMA with ASI"
# Sort
for length in 32 64 128 256 512 1024
do
    # MESI
    echo "Under MESI"
    echo "SORT_LEN=${length}"
    ./asi-mesi/sort_stratus-test-asi-mesi-${length}.exe
    # CDMA
    echo "Under Coherent DMA"
    echo "SORT_LEN=${length}"
    ./asi-cdma/sort_stratus-test-asi-cdma-${length}.exe
done
