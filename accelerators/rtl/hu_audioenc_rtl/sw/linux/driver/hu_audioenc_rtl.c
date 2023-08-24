// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "hu_audioenc_rtl.h"

#define DRV_NAME	"hu_audioenc_rtl"

/* <<--regs-->> */
#define HU_AUDIOENC_CFG_REGS_00_REG 0x40
#define HU_AUDIOENC_CFG_REGS_01_REG 0x44
#define HU_AUDIOENC_CFG_REGS_02_REG 0x48
#define HU_AUDIOENC_CFG_REGS_03_REG 0x4c
#define HU_AUDIOENC_CFG_REGS_04_REG 0x50
#define HU_AUDIOENC_CFG_REGS_05_REG 0x54
#define HU_AUDIOENC_CFG_REGS_06_REG 0x58
#define HU_AUDIOENC_CFG_REGS_07_REG 0x5c
#define HU_AUDIOENC_CFG_REGS_08_REG 0x60
#define HU_AUDIOENC_CFG_REGS_09_REG 0x64
#define HU_AUDIOENC_CFG_REGS_10_REG 0x68
#define HU_AUDIOENC_CFG_REGS_11_REG 0x6c
#define HU_AUDIOENC_CFG_REGS_12_REG 0x70
#define HU_AUDIOENC_CFG_REGS_13_REG 0x74
#define HU_AUDIOENC_CFG_REGS_14_REG 0x78
#define HU_AUDIOENC_CFG_REGS_15_REG 0x7c
#define HU_AUDIOENC_CFG_REGS_16_REG 0x80
#define HU_AUDIOENC_CFG_REGS_17_REG 0x84
#define HU_AUDIOENC_CFG_REGS_18_REG 0x88
#define HU_AUDIOENC_CFG_REGS_19_REG 0x8c
#define HU_AUDIOENC_CFG_REGS_20_REG 0x90
#define HU_AUDIOENC_CFG_REGS_21_REG 0x94
#define HU_AUDIOENC_CFG_REGS_22_REG 0x98
#define HU_AUDIOENC_CFG_REGS_23_REG 0x9c
#define HU_AUDIOENC_CFG_REGS_24_REG 0xa0
#define HU_AUDIOENC_CFG_REGS_25_REG 0xa4
#define HU_AUDIOENC_CFG_REGS_26_REG 0xa8
#define HU_AUDIOENC_CFG_REGS_27_REG 0xac
#define HU_AUDIOENC_CFG_REGS_28_REG 0xb0
#define HU_AUDIOENC_CFG_REGS_29_REG 0xb4
#define HU_AUDIOENC_CFG_REGS_30_REG 0xb8
#define HU_AUDIOENC_CFG_REGS_31_REG 0xbc
#define HU_AUDIOENC_CFG_REGS_32_REG 0xc0
#define HU_AUDIOENC_CFG_REGS_33_REG 0xc4
#define HU_AUDIOENC_CFG_REGS_34_REG 0xc8
#define HU_AUDIOENC_CFG_REGS_35_REG 0xcc
#define HU_AUDIOENC_CFG_REGS_36_REG 0xd0
#define HU_AUDIOENC_CFG_REGS_37_REG 0xd4
#define HU_AUDIOENC_CFG_REGS_38_REG 0xd8
#define HU_AUDIOENC_CFG_REGS_39_REG 0xdc
#define HU_AUDIOENC_CFG_REGS_40_REG 0xe0
#define HU_AUDIOENC_CFG_REGS_41_REG 0xe4
#define HU_AUDIOENC_CFG_REGS_42_REG 0xe8
#define HU_AUDIOENC_CFG_REGS_43_REG 0xec
#define HU_AUDIOENC_CFG_REGS_44_REG 0xf0
#define HU_AUDIOENC_CFG_REGS_45_REG 0xf4
#define HU_AUDIOENC_CFG_REGS_46_REG 0xf8
#define HU_AUDIOENC_CFG_REGS_47_REG 0xfc

struct hu_audioenc_rtl_device {
	struct esp_device esp;
};

static struct esp_driver hu_audioenc_driver;

static struct of_device_id hu_audioenc_device_ids[] = {
	{
		.name = "SLD_HU_AUDIOENC_RTL",
	},
	{
		.name = "eb_088",
	},
	{
		.compatible = "sld,hu_audioenc_rtl",
	},
	{ },
};

