// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "add_stratus.h"

#define DRV_NAME	"add_stratus"

/* <<--regs-->> */
#define ADD_DO_SHIFT_REG_0		0x70
#define ADD_DO_SHIFT_REG_1		0x74
#define ADD_DO_SHIFT_REG_2		0x78
#define ADD_DO_SHIFT_REG_3		0x7C
#define ADD_LOGN_SAMPLES_REG_0	0x80
#define ADD_LOGN_SAMPLES_REG_1	0x84
#define ADD_LOGN_SAMPLES_REG_2	0x88
#define ADD_LOGN_SAMPLES_REG_3	0x8C
#define ADD_DO_INVERSE_REG_0		0x90
#define ADD_DO_INVERSE_REG_1		0x94
#define ADD_DO_INVERSE_REG_2		0x98
#define ADD_DO_INVERSE_REG_3		0x9C
#define ADD_INPUT_QUEUE_BASE_0	0xA0
#define ADD_INPUT_QUEUE_BASE_1	0xA4
#define ADD_INPUT_QUEUE_BASE_2	0xA8
#define ADD_INPUT_QUEUE_BASE_3	0xAC
#define ADD_OUTPUT_QUEUE_BASE_0	0xB0
#define ADD_OUTPUT_QUEUE_BASE_1	0xB4
#define ADD_OUTPUT_QUEUE_BASE_2	0xB8
#define ADD_OUTPUT_QUEUE_BASE_3	0xBC
#define ADD_CONTEXT_NPRIO_0		0XC0
#define ADD_CONTEXT_NPRIO_1		0xC4
#define ADD_CONTEXT_NPRIO_2		0xC8
#define ADD_CONTEXT_NPRIO_3		0xCC
#define ADD_VALID_CONTEXTS		0xD0
#define ADD_SCHED_PERIOD			0xD4

struct add_stratus_device {
	struct esp_device esp;
};

static struct esp_driver add_driver;

static struct of_device_id add_device_ids[] = {
	{
		.name = "SLD_ADD_STRATUS",
	},
	{
		.name = "eb_063",
	},
	{
		.compatible = "sld,add_stratus",
	},
	{ },
};

static int add_devs;

static inline struct add_stratus_device *to_add(struct esp_device *esp)
{
	return container_of(esp, struct add_stratus_device, esp);
}

static void add_prep_xfer(struct esp_device *esp, void *arg)
{
	struct add_stratus_access *a = arg;

	/* <<--regs-config-->> */
	// iowrite32be(a->do_inverse, esp->iomem + ADD_DO_INVERSE_REG);
	// iowrite32be(a->logn_samples, esp->iomem + ADD_LOGN_SAMPLES_REG);
	// iowrite32be(a->do_shift, esp->iomem + ADD_DO_SHIFT_REG);

	// iowrite32be(a->input_queue_base, esp->iomem + ADD_INPUT_QUEUE_BASE);
	// iowrite32be(a->output_queue_base, esp->iomem + ADD_OUTPUT_QUEUE_BASE);
}

static void add_init_accel(struct esp_device *esp, void *arg)
{
	struct add_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->do_inverse, esp->iomem + ADD_DO_INVERSE_REG_0 + 0x4*esp->context_id);
	iowrite32be(a->logn_samples, esp->iomem + ADD_LOGN_SAMPLES_REG_0 + 0x4*esp->context_id);
	iowrite32be(a->do_shift, esp->iomem + ADD_DO_SHIFT_REG_0 + 0x4*esp->context_id);

	iowrite32be(a->input_queue_base, esp->iomem + ADD_INPUT_QUEUE_BASE_0 + 0x4*esp->context_id);
	iowrite32be(a->output_queue_base, esp->iomem + ADD_OUTPUT_QUEUE_BASE_0 + 0x4*esp->context_id);
	
	iowrite32be(a->esp.context_nprio, esp->iomem + ADD_CONTEXT_NPRIO_0 + 0x4*esp->context_id);
	iowrite32be(a->esp.valid_contexts, esp->iomem + ADD_VALID_CONTEXTS);
	iowrite32be(a->esp.sched_period, esp->iomem + ADD_SCHED_PERIOD);
}

static void add_add_context(struct esp_device *esp, void *arg)
{
	struct add_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->do_inverse, esp->iomem + ADD_DO_INVERSE_REG_0 + 0x4*esp->context_id);
	iowrite32be(a->logn_samples, esp->iomem + ADD_LOGN_SAMPLES_REG_0 + 0x4*esp->context_id);
	iowrite32be(a->do_shift, esp->iomem + ADD_DO_SHIFT_REG_0 + 0x4*esp->context_id);

	iowrite32be(a->input_queue_base, esp->iomem + ADD_INPUT_QUEUE_BASE_0 + 0x4*esp->context_id);
	iowrite32be(a->output_queue_base, esp->iomem + ADD_OUTPUT_QUEUE_BASE_0 + 0x4*esp->context_id);

	iowrite32be(a->esp.context_nprio, esp->iomem + ADD_CONTEXT_NPRIO_0 + 0x4*esp->context_id);
	iowrite32be(a->esp.valid_contexts, esp->iomem + ADD_VALID_CONTEXTS);
	iowrite32be(a->esp.sched_period, esp->iomem + ADD_SCHED_PERIOD);
}

static void add_del_context(struct esp_device *esp, void *arg)
{
	struct add_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->esp.valid_contexts, esp->iomem + ADD_VALID_CONTEXTS);
}

static void add_setprio(struct esp_device *esp, void *arg)
{
	struct add_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->esp.context_nprio, esp->iomem + ADD_CONTEXT_NPRIO_0 + 0x4*esp->context_id);
}

static bool add_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct add_stratus_device *add = to_add(esp); */
	/* struct add_stratus_access *a = arg; */

	return true;
}

static int add_probe(struct platform_device *pdev)
{
	struct add_stratus_device *add;
	struct esp_device *esp;
	int rc;

	add = kzalloc(sizeof(*add), GFP_KERNEL);
	if (add == NULL)
		return -ENOMEM;
	esp = &add->esp;
	esp->module = THIS_MODULE;
	esp->number = add_devs;
	esp->driver = &add_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	add_devs++;
	return 0;
 err:
	kfree(add);
	return rc;
}

static int __exit add_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct add_stratus_device *add = to_add(esp);

	esp_device_unregister(esp);
	kfree(add);
	return 0;
}

static struct esp_driver add_driver = {
	.plat = {
		.probe		= add_probe,
		.remove		= add_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = add_device_ids,
		},
	},
	.xfer_input_ok	= add_xfer_input_ok,
	.prep_xfer		= add_prep_xfer,
	.init_accel		= add_init_accel,
	.add_context	= add_add_context,
	.del_context	= add_del_context,
	.setprio		= add_setprio,
	.ioctl_cm		= ADD_STRATUS_IOC_ACCESS,
	.init_cm		= ADD_STRATUS_INIT_IOC_ACCESS,
	.add_cm			= ADD_STRATUS_ADD_IOC_ACCESS,
	.del_cm			= ADD_STRATUS_DEL_IOC_ACCESS,
	.prio_cm		= ADD_STRATUS_PRIO_IOC_ACCESS,
	.arg_size		= sizeof(struct add_stratus_access),
};

static int __init add_init(void)
{
	return esp_driver_register(&add_driver);
}

static void __exit add_exit(void)
{
	esp_driver_unregister(&add_driver);
}

module_init(add_init)
module_exit(add_exit)

MODULE_DEVICE_TABLE(of, add_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("add_stratus driver");
