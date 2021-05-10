// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "gemm_1_sparse_stratus.h"

#define DRV_NAME	"gemm_1_sparse_stratus"

/* <<--regs-->> */
#define GEMM_1_SPARSE_H_ORDER_SIZE_REG 0x54
#define GEMM_1_SPARSE_GEMM_M_REG 0x50
#define GEMM_1_SPARSE_GEMM_N_REG 0x4c
#define GEMM_1_SPARSE_GEMM_K_REG 0x48
#define GEMM_1_SPARSE_GEMM_BATCH_REG 0x44
#define GEMM_1_SPARSE_VAR_SIZE_REG 0x40

struct gemm_1_sparse_stratus_device {
	struct esp_device esp;
};

static struct esp_driver gemm_1_sparse_driver;

static struct of_device_id gemm_1_sparse_device_ids[] = {
	{
		.name = "SLD_GEMM_1_SPARSE_STRATUS",
	},
	{
		.name = "eb_071",
	},
	{
		.compatible = "sld,gemm_1_sparse_stratus",
	},
	{ },
};

static int gemm_1_sparse_devs;

static inline struct gemm_1_sparse_stratus_device *to_gemm_1_sparse(struct esp_device *esp)
{
	return container_of(esp, struct gemm_1_sparse_stratus_device, esp);
}

static void gemm_1_sparse_prep_xfer(struct esp_device *esp, void *arg)
{
	struct gemm_1_sparse_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->h_order_size, esp->iomem + GEMM_1_SPARSE_H_ORDER_SIZE_REG);
	iowrite32be(a->gemm_m, esp->iomem + GEMM_1_SPARSE_GEMM_M_REG);
	iowrite32be(a->gemm_n, esp->iomem + GEMM_1_SPARSE_GEMM_N_REG);
	iowrite32be(a->gemm_k, esp->iomem + GEMM_1_SPARSE_GEMM_K_REG);
	iowrite32be(a->gemm_batch, esp->iomem + GEMM_1_SPARSE_GEMM_BATCH_REG);
	iowrite32be(a->var_size, esp->iomem + GEMM_1_SPARSE_VAR_SIZE_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool gemm_1_sparse_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct gemm_1_sparse_stratus_device *gemm_1_sparse = to_gemm_1_sparse(esp); */
	/* struct gemm_1_sparse_stratus_access *a = arg; */

	return true;
}

static int gemm_1_sparse_probe(struct platform_device *pdev)
{
	struct gemm_1_sparse_stratus_device *gemm_1_sparse;
	struct esp_device *esp;
	int rc;

	gemm_1_sparse = kzalloc(sizeof(*gemm_1_sparse), GFP_KERNEL);
	if (gemm_1_sparse == NULL)
		return -ENOMEM;
	esp = &gemm_1_sparse->esp;
	esp->module = THIS_MODULE;
	esp->number = gemm_1_sparse_devs;
	esp->driver = &gemm_1_sparse_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	gemm_1_sparse_devs++;
	return 0;
 err:
	kfree(gemm_1_sparse);
	return rc;
}

static int __exit gemm_1_sparse_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct gemm_1_sparse_stratus_device *gemm_1_sparse = to_gemm_1_sparse(esp);

	esp_device_unregister(esp);
	kfree(gemm_1_sparse);
	return 0;
}

static struct esp_driver gemm_1_sparse_driver = {
	.plat = {
		.probe		= gemm_1_sparse_probe,
		.remove		= gemm_1_sparse_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = gemm_1_sparse_device_ids,
		},
	},
	.xfer_input_ok	= gemm_1_sparse_xfer_input_ok,
	.prep_xfer	= gemm_1_sparse_prep_xfer,
	.ioctl_cm	= GEMM_1_SPARSE_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct gemm_1_sparse_stratus_access),
};

static int __init gemm_1_sparse_init(void)
{
	return esp_driver_register(&gemm_1_sparse_driver);
}

static void __exit gemm_1_sparse_exit(void)
{
	esp_driver_unregister(&gemm_1_sparse_driver);
}

module_init(gemm_1_sparse_init)
module_exit(gemm_1_sparse_exit)

MODULE_DEVICE_TABLE(of, gemm_1_sparse_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gemm_1_sparse_stratus driver");
