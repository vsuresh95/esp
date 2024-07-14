# Run each baremetal program
# FFT
for length in {6..14}
do
    sleep $((length/2))    # Add this sleep in case the next fpga-program erases the previous run
    echo "\n############################"
    echo "############################"
    echo "FFT: MESI; Length: $(( 2**(length+1) ))"
    echo "############################"
    echo "############################\n"
    TEST_PROGRAM=./test/fft-bm/fft-bm-${length}.exe make fpga-program fpga-run
done

