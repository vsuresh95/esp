# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Bare metal
if [ ! -d test/sort-bm/ ]
then
    mkdir test/sort-bm/
fi

# OS
if [ ! -d test/sort-os/ ]
then
    mkdir test/sort-os/
fi

# MESI
if [ ! -d test/sort-mesi/ ]
then
    mkdir test/sort-mesi/
fi

# DMA
if [ ! -d test/sort-dma/ ]
then
    mkdir test/sort-dma/
fi

# Sort
for length in 32 64 128 256 512 1024
do
    # Bare metal
    SORT_LEN=$length ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make sort_stratus-baremetal-clean sort_stratus-baremetal
    cp soft-build/ariane/baremetal/sort_stratus.exe test/sort-bm/sort-bm-${length}.exe
done

# OS
ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make sort_stratus-app-clean sort_stratus-app
cp soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/sort-os/sort-os.exe

# MESI
ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make sort_stratus-app-clean sort_stratus-app
cp soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/sort-mesi/sort-mesi.exe

# DMA
ENABLE_SM=1 IS_ESP=1 COH_MODE=1 make sort_stratus-app-clean sort_stratus-app
cp soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/sort-dma/sort-dma.exe
