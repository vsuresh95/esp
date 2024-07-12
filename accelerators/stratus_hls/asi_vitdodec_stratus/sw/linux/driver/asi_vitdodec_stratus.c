// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "asi_vitdodec_stratus.h"

#define DRV_NAME	"asi_vitdodec_stratus"

/* <<--regs-->> */
#define ASI_VITDODEC_IN_LENGTH_REG           0x4C
#define ASI_VITDODEC_OUT_LENGTH_REG          0x50
#define ASI_VITDODEC_INPUT_START_OFFSET_REG  0x54
#define ASI_VITDODEC_OUTPUT_START_OFFSET_REG 0x58
#define ASI_VITDODEC_CONS_VLD_OFFSET_REG     0x5C
#define ASI_VITDODEC_PROD_RDY_OFFSET_REG     0x60
#define ASI_VITDODEC_CONS_RDY_OFFSET_REG     0x64
#define ASI_VITDODEC_PROD_VLD_OFFSET_REG     0x68

#define ASI_VITDODEC_CBPS_REG 0x48
#define ASI_VITDODEC_NTRACEBACK_REG 0x44
#define ASI_VITDODEC_DATA_BITS_REG 0x40

struct asi_vitdodec_stratus_device {
	struct esp_device esp;
};

static struct esp_driver asi_vitdodec_driver;

static struct of_device_id asi_vitdodec_device_ids[] = {
	{
		.name = "SLD_ASI_VITDODEC_STRATUS",
	},
	{
		.name = "eb_031",
	},
	{
		.compatible = "sld,asi_vitdodec_stratus",
	},
	{ },
};

static int asi_vitdodec_devs;

static inline struct asi_vitdodec_stratus_device *to_asi_vitdodec(struct esp_device *esp)
{
	return container_of(esp, struct asi_vitdodec_stratus_device, esp);
}

static void asi_vitdodec_prep_xfer(struct esp_device *esp, void *arg)
{
	struct asi_vitdodec_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->cbps, esp->iomem + ASI_VITDODEC_CBPS_REG);
	iowrite32be(a->ntraceback, esp->iomem + ASI_VITDODEC_NTRACEBACK_REG);
	iowrite32be(a->data_bits, esp->iomem + ASI_VITDODEC_DATA_BITS_REG);
	// iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	// iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

    iowrite32be(a->in_length 			,esp->iomem + ASI_VITDODEC_IN_LENGTH_REG);
    iowrite32be(a->out_length 			,esp->iomem + ASI_VITDODEC_OUT_LENGTH_REG);
    iowrite32be(a->input_start_offset 	,esp->iomem + ASI_VITDODEC_INPUT_START_OFFSET_REG);
    iowrite32be(a->output_start_offset 	,esp->iomem + ASI_VITDODEC_OUTPUT_START_OFFSET_REG);
    iowrite32be(a->accel_cons_vld_offset,esp->iomem + ASI_VITDODEC_CONS_VLD_OFFSET_REG) ;
    iowrite32be(a->accel_prod_rdy_offset,esp->iomem + ASI_VITDODEC_PROD_RDY_OFFSET_REG) ;
    iowrite32be(a->accel_cons_rdy_offset,esp->iomem + ASI_VITDODEC_CONS_RDY_OFFSET_REG) ;
    iowrite32be(a->accel_prod_vld_offset,esp->iomem + ASI_VITDODEC_PROD_VLD_OFFSET_REG) ;
	iowrite32be(a->spandex_reg, esp->iomem + SPANDEX_REG);

}

static bool asi_vitdodec_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct asi_vitdodec_stratus_device *asi_vitdodec = to_asi_vitdodec(esp); */
	/* struct asi_vitdodec_stratus_access *a = arg; */

	return true;
}

static int asi_vitdodec_probe(struct platform_device *pdev)
{
	struct asi_vitdodec_stratus_device *asi_vitdodec;
	struct esp_device *esp;
	int rc;

	asi_vitdodec = kzalloc(sizeof(*asi_vitdodec), GFP_KERNEL);
	if (asi_vitdodec == NULL)
		return -ENOMEM;
	esp = &asi_vitdodec->esp;
	esp->module = THIS_MODULE;
	esp->number = asi_vitdodec_devs;
	esp->driver = &asi_vitdodec_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	asi_vitdodec_devs++;
	return 0;
 err:
	kfree(asi_vitdodec);
	return rc;
}

static int __exit asi_vitdodec_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct asi_vitdodec_stratus_device *asi_vitdodec = to_asi_vitdodec(esp);

	esp_device_unregister(esp);
	kfree(asi_vitdodec);
	return 0;
}

static struct esp_driver asi_vitdodec_driver = {
	.plat = {
		.probe		= asi_vitdodec_probe,
		.remove		= asi_vitdodec_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = asi_vitdodec_device_ids,
		},
	},
	.xfer_input_ok	= asi_vitdodec_xfer_input_ok,
	.prep_xfer	= asi_vitdodec_prep_xfer,
	.ioctl_cm	= ASI_VITDODEC_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct asi_vitdodec_stratus_access),
};

static int __init asi_vitdodec_init(void)
{
	return esp_driver_register(&asi_vitdodec_driver);
}

static void __exit asi_vitdodec_exit(void)
{
	esp_driver_unregister(&asi_vitdodec_driver);
}

module_init(asi_vitdodec_init)
module_exit(asi_vitdodec_exit)

MODULE_DEVICE_TABLE(of, asi_vitdodec_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("asi_vitdodec_stratus driver");
