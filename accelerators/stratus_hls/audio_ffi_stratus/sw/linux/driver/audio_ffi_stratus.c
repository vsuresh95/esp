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
#define AUDIO_FFI_DO_SHIFT_REG_0 0x50
#define AUDIO_FFI_DO_SHIFT_REG_1 0x54
#define AUDIO_FFI_DO_SHIFT_REG_2 0x58
#define AUDIO_FFI_DO_SHIFT_REG_3 0x5C
#define AUDIO_FFI_LOGN_SAMPLES_REG_0 0x60
#define AUDIO_FFI_LOGN_SAMPLES_REG_1 0x64
#define AUDIO_FFI_LOGN_SAMPLES_REG_2 0x68
#define AUDIO_FFI_LOGN_SAMPLES_REG_3 0x6C
#define AUDIO_FFI_INPUT_QUEUE_BASE_0 0x70
#define AUDIO_FFI_INPUT_QUEUE_BASE_1 0x74
#define AUDIO_FFI_INPUT_QUEUE_BASE_2 0x78
#define AUDIO_FFI_INPUT_QUEUE_BASE_3 0x7C
#define AUDIO_FFI_OUTPUT_QUEUE_BASE_0 0x80
#define AUDIO_FFI_OUTPUT_QUEUE_BASE_1 0x84
#define AUDIO_FFI_OUTPUT_QUEUE_BASE_2 0x88
#define AUDIO_FFI_OUTPUT_QUEUE_BASE_3 0x8C
#define AUDIO_FFI_FILTER_QUEUE_BASE_0 0x90
#define AUDIO_FFI_FILTER_QUEUE_BASE_1 0x94
#define AUDIO_FFI_FILTER_QUEUE_BASE_2 0x98
#define AUDIO_FFI_FILTER_QUEUE_BASE_3 0x9C
#define AUDIO_FFI_CONTEXT_QUOTA 0xA0
#define AUDIO_FFI_VALID_CONTEXTS 0xA4

struct audio_ffi_stratus_device {
	struct esp_device esp;
};

static struct esp_driver audio_ffi_driver;

static struct of_device_id audio_ffi_device_ids[] = {
	{
		.name = "SLD_AUDIO_FFI_STRATUS",
	},
	{
		.name = "eb_065",
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
	// iowrite32be(a->logn_samples, esp->iomem + AUDIO_FFI_LOGN_SAMPLES_REG);
	// iowrite32be(a->do_shift, esp->iomem + AUDIO_FFI_DO_SHIFT_REG);

	// iowrite32be(a->input_queue_base, esp->iomem + AUDIO_FFI_INPUT_QUEUE_BASE);
	// iowrite32be(a->output_queue_base, esp->iomem + AUDIO_FFI_OUTPUT_QUEUE_BASE);
	// iowrite32be(a->filter_queue_base, esp->iomem + AUDIO_FFI_FILTER_QUEUE_BASE);
}

static void audio_ffi_init_accel(struct esp_device *esp, void *arg)
{
	struct audio_ffi_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->logn_samples, esp->iomem + AUDIO_FFI_LOGN_SAMPLES_REG_0 + 0x4*esp->context_id);
	iowrite32be(a->do_shift, esp->iomem + AUDIO_FFI_DO_SHIFT_REG_0 + 0x4*esp->context_id);

	iowrite32be(a->input_queue_base, esp->iomem + AUDIO_FFI_INPUT_QUEUE_BASE_0 + 0x4*esp->context_id);
	iowrite32be(a->output_queue_base, esp->iomem + AUDIO_FFI_OUTPUT_QUEUE_BASE_0 + 0x4*esp->context_id);
	iowrite32be(a->filter_queue_base, esp->iomem + AUDIO_FFI_FILTER_QUEUE_BASE_0 + 0x4*esp->context_id);
	
	iowrite32be(a->context_quota, esp->iomem + AUDIO_FFI_CONTEXT_QUOTA);
	iowrite32be(a->valid_contexts, esp->iomem + AUDIO_FFI_VALID_CONTEXTS);
}

static void audio_ffi_add_context(struct esp_device *esp, void *arg)
{
	struct audio_ffi_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->logn_samples, esp->iomem + AUDIO_FFI_LOGN_SAMPLES_REG_0 + 0x4*esp->context_id);
	iowrite32be(a->do_shift, esp->iomem + AUDIO_FFI_DO_SHIFT_REG_0 + 0x4*esp->context_id);

	iowrite32be(a->input_queue_base, esp->iomem + AUDIO_FFI_INPUT_QUEUE_BASE_0 + 0x4*esp->context_id);
	iowrite32be(a->output_queue_base, esp->iomem + AUDIO_FFI_OUTPUT_QUEUE_BASE_0 + 0x4*esp->context_id);
	iowrite32be(a->filter_queue_base, esp->iomem + AUDIO_FFI_FILTER_QUEUE_BASE_0 + 0x4*esp->context_id);

	iowrite32be(a->valid_contexts, esp->iomem + AUDIO_FFI_VALID_CONTEXTS);
}

static void audio_ffi_del_context(struct esp_device *esp, void *arg)
{
	struct audio_ffi_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->valid_contexts, esp->iomem + AUDIO_FFI_VALID_CONTEXTS);
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
	.prep_xfer		= audio_ffi_prep_xfer,
	.init_accel		= audio_ffi_init_accel,
	.add_context	= audio_ffi_add_context,
	.del_context	= audio_ffi_del_context,
	.ioctl_cm		= AUDIO_FFI_STRATUS_IOC_ACCESS,
	.init_cm		= AUDIO_FFI_STRATUS_INIT_IOC_ACCESS,
	.add_cm			= AUDIO_FFI_STRATUS_ADD_IOC_ACCESS,
	.del_cm			= AUDIO_FFI_STRATUS_DEL_IOC_ACCESS,
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
