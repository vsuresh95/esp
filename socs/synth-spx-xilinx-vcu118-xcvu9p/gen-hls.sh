for acc in "sort" "audio_fft" "audio_fir" "audio_ffi" "audio_dma" "gemm" "tiled_app" "vitdodec" "asi_vitdodec" "sensor_dma"
do
    make ${acc}_stratus-hls -j
done
