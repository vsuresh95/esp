# OS
for length in 32 64 128 256 512 1024
do
    echo ""
    echo "SORT: OS; Length=${length}"
    echo ""
    ./sort-os/sort-os.exe ${length}
done

# MESI
for length in 32 64 128 256 512 1024
do
    echo ""
    echo "SORT: MESI; Length=${length}"
    echo ""
    ./sort-mesi/sort-mesi.exe ${length}
done

# DMA
for length in 32 64 128 256 512 1024
do
    echo ""
    echo "SORT: DMA; Length=${length}"
    echo ""
    ./sort-dma/sort-dma.exe ${length}
done
