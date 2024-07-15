# Spandex
for length in 32 64 128 256 512 1024 # TODO
do
    echo ""
    echo "GEMM: Spandex; Length=${length}" # TODO
    echo ""
    ./gemm-spx/gemm-spx-${length}.exe
done
