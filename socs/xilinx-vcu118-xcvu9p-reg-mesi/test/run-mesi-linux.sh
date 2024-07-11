echo "Running Linux tests under MESI and Coherent DMA"
# Regular invocation
echo "Running Linux apps under MESI with regular invocation"
# Sort
for length in 32 64 128 256 512 1024
do
    # MESI
    echo "Under MESI"
    echo "SORT_LEN=${length}"
    ./reg-mesi/sort_stratus-test-reg-mesi-${length}.exe
done

