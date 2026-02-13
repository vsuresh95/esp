// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "basic_256_gemm_stratus.h"

#define DRV_NAME	"basic_256_gemm_stratus"

/* <<--regs-->> */
#define BASIC_256_GEMM_C_MAT_OFFSET_REG 0x54
#define BASIC_256_GEMM_B_MAT_OFFSET_REG 0x50
#define BASIC_256_GEMM_A_MAT_OFFSET_REG 0x4c
#define BASIC_256_GEMM_K_REG 0x48
#define BASIC_256_GEMM_M_REG 0x44
#define BASIC_256_GEMM_N_REG 0x40

struct basic_256_gemm_stratus_device {
	struct esp_device esp;
};

static struct esp_driver basic_256_gemm_driver;

static struct of_device_id basic_256_gemm_device_ids[] = {
	{
		.name = "SLD_BASIC_256_GEMM_STRATUS",
	},
	{
		.name = "eb_04a",
	},
	{
		.compatible = "sld,basic_256_gemm_stratus",
	},
	{ },
};

static int basic_256_gemm_devs;

static inline struct basic_256_gemm_stratus_device *to_basic_256_gemm(struct esp_device *esp)
{
	return container_of(esp, struct basic_256_gemm_stratus_device, esp);
}

static void basic_256_gemm_prep_xfer(struct esp_device *esp, void *arg)
{
	struct basic_256_gemm_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->C_mat_offset, esp->iomem + BASIC_256_GEMM_C_MAT_OFFSET_REG);
	iowrite32be(a->B_mat_offset, esp->iomem + BASIC_256_GEMM_B_MAT_OFFSET_REG);
	iowrite32be(a->A_mat_offset, esp->iomem + BASIC_256_GEMM_A_MAT_OFFSET_REG);
	iowrite32be(a->K, esp->iomem + BASIC_256_GEMM_K_REG);
	iowrite32be(a->M, esp->iomem + BASIC_256_GEMM_M_REG);
	iowrite32be(a->N, esp->iomem + BASIC_256_GEMM_N_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool basic_256_gemm_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct basic_256_gemm_stratus_device *basic_256_gemm = to_basic_256_gemm(esp); */
	/* struct basic_256_gemm_stratus_access *a = arg; */

	return true;
}

static int basic_256_gemm_probe(struct platform_device *pdev)
{
	struct basic_256_gemm_stratus_device *basic_256_gemm;
	struct esp_device *esp;
	int rc;

	basic_256_gemm = kzalloc(sizeof(*basic_256_gemm), GFP_KERNEL);
	if (basic_256_gemm == NULL)
		return -ENOMEM;
	esp = &basic_256_gemm->esp;
	esp->module = THIS_MODULE;
	esp->number = basic_256_gemm_devs;
	esp->driver = &basic_256_gemm_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	basic_256_gemm_devs++;
	return 0;
 err:
	kfree(basic_256_gemm);
	return rc;
}

static int __exit basic_256_gemm_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct basic_256_gemm_stratus_device *basic_256_gemm = to_basic_256_gemm(esp);

	esp_device_unregister(esp);
	kfree(basic_256_gemm);
	return 0;
}

static struct esp_driver basic_256_gemm_driver = {
	.plat = {
		.probe		= basic_256_gemm_probe,
		.remove		= basic_256_gemm_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = basic_256_gemm_device_ids,
		},
	},
	.xfer_input_ok	= basic_256_gemm_xfer_input_ok,
	.prep_xfer	= basic_256_gemm_prep_xfer,
	.ioctl_cm	= BASIC_256_GEMM_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct basic_256_gemm_stratus_access),
};

static int __init basic_256_gemm_init(void)
{
	return esp_driver_register(&basic_256_gemm_driver);
}

static void __exit basic_256_gemm_exit(void)
{
	esp_driver_unregister(&basic_256_gemm_driver);
}

module_init(basic_256_gemm_init)
module_exit(basic_256_gemm_exit)

MODULE_DEVICE_TABLE(of, basic_256_gemm_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("basic_256_gemm_stratus driver");
