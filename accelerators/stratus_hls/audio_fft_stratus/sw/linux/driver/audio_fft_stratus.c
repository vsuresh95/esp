// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "audio_fft_stratus.h"

#define DRV_NAME	"audio_fft_stratus"

/* <<--regs-->> */
#define AUDIO_FFT_DO_INVERSE_REG 0x48
#define AUDIO_FFT_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFT_DO_SHIFT_REG 0x40

#define AUDIO_FFT_PROD_VALID_OFFSET 0x4C
#define AUDIO_FFT_PROD_READY_OFFSET 0x50
#define AUDIO_FFT_CONS_VALID_OFFSET 0x54
#define AUDIO_FFT_CONS_READY_OFFSET 0x58
#define AUDIO_FFT_LOAD_DATA_OFFSET 0x5C
#define AUDIO_FFT_STORE_DATA_OFFSET 0x60

struct audio_fft_stratus_device {
	struct esp_device esp;
};

static struct esp_driver audio_fft_driver;

static struct of_device_id audio_fft_device_ids[] = {
	{
		.name = "SLD_AUDIO_FFT_STRATUS",
	},
	{
		.name = "eb_055",
	},
	{
		.compatible = "sld,audio_fft_stratus",
	},
	{ },
};

static int audio_fft_devs;

static inline struct audio_fft_stratus_device *to_audio_fft(struct esp_device *esp)
{
	return container_of(esp, struct audio_fft_stratus_device, esp);
}

static void audio_fft_prep_xfer(struct esp_device *esp, void *arg)
{
	struct audio_fft_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->do_inverse, esp->iomem + AUDIO_FFT_DO_INVERSE_REG);
	iowrite32be(a->logn_samples, esp->iomem + AUDIO_FFT_LOGN_SAMPLES_REG);
	iowrite32be(a->do_shift, esp->iomem + AUDIO_FFT_DO_SHIFT_REG);

	iowrite32be(a->prod_valid_offset, esp->iomem + AUDIO_FFT_PROD_VALID_OFFSET);
	iowrite32be(a->prod_ready_offset, esp->iomem + AUDIO_FFT_PROD_READY_OFFSET);
	iowrite32be(a->cons_valid_offset, esp->iomem + AUDIO_FFT_CONS_VALID_OFFSET);
	iowrite32be(a->cons_ready_offset, esp->iomem + AUDIO_FFT_CONS_READY_OFFSET);
	iowrite32be(a->load_data_offset, esp->iomem + AUDIO_FFT_LOAD_DATA_OFFSET);
	iowrite32be(a->store_data_offset, esp->iomem + AUDIO_FFT_STORE_DATA_OFFSET);

	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);
	iowrite32be(a->spandex_conf, esp->iomem + SPANDEX_REG);

}

static bool audio_fft_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct audio_fft_stratus_device *audio_fft = to_audio_fft(esp); */
	/* struct audio_fft_stratus_access *a = arg; */

	return true;
}

static int audio_fft_probe(struct platform_device *pdev)
{
	struct audio_fft_stratus_device *audio_fft;
	struct esp_device *esp;
	int rc;

	audio_fft = kzalloc(sizeof(*audio_fft), GFP_KERNEL);
	if (audio_fft == NULL)
		return -ENOMEM;
	esp = &audio_fft->esp;
	esp->module = THIS_MODULE;
	esp->number = audio_fft_devs;
	esp->driver = &audio_fft_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	audio_fft_devs++;
	return 0;
 err:
	kfree(audio_fft);
	return rc;
}

static int __exit audio_fft_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct audio_fft_stratus_device *audio_fft = to_audio_fft(esp);

	esp_device_unregister(esp);
	kfree(audio_fft);
	return 0;
}

static struct esp_driver audio_fft_driver = {
	.plat = {
		.probe		= audio_fft_probe,
		.remove		= audio_fft_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = audio_fft_device_ids,
		},
	},
	.xfer_input_ok	= audio_fft_xfer_input_ok,
	.prep_xfer	= audio_fft_prep_xfer,
	.ioctl_cm	= AUDIO_FFT_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct audio_fft_stratus_access),
};

static int __init audio_fft_init(void)
{
	return esp_driver_register(&audio_fft_driver);
}

static void __exit audio_fft_exit(void)
{
	esp_driver_unregister(&audio_fft_driver);
}

module_init(audio_fft_init)
module_exit(audio_fft_exit)

MODULE_DEVICE_TABLE(of, audio_fft_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("audio_fft_stratus driver");
