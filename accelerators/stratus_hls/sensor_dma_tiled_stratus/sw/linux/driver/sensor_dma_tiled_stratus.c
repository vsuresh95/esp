// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "sensor_dma_tiled_stratus.h"

#define DRV_NAME	"sensor_dma_tiled_stratus"

/* <<--regs-->> */
#define SENSOR_DMA_TILED_SIZE_REG 0x40

struct sensor_dma_tiled_stratus_device {
	struct esp_device esp;
};

static struct esp_driver sensor_dma_tiled_driver;

static struct of_device_id sensor_dma_tiled_device_ids[] = {
	{
		.name = "SLD_SENSOR_DMA_TILED_STRATUS",
	},
	{
		.name = "eb_060",
	},
	{
		.compatible = "sld,sensor_dma_tiled_stratus",
	},
	{ },
};

static int sensor_dma_tiled_devs;

static inline struct sensor_dma_tiled_stratus_device *to_sensor_dma_tiled(struct esp_device *esp)
{
	return container_of(esp, struct sensor_dma_tiled_stratus_device, esp);
}

static void sensor_dma_tiled_prep_xfer(struct esp_device *esp, void *arg)
{
	struct sensor_dma_tiled_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->size, esp->iomem + SENSOR_DMA_TILED_SIZE_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool sensor_dma_tiled_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct sensor_dma_tiled_stratus_device *sensor_dma_tiled = to_sensor_dma_tiled(esp); */
	/* struct sensor_dma_tiled_stratus_access *a = arg; */

	return true;
}

static int sensor_dma_tiled_probe(struct platform_device *pdev)
{
	struct sensor_dma_tiled_stratus_device *sensor_dma_tiled;
	struct esp_device *esp;
	int rc;

	sensor_dma_tiled = kzalloc(sizeof(*sensor_dma_tiled), GFP_KERNEL);
	if (sensor_dma_tiled == NULL)
		return -ENOMEM;
	esp = &sensor_dma_tiled->esp;
	esp->module = THIS_MODULE;
	esp->number = sensor_dma_tiled_devs;
	esp->driver = &sensor_dma_tiled_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	sensor_dma_tiled_devs++;
	return 0;
 err:
	kfree(sensor_dma_tiled);
	return rc;
}

static int __exit sensor_dma_tiled_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct sensor_dma_tiled_stratus_device *sensor_dma_tiled = to_sensor_dma_tiled(esp);

	esp_device_unregister(esp);
	kfree(sensor_dma_tiled);
	return 0;
}

static struct esp_driver sensor_dma_tiled_driver = {
	.plat = {
		.probe		= sensor_dma_tiled_probe,
		.remove		= sensor_dma_tiled_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = sensor_dma_tiled_device_ids,
		},
	},
	.xfer_input_ok	= sensor_dma_tiled_xfer_input_ok,
	.prep_xfer	= sensor_dma_tiled_prep_xfer,
	.ioctl_cm	= SENSOR_DMA_TILED_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct sensor_dma_tiled_stratus_access),
};

static int __init sensor_dma_tiled_init(void)
{
	return esp_driver_register(&sensor_dma_tiled_driver);
}

static void __exit sensor_dma_tiled_exit(void)
{
	esp_driver_unregister(&sensor_dma_tiled_driver);
}

module_init(sensor_dma_tiled_init)
module_exit(sensor_dma_tiled_exit)

MODULE_DEVICE_TABLE(of, sensor_dma_tiled_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sensor_dma_tiled_stratus driver");
