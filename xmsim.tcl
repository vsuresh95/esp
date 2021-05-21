set severity_pack_assert_off {warning}
set pack_assert_off { std_logic_arith numeric_std }
set intovf_severity_level {ignore}

database -open waves -into waves.shm -default

probe -create -shm :cpu:esp_1:tiles_gen(1):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.axi_req_o -waveform
probe -create -shm :cpu:esp_1:tiles_gen(2):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.axi_req_o -waveform
probe -create -shm :cpu:esp_1:tiles_gen(1):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.axi_resp_i -waveform
probe -create -shm :cpu:esp_1:tiles_gen(2):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.axi_resp_i -waveform

probe -create -shm :cpu:esp_1:tiles_gen(1):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.amo_req -waveform
probe -create -shm :cpu:esp_1:tiles_gen(2):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.amo_req -waveform
probe -create -shm :cpu:esp_1:tiles_gen(1):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.amo_resp -waveform
probe -create -shm :cpu:esp_1:tiles_gen(2):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.amo_resp -waveform

probe -create -shm :cpu:esp_1:tiles_gen(1):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.commit_stage_i.commit_instr_i[0] -waveform
probe -create -shm :cpu:esp_1:tiles_gen(2):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane.commit_stage_i.commit_instr_i[0] -waveform

probe -create -shm :cpu:esp_1:tiles_gen(1):cpu_tile:tile_cpu_i:ariane_cpu_gen:ariane_axi_wrap_1:ariane_wrap_1.i_ariane -waveform

probe -create -shm :cpu:esp_1:tiles_gen(1):cpu_tile:tile_cpu_i:with_cache_coherence:l2_wrapper_1:l2_spandex_gen:l2_cache_i -all -waveform
probe -create -shm :cpu:esp_1:tiles_gen(2):cpu_tile:tile_cpu_i:with_cache_coherence:l2_wrapper_1:l2_spandex_gen:l2_cache_i -all -waveform

probe -create -shm :cpu:esp_1:tiles_gen(0):mem_tile:tile_mem_i:with_cache_coherence:llc_wrapper_1:llc_spandex_gen:llc_cache_i -all -waveform

run