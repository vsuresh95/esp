# Spandex
loglen=6; while [ $loglen -le 14 ]; do
    echo ""
    echo "FFT: Spandex; Length: $(( 2**(loglen+1) ))"
    echo ""
    ./fft-spx/fft-spx-${loglen}.exe
    loglen=$(( loglen + 1 ))
done
