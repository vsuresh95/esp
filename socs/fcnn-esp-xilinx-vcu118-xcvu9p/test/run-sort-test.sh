# OS
for length in 32 64 128 256 512 1024
do
    echo "\n############################"
    echo "############################"
    echo "SORT: OS; Length: ${length}"
    echo "############################"
    echo "############################\n"
    ./sort-os/sort_stratus-test-sort-os-${loglen}.exe
done

# MESI
for length in 32 64 128 256 512 1024
do
    echo "\n############################"
    echo "############################"
    echo "SORT: MESI; Length: ${length}"
    echo "############################"
    echo "############################\n"
    ./sort-mesi/sort_stratus-test-sort-mesi-${loglen}.exe
done

# DMA
for length in 32 64 128 256 512 1024
do
    echo "\n############################"
    echo "############################"
    echo "SORT: DMA; Length: ${length}"
    echo "############################"
    echo "############################\n"
    ./sort-dma/sort_stratus-test-sort-dma-${loglen}.exe
done
