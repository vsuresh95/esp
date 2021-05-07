// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "gemm_block_stratus.h"

#define DRV_NAME	"gemm_block_stratus"

/* <<--regs-->> */
#define GEMM_BLOCK_GEMM_M_REG 0x4c
#define GEMM_BLOCK_GEMM_N_REG 0x48
#define GEMM_BLOCK_GEMM_K_REG 0x44
#define GEMM_BLOCK_GEMM_BATCH_REG 0x40

struct gemm_block_stratus_device {
	struct esp_device esp;
};

static struct esp_driver gemm_block_driver;

static struct of_device_id gemm_block_device_ids[] = {
	{
		.name = "SLD_GEMM_BLOCK_STRATUS",
	},
	{
		.name = "eb_100",
	},
	{
		.compatible = "sld,gemm_block_stratus",
	},
	{ },
};

static int gemm_block_devs;

static inline struct gemm_block_stratus_device *to_gemm_block(struct esp_device *esp)
{
	return container_of(esp, struct gemm_block_stratus_device, esp);
}

static void gemm_block_prep_xfer(struct esp_device *esp, void *arg)
{
	struct gemm_block_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->gemm_m, esp->iomem + GEMM_BLOCK_GEMM_M_REG);
	iowrite32be(a->gemm_n, esp->iomem + GEMM_BLOCK_GEMM_N_REG);
	iowrite32be(a->gemm_k, esp->iomem + GEMM_BLOCK_GEMM_K_REG);
	iowrite32be(a->gemm_batch, esp->iomem + GEMM_BLOCK_GEMM_BATCH_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool gemm_block_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct gemm_block_stratus_device *gemm_block = to_gemm_block(esp); */
	/* struct gemm_block_stratus_access *a = arg; */

	return true;
}

static int gemm_block_probe(struct platform_device *pdev)
{
	struct gemm_block_stratus_device *gemm_block;
	struct esp_device *esp;
	int rc;

	gemm_block = kzalloc(sizeof(*gemm_block), GFP_KERNEL);
	if (gemm_block == NULL)
		return -ENOMEM;
	esp = &gemm_block->esp;
	esp->module = THIS_MODULE;
	esp->number = gemm_block_devs;
	esp->driver = &gemm_block_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	gemm_block_devs++;
	return 0;
 err:
	kfree(gemm_block);
	return rc;
}

static int __exit gemm_block_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct gemm_block_stratus_device *gemm_block = to_gemm_block(esp);

	esp_device_unregister(esp);
	kfree(gemm_block);
	return 0;
}

static struct esp_driver gemm_block_driver = {
	.plat = {
		.probe		= gemm_block_probe,
		.remove		= gemm_block_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = gemm_block_device_ids,
		},
	},
	.xfer_input_ok	= gemm_block_xfer_input_ok,
	.prep_xfer	= gemm_block_prep_xfer,
	.ioctl_cm	= GEMM_BLOCK_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct gemm_block_stratus_access),
};

static int __init gemm_block_init(void)
{
	return esp_driver_register(&gemm_block_driver);
}

static void __exit gemm_block_exit(void)
{
	esp_driver_unregister(&gemm_block_driver);
}

module_init(gemm_block_init)
module_exit(gemm_block_exit)

MODULE_DEVICE_TABLE(of, gemm_block_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gemm_block_stratus driver");
