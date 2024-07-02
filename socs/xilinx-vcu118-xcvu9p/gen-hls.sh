for acc in "sort" "audio_fft" "audio_fir" "audio_ffi" "gemm" "synth"
do
    make "${acc}_stratus-hls"
done
