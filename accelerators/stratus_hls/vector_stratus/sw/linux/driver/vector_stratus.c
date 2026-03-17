// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "vector_stratus.h"

#define DRV_NAME	"vector_stratus"

/* <<--regs-->> */
#define VECTOR_TOTAL_LEN_REG 			0x98
#define VECTOR_INPUT1_OFFSET_REG 		0x9C
#define VECTOR_INPUT2_OFFSET_REG 		0xA0
#define VECTOR_OUTPUT_OFFSET_REG 		0xA4
#define VECTOR_DO_RELU_REG 			0xA8

struct vector_stratus_device {
	struct esp_device esp;
};

static struct esp_driver vector_driver;

static struct of_device_id vector_device_ids[] = {
	{
		.name = "SLD_VECTOR_STRATUS",
	},
	{
		.name = "eb_070",
	},
	{
		.compatible = "sld,vector_stratus",
	},
	{ },
};

static int vector_devs;

static inline struct vector_stratus_device *to_vector(struct esp_device *esp)
{
	return container_of(esp, struct vector_stratus_device, esp);
}

static void vector_prep_xfer(struct esp_device *esp, void *arg)
{
	struct vector_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->params.total_len, esp->iomem + VECTOR_TOTAL_LEN_REG);
	iowrite32be(a->params.output_offset, esp->iomem + VECTOR_OUTPUT_OFFSET_REG);
	iowrite32be(a->params.input1_offset, esp->iomem + VECTOR_INPUT1_OFFSET_REG);
	iowrite32be(a->params.input2_offset, esp->iomem + VECTOR_INPUT2_OFFSET_REG);
	iowrite32be(a->params.do_relu, esp->iomem + VECTOR_DO_RELU_REG);
}

static bool vector_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct vector_stratus_device *vector = to_vector(esp); */
	/* struct vector_stratus_access *a = arg; */

	return true;
}

static int vector_probe(struct platform_device *pdev)
{
	struct vector_stratus_device *vector;
	struct esp_device *esp;
	int rc;

	vector = kzalloc(sizeof(*vector), GFP_KERNEL);
	if (vector == NULL)
		return -ENOMEM;
	esp = &vector->esp;
	esp->module = THIS_MODULE;
	esp->number = vector_devs;
	esp->driver = &vector_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	vector_devs++;
	return 0;
 err:
	kfree(vector);
	return rc;
}

static int __exit vector_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct vector_stratus_device *vector = to_vector(esp);

	esp_device_unregister(esp);
	kfree(vector);
	return 0;
}

static struct esp_driver vector_driver = {
	.plat = {
		.probe		= vector_probe,
		.remove		= vector_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = vector_device_ids,
		},
	},
	.xfer_input_ok	= vector_xfer_input_ok,
	.prep_xfer	= vector_prep_xfer,
	.ioctl_cm	= VECTOR_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct vector_stratus_access),
};

static int __init vector_init(void)
{
	return esp_driver_register(&vector_driver);
}

static void __exit vector_exit(void)
{
	esp_driver_unregister(&vector_driver);
}

module_init(vector_init)
module_exit(vector_exit)

MODULE_DEVICE_TABLE(of, vector_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vector_stratus driver");
