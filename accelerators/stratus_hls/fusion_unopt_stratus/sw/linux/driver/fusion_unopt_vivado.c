// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "fusion_unopt_vivado.h"

#define DRV_NAME	"fusion_unopt_vivado"

/* <<--regs-->> */
#define FUSION_UNOPT_VENO_REG 0x54
#define FUSION_UNOPT_IMGWIDTH_REG 0x50
#define FUSION_UNOPT_HTDIM_REG 0x4c
#define FUSION_UNOPT_IMGHEIGHT_REG 0x48
#define FUSION_UNOPT_SDF_BLOCK_SIZE_REG 0x44
#define FUSION_UNOPT_SDF_BLOCK_SIZE3_REG 0x40

struct fusion_unopt_vivado_device {
	struct esp_device esp;
};

static struct esp_driver fusion_unopt_driver;

static struct of_device_id fusion_unopt_device_ids[] = {
	{
		.name = "SLD_FUSION_UNOPT_VIVADO",
	},
	{
		.name = "eb_166",
	},
	{
		.compatible = "sld,fusion_unopt_vivado",
	},
	{ },
};

static int fusion_unopt_devs;

static inline struct fusion_unopt_vivado_device *to_fusion_unopt(struct esp_device *esp)
{
	return container_of(esp, struct fusion_unopt_vivado_device, esp);
}

static void fusion_unopt_prep_xfer(struct esp_device *esp, void *arg)
{
	struct fusion_unopt_vivado_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->veno, esp->iomem + FUSION_UNOPT_VENO_REG);
	iowrite32be(a->imgwidth, esp->iomem + FUSION_UNOPT_IMGWIDTH_REG);
	iowrite32be(a->htdim, esp->iomem + FUSION_UNOPT_HTDIM_REG);
	iowrite32be(a->imgheight, esp->iomem + FUSION_UNOPT_IMGHEIGHT_REG);
	iowrite32be(a->sdf_block_size, esp->iomem + FUSION_UNOPT_SDF_BLOCK_SIZE_REG);
	iowrite32be(a->sdf_block_size3, esp->iomem + FUSION_UNOPT_SDF_BLOCK_SIZE3_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool fusion_unopt_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct fusion_unopt_vivado_device *fusion_unopt = to_fusion_unopt(esp); */
	/* struct fusion_unopt_vivado_access *a = arg; */

	return true;
}

static int fusion_unopt_probe(struct platform_device *pdev)
{
	struct fusion_unopt_vivado_device *fusion_unopt;
	struct esp_device *esp;
	int rc;

	fusion_unopt = kzalloc(sizeof(*fusion_unopt), GFP_KERNEL);
	if (fusion_unopt == NULL)
		return -ENOMEM;
	esp = &fusion_unopt->esp;
	esp->module = THIS_MODULE;
	esp->number = fusion_unopt_devs;
	esp->driver = &fusion_unopt_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	fusion_unopt_devs++;
	return 0;
 err:
	kfree(fusion_unopt);
	return rc;
}

static int __exit fusion_unopt_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct fusion_unopt_vivado_device *fusion_unopt = to_fusion_unopt(esp);

	esp_device_unregister(esp);
	kfree(fusion_unopt);
	return 0;
}

static struct esp_driver fusion_unopt_driver = {
	.plat = {
		.probe		= fusion_unopt_probe,
		.remove		= fusion_unopt_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = fusion_unopt_device_ids,
		},
	},
	.xfer_input_ok	= fusion_unopt_xfer_input_ok,
	.prep_xfer	= fusion_unopt_prep_xfer,
	.ioctl_cm	= FUSION_UNOPT_VIVADO_IOC_ACCESS,
	.arg_size	= sizeof(struct fusion_unopt_vivado_access),
};

static int __init fusion_unopt_init(void)
{
	return esp_driver_register(&fusion_unopt_driver);
}

static void __exit fusion_unopt_exit(void)
{
	esp_driver_unregister(&fusion_unopt_driver);
}

module_init(fusion_unopt_init)
module_exit(fusion_unopt_exit)

MODULE_DEVICE_TABLE(of, fusion_unopt_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("fusion_unopt_vivado driver");
