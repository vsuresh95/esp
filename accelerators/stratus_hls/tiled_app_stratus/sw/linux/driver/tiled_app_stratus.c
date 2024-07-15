// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "tiled_app_stratus.h"
// #include "stdio.h"

#define DRV_NAME	"tiled_app_stratus"

/* <<--regs-->> */
//#define TILED_APP_OUTPUT_TILE_START_OFFSET_REG (0x60+4)
//#define TILED_APP_INPUT_TILE_START_OFFSET_REG (0x5C+4)
//#define TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG (0x58+4)
//#define TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG (0x54+4)
//#define TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG (0x50+4)
//#define TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG (0x4C+4)
//#define TILED_APP_NUM_TILES_REG (0x48+4)
//#define TILED_APP_TILE_SIZE_REG (0x44+4)
//#define TILED_APP_RD_WR_ENABLE_REG (0x40+4)
//// New registers
//#define TILED_APP_PING_PONG_EN_REG (0x6C+4)
//#define TILED_APP_COMPUTE_OVER_DATA_REG (0x68+4)
//#define TILED_APP_COMPUTE_ITERS_REG (0x64+4)
//#define TILED_APP_NUM_COMP_UNITS_REG 0x40

/* <<--regs-->> */
#define TILED_APP_NUM_COMP_UNITS_REG 0x40
#define TILED_APP_RD_WR_ENABLE_REG 0x44
#define TILED_APP_TILE_SIZE_REG 0x48
#define TILED_APP_NUM_TILES_REG 0x4C
#define TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG 0x50
#define TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG 0x54
#define TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG 0x58
#define TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG 0x5C
#define TILED_APP_INPUT_TILE_START_OFFSET_REG 0x60
#define TILED_APP_OUTPUT_TILE_START_OFFSET_REG 0x64
#define TILED_APP_COMPUTE_OVER_DATA_REG 0x68
#define TILED_APP_COMPUTE_ITERS_REG 0x6C
#define TILED_APP_PING_PONG_EN_REG 0x70
// #define SPANDEX_REG 0x34

struct tiled_app_stratus_device {
	struct esp_device esp;
};

static struct esp_driver tiled_app_driver;

static struct of_device_id tiled_app_device_ids[] = {
	{
		.name = "SLD_TILED_APP_STRATUS",
	},
	{
		.name = "eb_033",
	},
	{
		.compatible = "sld,tiled_app_stratus",
	},
	{ },
};

static int tiled_app_devs;

static inline struct tiled_app_stratus_device *to_tiled_app(struct esp_device *esp)
{
	return container_of(esp, struct tiled_app_stratus_device, esp);
}

static void tiled_app_prep_xfer(struct esp_device *esp, void *arg)
{
	struct tiled_app_stratus_access *a = arg;
	/* <<--regs-config-->> */
	iowrite32be(a->num_tiles, esp->iomem + TILED_APP_NUM_TILES_REG);
	iowrite32be(a->tile_size, esp->iomem + TILED_APP_TILE_SIZE_REG);
	iowrite32be(a->rd_wr_enable, esp->iomem + TILED_APP_RD_WR_ENABLE_REG);
	// iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	// iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);
	iowrite32be(a->output_tile_start_offset, esp->iomem + TILED_APP_OUTPUT_TILE_START_OFFSET_REG);
	iowrite32be(a->input_tile_start_offset, esp->iomem + TILED_APP_INPUT_TILE_START_OFFSET_REG);
	iowrite32be(a->output_update_sync_offset, esp->iomem + TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG);
	iowrite32be(a->input_update_sync_offset, esp->iomem + TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG);
	iowrite32be(a->output_spin_sync_offset, esp->iomem + TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG);
	iowrite32be(a->input_spin_sync_offset, esp->iomem + TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG);

	iowrite32be(a->ping_pong_en, esp->iomem + TILED_APP_PING_PONG_EN_REG);
	iowrite32be(a->compute_over_data, esp->iomem + TILED_APP_COMPUTE_OVER_DATA_REG);
	iowrite32be(a->compute_iters, esp->iomem + TILED_APP_COMPUTE_ITERS_REG);
	iowrite32be(a->num_comp_units_reg, esp->iomem + TILED_APP_NUM_COMP_UNITS_REG);

	// printf("In %s func %s line %d Setting spandex reg: %x\n",  __FILE__, __func__, __LINE__, a->spandex_reg);
	iowrite32be(a->spandex_reg, esp->iomem + SPANDEX_REG);

}

static bool tiled_app_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct tiled_app_stratus_device *tiled_app = to_tiled_app(esp); */
	/* struct tiled_app_stratus_access *a = arg; */

	return true;
}

static int tiled_app_probe(struct platform_device *pdev)
{
	struct tiled_app_stratus_device *tiled_app;
	struct esp_device *esp;
	int rc;

	tiled_app = kzalloc(sizeof(*tiled_app), GFP_KERNEL);
	if (tiled_app == NULL)
		return -ENOMEM;
	esp = &tiled_app->esp;
	esp->module = THIS_MODULE;
	esp->number = tiled_app_devs;
	esp->driver = &tiled_app_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	tiled_app_devs++;
	return 0;
 err:
	kfree(tiled_app);
	return rc;
}

static int __exit tiled_app_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct tiled_app_stratus_device *tiled_app = to_tiled_app(esp);

	esp_device_unregister(esp);
	kfree(tiled_app);
	return 0;
}

static struct esp_driver tiled_app_driver = {
	.plat = {
		.probe		= tiled_app_probe,
		.remove		= tiled_app_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = tiled_app_device_ids,
		},
	},
	.xfer_input_ok	= tiled_app_xfer_input_ok,
	.prep_xfer	= tiled_app_prep_xfer,
	.ioctl_cm	= TILED_APP_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct tiled_app_stratus_access),
};

static int __init tiled_app_init(void)
{
	return esp_driver_register(&tiled_app_driver);
}

static void __exit tiled_app_exit(void)
{
	esp_driver_unregister(&tiled_app_driver);
}

module_init(tiled_app_init)
module_exit(tiled_app_exit)

MODULE_DEVICE_TABLE(of, tiled_app_device_ids);

MODULE_AUTHOR("Bakshree Mishra <bmishra3@illinois.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("tiled_app_stratus driver");
