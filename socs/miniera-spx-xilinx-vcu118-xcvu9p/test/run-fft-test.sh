# Spandex
loglen=6; while [ $loglen -le 14 ]; do
    echo "\n############################"
    echo "############################"
    echo "FFT: Spandex; Length: $(( 2**(loglen+1) ))"
    echo "############################"
    echo "############################\n"
    ./fft-dma/audio_fft_stratus-test-fft-spx-${loglen}.exe
    loglen=$(( loglen + 1 ))
done
