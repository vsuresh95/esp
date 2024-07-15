# repopulate pre-generate socgen folder
tar -xvf socgen.tar

# generate wrappers
make socketgen-distclean socketgen

# FPGA synthesis
make vivado-clean
rm -rf vivado
make vivado-syn
