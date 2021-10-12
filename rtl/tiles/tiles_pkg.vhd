-- Copyright (c) 2011-2021 Columbia University, System Level Design Group
-- SPDX-License-Identifier: Apache-2.0

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.esp_global.all;
use work.amba.all;
use work.stdlib.all;
use work.sld_devices.all;
use work.monitor_pkg.all;
use work.esp_csr_pkg.all;
use work.nocpackage.all;
use work.cachepackage.all;
use work.socmap.all;

package tiles_pkg is

  component esp is
    generic (
      SIMULATION : boolean := false);
    port (
      rst                : in  std_logic;
      sys_clk            : in    std_logic_vector(0 to MEM_ID_RANGE_MSB);
      refclk             : in  std_logic;
      pllbypass          : in  std_logic_vector(CFG_TILES_NUM - 1 downto 0);
      uart_rxd           : in  std_logic;
      uart_txd           : out std_logic;
      uart_ctsn          : in  std_logic;
      uart_rtsn          : out std_logic;
      cpuerr             : out   std_logic;
      ddr_ahbsi          : out ahb_slv_in_vector_type(0 to MEM_ID_RANGE_MSB);
      ddr_ahbso          : in  ahb_slv_out_vector_type(0 to MEM_ID_RANGE_MSB);
      eth0_apbi          : out apb_slv_in_type;
      eth0_apbo          : in  apb_slv_out_type;
      sgmii0_apbi        : out apb_slv_in_type;
      sgmii0_apbo        : in  apb_slv_out_type;
      eth0_ahbmi         : out ahb_mst_in_type;
      eth0_ahbmo         : in  ahb_mst_out_type;
      edcl_ahbmo         : in  ahb_mst_out_type;
      dvi_apbi           : out apb_slv_in_type;
      dvi_apbo           : in  apb_slv_out_type;
      dvi_ahbmi          : out ahb_mst_in_type;
      dvi_ahbmo          : in  ahb_mst_out_type;
      mon_noc            : out monitor_noc_matrix(1 to 6, 0 to CFG_TILES_NUM-1);
      mon_acc            : out monitor_acc_vector(0 to relu(accelerators_num-1));
      mon_mem            : out monitor_mem_vector(0 to CFG_NMEM_TILE + CFG_NSLM_TILE + CFG_NSLMDDR_TILE - 1);
      mon_l2             : out monitor_cache_vector(0 to relu(CFG_NL2 - 1));
      mon_llc            : out monitor_cache_vector(0 to relu(CFG_NLLC - 1));
      mon_dvfs           : out monitor_dvfs_vector(0 to CFG_TILES_NUM-1));
  end component esp;

  component tile_cpu is
    generic (
      SIMULATION         : boolean              := false;
      this_has_dvfs      : integer range 0 to 1 := 0;
      this_has_pll       : integer range 0 to 1 := 0;
      this_has_dco       : integer range 0 to 1 := 0;
      this_extra_clk_buf : integer range 0 to 1 := 0;
      test_if_en         : integer range 0 to 1 := 0;
      this_has_nfu       : integer range 0 to 1 := 0);
    port (
      raw_rstn           : in  std_ulogic;
      tile_rst           : in  std_ulogic;
      refclk             : in  std_ulogic;
      pllbypass          : in  std_ulogic;
      pllclk             : out std_ulogic;
      dco_clk            : out std_ulogic;
      dco_rstn           : out std_ulogic;
      -- DCO config
      dco_freq_sel       : in std_logic_vector(1 downto 0);
      dco_div_sel        : in std_logic_vector(2 downto 0);
      dco_fc_sel         : in std_logic_vector(5 downto 0);
      dco_cc_sel         : in std_logic_vector(5 downto 0);
      dco_clk_sel        : in std_ulogic;
      dco_en             : in std_ulogic;  
      cpuerr             : out std_ulogic;
      -- NOC
      test1_output_port   : in noc_flit_type;
      test1_data_void_out : in std_ulogic;
      test1_stop_in       : in std_ulogic;
      test2_output_port   : in noc_flit_type;
      test2_data_void_out : in std_ulogic;
      test2_stop_in       : in std_ulogic;
      test3_output_port   : in noc_flit_type;
      test3_data_void_out : in std_ulogic;
      test3_stop_in       : in std_ulogic;
      test4_output_port   : in noc_flit_type;
      test4_data_void_out : in std_ulogic;
      test4_stop_in       : in std_ulogic;
      test5_output_port   : in misc_noc_flit_type;
      test5_data_void_out : in std_ulogic;
      test5_stop_in       : in std_ulogic;
      test6_output_port   : in noc_flit_type;
      test6_data_void_out : in std_ulogic;
      test6_stop_in       : in std_ulogic;
      test1_input_port    : out noc_flit_type;
      test1_data_void_in  : out std_ulogic;
      test1_stop_out      : out std_ulogic;
      test2_input_port    : out noc_flit_type;
      test2_data_void_in  : out std_ulogic;
      test2_stop_out      : out std_ulogic;
      test3_input_port    : out noc_flit_type;
      test3_data_void_in  : out std_ulogic;
      test3_stop_out      : out std_ulogic;
      test4_input_port    : out noc_flit_type;
      test4_data_void_in  : out std_ulogic;
      test4_stop_out      : out std_ulogic;
      test5_input_port    : out misc_noc_flit_type;
      test5_data_void_in  : out std_ulogic;
      test5_stop_out      : out std_ulogic;
      test6_input_port    : out noc_flit_type;
      test6_data_void_in  : out std_ulogic;
      test6_stop_out      : out std_ulogic;
      mon_noc             : in  monitor_noc_vector(1 to 6);
      mon_cache           : out monitor_cache_type;
      mon_dvfs_in         : in  monitor_dvfs_type;
      mon_dvfs            : out monitor_dvfs_type);
  end component tile_cpu;

  component tile_acc is
    generic (
      this_hls_conf      : hlscfg_t             := 0;
      this_device        : devid_t              := 0;
      this_irq_type      : integer              := 0;
      this_has_l2        : integer range 0 to 1 := 0;
      this_has_dvfs      : integer range 0 to 1 := 0;
      this_has_pll       : integer range 0 to 1 := 0;
      this_has_dco       : integer range 0 to 1 := 0;
      this_extra_clk_buf : integer range 0 to 1 := 0);
    port (
      raw_rstn           : in  std_ulogic;
      tile_rst           : in  std_ulogic;
      refclk             : in  std_ulogic;
      pllbypass          : in  std_ulogic;
      pllclk             : out std_ulogic;
      dco_clk            : out std_ulogic;
      dco_rstn           : out std_ulogic;
      -- DCO config
      dco_freq_sel       : in std_logic_vector(1 downto 0);
      dco_div_sel        : in std_logic_vector(2 downto 0);
      dco_fc_sel         : in std_logic_vector(5 downto 0);
      dco_cc_sel         : in std_logic_vector(5 downto 0);
      dco_clk_sel        : in std_ulogic;
      dco_en             : in std_ulogic;  
      ext_dco_cc_sel     : out std_logic_vector(5 downto 0);
      ext_ldo_res_sel    : out std_logic_vector(7 downto 0);
      -- NOC
      test1_output_port   : in noc_flit_type;
      test1_data_void_out : in std_ulogic;
      test1_stop_in       : in std_ulogic;
      test2_output_port   : in noc_flit_type;
      test2_data_void_out : in std_ulogic;
      test2_stop_in       : in std_ulogic;
      test3_output_port   : in noc_flit_type;
      test3_data_void_out : in std_ulogic;
      test3_stop_in       : in std_ulogic;
      test4_output_port   : in noc_flit_type;
      test4_data_void_out : in std_ulogic;
      test4_stop_in       : in std_ulogic;
      test5_output_port   : in misc_noc_flit_type;
      test5_data_void_out : in std_ulogic;
      test5_stop_in       : in std_ulogic;
      test6_output_port   : in noc_flit_type;
      test6_data_void_out : in std_ulogic;
      test6_stop_in       : in std_ulogic;
      test1_input_port    : out noc_flit_type;
      test1_data_void_in  : out std_ulogic;
      test1_stop_out      : out std_ulogic;
      test2_input_port    : out noc_flit_type;
      test2_data_void_in  : out std_ulogic;
      test2_stop_out      : out std_ulogic;
      test3_input_port    : out noc_flit_type;
      test3_data_void_in  : out std_ulogic;
      test3_stop_out      : out std_ulogic;
      test4_input_port    : out noc_flit_type;
      test4_data_void_in  : out std_ulogic;
      test4_stop_out      : out std_ulogic;
      test5_input_port    : out misc_noc_flit_type;
      test5_data_void_in  : out std_ulogic;
      test5_stop_out      : out std_ulogic;
      test6_input_port    : out noc_flit_type;
      test6_data_void_in  : out std_ulogic;
      test6_stop_out      : out std_ulogic;
      --Monitor signals
      mon_dvfs_in         : in  monitor_dvfs_type;
      mon_noc             : in  monitor_noc_vector(1 to 6);
      mon_acc             : out monitor_acc_type;
      mon_cache           : out monitor_cache_type;
      mon_dvfs            : out monitor_dvfs_type
      );
  end component tile_acc;

  component tile_io is
    generic (
      SIMULATION   : boolean              := false;
      this_has_dco : integer range 0 to 1 := 0);
    port (
      raw_rstn           : in  std_ulogic;
      tile_rst           : in  std_ulogic;
      clk                : in  std_ulogic;
      refclk_noc         : in  std_ulogic;
      pllclk_noc         : out std_ulogic;
      refclk             : in  std_ulogic;
      pllbypass          : in  std_ulogic;
      pllclk             : out std_ulogic;
      dco_clk            : out std_ulogic;
      dco_rstn           : out std_ulogic;
      -- DCO config
      dco_freq_sel       : in std_logic_vector(1 downto 0);
      dco_div_sel        : in std_logic_vector(2 downto 0);
      dco_fc_sel         : in std_logic_vector(5 downto 0);
      dco_cc_sel         : in std_logic_vector(5 downto 0);
      dco_clk_sel        : in std_ulogic;
      dco_en             : in std_ulogic;  
      -- NoC DCO config
      dco_noc_freq_sel       : in std_logic_vector(1 downto 0);
      dco_noc_div_sel        : in std_logic_vector(2 downto 0);
      dco_noc_fc_sel         : in std_logic_vector(5 downto 0);
      dco_noc_cc_sel         : in std_logic_vector(5 downto 0);
      dco_noc_clk_sel        : in std_ulogic;
      dco_noc_en             : in std_ulogic;  
      -- I/O bus interfaces
      eth0_apbi          : out apb_slv_in_type;
      eth0_apbo          : in  apb_slv_out_type;
      sgmii0_apbi        : out apb_slv_in_type;
      sgmii0_apbo        : in  apb_slv_out_type;
      eth0_ahbmi         : out ahb_mst_in_type;
      eth0_ahbmo         : in  ahb_mst_out_type;
      edcl_ahbmo         : in  ahb_mst_out_type;
      dvi_apbi           : out apb_slv_in_type;
      dvi_apbo           : in  apb_slv_out_type;
      dvi_ahbmi          : out ahb_mst_in_type;
      dvi_ahbmo          : in  ahb_mst_out_type;
      uart_rxd           : in  std_ulogic;
      uart_txd           : out std_ulogic;
      uart_ctsn          : in  std_ulogic;
      uart_rtsn          : out std_ulogic;
      -- NOC
      sys_clk_out        : out std_ulogic;
      sys_clk_lock       : out std_ulogic;
      test1_output_port   : in noc_flit_type;
      test1_data_void_out : in std_ulogic;
      test1_stop_in       : in std_ulogic;
      test2_output_port   : in noc_flit_type;
      test2_data_void_out : in std_ulogic;
      test2_stop_in       : in std_ulogic;
      test3_output_port   : in noc_flit_type;
      test3_data_void_out : in std_ulogic;
      test3_stop_in       : in std_ulogic;
      test4_output_port   : in noc_flit_type;
      test4_data_void_out : in std_ulogic;
      test4_stop_in       : in std_ulogic;
      test5_output_port   : in misc_noc_flit_type;
      test5_data_void_out : in std_ulogic;
      test5_stop_in       : in std_ulogic;
      test6_output_port   : in noc_flit_type;
      test6_data_void_out : in std_ulogic;
      test6_stop_in       : in std_ulogic;
      test1_input_port    : out noc_flit_type;
      test1_data_void_in  : out std_ulogic;
      test1_stop_out      : out std_ulogic;
      test2_input_port    : out noc_flit_type;
      test2_data_void_in  : out std_ulogic;
      test2_stop_out      : out std_ulogic;
      test3_input_port    : out noc_flit_type;
      test3_data_void_in  : out std_ulogic;
      test3_stop_out      : out std_ulogic;
      test4_input_port    : out noc_flit_type;
      test4_data_void_in  : out std_ulogic;
      test4_stop_out      : out std_ulogic;
      test5_input_port    : out misc_noc_flit_type;
      test5_data_void_in  : out std_ulogic;
      test5_stop_out      : out std_ulogic;
      test6_input_port    : out noc_flit_type;
      test6_data_void_in  : out std_ulogic;
      test6_stop_out      : out std_ulogic;
      mon_noc             : in  monitor_noc_vector(1 to 6);
      mon_dvfs            : out monitor_dvfs_type);
  end component tile_io;

  component tile_mem is
    generic (
      SIMULATION   : boolean := false;
      this_has_dco : integer range 0 to 1 := 0;
      this_has_ddr : integer range 0 to 1 := 1);
    port (
      raw_rstn           : in  std_ulogic;
      tile_rst           : in  std_ulogic;
      refclk             : in  std_ulogic;
      clk                : in  std_ulogic;
      pllbypass          : in  std_ulogic;
      pllclk             : out std_ulogic;
      dco_clk            : out std_ulogic;
      -- DCO config
      dco_freq_sel       : in std_logic_vector(1 downto 0);
      dco_div_sel        : in std_logic_vector(2 downto 0);
      dco_fc_sel         : in std_logic_vector(5 downto 0);
      dco_cc_sel         : in std_logic_vector(5 downto 0);
      dco_clk_sel        : in std_ulogic;
      dco_en             : in std_ulogic;  
      dco_clk_delay_sel  : in std_logic_vector(3 downto 0);
      -- DDR controller ports (this_has_ddr -> 1)
      dco_clk_div2       : out std_ulogic;
      dco_clk_div2_90    : out std_ulogic;
      dco_rstn           : out std_ulogic;
      phy_rstn           : out std_ulogic;
      ddr_ahbsi          : out ahb_slv_in_type;
      ddr_ahbso          : in  ahb_slv_out_type;
      -- FPGA proxy memory link (this_has_ddr -> 0)
      fpga_data_in       : in  std_logic_vector(ARCH_BITS - 1 downto 0);
      fpga_data_out      : out std_logic_vector(ARCH_BITS - 1 downto 0);
      fpga_oen           : out std_ulogic;
      fpga_valid_in      : in  std_ulogic;
      fpga_valid_out     : out std_ulogic;
      fpga_clk_in        : in  std_ulogic;
      fpga_clk_out       : out std_ulogic;
      fpga_credit_in     : in  std_ulogic;
      fpga_credit_out    : out std_ulogic;
      -- NOC
      test1_output_port   : in noc_flit_type;
      test1_data_void_out : in std_ulogic;
      test1_stop_in       : in std_ulogic;
      test2_output_port   : in noc_flit_type;
      test2_data_void_out : in std_ulogic;
      test2_stop_in       : in std_ulogic;
      test3_output_port   : in noc_flit_type;
      test3_data_void_out : in std_ulogic;
      test3_stop_in       : in std_ulogic;
      test4_output_port   : in noc_flit_type;
      test4_data_void_out : in std_ulogic;
      test4_stop_in       : in std_ulogic;
      test5_output_port   : in misc_noc_flit_type;
      test5_data_void_out : in std_ulogic;
      test5_stop_in       : in std_ulogic;
      test6_output_port   : in noc_flit_type;
      test6_data_void_out : in std_ulogic;
      test6_stop_in       : in std_ulogic;
      test1_input_port    : out noc_flit_type;
      test1_data_void_in  : out std_ulogic;
      test1_stop_out      : out std_ulogic;
      test2_input_port    : out noc_flit_type;
      test2_data_void_in  : out std_ulogic;
      test2_stop_out      : out std_ulogic;
      test3_input_port    : out noc_flit_type;
      test3_data_void_in  : out std_ulogic;
      test3_stop_out      : out std_ulogic;
      test4_input_port    : out noc_flit_type;
      test4_data_void_in  : out std_ulogic;
      test4_stop_out      : out std_ulogic;
      test5_input_port    : out misc_noc_flit_type;
      test5_data_void_in  : out std_ulogic;
      test5_stop_out      : out std_ulogic;
      test6_input_port    : out noc_flit_type;
      test6_data_void_in  : out std_ulogic;
      test6_stop_out      : out std_ulogic;
      mon_noc             : in  monitor_noc_vector(1 to 6);
      mon_mem             : out monitor_mem_type;
      mon_cache           : out monitor_cache_type;
      mon_dvfs            : out monitor_dvfs_type);
  end component tile_mem;

  component tile_empty is
    generic (
      SIMULATION   : boolean              := false;
      this_has_dco : integer range 0 to 1 := 0);
    port (
      raw_rstn           : in  std_ulogic;
      tile_rst           : in  std_logic;
      clk                : in  std_logic;
      refclk             : in  std_ulogic;
      pllbypass          : in  std_ulogic;
      pllclk             : out std_ulogic;
      dco_clk            : out std_ulogic;
      dco_rstn           : out std_ulogic;
      -- DCO config
      dco_freq_sel       : in std_logic_vector(1 downto 0);
      dco_div_sel        : in std_logic_vector(2 downto 0);
      dco_fc_sel         : in std_logic_vector(5 downto 0);
      dco_cc_sel         : in std_logic_vector(5 downto 0);
      dco_clk_sel        : in std_ulogic;
      dco_en             : in std_ulogic;  
      -- NoC
      test1_output_port   : in noc_flit_type;
      test1_data_void_out : in std_ulogic;
      test1_stop_in       : in std_ulogic;
      test2_output_port   : in noc_flit_type;
      test2_data_void_out : in std_ulogic;
      test2_stop_in       : in std_ulogic;
      test3_output_port   : in noc_flit_type;
      test3_data_void_out : in std_ulogic;
      test3_stop_in       : in std_ulogic;
      test4_output_port   : in noc_flit_type;
      test4_data_void_out : in std_ulogic;
      test4_stop_in       : in std_ulogic;
      test5_output_port   : in misc_noc_flit_type;
      test5_data_void_out : in std_ulogic;
      test5_stop_in       : in std_ulogic;
      test6_output_port   : in noc_flit_type;
      test6_data_void_out : in std_ulogic;
      test6_stop_in       : in std_ulogic;
      test1_input_port    : out noc_flit_type;
      test1_data_void_in  : out std_ulogic;
      test1_stop_out      : out std_ulogic;
      test2_input_port    : out noc_flit_type;
      test2_data_void_in  : out std_ulogic;
      test2_stop_out      : out std_ulogic;
      test3_input_port    : out noc_flit_type;
      test3_data_void_in  : out std_ulogic;
      test3_stop_out      : out std_ulogic;
      test4_input_port    : out noc_flit_type;
      test4_data_void_in  : out std_ulogic;
      test4_stop_out      : out std_ulogic;
      test5_input_port    : out misc_noc_flit_type;
      test5_data_void_in  : out std_ulogic;
      test5_stop_out      : out std_ulogic;
      test6_input_port    : out noc_flit_type;
      test6_data_void_in  : out std_ulogic;
      test6_stop_out      : out std_ulogic;
      mon_noc             : in  monitor_noc_vector(1 to 6);
      mon_dvfs_out        : out monitor_dvfs_type);
  end component tile_empty;

  component tile_slm is
    generic (
      SIMULATION   : boolean := false;
      this_has_dco : integer range 0 to 1 := 0;
      this_has_ddr : integer range 0 to 1 := 0);
    port (
      raw_rstn           : in  std_ulogic;
      tile_rst           : in  std_ulogic;
      clk                : in  std_ulogic;
      refclk             : in  std_ulogic;
      pllbypass          : in  std_ulogic;
      pllclk             : out std_ulogic;
      dco_clk            : out std_ulogic;
      -- DCO config
      dco_freq_sel       : in std_logic_vector(1 downto 0);
      dco_div_sel        : in std_logic_vector(2 downto 0);
      dco_fc_sel         : in std_logic_vector(5 downto 0);
      dco_cc_sel         : in std_logic_vector(5 downto 0);
      dco_clk_sel        : in std_ulogic;
      dco_en             : in std_ulogic;  
      dco_clk_delay_sel  : in std_logic_vector(3 downto 0);
      -- DDR controller ports (this_has_ddr -> 1)
      dco_clk_div2       : out std_ulogic;
      dco_clk_div2_90    : out std_ulogic;
      dco_rstn           : out std_ulogic;
      phy_rstn           : out std_ulogic;
      ddr_ahbsi          : out ahb_slv_in_type;
      ddr_ahbso          : in  ahb_slv_out_type;
      -- NoC
      test1_output_port   : in noc_flit_type;
      test1_data_void_out : in std_ulogic;
      test1_stop_in       : in std_ulogic;
      test2_output_port   : in noc_flit_type;
      test2_data_void_out : in std_ulogic;
      test2_stop_in       : in std_ulogic;
      test3_output_port   : in noc_flit_type;
      test3_data_void_out : in std_ulogic;
      test3_stop_in       : in std_ulogic;
      test4_output_port   : in noc_flit_type;
      test4_data_void_out : in std_ulogic;
      test4_stop_in       : in std_ulogic;
      test5_output_port   : in misc_noc_flit_type;
      test5_data_void_out : in std_ulogic;
      test5_stop_in       : in std_ulogic;
      test6_output_port   : in noc_flit_type;
      test6_data_void_out : in std_ulogic;
      test6_stop_in       : in std_ulogic;
      test1_input_port    : out noc_flit_type;
      test1_data_void_in  : out std_ulogic;
      test1_stop_out      : out std_ulogic;
      test2_input_port    : out noc_flit_type;
      test2_data_void_in  : out std_ulogic;
      test2_stop_out      : out std_ulogic;
      test3_input_port    : out noc_flit_type;
      test3_data_void_in  : out std_ulogic;
      test3_stop_out      : out std_ulogic;
      test4_input_port    : out noc_flit_type;
      test4_data_void_in  : out std_ulogic;
      test4_stop_out      : out std_ulogic;
      test5_input_port    : out misc_noc_flit_type;
      test5_data_void_in  : out std_ulogic;
      test5_stop_out      : out std_ulogic;
      test6_input_port    : out noc_flit_type;
      test6_data_void_in  : out std_ulogic;
      test6_stop_out      : out std_ulogic;
      mon_noc             : in  monitor_noc_vector(1 to 6);
      mon_mem             : out monitor_mem_type;
      mon_dvfs            : out monitor_dvfs_type);
  end component tile_slm;

end tiles_pkg;
