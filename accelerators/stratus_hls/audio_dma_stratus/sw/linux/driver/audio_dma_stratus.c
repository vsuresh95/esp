// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "audio_dma_stratus.h"

#define DRV_NAME	"audio_dma_stratus"

/* <<--regs-->> */
#define SM_SENSOR_SIZE_REG 0x40

struct audio_dma_stratus_device {
	struct esp_device esp;
};

static struct esp_driver audio_dma_driver;

static struct of_device_id audio_dma_device_ids[] = {
	{
		.name = "SLD_SM_SENSOR_STRATUS",
	},
	{
		.name = "eb_050",
	},
	{
		.compatible = "sld,audio_dma_stratus",
	},
	{ },
};

static int audio_dma_devs;

static inline struct audio_dma_stratus_device *to_audio_dma(struct esp_device *esp)
{
	return container_of(esp, struct audio_dma_stratus_device, esp);
}

static void audio_dma_prep_xfer(struct esp_device *esp, void *arg)
{
	struct audio_dma_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->size, esp->iomem + SM_SENSOR_SIZE_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);
	iowrite32be(a->spandex_conf, esp->iomem + SPANDEX_REG);

}

static bool audio_dma_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct audio_dma_stratus_device *audio_dma = to_audio_dma(esp); */
	/* struct audio_dma_stratus_access *a = arg; */

	return true;
}

static int audio_dma_probe(struct platform_device *pdev)
{
	struct audio_dma_stratus_device *audio_dma;
	struct esp_device *esp;
	int rc;

	audio_dma = kzalloc(sizeof(*audio_dma), GFP_KERNEL);
	if (audio_dma == NULL)
		return -ENOMEM;
	esp = &audio_dma->esp;
	esp->module = THIS_MODULE;
	esp->number = audio_dma_devs;
	esp->driver = &audio_dma_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	audio_dma_devs++;
	return 0;
 err:
	kfree(audio_dma);
	return rc;
}

static int __exit audio_dma_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct audio_dma_stratus_device *audio_dma = to_audio_dma(esp);

	esp_device_unregister(esp);
	kfree(audio_dma);
	return 0;
}

static struct esp_driver audio_dma_driver = {
	.plat = {
		.probe		= audio_dma_probe,
		.remove		= audio_dma_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = audio_dma_device_ids,
		},
	},
	.xfer_input_ok	= audio_dma_xfer_input_ok,
	.prep_xfer	= audio_dma_prep_xfer,
	.ioctl_cm	= SM_SENSOR_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct audio_dma_stratus_access),
};

static int __init audio_dma_init(void)
{
	return esp_driver_register(&audio_dma_driver);
}

static void __exit audio_dma_exit(void)
{
	esp_driver_unregister(&audio_dma_driver);
}

module_init(audio_dma_init)
module_exit(audio_dma_exit)

MODULE_DEVICE_TABLE(of, audio_dma_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("audio_dma_stratus driver");
