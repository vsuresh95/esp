# OS
for length in 32 64 128 256 512 1024 # TODO
do
    echo "\n############################"
    echo "############################"
    echo "GEMM: OS; Length: ${length}"
    echo "############################"
    echo "############################\n"
    ./gemm-os/gemm-os-${length}.exe
done

# MESI
for length in 32 64 128 256 512 1024 # TODO
do
    echo "\n############################"
    echo "############################"
    echo "GEMM: MESI; Length: ${length}"
    echo "############################"
    echo "############################\n"
    ./gemm-mesi/gemm-mesi-${length}.exe
done

# DMA
for length in 32 64 128 256 512 1024 # TODO
do
    echo "\n############################"
    echo "############################"
    echo "GEMM: DMA; Length: ${length}"
    echo "############################"
    echo "############################\n"
    ./gemm-dma/gemm-dma-${length}.exe
done
