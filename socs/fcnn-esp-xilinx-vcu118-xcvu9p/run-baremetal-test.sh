# Run each baremetal program
# GEMM
for length in {6..14} # TODO
do
    sleep $((length/2))    # Add this sleep in case the next fpga-program erases the previous run
    echo "\n############################"
    echo "############################"
    echo "GEMM: Bare metal; Length: " # TODO
    echo "############################"
    echo "############################\n"
    TEST_PROGRAM=./test/gemm-bm/gemm_stratus-gemm-bm-${length}.exe make fpga-program fpga-run
done

