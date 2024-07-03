echo "Running Linux tests under MESI and Coherent DMA"
# Regular invocation
echo "Running Linux apps under MESI and Coherent DMA with regular invocation"
# Sort
for length in 32 64 128 256 512 1024
do
    # MESI
    echo "Under MESI"
    ./reg-mesi/sort_stratus-reg-mesi-${length}.exe
    # CDMA
    echo "Under Coherent DMA"
    ./reg-mesi/sort_stratus-reg-cdma-${length}.exe
done

# ASI
echo "Running Linux apps under MESI and CDMA with ASI"
# Sort
for length in 32 64 128 256 512 1024
do
    # MESI
    echo "Under MESI"
    ./asi-mesi/sort_stratus-asi-mesi-${length}.exe
    # CDMA
    echo "Under Coherent DMA"
    ./asi-mesi/sort_stratus-asi-cdma-${length}.exe
done
