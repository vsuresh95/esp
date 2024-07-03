# Build Linux to run the experiments
echo "Generating Linux for running the Linux tests"
make linux

# Copy the test programs to soft-build/ariane/sysroot/applications/test
cp -r test/ soft-build/ariane/sysroot/applications/test/
make linux fpga-program fpga-run-linux
