# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Spandex
if [ ! -d test/fft-spx/ ]
then
    mkdir test/fft-spx/
fi

# FFT
for length in {6..14}
do
    # Spandex
    LOG_LEN=$length ENABLE_SM=1 IS_ESP=0 COH_MODE=2 make audio_fft_stratus-app-clean audio_fft_stratus-app
    mv soft-build/ariane/sysroot/applications/test/audio_fft_stratus.exe test/fft-spx/audio_fft_stratus-test-fft-spx-${length}.exe

done

