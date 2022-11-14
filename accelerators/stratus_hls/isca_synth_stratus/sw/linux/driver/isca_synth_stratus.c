// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "isca_synth_stratus.h"

#define DRV_NAME	"isca_synth_stratus"

/* <<--regs-->> */
#define ISCA_SYNTH_COMPUTE_RATIO_REG 0x44
#define ISCA_SYNTH_SIZE_REG 0x40

struct isca_synth_stratus_device {
	struct esp_device esp;
};

static struct esp_driver isca_synth_driver;

static struct of_device_id isca_synth_device_ids[] = {
	{
		.name = "SLD_ISCA_SYNTH_STRATUS",
	},
	{
		.name = "eb_04c",
	},
	{
		.compatible = "sld,isca_synth_stratus",
	},
	{ },
};

static int isca_synth_devs;

static inline struct isca_synth_stratus_device *to_isca_synth(struct esp_device *esp)
{
	return container_of(esp, struct isca_synth_stratus_device, esp);
}

static void isca_synth_prep_xfer(struct esp_device *esp, void *arg)
{
	struct isca_synth_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->compute_ratio, esp->iomem + ISCA_SYNTH_COMPUTE_RATIO_REG);
	iowrite32be(a->size, esp->iomem + ISCA_SYNTH_SIZE_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool isca_synth_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct isca_synth_stratus_device *isca_synth = to_isca_synth(esp); */
	/* struct isca_synth_stratus_access *a = arg; */

	return true;
}

static int isca_synth_probe(struct platform_device *pdev)
{
	struct isca_synth_stratus_device *isca_synth;
	struct esp_device *esp;
	int rc;

	isca_synth = kzalloc(sizeof(*isca_synth), GFP_KERNEL);
	if (isca_synth == NULL)
		return -ENOMEM;
	esp = &isca_synth->esp;
	esp->module = THIS_MODULE;
	esp->number = isca_synth_devs;
	esp->driver = &isca_synth_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	isca_synth_devs++;
	return 0;
 err:
	kfree(isca_synth);
	return rc;
}

static int __exit isca_synth_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct isca_synth_stratus_device *isca_synth = to_isca_synth(esp);

	esp_device_unregister(esp);
	kfree(isca_synth);
	return 0;
}

static struct esp_driver isca_synth_driver = {
	.plat = {
		.probe		= isca_synth_probe,
		.remove		= isca_synth_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = isca_synth_device_ids,
		},
	},
	.xfer_input_ok	= isca_synth_xfer_input_ok,
	.prep_xfer	= isca_synth_prep_xfer,
	.ioctl_cm	= ISCA_SYNTH_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct isca_synth_stratus_access),
};

static int __init isca_synth_init(void)
{
	return esp_driver_register(&isca_synth_driver);
}

static void __exit isca_synth_exit(void)
{
	esp_driver_unregister(&isca_synth_driver);
}

module_init(isca_synth_init)
module_exit(isca_synth_exit)

MODULE_DEVICE_TABLE(of, isca_synth_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("isca_synth_stratus driver");
