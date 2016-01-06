/*
 *  linux/arch/arm/kernel/early_kgdb.c
 *
 *  Copyright (C) 2016 Stéphan Kochen <stephan@kochen.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kgdb.h>
#include <linux/init.h>

extern int readch(void);
extern void printch(int);

static int early_kgdb_read_char(void)
{
	return readch();
}

static void early_kgdb_write_char(u8 ch)
{
	printch(ch);
}

static struct kgdb_io early_kgdb_io_ops = {
	.name		= "earlykgdb",
	.read_char	= early_kgdb_read_char,
	.write_char	= early_kgdb_write_char
};

/*
 * Registering a kgdb io module may trigger kgdbwait, but the ARM undefined
 * instruction handler is not yet setup during early_initcall. So use the next
 * step, which is pure_initcall.
 */
static int __init setup_early_kgdb(void)
{
	kgdb_register_io_module(&early_kgdb_io_ops);
	return 0;
}
pure_initcall(setup_early_kgdb);
