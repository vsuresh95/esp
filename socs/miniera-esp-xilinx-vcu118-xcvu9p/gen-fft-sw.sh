# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Bare metal
if [ ! -d test/fft-bm/ ]
then
    mkdir test/fft-bm/
fi

# OS
if [ ! -d test/fft-os/ ]
then
    mkdir test/fft-os/
fi

# MESI
if [ ! -d test/fft-mesi/ ]
then
    mkdir test/fft-mesi/
fi

# DMA
if [ ! -d test/fft-dma/ ]
then
    mkdir test/fft-dma/
fi

# FFT
for length in {6..14}
do
    # Bare metal
    LOG_LEN=$length ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make audio_fft_stratus-baremetal-clean audio_fft_stratus-baremetal
    mv soft-build/ariane/baremetal/audio_fft_stratus.exe fft-bm/audio_fft_stratus-fft-bm-${length}.exe

    # OS
    LOG_LEN=$length ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make audio_fft_stratus-app-clean audio_fft_stratus-app
    mv soft-build/ariane/sysroot/applications/test/audio_fft_stratus.exe test/fft-os/audio_fft_stratus-test-fft-os-${length}.exe

    # MESI
    LOG_LEN=$length ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make audio_fft_stratus-app-clean audio_fft_stratus-app
    mv soft-build/ariane/sysroot/applications/test/audio_fft_stratus.exe test/fft-mesi/audio_fft_stratus-test-fft-mesi-${length}.exe

    # DMA
    LOG_LEN=$length ENABLE_SM=1 IS_ESP=1 COH_MODE=1 make audio_fft_stratus-app-clean audio_fft_stratus-app
    mv soft-build/ariane/sysroot/applications/test/audio_fft_stratus.exe test/fft-dma/audio_fft_stratus-test-fft-dma-${length}.exe

done

