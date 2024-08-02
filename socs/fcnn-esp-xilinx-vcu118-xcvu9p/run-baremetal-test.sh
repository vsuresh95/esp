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
for index in 1 2 3 4 5 6 7
do
    sleep $((index/10))    # Add this sleep in case the next fpga-program erases the previous run
    TEST_PROGRAM=./test/gemm-bm/gemm-bm-${index}.exe make fpga-program fpga-run
done
