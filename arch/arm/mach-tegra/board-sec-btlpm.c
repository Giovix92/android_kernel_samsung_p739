/* linux/arch/arm/mach-tegra/board-p3-btlpm.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/serial_core.h>
#include <asm/mach-types.h>
#include <mach/gpio-sec.h>
#include <linux/wakelock.h>

static struct bt_lpm {
	struct hrtimer bt_lpm_timer;
	ktime_t bt_lpm_delay;
} bt_lpm;

static struct wake_lock rfkill_wake_lock2;
static struct timer_list tx_timer;
static int ignore_set_wake_lock = 0;

static enum hrtimer_restart bt_enter_lpm(struct hrtimer *timer)
{
	pr_err("bt_enter_lpm\n");
	gpio_set_value(GPIO_BT_WAKE, 0);

	wake_unlock(&rfkill_wake_lock2);
	ignore_set_wake_lock = 0;
	return HRTIMER_NORESTART;
}

void p3_bt_uart_wake_peer(struct uart_port *unused)
{
	if (!bt_lpm.bt_lpm_timer.function)
		return;
	
	if (!ignore_set_wake_lock) {
		pr_err("p3_bt_uart_wake_peer - wake_lock\n");
		wake_lock_timeout(&rfkill_wake_lock2, 2*HZ); //2sec
		mod_timer(&tx_timer, jiffies + (HZ / 5)); //200msec
		ignore_set_wake_lock = 1;
	}
	
	hrtimer_try_to_cancel(&bt_lpm.bt_lpm_timer);
	gpio_set_value(GPIO_BT_WAKE, 1);
	hrtimer_start(&bt_lpm.bt_lpm_timer, bt_lpm.bt_lpm_delay,
		HRTIMER_MODE_REL);
}

static void tx_timer_expire(unsigned long data)
{
	ignore_set_wake_lock = 0;
}

int p3_bt_lpm_init(void)
{
	int ret;
	pr_err("p3_bt_lpm_init\n");

	wake_lock_init(&rfkill_wake_lock2, WAKE_LOCK_SUSPEND, "bt_wake");
	
	tegra_gpio_enable(GPIO_BT_WAKE);
	ret = gpio_request(GPIO_BT_WAKE, "gpio_bt_wake");
	if (ret) {
		pr_err("Failed to request gpio_bt_wake control\n");
		return 0;
	}

	ret = gpio_direction_output(GPIO_BT_WAKE, 0);
	if (ret) {
		pr_err("Failed to set gpio_direction_output\n");
		return 0;
	}
	//add timer because wake_lock_timeout is called too frequently
	init_timer(&tx_timer);
	tx_timer.function = tx_timer_expire;
	tx_timer.data = 0;

	hrtimer_init(&bt_lpm.bt_lpm_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	bt_lpm.bt_lpm_delay = ktime_set(1, 0);	/* 1 sec */
	bt_lpm.bt_lpm_timer.function = bt_enter_lpm;
	return 0;
}
