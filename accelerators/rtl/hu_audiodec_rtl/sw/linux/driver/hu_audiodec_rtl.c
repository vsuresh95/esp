// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "hu_audiodec_rtl.h"

#define DRV_NAME	"hu_audiodec_rtl"

/* <<--regs-->> */
#define HU_AUDIODEC_CFG_REGS_31_REG 0xbc
#define HU_AUDIODEC_CFG_REGS_30_REG 0xb8
#define HU_AUDIODEC_CFG_REGS_26_REG 0xb4
#define HU_AUDIODEC_CFG_REGS_27_REG 0xb0
#define HU_AUDIODEC_CFG_REGS_24_REG 0xac
#define HU_AUDIODEC_CFG_REGS_25_REG 0xa8
#define HU_AUDIODEC_CFG_REGS_22_REG 0xa4
#define HU_AUDIODEC_CFG_REGS_23_REG 0xa0
#define HU_AUDIODEC_CFG_REGS_8_REG 0x9c
#define HU_AUDIODEC_CFG_REGS_20_REG 0x98
#define HU_AUDIODEC_CFG_REGS_9_REG 0x94
#define HU_AUDIODEC_CFG_REGS_21_REG 0x90
#define HU_AUDIODEC_CFG_REGS_6_REG 0x8c
#define HU_AUDIODEC_CFG_REGS_7_REG 0x88
#define HU_AUDIODEC_CFG_REGS_4_REG 0x84
#define HU_AUDIODEC_CFG_REGS_5_REG 0x80
#define HU_AUDIODEC_CFG_REGS_2_REG 0x7c
#define HU_AUDIODEC_CFG_REGS_3_REG 0x78
#define HU_AUDIODEC_CFG_REGS_0_REG 0x74
#define HU_AUDIODEC_CFG_REGS_28_REG 0x70
#define HU_AUDIODEC_CFG_REGS_1_REG 0x6c
#define HU_AUDIODEC_CFG_REGS_29_REG 0x68
#define HU_AUDIODEC_CFG_REGS_19_REG 0x64
#define HU_AUDIODEC_CFG_REGS_18_REG 0x60
#define HU_AUDIODEC_CFG_REGS_17_REG 0x5c
#define HU_AUDIODEC_CFG_REGS_16_REG 0x58
#define HU_AUDIODEC_CFG_REGS_15_REG 0x54
#define HU_AUDIODEC_CFG_REGS_14_REG 0x50
#define HU_AUDIODEC_CFG_REGS_13_REG 0x4c
#define HU_AUDIODEC_CFG_REGS_12_REG 0x48
#define HU_AUDIODEC_CFG_REGS_11_REG 0x44
#define HU_AUDIODEC_CFG_REGS_10_REG 0x40

struct hu_audiodec_rtl_device {
	struct esp_device esp;
};

static struct esp_driver hu_audiodec_driver;

static struct of_device_id hu_audiodec_device_ids[] = {
	{
		.name = "SLD_HU_AUDIODEC_RTL",
	},
	{
		.name = "eb_040",
	},
	{
		.compatible = "sld,hu_audiodec_rtl",
	},
	{ },
};

static int hu_audiodec_devs;

static inline struct hu_audiodec_rtl_device *to_hu_audiodec(struct esp_device *esp)
{
	return container_of(esp, struct hu_audiodec_rtl_device, esp);
}

