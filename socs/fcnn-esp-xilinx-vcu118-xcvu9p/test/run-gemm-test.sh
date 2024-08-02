#!/bin/sh
# for SW and Linux
echo ""
echo "GEMM: OS"
echo ""
./gemm/gemm-os.exe 64 64 1; 
./gemm/gemm-os.exe 8; 
./gemm/gemm-os.exe 16; 
./gemm/gemm-os.exe 16 32 16; 
./gemm/gemm-os.exe 32; 
./gemm/gemm-os.exe 32 64 32; 
./gemm/gemm-os.exe 64; 


# ASI MESI
echo ""
echo "GEMM: ASI MESI"
echo ""
./gemm/gemm-chaining-mesi.exe 64 64 1
./gemm/gemm-chaining-mesi.exe 8
./gemm/gemm-chaining-mesi.exe 16
./gemm/gemm-chaining-mesi.exe 16 32 16
./gemm/gemm-chaining-mesi.exe 32
./gemm/gemm-chaining-mesi.exe 32 64 32
./gemm/gemm-chaining-mesi.exe 64


# ASI DMA
echo ""
echo "GEMM: ASI DMA"
echo ""
./gemm/gemm-chaining-dma.exe 64 64 1
./gemm/gemm-chaining-dma.exe 8
./gemm/gemm-chaining-dma.exe 16
./gemm/gemm-chaining-dma.exe 16 32 16
./gemm/gemm-chaining-dma.exe 32
./gemm/gemm-chaining-dma.exe 32 64 32
./gemm/gemm-chaining-dma.exe 64
