/*
 *  linux/arch/arm/mach-tegra/misc-ouya.c
 *
 *  Copyright (C) 2016 Stéphan Kochen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/of.h>
#include <linux/usb.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>

/*
 * The SMSC is connected to USB, but needs a reset to become visible on the
 * bus. We need to wait for both the USB bus and the GPIO chip to be available.
 */

static void ouya_bus_notifier_register(void);
static void ouya_bus_notifier_unregister(void);

static struct usb_bus *pending_smsc_ubus;

/* Returns whether it changed pending_smsc_ubus */
static bool ouya_try_reset_smsc(void)
{
	struct gpio_desc *gpio;

	if (!pending_smsc_ubus)
		return false;

	gpio = gpiod_get(pending_smsc_ubus->controller,
			"ouya,smsc-reset", GPIOD_OUT_HIGH);
	if (!IS_ERR(gpio)) {
		pr_info("ouya-smsc: issued reset through gpio\n");
		pending_smsc_ubus = NULL;
		return true;
	}

	if (PTR_ERR(gpio) == -ENOENT) {
		pending_smsc_ubus = NULL;
		return true;
	}

	if (PTR_ERR(gpio) != -EPROBE_DEFER) {
		pr_err("ouya-smsc: failed to set reset gpio\n");
		pending_smsc_ubus = NULL;
		return true;
	}

	return false;
}

/* Try to trigger the reset every time a bus is added with the devicetree
 * property set. Sometimes, the gpio chip won't be available at this point,
 * in which case we wait for it through a platform bus notifier. */
static int ouya_usb_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct usb_bus *ubus = data;

	if (action == USB_BUS_ADD && !pending_smsc_ubus) {
		pending_smsc_ubus = ubus;
		if (!ouya_try_reset_smsc())
			ouya_bus_notifier_register();
	}
	else if (action == USB_BUS_REMOVE && pending_smsc_ubus == ubus) {
		pending_smsc_ubus = NULL;
		ouya_bus_notifier_unregister();
	}

	return 0;
}
static struct notifier_block ouya_usb_notifier_block = {
	.notifier_call = ouya_usb_notifier
};

/* The platform bus notifier is installed only while
 * we wait for the gpio chip to come up. */
static int ouya_bus_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct device *dev = data;

	if (action == BUS_NOTIFY_BOUND_DRIVER
			&& !strcmp(dev->driver->name, "tegra-gpio")
			&& ouya_try_reset_smsc())
		ouya_bus_notifier_unregister();

	return 0;
}
static struct notifier_block ouya_bus_notifier_block = {
	.notifier_call = ouya_bus_notifier
};
static void ouya_bus_notifier_register(void)
{
	bus_register_notifier(&platform_bus_type, &ouya_bus_notifier_block);
}
static void ouya_bus_notifier_unregister(void)
{
	bus_unregister_notifier(&platform_bus_type, &ouya_bus_notifier_block);
}

/* Register USB bus notifier */
static int __init ouya_misc_init(void)
{
	if (of_machine_is_compatible("ouya"))
		usb_register_notify(&ouya_usb_notifier_block);
	return 0;
}
arch_initcall(ouya_misc_init);
