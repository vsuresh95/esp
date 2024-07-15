# OS
for length in 32 64 128 256 512 1024 # TODO
do
    echo ""
    echo "GEMM: OS; Length=${length}" # TODO
    echo ""
    ./gemm-os/gemm-os-${length}.exe
done

# MESI
for length in 32 64 128 256 512 1024 # TODO
do
    echo ""
    echo "GEMM: MESI; Length=${length}" # TODO
    echo ""
    ./gemm-mesi/gemm-mesi-${length}.exe
done

# DMA
for length in 32 64 128 256 512 1024 # TODO
do
    echo ""
    echo "GEMM: DMA; Length=${length}" # TODO
    echo ""
    ./gemm-dma/gemm-dma-${length}.exe
done
