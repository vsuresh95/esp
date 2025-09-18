// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "gemm_stratus.h"

#define DRV_NAME	"gemm_stratus"

/* <<--regs-->> */
#define GEMM_CONTEXT_BASE_PTR_0	0x70
#define GEMM_CONTEXT_BASE_PTR_1	0x74
#define GEMM_CONTEXT_BASE_PTR_2	0x78
#define GEMM_CONTEXT_BASE_PTR_3	0x7C
#define GEMM_CONTEXT_NPRIO_0	0x80
#define GEMM_CONTEXT_NPRIO_1	0x84
#define GEMM_CONTEXT_NPRIO_2	0x88
#define GEMM_CONTEXT_NPRIO_3	0x8C
#define GEMM_VALID_CONTEXTS		0x90
#define GEMM_SCHED_PERIOD		0x94

struct gemm_stratus_device {
	struct esp_device esp;
};

static struct esp_driver gemm_driver;

static struct of_device_id gemm_device_ids[] = {
	{
		.name = "SLD_GEMM_STRATUS",
	},
	{
		.name = "eb_063",
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
}

static void gemm_reset_accel(struct esp_device *esp)
{
	/* <<--regs-config-->> */
	iowrite32be(0x0, esp->iomem + GEMM_VALID_CONTEXTS);
}

static void gemm_init_accel(struct esp_device *esp, void *arg)
{
	struct gemm_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->esp.context_base_ptr, esp->iomem + GEMM_CONTEXT_BASE_PTR_0 + 0x4*esp->context_id);
	iowrite32be(a->esp.context_nprio, esp->iomem + GEMM_CONTEXT_NPRIO_0 + 0x4*esp->context_id);
	iowrite32be(a->esp.valid_contexts, esp->iomem + GEMM_VALID_CONTEXTS);
	iowrite32be(a->esp.sched_period, esp->iomem + GEMM_SCHED_PERIOD);
}

static void gemm_add_context(struct esp_device *esp, void *arg)
{
	struct gemm_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->esp.context_base_ptr, esp->iomem + GEMM_CONTEXT_BASE_PTR_0 + 0x4*esp->context_id);
	iowrite32be(a->esp.context_nprio, esp->iomem + GEMM_CONTEXT_NPRIO_0 + 0x4*esp->context_id);
	iowrite32be(a->esp.valid_contexts, esp->iomem + GEMM_VALID_CONTEXTS);
	iowrite32be(a->esp.sched_period, esp->iomem + GEMM_SCHED_PERIOD);
}

static void gemm_del_context(struct esp_device *esp, void *arg)
{
	struct gemm_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->esp.valid_contexts, esp->iomem + GEMM_VALID_CONTEXTS);
}

static void gemm_setprio(struct esp_device *esp, void *arg)
{
	struct gemm_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->esp.context_nprio, esp->iomem + GEMM_CONTEXT_NPRIO_0 + 0x4*esp->context_id);
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
	.res_accel		= gemm_reset_accel,
	.init_accel		= gemm_init_accel,
	.add_context	= gemm_add_context,
	.del_context	= gemm_del_context,
	.setprio		= gemm_setprio,
	.ioctl_cm		= GEMM_STRATUS_IOC_ACCESS,
	.reset_cm		= GEMM_STRATUS_RESET_IOC_ACCESS,
	.init_cm		= GEMM_STRATUS_INIT_IOC_ACCESS,
	.add_cm			= GEMM_STRATUS_ADD_IOC_ACCESS,
	.del_cm			= GEMM_STRATUS_DEL_IOC_ACCESS,
	.prio_cm		= GEMM_STRATUS_PRIO_IOC_ACCESS,
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
