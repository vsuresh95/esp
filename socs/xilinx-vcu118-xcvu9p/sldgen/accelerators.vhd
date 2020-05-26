-- Copyright (c) 2011-2019 Columbia University, System Level Design Group
-- SPDX-License-Identifier: Apache-2.0


library ieee;
use ieee.std_logic_1164.all;
use work.sld_devices.all;
use work.allacc.all;

entity softmax_cxx_rtl is

    generic (
      hls_conf  : hlscfg_t
    );

    port (
      conf_info_batch            : in  std_logic_vector(31 downto 0);
      clk                        : in  std_ulogic;
      acc_rst                    : in  std_ulogic;
      conf_done                  : in  std_ulogic;
      dma_read_ctrl_valid        : out std_ulogic;
      dma_read_ctrl_ready        : in  std_ulogic;
      dma_read_ctrl_data_index   : out std_logic_vector(31 downto 0);
      dma_read_ctrl_data_length  : out std_logic_vector(31 downto 0);
      dma_read_ctrl_data_size    : out std_logic_vector(2 downto 0);
      dma_write_ctrl_valid       : out std_ulogic;
      dma_write_ctrl_ready       : in  std_ulogic;
      dma_write_ctrl_data_index  : out std_logic_vector(31 downto 0);
      dma_write_ctrl_data_length : out std_logic_vector(31 downto 0);
      dma_write_ctrl_data_size   : out std_logic_vector(2 downto 0);
      dma_read_chnl_valid        : in  std_ulogic;
      dma_read_chnl_ready        : out std_ulogic;
      dma_read_chnl_data         : in  std_logic_vector(63 downto 0);
      dma_write_chnl_valid       : out std_ulogic;
      dma_write_chnl_ready       : in  std_ulogic;
      dma_write_chnl_data        : out std_logic_vector(63 downto 0);
      acc_done                   : out std_ulogic
    );

end entity softmax_cxx_rtl;


architecture mapping of softmax_cxx_rtl is

begin  -- mapping


  impl_basic_fx32_dma64_gen: if hls_conf = HLSCFG_SOFTMAX_CXX_BASIC_FX32_DMA64 generate
    softmax_cxx_basic_fx32_dma64_i: softmax_cxx_basic_fx32_dma64
    port map(
      clk                        => clk,
      rst                        => acc_rst,
          
      conf_info_batch_rsc_dat    => conf_info_batch,
      conf_done_rsc_dat          => conf_done,

      dma_read_ctrl_rsc_dat(66 downto 64) => dma_read_ctrl_data_size,
      dma_read_ctrl_rsc_dat(63 downto 32) => dma_read_ctrl_data_length,
      dma_read_ctrl_rsc_dat(31 downto 0)  => dma_read_ctrl_data_index,
      dma_read_ctrl_rsc_vld      => dma_read_ctrl_valid,
      dma_read_ctrl_rsc_rdy      => dma_read_ctrl_ready,

      dma_write_ctrl_rsc_dat(66 downto 64) => dma_write_ctrl_data_size,
      dma_write_ctrl_rsc_dat(63 downto 32) => dma_write_ctrl_data_length,
      dma_write_ctrl_rsc_dat(31 downto 0)  => dma_write_ctrl_data_index,
      dma_write_ctrl_rsc_vld     => dma_write_ctrl_valid,
      dma_write_ctrl_rsc_rdy     => dma_write_ctrl_ready,

      dma_read_chnl_rsc_dat      => dma_read_chnl_data,
      dma_read_chnl_rsc_vld      => dma_read_chnl_valid,
      dma_read_chnl_rsc_rdy      => dma_read_chnl_ready,

      dma_write_chnl_rsc_dat     => dma_write_chnl_data,
      dma_write_chnl_rsc_vld     => dma_write_chnl_valid,
      dma_write_chnl_rsc_rdy     => dma_write_chnl_ready,

      store_acc_done_sync_vld    => acc_done
    );
  end generate impl_basic_fx32_dma64_gen;

end mapping;

