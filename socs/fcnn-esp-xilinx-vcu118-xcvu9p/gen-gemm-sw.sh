# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Bare metal
if [ ! -d test/gemm-bm/ ]
then
    mkdir test/gemm-bm/
fi

# Linux
if [ ! -d test/gemm/ ]
then
    mkdir test/gemm/
fi

# GEMM
# Bare metal
D3=64 D2=64 D1=1 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
cp soft-build/ariane/baremetal/gemm_stratus.bin test/gemm-bm/gemm-bm-1.bin
cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm-bm-1.exe

D3=8 D2=8 D1=8 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
cp soft-build/ariane/baremetal/gemm_stratus.bin test/gemm-bm/gemm-bm-2.bin
cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm-bm-2.exe

D3=16 D2=16 D1=16 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
cp soft-build/ariane/baremetal/gemm_stratus.bin test/gemm-bm/gemm-bm-3.bin
cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm-bm-3.exe

D3=16 D2=32 D1=16 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
cp soft-build/ariane/baremetal/gemm_stratus.bin test/gemm-bm/gemm-bm-4.bin
cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm-bm-4.exe

D3=32 D2=32 D1=32 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
cp soft-build/ariane/baremetal/gemm_stratus.bin test/gemm-bm/gemm-bm-5.bin
cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm-bm-5.exe

D3=32 D2=64 D1=32 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
cp soft-build/ariane/baremetal/gemm_stratus.bin test/gemm-bm/gemm-bm-6.bin
cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm-bm-6.exe

D3=64 D2=64 D1=64 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
cp soft-build/ariane/baremetal/gemm_stratus.bin test/gemm-bm/gemm-bm-7.bin
cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm-bm-7.exe

# OS
COMP_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm/gemm-os.exe

# Chaining
# MESI
ENABLE_SM=1 COMP_MODE=1 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm/gemm-chaining-mesi.exe

# DMA
ENABLE_SM=1 COH_MODE=1 COMP_MODE=1 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm/gemm-chaining-dma.exe

