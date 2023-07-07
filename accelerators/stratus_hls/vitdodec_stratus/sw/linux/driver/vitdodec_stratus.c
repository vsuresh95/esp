// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "vitdodec_stratus.h"

#define DRV_NAME	"vitdodec_stratus"

/* <<--regs-->> */
#define VITDODEC_IN_LENGTH_REG           0x4C
#define VITDODEC_OUT_LENGTH_REG          0x50
#define VITDODEC_INPUT_START_OFFSET_REG  0x54
#define VITDODEC_OUTPUT_START_OFFSET_REG 0x58
#define VITDODEC_CONS_VLD_OFFSET_REG     0x5C
#define VITDODEC_PROD_RDY_OFFSET_REG     0x60
#define VITDODEC_CONS_RDY_OFFSET_REG     0x64
#define VITDODEC_PROD_VLD_OFFSET_REG     0x68

#define VITDODEC_CBPS_REG 0x48
#define VITDODEC_NTRACEBACK_REG 0x44
#define VITDODEC_DATA_BITS_REG 0x40

struct vitdodec_stratus_device {
	struct esp_device esp;
};

static struct esp_driver vitdodec_driver;

static struct of_device_id vitdodec_device_ids[] = {
	{
		.name = "SLD_VITDODEC_STRATUS",
	},
	{
		.name = "eb_030",
	},
	{
		.compatible = "sld,vitdodec_stratus",
	},
	{ },
};

static int vitdodec_devs;

static inline struct vitdodec_stratus_device *to_vitdodec(struct esp_device *esp)
{
	return container_of(esp, struct vitdodec_stratus_device, esp);
}

static void vitdodec_prep_xfer(struct esp_device *esp, void *arg)
{
	struct vitdodec_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->cbps, esp->iomem + VITDODEC_CBPS_REG);
	iowrite32be(a->ntraceback, esp->iomem + VITDODEC_NTRACEBACK_REG);
	iowrite32be(a->data_bits, esp->iomem + VITDODEC_DATA_BITS_REG);
	// iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	// iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

    iowrite32be(a->in_length 			,esp->iomem + VITDODEC_IN_LENGTH_REG);
    iowrite32be(a->out_length 			,esp->iomem + VITDODEC_OUT_LENGTH_REG);
    iowrite32be(a->input_start_offset 	,esp->iomem + VITDODEC_INPUT_START_OFFSET_REG);
    iowrite32be(a->output_start_offset 	,esp->iomem + VITDODEC_OUTPUT_START_OFFSET_REG);
    iowrite32be(a->accel_cons_vld_offset,esp->iomem + VITDODEC_CONS_VLD_OFFSET_REG) ;
    iowrite32be(a->accel_prod_rdy_offset,esp->iomem + VITDODEC_PROD_RDY_OFFSET_REG) ;
    iowrite32be(a->accel_cons_rdy_offset,esp->iomem + VITDODEC_CONS_RDY_OFFSET_REG) ;
    iowrite32be(a->accel_prod_vld_offset,esp->iomem + VITDODEC_PROD_VLD_OFFSET_REG) ;
	iowrite32be(a->spandex_reg, esp->iomem + SPANDEX_REG);

}

static bool vitdodec_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct vitdodec_stratus_device *vitdodec = to_vitdodec(esp); */
	/* struct vitdodec_stratus_access *a = arg; */

	return true;
}

static int vitdodec_probe(struct platform_device *pdev)
{
	struct vitdodec_stratus_device *vitdodec;
	struct esp_device *esp;
	int rc;

	vitdodec = kzalloc(sizeof(*vitdodec), GFP_KERNEL);
	if (vitdodec == NULL)
		return -ENOMEM;
	esp = &vitdodec->esp;
	esp->module = THIS_MODULE;
	esp->number = vitdodec_devs;
	esp->driver = &vitdodec_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	vitdodec_devs++;
	return 0;
 err:
	kfree(vitdodec);
	return rc;
}

static int __exit vitdodec_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct vitdodec_stratus_device *vitdodec = to_vitdodec(esp);

	esp_device_unregister(esp);
	kfree(vitdodec);
	return 0;
}

static struct esp_driver vitdodec_driver = {
	.plat = {
		.probe		= vitdodec_probe,
		.remove		= vitdodec_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = vitdodec_device_ids,
		},
	},
	.xfer_input_ok	= vitdodec_xfer_input_ok,
	.prep_xfer	= vitdodec_prep_xfer,
	.ioctl_cm	= VITDODEC_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct vitdodec_stratus_access),
};

static int __init vitdodec_init(void)
{
	return esp_driver_register(&vitdodec_driver);
}

static void __exit vitdodec_exit(void)
{
	esp_driver_unregister(&vitdodec_driver);
}

module_init(vitdodec_init)
module_exit(vitdodec_exit)

MODULE_DEVICE_TABLE(of, vitdodec_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vitdodec_stratus driver");
