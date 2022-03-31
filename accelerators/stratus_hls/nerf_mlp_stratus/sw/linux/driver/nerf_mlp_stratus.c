// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "nerf_mlp_stratus.h"

#define DRV_NAME	"nerf_mlp_stratus"

/* <<--regs-->> */
#define NERF_MLP_BATCH_SIZE_REG 0x40

struct nerf_mlp_stratus_device {
	struct esp_device esp;
};

static struct esp_driver nerf_mlp_driver;

static struct of_device_id nerf_mlp_device_ids[] = {
	{
		.name = "SLD_NERF_MLP_STRATUS",
	},
	{
		.name = "eb_100",
	},
	{
		.compatible = "sld,nerf_mlp_stratus",
	},
	{ },
};

static int nerf_mlp_devs;

static inline struct nerf_mlp_stratus_device *to_nerf_mlp(struct esp_device *esp)
{
	return container_of(esp, struct nerf_mlp_stratus_device, esp);
}

static void nerf_mlp_prep_xfer(struct esp_device *esp, void *arg)
{
	struct nerf_mlp_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->batch_size, esp->iomem + NERF_MLP_BATCH_SIZE_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool nerf_mlp_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct nerf_mlp_stratus_device *nerf_mlp = to_nerf_mlp(esp); */
	/* struct nerf_mlp_stratus_access *a = arg; */

	return true;
}

static int nerf_mlp_probe(struct platform_device *pdev)
{
	struct nerf_mlp_stratus_device *nerf_mlp;
	struct esp_device *esp;
	int rc;

	nerf_mlp = kzalloc(sizeof(*nerf_mlp), GFP_KERNEL);
	if (nerf_mlp == NULL)
		return -ENOMEM;
	esp = &nerf_mlp->esp;
	esp->module = THIS_MODULE;
	esp->number = nerf_mlp_devs;
	esp->driver = &nerf_mlp_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	nerf_mlp_devs++;
	return 0;
 err:
	kfree(nerf_mlp);
	return rc;
}

static int __exit nerf_mlp_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct nerf_mlp_stratus_device *nerf_mlp = to_nerf_mlp(esp);

	esp_device_unregister(esp);
	kfree(nerf_mlp);
	return 0;
}

static struct esp_driver nerf_mlp_driver = {
	.plat = {
		.probe		= nerf_mlp_probe,
		.remove		= nerf_mlp_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = nerf_mlp_device_ids,
		},
	},
	.xfer_input_ok	= nerf_mlp_xfer_input_ok,
	.prep_xfer	= nerf_mlp_prep_xfer,
	.ioctl_cm	= NERF_MLP_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct nerf_mlp_stratus_access),
};

static int __init nerf_mlp_init(void)
{
	return esp_driver_register(&nerf_mlp_driver);
}

static void __exit nerf_mlp_exit(void)
{
	esp_driver_unregister(&nerf_mlp_driver);
}

module_init(nerf_mlp_init)
module_exit(nerf_mlp_exit)

MODULE_DEVICE_TABLE(of, nerf_mlp_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("nerf_mlp_stratus driver");
