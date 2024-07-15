# OS
loglen=6; while [ $loglen -le 14 ]; do
    echo ""
    echo "FFT: OS; Length: $(( 2**(loglen+1) ))"
    echo ""
    ./fft-os/fft-os-${loglen}.exe
    loglen=$(( loglen + 1 ))
done

# MESI
loglen=6; while [ $loglen -le 14 ]; do
    echo ""
    echo "FFT: MESI; Length: $(( 2**(loglen+1) ))"
    echo ""
    ./fft-mesi/fft-mesi-${loglen}.exe
    loglen=$(( loglen + 1 ))
done

# DMA
loglen=6; while [ $loglen -le 14 ]; do
    echo ""
    echo "FFT: DMA; Length: $(( 2**(loglen+1) ))"
    echo ""
    ./fft-dma/fft-dma-${loglen}.exe
    loglen=$(( loglen + 1 ))
done
