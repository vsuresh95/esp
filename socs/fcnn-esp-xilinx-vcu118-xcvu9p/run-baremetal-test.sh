# Run each baremetal program
# Sort
for length in 32 64 128 256 512 1024
do
    sleep $((length/10))    # Add this sleep in case the next fpga-program erases the previous run
    # MESI
    echo "SORT_LEN=${length}"
    TEST_PROGRAM=./test/sort-bm/sort-bm-${length}.exe make fpga-program fpga-run
done

# GEMM
for length in 32 64 128 256 512 1024 # TODO
do
    sleep $((length/10))    # Add this sleep in case the next fpga-program erases the previous run
    echo "GEMM Length=${length}" # TODO
    TEST_PROGRAM=./test/gemm-bm/gemm-bm-${length}.exe make fpga-program fpga-run
done

