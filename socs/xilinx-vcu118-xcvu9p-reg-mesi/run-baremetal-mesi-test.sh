# Run each baremetal program
# Regular invocation
echo "Running baremetal programs under MESI and Coherent DMA with regular invocation"
# Sort
for length in 32 64 128 256 512 1024
do
    sleep $((length/10))    # Add this sleep in case the next fpga-program erases the previous run
    # MESI
    echo "Under MESI"
    echo "SORT_LEN=${length}"
    TEST_PROGRAM=./reg-mesi/sort_stratus-reg-mesi-${length}.exe make fpga-program fpga-run
done
# FFT
for length in {6..14}
do
    sleep $((length/10))    # Add this sleep in case the next fpga-program erases the previous run
    # MESI
    echo "Under MESI"
    echo "FFT_LEN=$(( 2**(length+1) ))"
    TEST_PROGRAM=./reg-mesi/audio_fft_stratus-reg-mesi-${length}.exe make fpga-program fpga-run
done

