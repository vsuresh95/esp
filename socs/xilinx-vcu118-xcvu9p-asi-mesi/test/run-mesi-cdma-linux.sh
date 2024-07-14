echo "Running Linux tests under MESI and Coherent DMA"
# ASI
echo "Running Linux apps under MESI and CDMA with ASI"
# Sort
for length in 32 64 128 256 512 1024
do
    # MESI
    ./asi-mesi/sort_stratus-test-asi-mesi.exe ${length}
    # CDMA
    ./asi-cdma/sort_stratus-test-asi-cdma.exe ${length}
done
