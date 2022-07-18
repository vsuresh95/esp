// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "tiled_app_stratus.h"

#define DRV_NAME	"tiled_app_stratus"

/* <<--regs-->> */
#define TILED_APP_NUM_TILES_REG 0x48
#define TILED_APP_TILE_SIZE_REG 0x44
#define TILED_APP_RD_WR_ENABLE_REG 0x40

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
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

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

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("tiled_app_stratus driver");
