# ASI SPX
echo ""
echo "GEMM: ASI SPX"
echo ""
./gemm/gemm-chaining-spx.exe 64 64 1
./gemm/gemm-chaining-spx.exe 8
./gemm/gemm-chaining-spx.exe 16
./gemm/gemm-chaining-spx.exe 16 32 16
./gemm/gemm-chaining-spx.exe 32
./gemm/gemm-chaining-spx.exe 32 64 32
./gemm/gemm-chaining-spx.exe 64
