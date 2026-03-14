// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "add_stratus.h"

#define DRV_NAME	"add_stratus"

/* <<--regs-->> */
#define ADD_TOTAL_LEN_REG 			0x98
#define ADD_INPUT1_OFFSET_REG 		0x9C
#define ADD_INPUT2_OFFSET_REG 		0xA0
#define ADD_OUTPUT_OFFSET_REG 		0xA4
#define ADD_DO_RELU_REG 			0xA8

struct add_stratus_device {
	struct esp_device esp;
};

static struct esp_driver add_driver;

static struct of_device_id add_device_ids[] = {
	{
		.name = "SLD_ADD_STRATUS",
	},
	{
		.name = "eb_070",
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
	iowrite32be(a->params.total_len, esp->iomem + ADD_TOTAL_LEN_REG);
	iowrite32be(a->params.output_offset, esp->iomem + ADD_OUTPUT_OFFSET_REG);
	iowrite32be(a->params.input1_offset, esp->iomem + ADD_INPUT1_OFFSET_REG);
	iowrite32be(a->params.input2_offset, esp->iomem + ADD_INPUT2_OFFSET_REG);
	iowrite32be(a->params.do_relu, esp->iomem + ADD_DO_RELU_REG);
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
	.prep_xfer	= add_prep_xfer,
	.ioctl_cm	= ADD_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct add_stratus_access),
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
