# repopulate pre-generate socgen folder
tar -xvf socgen.tar

# generate wrappers
make socketgen-distclean socketgen

cp ../../accelerators/stratus_hls/gemm_stratus/hw/hls-work-virtexup/bdw_work/modules/gemm/BASIC_CHK4096_DMA64_WORD32_PARAL8_INVasi/v_rtl/gemm_*.v ../../tech/virtexup/acc/gemm_stratus/gemm_stratus_basic_chk4096_dma64_word32_paral8_invasi/

cp ../../accelerators/stratus_hls/gemm_stratus/hw/hls-work-virtexup/bdw_work/modules/gemm/BASIC_CHK4096_DMA64_WORD32_PARAL8_INVreg/v_rtl/gemm_*.v ../../tech/virtexup/acc/gemm_stratus/gemm_stratus_basic_chk4096_dma64_word32_paral8_invreg/

# FPGA synthesis
make vivado-clean
rm -rf vivado
make vivado-syn