static int hu_audioenc_devs;

static inline struct hu_audioenc_rtl_device *to_hu_audioenc(struct esp_device *esp)
{
	return container_of(esp, struct hu_audioenc_rtl_device, esp);
}

static void hu_audioenc_prep_xfer(struct esp_device *esp, void *arg)
{
	struct hu_audioenc_rtl_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->cfg_regs_00, esp->iomem + HU_AUDIOENC_CFG_REGS_00_REG);
	iowrite32be(a->cfg_regs_01, esp->iomem + HU_AUDIOENC_CFG_REGS_01_REG);
	iowrite32be(a->cfg_regs_02, esp->iomem + HU_AUDIOENC_CFG_REGS_02_REG);
	iowrite32be(a->cfg_regs_03, esp->iomem + HU_AUDIOENC_CFG_REGS_03_REG);
	iowrite32be(a->cfg_regs_04, esp->iomem + HU_AUDIOENC_CFG_REGS_04_REG);
	iowrite32be(a->cfg_regs_05, esp->iomem + HU_AUDIOENC_CFG_REGS_05_REG);
	iowrite32be(a->cfg_regs_06, esp->iomem + HU_AUDIOENC_CFG_REGS_06_REG);
	iowrite32be(a->cfg_regs_07, esp->iomem + HU_AUDIOENC_CFG_REGS_07_REG);
	iowrite32be(a->cfg_regs_08, esp->iomem + HU_AUDIOENC_CFG_REGS_08_REG);
	iowrite32be(a->cfg_regs_09, esp->iomem + HU_AUDIOENC_CFG_REGS_09_REG);
	iowrite32be(a->cfg_regs_10, esp->iomem + HU_AUDIOENC_CFG_REGS_10_REG);
	iowrite32be(a->cfg_regs_11, esp->iomem + HU_AUDIOENC_CFG_REGS_11_REG);
	iowrite32be(a->cfg_regs_12, esp->iomem + HU_AUDIOENC_CFG_REGS_12_REG);
	iowrite32be(a->cfg_regs_13, esp->iomem + HU_AUDIOENC_CFG_REGS_13_REG);
	iowrite32be(a->cfg_regs_14, esp->iomem + HU_AUDIOENC_CFG_REGS_14_REG);
	iowrite32be(a->cfg_regs_15, esp->iomem + HU_AUDIOENC_CFG_REGS_15_REG);
	iowrite32be(a->cfg_regs_16, esp->iomem + HU_AUDIOENC_CFG_REGS_16_REG);
	iowrite32be(a->cfg_regs_17, esp->iomem + HU_AUDIOENC_CFG_REGS_17_REG);
	iowrite32be(a->cfg_regs_18, esp->iomem + HU_AUDIOENC_CFG_REGS_18_REG);
	iowrite32be(a->cfg_regs_19, esp->iomem + HU_AUDIOENC_CFG_REGS_19_REG);
	iowrite32be(a->cfg_regs_20, esp->iomem + HU_AUDIOENC_CFG_REGS_20_REG);
	iowrite32be(a->cfg_regs_21, esp->iomem + HU_AUDIOENC_CFG_REGS_21_REG);
	iowrite32be(a->cfg_regs_22, esp->iomem + HU_AUDIOENC_CFG_REGS_22_REG);
	iowrite32be(a->cfg_regs_23, esp->iomem + HU_AUDIOENC_CFG_REGS_23_REG);
	iowrite32be(a->cfg_regs_24, esp->iomem + HU_AUDIOENC_CFG_REGS_24_REG);
	iowrite32be(a->cfg_regs_25, esp->iomem + HU_AUDIOENC_CFG_REGS_25_REG);
	iowrite32be(a->cfg_regs_26, esp->iomem + HU_AUDIOENC_CFG_REGS_26_REG);
	iowrite32be(a->cfg_regs_27, esp->iomem + HU_AUDIOENC_CFG_REGS_27_REG);
	iowrite32be(a->cfg_regs_28, esp->iomem + HU_AUDIOENC_CFG_REGS_28_REG);
	iowrite32be(a->cfg_regs_29, esp->iomem + HU_AUDIOENC_CFG_REGS_29_REG);
	iowrite32be(a->cfg_regs_30, esp->iomem + HU_AUDIOENC_CFG_REGS_30_REG);
	iowrite32be(a->cfg_regs_31, esp->iomem + HU_AUDIOENC_CFG_REGS_31_REG);
	iowrite32be(a->cfg_regs_32, esp->iomem + HU_AUDIOENC_CFG_REGS_32_REG);
	iowrite32be(a->cfg_regs_33, esp->iomem + HU_AUDIOENC_CFG_REGS_33_REG);
	iowrite32be(a->cfg_regs_34, esp->iomem + HU_AUDIOENC_CFG_REGS_34_REG);
	iowrite32be(a->cfg_regs_35, esp->iomem + HU_AUDIOENC_CFG_REGS_35_REG);
	iowrite32be(a->cfg_regs_36, esp->iomem + HU_AUDIOENC_CFG_REGS_36_REG);
	iowrite32be(a->cfg_regs_37, esp->iomem + HU_AUDIOENC_CFG_REGS_37_REG);
	iowrite32be(a->cfg_regs_38, esp->iomem + HU_AUDIOENC_CFG_REGS_38_REG);
	iowrite32be(a->cfg_regs_39, esp->iomem + HU_AUDIOENC_CFG_REGS_39_REG);
	iowrite32be(a->cfg_regs_40, esp->iomem + HU_AUDIOENC_CFG_REGS_40_REG);
	iowrite32be(a->cfg_regs_41, esp->iomem + HU_AUDIOENC_CFG_REGS_41_REG);
	iowrite32be(a->cfg_regs_42, esp->iomem + HU_AUDIOENC_CFG_REGS_42_REG);
	iowrite32be(a->cfg_regs_43, esp->iomem + HU_AUDIOENC_CFG_REGS_43_REG);
	iowrite32be(a->cfg_regs_44, esp->iomem + HU_AUDIOENC_CFG_REGS_44_REG);
	iowrite32be(a->cfg_regs_45, esp->iomem + HU_AUDIOENC_CFG_REGS_45_REG);
	iowrite32be(a->cfg_regs_46, esp->iomem + HU_AUDIOENC_CFG_REGS_46_REG);
	iowrite32be(a->cfg_regs_47, esp->iomem + HU_AUDIOENC_CFG_REGS_47_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool hu_audioenc_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct hu_audioenc_rtl_device *hu_audioenc = to_hu_audioenc(esp); */
	/* struct hu_audioenc_rtl_access *a = arg; */

	return true;
}

