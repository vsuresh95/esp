// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "audio_fir_stratus.h"

#define DRV_NAME	"audio_fir_stratus"

/* <<--regs-->> */
#define AUDIO_FIR_DO_INVERSE_REG 0x48
#define AUDIO_FIR_LOGN_SAMPLES_REG 0x44
#define AUDIO_FIR_DO_SHIFT_REG 0x40

#define AUDIO_FIR_PROD_VALID_OFFSET 0x4C
#define AUDIO_FIR_PROD_READY_OFFSET 0x50
#define AUDIO_FIR_FLT_PROD_VALID_OFFSET 0x54
#define AUDIO_FIR_FLT_PROD_READY_OFFSET 0x58
#define AUDIO_FIR_CONS_VALID_OFFSET 0x5C
#define AUDIO_FIR_CONS_READY_OFFSET 0x60
#define AUDIO_FIR_LOAD_DATA_OFFSET 0x64
#define AUDIO_FIR_FLT_LOAD_DATA_OFFSET 0x68
#define AUDIO_FIR_TWD_LOAD_DATA_OFFSET 0x6C
#define AUDIO_FIR_STORE_DATA_OFFSET 0x70

struct audio_fir_stratus_device {
	struct esp_device esp;
};

static struct esp_driver audio_fir_driver;

static struct of_device_id audio_fir_device_ids[] = {
	{
		.name = "SLD_AUDIO_FIR_STRATUS",
	},
	{
		.name = "eb_056",
	},
	{
		.compatible = "sld,audio_fir_stratus",
	},
	{ },
};

static int audio_fir_devs;

static inline struct audio_fir_stratus_device *to_audio_fir(struct esp_device *esp)
{
	return container_of(esp, struct audio_fir_stratus_device, esp);
}

static void audio_fir_prep_xfer(struct esp_device *esp, void *arg)
{
	struct audio_fir_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->do_inverse, esp->iomem + AUDIO_FIR_DO_INVERSE_REG);
	iowrite32be(a->logn_samples, esp->iomem + AUDIO_FIR_LOGN_SAMPLES_REG);
	iowrite32be(a->do_shift, esp->iomem + AUDIO_FIR_DO_SHIFT_REG);

	iowrite32be(a->prod_valid_offset, esp->iomem + AUDIO_FIR_PROD_VALID_OFFSET);
	iowrite32be(a->prod_ready_offset, esp->iomem + AUDIO_FIR_PROD_READY_OFFSET);
	iowrite32be(a->flt_prod_valid_offset, esp->iomem + AUDIO_FIR_FLT_PROD_VALID_OFFSET);
	iowrite32be(a->flt_prod_ready_offset, esp->iomem + AUDIO_FIR_FLT_PROD_READY_OFFSET);
	iowrite32be(a->cons_valid_offset, esp->iomem + AUDIO_FIR_CONS_VALID_OFFSET);
	iowrite32be(a->cons_ready_offset, esp->iomem + AUDIO_FIR_CONS_READY_OFFSET);
	iowrite32be(a->load_data_offset, esp->iomem + AUDIO_FIR_LOAD_DATA_OFFSET);
	iowrite32be(a->flt_load_data_offset, esp->iomem + AUDIO_FIR_FLT_LOAD_DATA_OFFSET);
	iowrite32be(a->twd_load_data_offset, esp->iomem + AUDIO_FIR_TWD_LOAD_DATA_OFFSET);
	iowrite32be(a->store_data_offset, esp->iomem + AUDIO_FIR_STORE_DATA_OFFSET);

	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);
	iowrite32be(a->spandex_conf, esp->iomem + SPANDEX_REG);

}

static bool audio_fir_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct audio_fir_stratus_device *audio_fir = to_audio_fir(esp); */
	/* struct audio_fir_stratus_access *a = arg; */

	return true;
}

static int audio_fir_probe(struct platform_device *pdev)
{
	struct audio_fir_stratus_device *audio_fir;
	struct esp_device *esp;
	int rc;

	audio_fir = kzalloc(sizeof(*audio_fir), GFP_KERNEL);
	if (audio_fir == NULL)
		return -ENOMEM;
	esp = &audio_fir->esp;
	esp->module = THIS_MODULE;
	esp->number = audio_fir_devs;
	esp->driver = &audio_fir_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	audio_fir_devs++;
	return 0;
 err:
	kfree(audio_fir);
	return rc;
}

static int __exit audio_fir_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct audio_fir_stratus_device *audio_fir = to_audio_fir(esp);

	esp_device_unregister(esp);
	kfree(audio_fir);
	return 0;
}

static struct esp_driver audio_fir_driver = {
	.plat = {
		.probe		= audio_fir_probe,
		.remove		= audio_fir_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = audio_fir_device_ids,
		},
	},
	.xfer_input_ok	= audio_fir_xfer_input_ok,
	.prep_xfer	= audio_fir_prep_xfer,
	.ioctl_cm	= AUDIO_FIR_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct audio_fir_stratus_access),
};

static int __init audio_fir_init(void)
{
	return esp_driver_register(&audio_fir_driver);
}

static void __exit audio_fir_exit(void)
{
	esp_driver_unregister(&audio_fir_driver);
}

module_init(audio_fir_init)
module_exit(audio_fir_exit)

MODULE_DEVICE_TABLE(of, audio_fir_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("audio_fir_stratus driver");
