# Untar pre-generated sysroot
tar -xvf ../sysroot.tar
cp -rf sysroot/ ../../soft/ariane
rm -rf sysroot

# Build Linux to run the experiments
make linux

# Copy the test programs to soft-build/ariane/sysroot/applications/test
cp -r test/ soft-build/ariane/sysroot/applications/

cp riscv.dts socgen/esp/riscv.dts
make linux fpga-program fpga-run-linux