static void hu_audiodec_prep_xfer(struct esp_device *esp, void *arg)
{
	struct hu_audiodec_rtl_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->cfg_regs_31, esp->iomem + HU_AUDIODEC_CFG_REGS_31_REG);
	iowrite32be(a->cfg_regs_30, esp->iomem + HU_AUDIODEC_CFG_REGS_30_REG);
	iowrite32be(a->cfg_regs_26, esp->iomem + HU_AUDIODEC_CFG_REGS_26_REG);
	iowrite32be(a->cfg_regs_27, esp->iomem + HU_AUDIODEC_CFG_REGS_27_REG);
	iowrite32be(a->cfg_regs_24, esp->iomem + HU_AUDIODEC_CFG_REGS_24_REG);
	iowrite32be(a->cfg_regs_25, esp->iomem + HU_AUDIODEC_CFG_REGS_25_REG);
	iowrite32be(a->cfg_regs_22, esp->iomem + HU_AUDIODEC_CFG_REGS_22_REG);
	iowrite32be(a->cfg_regs_23, esp->iomem + HU_AUDIODEC_CFG_REGS_23_REG);
	iowrite32be(a->cfg_regs_8, esp->iomem + HU_AUDIODEC_CFG_REGS_8_REG);
	iowrite32be(a->cfg_regs_20, esp->iomem + HU_AUDIODEC_CFG_REGS_20_REG);
	iowrite32be(a->cfg_regs_9, esp->iomem + HU_AUDIODEC_CFG_REGS_9_REG);
	iowrite32be(a->cfg_regs_21, esp->iomem + HU_AUDIODEC_CFG_REGS_21_REG);
	iowrite32be(a->cfg_regs_6, esp->iomem + HU_AUDIODEC_CFG_REGS_6_REG);
	iowrite32be(a->cfg_regs_7, esp->iomem + HU_AUDIODEC_CFG_REGS_7_REG);
	iowrite32be(a->cfg_regs_4, esp->iomem + HU_AUDIODEC_CFG_REGS_4_REG);
	iowrite32be(a->cfg_regs_5, esp->iomem + HU_AUDIODEC_CFG_REGS_5_REG);
	iowrite32be(a->cfg_regs_2, esp->iomem + HU_AUDIODEC_CFG_REGS_2_REG);
	iowrite32be(a->cfg_regs_3, esp->iomem + HU_AUDIODEC_CFG_REGS_3_REG);
	iowrite32be(a->cfg_regs_0, esp->iomem + HU_AUDIODEC_CFG_REGS_0_REG);
	iowrite32be(a->cfg_regs_28, esp->iomem + HU_AUDIODEC_CFG_REGS_28_REG);
	iowrite32be(a->cfg_regs_1, esp->iomem + HU_AUDIODEC_CFG_REGS_1_REG);
	iowrite32be(a->cfg_regs_29, esp->iomem + HU_AUDIODEC_CFG_REGS_29_REG);
	iowrite32be(a->cfg_regs_19, esp->iomem + HU_AUDIODEC_CFG_REGS_19_REG);
	iowrite32be(a->cfg_regs_18, esp->iomem + HU_AUDIODEC_CFG_REGS_18_REG);
	iowrite32be(a->cfg_regs_17, esp->iomem + HU_AUDIODEC_CFG_REGS_17_REG);
	iowrite32be(a->cfg_regs_16, esp->iomem + HU_AUDIODEC_CFG_REGS_16_REG);
	iowrite32be(a->cfg_regs_15, esp->iomem + HU_AUDIODEC_CFG_REGS_15_REG);
	iowrite32be(a->cfg_regs_14, esp->iomem + HU_AUDIODEC_CFG_REGS_14_REG);
	iowrite32be(a->cfg_regs_13, esp->iomem + HU_AUDIODEC_CFG_REGS_13_REG);
	iowrite32be(a->cfg_regs_12, esp->iomem + HU_AUDIODEC_CFG_REGS_12_REG);
	iowrite32be(a->cfg_regs_11, esp->iomem + HU_AUDIODEC_CFG_REGS_11_REG);
	iowrite32be(a->cfg_regs_10, esp->iomem + HU_AUDIODEC_CFG_REGS_10_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool hu_audiodec_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct hu_audiodec_rtl_device *hu_audiodec = to_hu_audiodec(esp); */
	/* struct hu_audiodec_rtl_access *a = arg; */

	return true;
}

static int hu_audiodec_probe(struct platform_device *pdev)
{
	struct hu_audiodec_rtl_device *hu_audiodec;
	struct esp_device *esp;
	int rc;

	hu_audiodec = kzalloc(sizeof(*hu_audiodec), GFP_KERNEL);
	if (hu_audiodec == NULL)
		return -ENOMEM;
	esp = &hu_audiodec->esp;
	esp->module = THIS_MODULE;
	esp->number = hu_audiodec_devs;
	esp->driver = &hu_audiodec_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	hu_audiodec_devs++;
	return 0;
 err:
	kfree(hu_audiodec);
	return rc;
}

static int __exit hu_audiodec_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct hu_audiodec_rtl_device *hu_audiodec = to_hu_audiodec(esp);

	esp_device_unregister(esp);
	kfree(hu_audiodec);
	return 0;
}

static struct esp_driver hu_audiodec_driver = {
	.plat = {
		.probe		= hu_audiodec_probe,
		.remove		= hu_audiodec_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = hu_audiodec_device_ids,
		},
	},
	.xfer_input_ok	= hu_audiodec_xfer_input_ok,
	.prep_xfer	= hu_audiodec_prep_xfer,
	.ioctl_cm	= HU_AUDIODEC_RTL_IOC_ACCESS,
	.arg_size	= sizeof(struct hu_audiodec_rtl_access),
};

static int __init hu_audiodec_init(void)
{
	return esp_driver_register(&hu_audiodec_driver);
}

static void __exit hu_audiodec_exit(void)
{
	esp_driver_unregister(&hu_audiodec_driver);
}

module_init(hu_audiodec_init)
module_exit(hu_audiodec_exit)

MODULE_DEVICE_TABLE(of, hu_audiodec_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hu_audiodec_rtl driver");
