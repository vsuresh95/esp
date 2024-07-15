# repopulate pre-generate socgen folder
tar -xvf socgen.tar

# generate wrappers
make socketgen-distclean socketgen

cp ../../accelerators/stratus_hls/asi_vitdodec_stratus/hw/hls-work-virtexup/bdw_work/modules/asi_vitdodec/BASIC_DMA64/v_rtl/asi_vitdodec_Mod_32Ux32U_32U_4.v ../../tech/virtexup/acc/asi_vitdodec_stratus/asi_vitdodec_stratus_basic_dma64/

cp ../../accelerators/stratus_hls/vitdodec_stratus/hw/hls-work-virtexup/bdw_work/modules/vitdodec/BASIC_DMA64/v_rtl/vitdodec_Mod_32Ux32U_32U_4.v ../../tech/virtexup/acc/vitdodec_stratus/vitdodec_stratus_basic_dma64/

# FPGA synthesis
make vivado-clean
rm -rf vivado
make vivado-syn
