// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "audio_ffi_stratus.h"

#define DRV_NAME	"audio_ffi_stratus"

/* <<--regs-->> */
#define AUDIO_FFI_DO_INVERSE_REG 0x48
#define AUDIO_FFI_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFI_DO_SHIFT_REG 0x40

#define AUDIO_FFI_PROD_VALID_OFFSET 0x4C
#define AUDIO_FFI_PROD_READY_OFFSET 0x50
#define AUDIO_FFI_FLT_PROD_VALID_OFFSET 0x54
#define AUDIO_FFI_FLT_PROD_READY_OFFSET 0x58
#define AUDIO_FFI_CONS_VALID_OFFSET 0x5C
#define AUDIO_FFI_CONS_READY_OFFSET 0x60
#define AUDIO_FFI_LOAD_DATA_OFFSET 0x64
#define AUDIO_FFI_FLT_LOAD_DATA_OFFSET 0x68
#define AUDIO_FFI_TWD_LOAD_DATA_OFFSET 0x6C
#define AUDIO_FFI_STORE_DATA_OFFSET 0x70

struct audio_ffi_stratus_device {
	struct esp_device esp;
};

static struct esp_driver audio_ffi_driver;

static struct of_device_id audio_ffi_device_ids[] = {
	{
		.name = "SLD_AUDIO_FFI_STRATUS",
	},
	{
		.name = "eb_057",
	},
	{
		.compatible = "sld,audio_ffi_stratus",
	},
	{ },
};

static int audio_ffi_devs;

static inline struct audio_ffi_stratus_device *to_audio_ffi(struct esp_device *esp)
{
	return container_of(esp, struct audio_ffi_stratus_device, esp);
}

static void audio_ffi_prep_xfer(struct esp_device *esp, void *arg)
{
	struct audio_ffi_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->do_inverse, esp->iomem + AUDIO_FFI_DO_INVERSE_REG);
	iowrite32be(a->logn_samples, esp->iomem + AUDIO_FFI_LOGN_SAMPLES_REG);
	iowrite32be(a->do_shift, esp->iomem + AUDIO_FFI_DO_SHIFT_REG);

	iowrite32be(a->prod_valid_offset, esp->iomem + AUDIO_FFI_PROD_VALID_OFFSET);
	iowrite32be(a->prod_ready_offset, esp->iomem + AUDIO_FFI_PROD_READY_OFFSET);
	iowrite32be(a->flt_prod_valid_offset, esp->iomem + AUDIO_FFI_FLT_PROD_VALID_OFFSET);
	iowrite32be(a->flt_prod_ready_offset, esp->iomem + AUDIO_FFI_FLT_PROD_READY_OFFSET);
	iowrite32be(a->cons_valid_offset, esp->iomem + AUDIO_FFI_CONS_VALID_OFFSET);
	iowrite32be(a->cons_ready_offset, esp->iomem + AUDIO_FFI_CONS_READY_OFFSET);
	iowrite32be(a->load_data_offset, esp->iomem + AUDIO_FFI_LOAD_DATA_OFFSET);
	iowrite32be(a->flt_load_data_offset, esp->iomem + AUDIO_FFI_FLT_LOAD_DATA_OFFSET);
	iowrite32be(a->twd_load_data_offset, esp->iomem + AUDIO_FFI_TWD_LOAD_DATA_OFFSET);
	iowrite32be(a->store_data_offset, esp->iomem + AUDIO_FFI_STORE_DATA_OFFSET);

	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool audio_ffi_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct audio_ffi_stratus_device *audio_ffi = to_audio_ffi(esp); */
	/* struct audio_ffi_stratus_access *a = arg; */

	return true;
}

static int audio_ffi_probe(struct platform_device *pdev)
{
	struct audio_ffi_stratus_device *audio_ffi;
	struct esp_device *esp;
	int rc;

	audio_ffi = kzalloc(sizeof(*audio_ffi), GFP_KERNEL);
	if (audio_ffi == NULL)
		return -ENOMEM;
	esp = &audio_ffi->esp;
	esp->module = THIS_MODULE;
	esp->number = audio_ffi_devs;
	esp->driver = &audio_ffi_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	audio_ffi_devs++;
	return 0;
 err:
	kfree(audio_ffi);
	return rc;
}

static int __exit audio_ffi_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct audio_ffi_stratus_device *audio_ffi = to_audio_ffi(esp);

	esp_device_unregister(esp);
	kfree(audio_ffi);
	return 0;
}

static struct esp_driver audio_ffi_driver = {
	.plat = {
		.probe		= audio_ffi_probe,
		.remove		= audio_ffi_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = audio_ffi_device_ids,
		},
	},
	.xfer_input_ok	= audio_ffi_xfer_input_ok,
	.prep_xfer	= audio_ffi_prep_xfer,
	.ioctl_cm	= AUDIO_FFI_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct audio_ffi_stratus_access),
};

static int __init audio_ffi_init(void)
{
	return esp_driver_register(&audio_ffi_driver);
}

static void __exit audio_ffi_exit(void)
{
	esp_driver_unregister(&audio_ffi_driver);
}

module_init(audio_ffi_init)
module_exit(audio_ffi_exit)

MODULE_DEVICE_TABLE(of, audio_ffi_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("audio_ffi_stratus driver");
