// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "rotate_order_stratus.h"

#define DRV_NAME	"rotate_order_stratus"

/* <<--regs-->> */
#define ROTATE_ORDER_NUM_CHANNEL_REG 0x44
#define ROTATE_ORDER_BLOCK_SIZE_REG 0x40

struct rotate_order_stratus_device {
	struct esp_device esp;
};

static struct esp_driver rotate_order_driver;

static struct of_device_id rotate_order_device_ids[] = {
	{
		.name = "SLD_ROTATE_ORDER_STRATUS",
	},
	{
		.name = "eb_059",
	},
	{
		.compatible = "sld,rotate_order_stratus",
	},
	{ },
};

static int rotate_order_devs;

static inline struct rotate_order_stratus_device *to_rotate_order(struct esp_device *esp)
{
	return container_of(esp, struct rotate_order_stratus_device, esp);
}

static void rotate_order_prep_xfer(struct esp_device *esp, void *arg)
{
	struct rotate_order_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->num_channel, esp->iomem + ROTATE_ORDER_NUM_CHANNEL_REG);
	iowrite32be(a->block_size, esp->iomem + ROTATE_ORDER_BLOCK_SIZE_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool rotate_order_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct rotate_order_stratus_device *rotate_order = to_rotate_order(esp); */
	/* struct rotate_order_stratus_access *a = arg; */

	return true;
}

static int rotate_order_probe(struct platform_device *pdev)
{
	struct rotate_order_stratus_device *rotate_order;
	struct esp_device *esp;
	int rc;

	rotate_order = kzalloc(sizeof(*rotate_order), GFP_KERNEL);
	if (rotate_order == NULL)
		return -ENOMEM;
	esp = &rotate_order->esp;
	esp->module = THIS_MODULE;
	esp->number = rotate_order_devs;
	esp->driver = &rotate_order_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	rotate_order_devs++;
	return 0;
 err:
	kfree(rotate_order);
	return rc;
}

static int __exit rotate_order_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct rotate_order_stratus_device *rotate_order = to_rotate_order(esp);

	esp_device_unregister(esp);
	kfree(rotate_order);
	return 0;
}

static struct esp_driver rotate_order_driver = {
	.plat = {
		.probe		= rotate_order_probe,
		.remove		= rotate_order_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = rotate_order_device_ids,
		},
	},
	.xfer_input_ok	= rotate_order_xfer_input_ok,
	.prep_xfer	= rotate_order_prep_xfer,
	.ioctl_cm	= ROTATE_ORDER_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct rotate_order_stratus_access),
};

static int __init rotate_order_init(void)
{
	return esp_driver_register(&rotate_order_driver);
}

static void __exit rotate_order_exit(void)
{
	esp_driver_unregister(&rotate_order_driver);
}

module_init(rotate_order_init)
module_exit(rotate_order_exit)

MODULE_DEVICE_TABLE(of, rotate_order_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rotate_order_stratus driver");