static int hu_audioenc_probe(struct platform_device *pdev)
{
	struct hu_audioenc_rtl_device *hu_audioenc;
	struct esp_device *esp;
	int rc;

	hu_audioenc = kzalloc(sizeof(*hu_audioenc), GFP_KERNEL);
	if (hu_audioenc == NULL)
		return -ENOMEM;
	esp = &hu_audioenc->esp;
	esp->module = THIS_MODULE;
	esp->number = hu_audioenc_devs;
	esp->driver = &hu_audioenc_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	hu_audioenc_devs++;
	return 0;
 err:
	kfree(hu_audioenc);
	return rc;
}

static int __exit hu_audioenc_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct hu_audioenc_rtl_device *hu_audioenc = to_hu_audioenc(esp);

	esp_device_unregister(esp);
	kfree(hu_audioenc);
	return 0;
}

static struct esp_driver hu_audioenc_driver = {
	.plat = {
		.probe		= hu_audioenc_probe,
		.remove		= hu_audioenc_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = hu_audioenc_device_ids,
		},
	},
	.xfer_input_ok	= hu_audioenc_xfer_input_ok,
	.prep_xfer	= hu_audioenc_prep_xfer,
	.ioctl_cm	= HU_AUDIOENC_RTL_IOC_ACCESS,
	.arg_size	= sizeof(struct hu_audioenc_rtl_access),
};

static int __init hu_audioenc_init(void)
{
	return esp_driver_register(&hu_audioenc_driver);
}

static void __exit hu_audioenc_exit(void)
{
	esp_driver_unregister(&hu_audioenc_driver);
}

module_init(hu_audioenc_init)
module_exit(hu_audioenc_exit)

MODULE_DEVICE_TABLE(of, hu_audioenc_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hu_audioenc_rtl driver");
