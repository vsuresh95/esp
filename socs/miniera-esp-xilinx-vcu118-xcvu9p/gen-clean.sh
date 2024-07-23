# clean all accelerator HLS folders
for acc in "sort" "audio_fft" "audio_fir" "audio_ffi" "audio_dma" "gemm" "tiled_app" "vitdodec" "asi_vitdodec" "sensor_dma"
do
    make ${acc}_stratus-distclean
done

# clean vivado build
make vivado-clean
rm -rf vivado

# clean linux
make linux-distclean

# clean everything else
make clean

rm -rf test/fft-*
rm -rf test/miniera