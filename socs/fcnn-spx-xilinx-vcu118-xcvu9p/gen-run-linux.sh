# repopulate pre-generate socgen folder
tar -xvf socgen.tar

# generate wrappers
make socketgen-distclean socketgen

# Untar pre-generated sysroot
tar -xvf ../sysroot.tar
cp -rf sysroot/ ../../soft/ariane
cp -rf sysroot/ ./soft-build/ariane
rm -rf sysroot

# Build Linux to run the experiments
make linux

# Copy the test programs to soft-build/ariane/sysroot/applications/test
cp -r test/ soft-build/ariane/sysroot/applications/

make linux # fpga-program fpga-run-linux
