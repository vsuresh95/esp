# Untar pre-generated sysroot
tar -xvf ../sysroot.tar
mv sysroot ../../soft/ariane/

# Build Linux to run the experiments
make linux

# Copy the test programs to soft-build/ariane/sysroot/applications/test
tar -xvf test/audio.tar
mv audio test
cp -r test/ soft-build/ariane/sysroot/applications/

make linux fpga-program fpga-run-linux
