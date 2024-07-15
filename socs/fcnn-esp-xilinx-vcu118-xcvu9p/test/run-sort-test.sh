# OS
for length in 32 64 128 256 512 1024
do
    echo ""
    echo "SORT: OS; Length=${length}"
    echo ""
    ./sort-os/sort-os-${length}.exe
done

# MESI
for length in 32 64 128 256 512 1024
do
    echo ""
    echo "SORT: MESI; Length=${length}"
    echo ""
    ./sort-mesi/sort-mesi-${length}.exe
done

# DMA
for length in 32 64 128 256 512 1024
do
    echo ""
    echo "SORT: DMA; Length=${length}"
    echo ""
    ./sort-dma/sort-dma-${length}.exe
done
