// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "mesh_gen_stratus.h"

#define DRV_NAME	"mesh_gen_stratus"

/* <<--regs-->> */
#define MESH_GEN_NUM_HASH_TABLE_REG 0x40

struct mesh_gen_stratus_device {
	struct esp_device esp;
};

static struct esp_driver mesh_gen_driver;

static struct of_device_id mesh_gen_device_ids[] = {
	{
		.name = "SLD_MESH_GEN_STRATUS",
	},
	{
		.name = "eb_040",
	},
	{
		.compatible = "sld,mesh_gen_stratus",
	},
	{ },
};

static int mesh_gen_devs;

static inline struct mesh_gen_stratus_device *to_mesh_gen(struct esp_device *esp)
{
	return container_of(esp, struct mesh_gen_stratus_device, esp);
}

static void mesh_gen_prep_xfer(struct esp_device *esp, void *arg)
{
	struct mesh_gen_stratus_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->num_hash_table, esp->iomem + MESH_GEN_NUM_HASH_TABLE_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool mesh_gen_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct mesh_gen_stratus_device *mesh_gen = to_mesh_gen(esp); */
	/* struct mesh_gen_stratus_access *a = arg; */

	return true;
}

static int mesh_gen_probe(struct platform_device *pdev)
{
	struct mesh_gen_stratus_device *mesh_gen;
	struct esp_device *esp;
	int rc;

	mesh_gen = kzalloc(sizeof(*mesh_gen), GFP_KERNEL);
	if (mesh_gen == NULL)
		return -ENOMEM;
	esp = &mesh_gen->esp;
	esp->module = THIS_MODULE;
	esp->number = mesh_gen_devs;
	esp->driver = &mesh_gen_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	mesh_gen_devs++;
	return 0;
 err:
	kfree(mesh_gen);
	return rc;
}

static int __exit mesh_gen_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct mesh_gen_stratus_device *mesh_gen = to_mesh_gen(esp);

	esp_device_unregister(esp);
	kfree(mesh_gen);
	return 0;
}

static struct esp_driver mesh_gen_driver = {
	.plat = {
		.probe		= mesh_gen_probe,
		.remove		= mesh_gen_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = mesh_gen_device_ids,
		},
	},
	.xfer_input_ok	= mesh_gen_xfer_input_ok,
	.prep_xfer	= mesh_gen_prep_xfer,
	.ioctl_cm	= MESH_GEN_STRATUS_IOC_ACCESS,
	.arg_size	= sizeof(struct mesh_gen_stratus_access),
};

static int __init mesh_gen_init(void)
{
	return esp_driver_register(&mesh_gen_driver);
}

static void __exit mesh_gen_exit(void)
{
	esp_driver_unregister(&mesh_gen_driver);
}

module_init(mesh_gen_init)
module_exit(mesh_gen_exit)

MODULE_DEVICE_TABLE(of, mesh_gen_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("mesh_gen_stratus driver");
