/* Copyright (c) 2016, Fergy (ramon.rebersak@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "msm_sensor.h"
#define imx300_SENSOR_NAME "imx300"
DEFINE_MSM_MUTEX(imx300_mut);

static struct msm_sensor_ctrl_t imx300_s_ctrl;

static struct msm_sensor_power_setting imx300_power_setting[] = {
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 1,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 1,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 1,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct v4l2_subdev_info imx300_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id imx300_i2c_id[] = {
	{imx300_SENSOR_NAME, (kernel_ulong_t)&imx300_s_ctrl},
	{ }
};

static int32_t msm_imx300_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &imx300_s_ctrl);
}

static struct i2c_driver imx300_i2c_driver = {
	.id_table = imx300_i2c_id,
	.probe  = msm_imx300_i2c_probe,
	.driver = {
		.name = imx300_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx300_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id imx300_dt_match[] = {
	{.compatible = "sony,imx300", .data = &imx300_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, imx300_dt_match);

static struct platform_driver imx300_platform_driver = {
	.driver = {
		.name = "sony,imx300",
		.owner = THIS_MODULE,
		.of_match_table = imx300_dt_match,
	},
};

static const char *imx300Vendor = "Sony";
static const char *imx300NAME = "imx300";
static const char *imx300Size = "23M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", imx300Vendor, imx300NAME, imx300Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_imx300;

static int imx300_sysfs_init(void)
{
	int ret ;
	pr_info("imx300:kobject creat and add\n");
	android_imx300 = kobject_create_and_add("android_camera", NULL);
	if (android_imx300 == NULL) {
		pr_info("imx300_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("imx300:sysfs_create_file\n");
	ret = sysfs_create_file(android_imx300, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("imx300_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_imx300);
	}

	return 0 ;
}

static int32_t imx300_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	match = of_match_device(imx300_dt_match, &pdev->dev);
	if (match)
		rc = msm_sensor_platform_probe(pdev, match->data);
	else {
		pr_err("%s:%d match is null\n", __func__, __LINE__);
		rc = -EINVAL;
	}
	return rc;
}

static int __init imx300_init_module(void)
{
	int32_t rc = 0;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&imx300_platform_driver,
		imx300_platform_probe);
	if (!rc) {
		imx300_sysfs_init();
		return rc;
	}
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&imx300_i2c_driver);
}

static void __exit imx300_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (imx300_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&imx300_s_ctrl);
		platform_driver_unregister(&imx300_platform_driver);
	} else
		i2c_del_driver(&imx300_i2c_driver);
	return;
}

static struct msm_sensor_fn_t imx300_sensor_func_tbl = {
	.sensor_config = msm_sensor_config,
	.sensor_config32 = msm_sensor_config32,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
};

static struct msm_sensor_ctrl_t imx300_s_ctrl = {
	.sensor_i2c_client = &imx300_sensor_i2c_client,
	.power_setting_array.power_setting = imx300_power_setting,
	.power_setting_array.size = ARRAY_SIZE(imx300_power_setting),
	.msm_sensor_mutex = &imx300_mut,
	.sensor_v4l2_subdev_info = imx300_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx300_subdev_info),
	.func_tbl = &imx300_sensor_func_tbl
};

module_init(imx300_init_module);
module_exit(imx300_exit_module);
MODULE_AUTHOR("ramon.rebersak@gmail.com");
MODULE_DESCRIPTION("Sony imx300 (23MP) back sensor module");
MODULE_LICENSE("GPL v2");

