# Spandex
for length in 32 64 128 256 512 1024 # TODO
do
    echo "\n############################"
    echo "############################"
    echo "GEMM: Spandex; Length: ${length}"
    echo "############################"
    echo "############################\n"
    ./gemm-spx/gemm-spx-${length}.exe
done
