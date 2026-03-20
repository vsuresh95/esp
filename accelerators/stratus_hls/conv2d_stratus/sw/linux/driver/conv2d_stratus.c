#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "conv2d_stratus.h"

#define DRV_NAME	"conv2d_stratus"

/* <<--regs-->> */
#define CONV2D_N_CHANNELS_REG 			0x98
#define CONV2D_FEATURE_MAP_HEIGHT_REG	0x9C
#define CONV2D_FEATURE_MAP_WIDTH_REG	0xA0
#define CONV2D_N_FILTERS_REG			0xA4
#define CONV2D_FILTER_HEIGHT_REG		0xA8
#define CONV2D_FILTER_WIDTH_REG			0xAC
#define CONV2D_IS_PADDED_REG			0xB0
#define CONV2D_STRIDE_REG				0xB4
#define CONV2D_DO_RELU_REG				0xB8
#define CONV2D_POOL_TYPE_REG			0xBC
#define CONV2D_BATCH_SIZE_REG			0xC0
#define CONV2D_INPUT_OFFSET_REG			0xC4
#define CONV2D_FILTERS_OFFSET_REG		0xC8
#define CONV2D_BIAS_OFFSET_REG			0xCC
#define CONV2D_OUTPUT_OFFSET_REG 		0xD0

struct conv2d_stratus_device {
	struct esp_device esp;
};

static struct esp_driver conv2d_driver;

static struct of_device_id conv2d_device_ids[] = {
	{
		.name = "SLD_CONV2D_STRATUS",
	},
	{
		.name = "eb_052",
	},
	{
		.compatible = "sld,conv2d_stratus",
	},
	{ },
};

static int conv2d_devs;

static inline struct conv2d_stratus_device *to_conv2d(struct esp_device *esp)
{
	return container_of(esp, struct conv2d_stratus_device, esp);
}

static void conv2d_prep_xfer(struct esp_device *esp, void *arg)
{
	struct conv2d_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->params.n_channels, esp->iomem + CONV2D_N_CHANNELS_REG);
	iowrite32be(a->params.feature_map_height, esp->iomem + CONV2D_FEATURE_MAP_HEIGHT_REG);
	iowrite32be(a->params.feature_map_width, esp->iomem + CONV2D_FEATURE_MAP_WIDTH_REG);
	iowrite32be(a->params.n_filters, esp->iomem + CONV2D_N_FILTERS_REG);
	iowrite32be(a->params.filter_height, esp->iomem + CONV2D_FILTER_HEIGHT_REG);
	iowrite32be(a->params.filter_width, esp->iomem + CONV2D_FILTER_WIDTH_REG);
	iowrite32be(a->params.is_padded, esp->iomem + CONV2D_IS_PADDED_REG);
	iowrite32be(a->params.stride, esp->iomem + CONV2D_STRIDE_REG);
	iowrite32be(a->params.do_relu, esp->iomem + CONV2D_DO_RELU_REG);
	iowrite32be(a->params.pool_type, esp->iomem + CONV2D_POOL_TYPE_REG);
	iowrite32be(a->params.batch_size, esp->iomem + CONV2D_BATCH_SIZE_REG);
	iowrite32be(a->params.input_offset, esp->iomem + CONV2D_INPUT_OFFSET_REG);
	iowrite32be(a->params.filters_offset, esp->iomem + CONV2D_FILTERS_OFFSET_REG);
	iowrite32be(a->params.bias_offset, esp->iomem + CONV2D_BIAS_OFFSET_REG);
	iowrite32be(a->params.output_offset, esp->iomem + CONV2D_OUTPUT_OFFSET_REG);
}

static bool conv2d_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct conv2d_stratus_device *conv2d = to_conv2d(esp); */
	/* struct conv2d_stratus_access *a = arg; */

	return true;
}

static int conv2d_probe(struct platform_device *pdev)
{
	struct conv2d_stratus_device *conv2d;
	struct esp_device *esp;
	int rc;

	conv2d = kzalloc(sizeof(*conv2d), GFP_KERNEL);
	if (conv2d == NULL)
		return -ENOMEM;
	esp = &conv2d->esp;
	esp->module = THIS_MODULE;
	esp->number = conv2d_devs;
	esp->driver = &conv2d_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	conv2d_devs++;
	return 0;
 err:
	kfree(conv2d);
	return rc;
}

static int __exit conv2d_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct conv2d_stratus_device *conv2d = to_conv2d(esp);

	esp_device_unregister(esp);
	kfree(conv2d);
	return 0;
}

static struct esp_driver conv2d_driver = {
	.plat = {
		.probe		= conv2d_probe,
		.remove		= conv2d_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = conv2d_device_ids,
		},
	},
	.xfer_input_ok	= conv2d_xfer_input_ok,
	.prep_xfer	= conv2d_prep_xfer,
	.ioctl_cm	= CONV2D_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct conv2d_stratus_access),
};

static int __init conv2d_init(void)
{
	return esp_driver_register(&conv2d_driver);
}

static void __exit conv2d_exit(void)
{
	esp_driver_unregister(&conv2d_driver);
}

module_init(conv2d_init)
module_exit(conv2d_exit)

MODULE_DEVICE_TABLE(of, conv2d_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("conv2d_stratus driver");
