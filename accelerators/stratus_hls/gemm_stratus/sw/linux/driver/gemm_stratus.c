// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "gemm_stratus.h"

#define DRV_NAME	"gemm_stratus"

/* <<--regs-->> */
#define GEMM_GEMM_M_REG 0x4c
#define GEMM_GEMM_N_REG 0x48
#define GEMM_GEMM_K_REG 0x44
#define GEMM_GEMM_BATCH_REG 0x40

struct gemm_stratus_device {
	struct esp_device esp;
};

static struct esp_driver gemm_driver;

static struct of_device_id gemm_device_ids[] = {
	{
		.name = "SLD_GEMM_STRATUS",
	},
	{
		.name = "eb_05a",
	},
	{
		.compatible = "sld,gemm_stratus",
	},
	{ },
};

static int gemm_devs;

static inline struct gemm_stratus_device *to_gemm(struct esp_device *esp)
{
	return container_of(esp, struct gemm_stratus_device, esp);
}

static void gemm_prep_xfer(struct esp_device *esp, void *arg)
{
	struct gemm_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->gemm_m, esp->iomem + GEMM_GEMM_M_REG);
	iowrite32be(a->gemm_n, esp->iomem + GEMM_GEMM_N_REG);
	iowrite32be(a->gemm_k, esp->iomem + GEMM_GEMM_K_REG);
	iowrite32be(a->gemm_batch, esp->iomem + GEMM_GEMM_BATCH_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool gemm_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct gemm_stratus_device *gemm = to_gemm(esp); */
	/* struct gemm_stratus_access *a = arg; */

	return true;
}

static int gemm_probe(struct platform_device *pdev)
{
	struct gemm_stratus_device *gemm;
	struct esp_device *esp;
	int rc;

	gemm = kzalloc(sizeof(*gemm), GFP_KERNEL);
	if (gemm == NULL)
		return -ENOMEM;
	esp = &gemm->esp;
	esp->module = THIS_MODULE;
	esp->number = gemm_devs;
	esp->driver = &gemm_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	gemm_devs++;
	return 0;
 err:
	kfree(gemm);
	return rc;
}

static int __exit gemm_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct gemm_stratus_device *gemm = to_gemm(esp);

	esp_device_unregister(esp);
	kfree(gemm);
	return 0;
}

static struct esp_driver gemm_driver = {
	.plat = {
		.probe		= gemm_probe,
		.remove		= gemm_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = gemm_device_ids,
		},
	},
	.xfer_input_ok	= gemm_xfer_input_ok,
	.prep_xfer	= gemm_prep_xfer,
	.ioctl_cm	= GEMM_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct gemm_stratus_access),
};

static int __init gemm_init(void)
{
	return esp_driver_register(&gemm_driver);
}

static void __exit gemm_exit(void)
{
	esp_driver_unregister(&gemm_driver);
}

module_init(gemm_init)
module_exit(gemm_exit)

MODULE_DEVICE_TABLE(of, gemm_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gemm_stratus driver");
