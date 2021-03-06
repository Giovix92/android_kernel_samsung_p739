/*
 *  atmel_maxtouch.c - Atmel maXTouch Touchscreen Controller
 *
 *  Version 0.2a
 *
 *  An early alpha version of the maXTouch Linux driver.
 *
 *
 *  Copyright (C) 2010 Iiro Valkonen <iiro.valkonen@atmel.com>
 *  Copyright (C) 2009 Ulf Samuelsson <ulf.samuelsson@atmel.com>
 *  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define	DEBUG_INFO      1
#define	DEBUG_VERBOSE   2
#define	DEBUG_MESSAGES  5
#define	DEBUG_RAW       8
#define	DEBUG_TRACE     10

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>

#include <linux/init.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/major.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
#include <linux/smp_lock.h>

#include <linux/delay.h>
#include <linux/atmel_mxt1386.h>
#include "atmel_mxt1386_cfg.h"

#include <linux/reboot.h>


/*
 * This is a driver for the Atmel maXTouch Object Protocol
 *
 * When the driver is loaded, mxt_init is called.
 * mxt_driver registers the "mxt_driver" structure in the i2c subsystem
 * The mxt_idtable.name string allows the board support to associate
 * the driver with its own data.
 *
 * The i2c subsystem will call the mxt_driver.probe == mxt_probe
 * to detect the device.
 * mxt_probe will reset the maXTouch device, and then
 * determine the capabilities of the I2C peripheral in the
 * host processor (needs to support BYTE transfers)
 *
 * If OK; mxt_probe will try to identify which maXTouch device it is
 * by calling mxt_identify.
 *
 * If a known device is found, a linux input device is initialized
 * the "mxt" device data structure is allocated
 * as well as an input device structure "mxt->input"
 * "mxt->client" is provided as a parameter to mxt_probe.
 *
 * mxt_read_object_table is called to determine which objects
 * are present in the device, and to determine their adresses
 *
 *
 * Addressing an object:
 *
 * The object is located at a 16 address in the object address space
 *
 * The object address can vary between revisions of the firmware
 *
 * The address is provided through an object descriptor table in the beginning
 * of the object address space.
 * It is assumed that an object type is only listed once in this table,
 * Each object type can have several instances, and the number of
 * instances is available in the object table
 *
 * The base address of the first instance of an object is stored in
 * "mxt->object_table[object_type].chip_addr",
 * This is indexed by the object type and allows direct access to the
 * first instance of an object.
 *
 * Each instance of an object is assigned a "Report Id" uniquely identifying
 * this instance. Information about this instance is available in the
 * "mxt->report_id" variable, which is a table indexed by the "Report Id".
 *
 * The maXTouch object protocol supports adding a checksum to messages.
 * By setting the most significant bit of the maXTouch address
 * an 8 bit checksum is added to all writes.
 *
 *
 * How to use driver.
 * -----------------
 * Example:
 * In arch/avr32/boards/atstk1000/atstk1002.c
 * an "i2c_board_info" descriptor is declared.
 * This contains info about which driver ("mXT224"),
 * which i2c address and which pin for CHG interrupts are used.
 *
 * In the "atstk1002_init" routine, "i2c_register_board_info" is invoked
 * with this information. Also, the I/O pins are configured, and the I2C
 * controller registered is on the application processor.
 *
 */


#if 1/*for debugging, enable DEBUG_TRACE*/
static int debug = DEBUG_INFO;
#else
static int debug = DEBUG_TRACE;  /* for debugging*/
#endif

static int resume_flag = 0;
#define MXT_MESSAGE_LENGTH 8

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activate debugging output");

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *h);
static void mxt_late_resume(struct early_suspend *h);
#endif

extern struct class *sec_class;

#define I2C_RETRY_COUNT 5

/* Returns the start address of object in mXT memory. */
#define	MXT_BASE_ADDR(object_type) \
get_object_address(object_type, 0, mxt->object_table, mxt->device_info.num_objs)

/* If report_id (rid) == 0, then "mxt->report_id[rid].object" will be 0. */
#define	REPORT_ID_TO_OBJECT(rid) \
(((rid) == 0xff) ? 0 : mxt->rid_map[rid].object)

#define	REPORT_ID_TO_OBJECT_NAME(rid) \
object_type_name[REPORT_ID_TO_OBJECT(rid)]

#define	T6_REG(x) (MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + (x))
#define	T37_REG(x) (MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37) +  (x))

/* ADD TRACKING_ID */
#define REPORT_MT(touch_number, x, y, amplitude, size) \
do {     \
	input_report_abs(mxt->input, ABS_MT_TRACKING_ID, touch_number);\
	input_report_abs(mxt->input, ABS_MT_POSITION_X, x);             \
	input_report_abs(mxt->input, ABS_MT_POSITION_Y, y);             \
	input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR, amplitude);         \
	input_report_abs(mxt->input, ABS_MT_WIDTH_MAJOR, size); \
	input_mt_sync(mxt->input);                                      \
} while (0)

const u8 *maxtouch_family = "maXTouch";
const u8 *mxt224_variant  = "mXT1386";

u8	*object_type_name[MXT_MAX_OBJECT_TYPES]	= {
/*	[0]	= "Reserved",	*/
/*	[2]	= "T2 - Obsolete",	*/
/*	[3]	= "T3 - Obsolete",	*/
	[5]	= "GEN_MESSAGEPROCESSOR_T5",
	[6]	= "GEN_COMMANDPROCESSOR_T6",
	[7]	= "GEN_POWERCONFIG_T7",
	[8]	= "GEN_ACQUIRECONFIG_T8",
	[9]	= "TOUCH_MULTITOUCHSCREEN_T9",
	[15]	= "TOUCH_KEYARRAY_T15",
	[18]	= "SPT_COMMSCONFIG_T18",
/*	[19]	= "T19 - Obsolete",*/
	[22]	= "PROCG_NOISESUPPRESSION_T22",
/*	[23]	= "T23 - Obsolete",*/
	[24]	= "PROCI_ONETOUCHGESTUREPROCESSOR_T24",
	[25]	= "SPT_SELFTEST_T25",
/*	[26]	= "T26 - Obsolete",*/
	[27]	= "PROCI_TWOTOUCHGESTUREPROCESSOR_T27",
	[28]	= "SPT_CTECONFIG_T28",
	[37]	= "DEBUG_DIAGNOSTICS_T37",
	[38]	= "USER_DATA_T38",
	[40]	= "PROCI_GRIPSUPPRESSION_T40",
	[41]	= "PROCI_PALMSUPPRESSION_T41",
	[43]	= "SPT_DIGITIZER_T43",
	[44]	= "SPT_MESSAGECOUNT_T44",
};

#if 1/*def _SUPPORT_MULTITOUCH_*/
struct multi_touch_info {
	uint16_t size;
	int16_t pressure;
	int16_t x;
	int16_t y;
};

static struct multi_touch_info mtouch_info[MXT_MAX_NUM_TOUCHES];
#endif

int backup_to_nv(struct mxt_data *mxt)
{
	/* backs up settings to the non-volatile memory */
	return mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) +
	       MXT_ADR_T6_BACKUPNV,
	       0x55);
}

int reset_chip(struct mxt_data *mxt, u8 mode)
{
	u8 data;
	pr_info("Reset chip Reset mode (%d)", mode);
	if (mode == RESET_TO_NORMAL)
		data = 0x1;/* non-zero value*/
	else if (mode == RESET_TO_BOOTLOADER)
		data = 0xA5;
	else {
		pr_err("Invalid reset mode(%d)", mode);
		return -1;
	}

	/* Any non-zero value written to reset reg will reset the chip */
	return mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) +
				MXT_ADR_T6_RESET,
				data);
}

#ifdef MXT_ERROR_WORKAROUND
static void mxt_forced_release(struct mxt_data *mxt)
{
	int i;
	int temp_value = 0;
	pr_warning("%s has been called", __func__);
	for (i = 0; i < MXT_MAX_NUM_TOUCHES ; i++)	{
		if (mtouch_info[i].pressure == -1)
			continue;

		mtouch_info[i].pressure = 0;

		REPORT_MT(i,
			mtouch_info[i].x,	mtouch_info[i].y,
			mtouch_info[i].pressure, mtouch_info[i].size);

		if (mtouch_info[i].pressure == 0)
			mtouch_info[i].pressure = -1;
		temp_value++;
	}
	if (temp_value > 0)
		input_sync(mxt->input);
}


static void mxt_force_reset(struct mxt_data *mxt)
{
	pr_warning("%s: has been called!\n", __func__);

	if (mxt->pdata->suspend_platform_hw && mxt->pdata->resume_platform_hw) {
		mxt->pdata->suspend_platform_hw();
		msleep(400);
		mxt->pdata->resume_platform_hw();
	}
	mxt_forced_release(mxt);
}
#endif


#if defined(MXT_DRIVER_FILTER)
static void equalize_coordinate(bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[MXT_MAX_NUM_TOUCHES] = { 0, };
	static u16 pre_x[MXT_MAX_NUM_TOUCHES][4] = {{0}, };
	static u16 pre_y[MXT_MAX_NUM_TOUCHES][4] = {{0}, };
	int coff[4] = {0,};
	int distance = 0;

	if (detect)
		tcount[id] = 0;

	pre_x[id][tcount[id]%4] = *px;
	pre_y[id][tcount[id]%4] = *py;

	if (tcount[id] > 3) {
		distance = abs(pre_x[id][(tcount[id]-1)%4] - *px)
			+ abs(pre_y[id][(tcount[id]-1)%4] - *py);

		coff[0] = (u8)(4 + distance/5);
		if (coff[0] < 8) {
			coff[0] = max(4, coff[0]);
			coff[1] = min((10 - coff[0]), (coff[0]>>1)+1);
			coff[2] = min((10 - coff[0] - coff[1]), (coff[1]>>1)+1);
			coff[3] = 10 - coff[0] - coff[1] - coff[2];
			pr_debug("[sec_touch] %d, %d, %d, %d\n",
					coff[0], coff[1], coff[2], coff[3]);
			*px = (u16)((*px*(coff[0])
				+ pre_x[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_x[id][(tcount[id]-2)%4]*(coff[2])
				+ pre_x[id][(tcount[id]-3)%4]*(coff[3]))/10);
			*py = (u16)((*py*(coff[0])
				+ pre_y[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_y[id][(tcount[id]-2)%4]*(coff[2])
				+ pre_y[id][(tcount[id]-3)%4]*(coff[3]))/10);
		} else {
			*px = (u16)((*px*4 + pre_x[id][(tcount[id]-1)%4])/5);
			*py = (u16)((*py*4 + pre_y[id][(tcount[id]-1)%4])/5);
		}
	}
	tcount[id]++;
}
#endif  /*MXT_DRIVER_FILTER*/

/*mode 1 = Charger connected */
/*mode 0 = Charger disconnected*/

static void mxt_inform_charger_connection(struct mxt_callbacks *cb, int mode)
{
	struct mxt_data *mxt = container_of(cb, struct mxt_data, callbacks);

	mxt->set_mode_for_ta = !!mode;
	if (mxt->enabled && !work_pending(&mxt->ta_work))
		schedule_work(&mxt->ta_work);
}

static void mxt_ta_worker(struct work_struct *work)
{
	struct mxt_data *mxt = container_of(work, struct mxt_data, ta_work);
	u8 blen = mxt->pdata->touchscreen_config.blen;
	u8 tchthr = mxt->pdata->touchscreen_config.tchthr;
	u8 noisethr = mxt->pdata->noise_suppression_config.noisethr;
	u8 idlegcafdepth = mxt->pdata->cte_config.idlegcafdepth;
	u8 movefilter = mxt->pdata->touchscreen_config.movfilter;
	u8 freq[5] = {0,};
	int error = 0, i = 0;

	pr_info("[sec_touch] TA/USB is%sconnected.\n",
		mxt->set_mode_for_ta ? " " : " dis");

	for (i = 0; i < 5; i++)
		freq[i] = mxt->pdata->noise_suppression_config.freq[i];

	if (mxt->set_mode_for_ta) {
		tchthr = mxt->pdata->tchthr_for_ta_connect;
		noisethr = mxt->pdata->noisethr_for_ta_connect;
		idlegcafdepth = mxt->pdata->idlegcafdepth_ta_connect;

		if (mxt->pdata->fherr_cnt >= 3) {
			blen = mxt->pdata->tch_blen_for_fherr;
			tchthr = mxt->pdata->tchthr_for_fherr;
			noisethr = mxt->pdata->noisethr_for_fherr;
			movefilter = mxt->pdata->movefilter_for_fherr;
			for (i = 0; i < 5; i++) {
				if (mxt->pdata->fherr_cnt >= 9)
				freq[i] = mxt->pdata->freq_for_fherr3[i];
				else if (mxt->pdata->fherr_cnt >= 6)
				freq[i] = mxt->pdata->freq_for_fherr2[i];
				else
				freq[i] = mxt->pdata->freq_for_fherr1[i];
		}
	}
	}

	pr_info("[TSP] frequency table \n", freq[i]);
	for (i = 0; i < 5; i++)
		pr_info("[TSP] frequency[%d] : %u\n", i, freq[i]);

	disable_irq(mxt->client->irq);

	mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
		+ MXT_ADR_T9_BLEN, blen);

	/* change to ta_connect config*/
	/* tchthr change*/
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
		+ MXT_ADR_T9_TCHTHR, tchthr);
	if (error < 0)
		pr_err("[sec_touch] error %s: write_object : tchthr\n",
				__func__);

	/* noisethr change*/
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T22)
		+ MXT_ADR_T22_NOISETHR,noisethr);
	if (error < 0)
		pr_err("[sec_touch] error %s: write_object : noisethr\n",
				__func__);

	/* freq change*/
	error = mxt_write_block(mxt->client,
		MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T22)
		+ MXT_ADR_T22_FREQ, 5, freq);
	if (error < 0)
		pr_err("[sec_touch] error %s: write_object : freq\n",
				__func__);

	/* idlegcafdepth change*/
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T28)
		+ MXT_ADR_T28_IDLEGCAFDEPTH,
		idlegcafdepth);
	if (error < 0)
		pr_err("[sec_touch] error %s: write_object : idlegcafdepth\n",
				__func__);

	/* move filter change */
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
		+ MXT_ADR_T9_MOVFILTER,
		movefilter);
	if (error < 0)
		pr_err("[sec_touch] error %s: write_object : idlegcafdepth\n",
				__func__);

	/* mxt_calibrate : non-zero value*/
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6)
		+ MXT_ADR_T6_CALIBRATE,
	       0x1);
	if (error < 0)
		pr_err("[sec_touch] error %s: mxt_calibrate\n",
				__func__);

	enable_irq(mxt->client->irq);
}

#ifdef MXT_CALIBRATE_WORKAROUND
static void mxt_calibrate_worker(struct work_struct *work)
{
	struct	mxt_data *mxt;
	u8		buf[4];
	int error;
	mxt = container_of(work, struct mxt_data, calibrate_dwork.work);

	if (mxt->enabled == true) {
		disable_irq(mxt->client->irq);
		memcpy(buf,  &mxt->pdata->atchcalst_idle, sizeof(buf));
#ifdef CONFIG_KERNEL_DEBUG_SEC
		pr_info("%s : Idle calibrate buff value is %d, %d, %d, %d",
			__func__, buf[0], buf[1], buf[2], buf[3]);
#endif
		/* change auto calibration config*/
		/* from atchcalst to atchcalfrcratio change*/
		error = mxt_write_block(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8)
			+ MXT_ADR_T8_ATCHCALST,
			4,
			(u8 *) buf);
		if (error < 0)
			pr_err("[sec_touch] error %s: write_object : from atchcalst to atchcalfrcratio\n",
					__func__);
		enable_irq(mxt->client->irq);
	}
}
#endif

/* Calculates the 24-bit CRC sum. */

static u32 mxt_CRC_24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 result;
	u16 data_word;

	data_word = (u16) ((u16) (byte2 << 8u) | byte1);
	result = ((crc << 1u) ^ (u32) data_word);
	if (result & 0x1000000)
		result ^= crcpoly;
	return result;
}

/* Returns object address in mXT chip, or zero if object is not found */
u16 get_object_address(uint8_t object_type,
		       uint8_t instance,
		       struct mxt_object *object_table,
		       int max_objs)
{
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = 0;

	struct mxt_object obj;

	while ((object_table_index < max_objs) && !address_found) {
		obj = object_table[object_table_index];
		if (obj.type == object_type) {
			address_found = 1;
			/* Are there enough instances defined in the FW? */
			if (obj.instances >= instance)
				address = obj.chip_addr +
					  (obj.size + 1) * instance;
			else
				return 0;
		}
		object_table_index++;
	}

	return address;
}

/* Returns object size in mXT chip, or zero if object is not found */
u16 get_object_size(uint8_t object_type,
				struct mxt_object *object_table,
				int max_objs)
{
	uint8_t object_table_index = 0;
	struct mxt_object obj;

	while (object_table_index < max_objs) {
		obj = object_table[object_table_index];
		if (obj.type == object_type)
			return obj.size;
		object_table_index++;
	}
	return 0;
}

/*
 * Reads one byte from given address from mXT chip (which requires
 * writing the 16-bit address pointer first).
 */

int mxt_read_byte(struct i2c_client *client, u16 addr, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16 le_addr = cpu_to_le16(addr);
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);


	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = 1;
	msg[1].buf   = (u8 *) value;
	if  (i2c_transfer(adapter, msg, 2) == 2) {
		mxt->last_read_addr = addr;
		return 0;
	} else {
		/*
		 * In case the transfer failed, set last read addr to invalid
		 * address, so that the next reads won't get confused.
		 */
		mxt->last_read_addr = -1;
		return -EIO;
	}
}

/*
 * Reads a block of bytes from given address from mXT chip. If we are
 * reading from message window, and previous read was from message window,
 * there's no need to write the address pointer: the mXT chip will
 * automatically set the address pointer back to message window start.
 */

static int mxt_read_block(struct i2c_client *client,
		   u16 addr,
		   u16 length,
		   u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16	le_addr;
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	if (mxt != NULL) {
		if ((mxt->last_read_addr == addr) &&
			(addr == mxt->msg_proc_addr)) {
			if  (i2c_master_recv(client, value, length) == length)
				return 0;
			else
				return -EIO;
		} else {
			mxt->last_read_addr = addr;
		}
	}

	le_addr = cpu_to_le16(addr);
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;
	if  (i2c_transfer(adapter, msg, 2) == 2)
		return 0;
	else
		return -EIO;

}

/* Reads a block of bytes from current address from mXT chip. */

static int mxt_read_block_wo_addr(struct i2c_client *client,
			   u16 length,
			   u8 *value)
{


	if  (i2c_master_recv(client, value, length) == length) {
		pr_info("read ok\n");
		return length;
	} else {
		pr_warning("read failed\n");
		return -EIO;
	}

}


/* Writes one byte to given address in mXT chip. */

int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value)
{
	struct {
		__le16 le_addr;
		u8 data;

	} i2c_byte_transfer;

	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	i2c_byte_transfer.le_addr = cpu_to_le16(addr);
	i2c_byte_transfer.data = value;


	if  (i2c_master_send(client, (u8 *) &i2c_byte_transfer, 3) == 3)
		return 0;
	else
		return -EIO;
}


/* Writes a block of bytes (max 256) to given address in mXT chip. */

int mxt_write_block(struct i2c_client *client,
		    u16 addr,
		    u16 length,
		    u8 *value)
{
	int i;
	struct {
		__le16	le_addr;
		u8	data[256];

	} i2c_block_transfer;

	struct mxt_data *mxt;

	if (length > 256)
		return -EINVAL;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;



	for (i = 0; i < length; i++)
		i2c_block_transfer.data[i] = *value++;


	i2c_block_transfer.le_addr = cpu_to_le16(addr);

	i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);

	if (i == (length + 2))
		return length;
	else
		return -EIO;
}

/* TODO: make all other access block until the read has been done? Otherwise
an arriving message for example could set the ap to message window, and then
the read would be done from wrong address! */

/* Writes the address pointer (to set up following reads). */

static int mxt_write_ap(struct i2c_client *client, u16 ap)
{

	__le16	le_ap = cpu_to_le16(ap);
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	pr_info("Address pointer set to %d\n", ap);

	if (i2c_master_send(client, (u8 *) &le_ap, 2) == 2)
		return 0;
	else
		return -EIO;
}

/* Calculates the CRC value for mXT infoblock. */
static int calculate_infoblock_crc(struct mxt_data *mxt, u32 *crc_result)
{
	u32 crc = 0;
	u16 crc_area_size;
	u8 *mem;
	int i;

	int error;
	struct i2c_client *client;

	client = mxt->client;

	crc_area_size = MXT_ID_BLOCK_SIZE +
		mxt->device_info.num_objs * MXT_OBJECT_TABLE_ELEMENT_SIZE;

	mem = kmalloc(crc_area_size, GFP_KERNEL);

	if (mem == NULL) {
		dev_err(&client->dev, "Error allocating memory\n");
		return -ENOMEM;
	}

	error = mxt_read_block(client, 0, crc_area_size, mem);
	if (error < 0) {
		kfree(mem);
		return error;
	}

	for (i = 0; i < (crc_area_size - 1); i = i + 2)
		crc = mxt_CRC_24(crc, *(mem + i), *(mem + i + 1));

	/* If uneven size, pad with zero */
	if (crc_area_size & 0x0001)
		crc = mxt_CRC_24(crc, *(mem + i), 0);

	kfree(mem);

	/* Return only 24 bits of CRC. */
	*crc_result = (crc & 0x00FFFFFF);
	return 1;

}

static void process_T9_message(struct mxt_data *mxt, u8 *message)
{
	struct	input_dev *input;
	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8 report_id;
	u8 touch_id;  /* to identify each touches. starts from 0 to 15*/
	u8 pressed_or_released = 0;
	static int prev_touch_id = -1;
	int i;

	input = mxt->input;
	status = message[MXT_MSG_T9_STATUS];
	report_id = message[0];
	touch_id = report_id - 2;

	if (touch_id >= MXT_MAX_NUM_TOUCHES) {
		pr_err("Invalid touch_id (toud_id=%d)", touch_id);
		return;
	}

	/* Put together the 10-/12-bit coordinate values. */
	xpos = message[MXT_MSG_T9_XPOSMSB] * 16 +
		((message[MXT_MSG_T9_XYPOSLSB] >> 4) & 0xF);
	ypos = message[MXT_MSG_T9_YPOSMSB] * 16 +
		((message[MXT_MSG_T9_XYPOSLSB] >> 0) & 0xF);

	if (mxt->pdata->max_x < 1024)
		xpos >>= 2;
	if (mxt->pdata->max_y < 1024)
		ypos >>= 2;

	/*touch_number = message[MXT_MSG_REPORTID]
	- mxt->rid_map[report_id].first_rid;*/
	if (status & MXT_MSGB_T9_DETECT) {  /* case 1: detected */
		 /* touch amplitude */
		mtouch_info[touch_id].pressure
					= message[MXT_MSG_T9_TCHAMPLITUDE];
		mtouch_info[touch_id].x = (int16_t)xpos;
		mtouch_info[touch_id].y = (int16_t)ypos;

		if (status & MXT_MSGB_T9_PRESS) {
			pressed_or_released = 1;  /* pressed */
#if defined(MXT_DRIVER_FILTER)
			equalize_coordinate(1, touch_id,
				&mtouch_info[touch_id].x,
				&mtouch_info[touch_id].y);
#endif
#if 0//def CONFIG_KERNEL_DEBUG_SEC
			pr_info("[TSP] P [F=%d, X=%d, Y=%d, S=%d]\n", touch_id,
				mtouch_info[touch_id].x,
				mtouch_info[touch_id].y,
				mtouch_info[touch_id].size);
//#else
			pr_info("[TSP] P [F=%d]\n", touch_id);
#endif
		} else if (status & MXT_MSGB_T9_MOVE) {
#if defined(MXT_DRIVER_FILTER)
			equalize_coordinate(0, touch_id,
				&mtouch_info[touch_id].x,
				&mtouch_info[touch_id].y);
#endif
		}
#if 0
		/* Amplitude of touch has changed */
		if (status & MXT_MSGB_T9_AMP)
			amplitude = message[MXT_MSG_T9_TCHAMPLITUDE];
#endif
	} else if (status & MXT_MSGB_T9_RELEASE) {  /* case 2: released */
		pressed_or_released = 1;
		mtouch_info[touch_id].pressure = 0;
#if 0//def CONFIG_KERNEL_DEBUG_SEC
		pr_info("[TSP] R [F=%d, X=%d, Y=%d, S=%d]\n", touch_id,
			mtouch_info[touch_id].x,
			mtouch_info[touch_id].y,
			mtouch_info[touch_id].size);
//#else
			pr_info("[TSP] R [F=%d]\n", touch_id);
#endif
	} else if (status & MXT_MSGB_T9_SUPPRESS) {  /* case 3: suppressed */
		/*
		 * Atmel's recommendation:
		 * In the case of supression,
		 * mxt1386 chip doesn't make a release event.
		 * So we need to release them forcibly.
		 */
		pressed_or_released = 1;
		mtouch_info[touch_id].pressure = 0;
	} else {
		pr_err("Unknown status (0x%x)", status);
	}

	/*only get size , id would use TRACKING_ID*/
	mtouch_info[touch_id].size = message[MXT_MSG_T9_TCHAREA];

	if (prev_touch_id >= touch_id || pressed_or_released) {
		for (i = 0; i < MXT_MAX_NUM_TOUCHES; ++i) {
			if (mtouch_info[i].pressure == -1)
				continue;

			/* ADD TRACKING_ID*/
			REPORT_MT(i,
				mtouch_info[i].x,	mtouch_info[i].y,
				mtouch_info[i].pressure, mtouch_info[i].size);
			/*input_sync(input);*/

			if (mtouch_info[i].pressure == 0)/* if released*/
				mtouch_info[i].pressure = -1;
		}
		input_sync(input);
	}
	prev_touch_id = touch_id;

	if (debug >= DEBUG_TRACE) {
		char msg[64] = {0};
		char info[64] = {0};

		if (status & MXT_MSGB_T9_SUPPRESS) {
			strcpy(msg, "Suppress: ");
		} else {
			if (status & MXT_MSGB_T9_DETECT) {
				strcpy(msg, "Detect(");
				if (status & MXT_MSGB_T9_PRESS)
					strcat(msg, "P");
				if (status & MXT_MSGB_T9_MOVE)
					strcat(msg, "M");
				if (status & MXT_MSGB_T9_AMP)
					strcat(msg, "A");
				if (status & MXT_MSGB_T9_VECTOR)
					strcat(msg, "V");
				strcat(msg, "): ");
			} else if (status & MXT_MSGB_T9_RELEASE) {
				strcpy(msg, "Release: ");
			} else {
				strcpy(msg, "[!] Unknown status: ");
			}
		}
		sprintf(info, "(%d,%d) amp=%d, size=%d", xpos, ypos,
			message[MXT_MSG_T9_TCHAMPLITUDE],
			message[MXT_MSG_T9_TCHAREA]);
		strcat(msg, info);
		pr_info("%s\n", msg);
	}

	return;
}

int process_message(struct mxt_data *mxt, u8 *message, u8 object)
{

	struct i2c_client *client;

	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8  event;
	u8  length;
	u8  report_id;

	client = mxt->client;
	length = mxt->message_size;
	report_id = message[0];

	switch (object) {
	case MXT_GEN_COMMANDPROCESSOR_T6:
		status = message[1];
		if (status & MXT_MSGB_T6_COMSERR) {
			dev_err(&client->dev,
				"maXTouch checksum error\n");
		}
		if (status & MXT_MSGB_T6_CFGERR) {
			dev_err(&client->dev,
				"maXTouch configuration error\n");
		}
		if (status & MXT_MSGB_T6_CAL) {
			dev_info(&client->dev,
				"maXTouch calibration in progress\n");
		}
		if (status & MXT_MSGB_T6_SIGERR) {
			dev_err(&client->dev,
				"maXTouch acquisition error\n");
#ifdef MXT_ERROR_WORKAROUND
			mxt_force_reset(mxt);
#endif
		}
		if (status & MXT_MSGB_T6_OFL) {
			dev_err(&client->dev,
				"maXTouch cycle overflow\n");
#ifdef MXT_ERROR_WORKAROUND
			/* soft reset */
			/*typical atmel spec. value is 250ms,
			but it sometimes fails to recover so it needs more*/
			reset_chip(mxt, RESET_TO_NORMAL);
			msleep(300);
#endif
		}
		if (status & MXT_MSGB_T6_RESET) {
			dev_info(&client->dev,
				"maXTouch chip reset\n");
			resume_flag = 0;
		} 

		if ((!(status & MXT_MSGB_T6_RESET) && status != 0)&& resume_flag ) {
			printk("[TSP] maXTouch chip reset is not work => mxt_force_reset \n");
			mxt_force_reset(mxt);		//HASH
		}
		if (status == 0) {
			dev_info(&client->dev,
				"maXTouch status normal\n");
#if defined(MXT_FACTORY_TEST)
				/*check if firmware started*/
				if (mxt->firm_status_data == 1) {
					dev_info(&client->dev,
						"maXTouch mxt->firm_normal_status_ack after firm up\n");
					/*got normal status ack*/
					mxt->firm_normal_status_ack = 1;
				}
#endif
		}
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		process_T9_message(mxt, message);
		break;

#if 0  /* note: deprecated in ver0.9*/
	case MXT_SPT_GPIOPWM_T19:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving GPIO message\n");
		break;


	case MXT_PROCI_GRIPFACESUPPRESSION_T20:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving face suppression msg\n");
		break;
#endif

	case MXT_PROCG_NOISESUPPRESSION_T22:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving noise suppression msg\n");
		status = message[MXT_MSG_T22_STATUS];
		if (status & MXT_MSGB_T22_FHCHG) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: Freq changed\n");
		}
		if (status & MXT_MSGB_T22_GCAFERR) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: High noise "
					"level\n");
		}
		if (status & MXT_MSGB_T22_FHERR) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: Freq changed - "
					"Noise level too high\n");

			mxt->pdata->fherr_cnt++;
			if (12 == mxt->pdata->fherr_cnt)
				mxt->pdata->fherr_cnt = 1;
			if (0 == (mxt->pdata->fherr_cnt%3)) {
				if (!work_pending(&mxt->ta_work))
					schedule_work(&mxt->ta_work);
			}

		}
		break;

	case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving one-touch gesture msg\n");

		event = message[MXT_MSG_T24_STATUS] & 0x0F;
		xpos = message[MXT_MSG_T24_XPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 4) & 0x0F);
		ypos = message[MXT_MSG_T24_YPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 0) & 0x0F);
		xpos >>= 2;
		ypos >>= 2;

		switch (event) {
		case	MT_GESTURE_RESERVED:
			break;
		case	MT_GESTURE_PRESS:
			break;
		case	MT_GESTURE_RELEASE:
			break;
		case	MT_GESTURE_TAP:
			break;
		case	MT_GESTURE_DOUBLE_TAP:
			break;
		case	MT_GESTURE_FLICK:
			break;
		case	MT_GESTURE_DRAG:
			break;
		case	MT_GESTURE_SHORT_PRESS:
			break;
		case	MT_GESTURE_LONG_PRESS:
			break;
		case	MT_GESTURE_REPEAT_PRESS:
			break;
		case	MT_GESTURE_TAP_AND_PRESS:
			break;
		case	MT_GESTURE_THROW:
			break;
		default:
			break;
		}
		break;

	case MXT_SPT_SELFTEST_T25:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving Self-Test msg\n");

		if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_OK) {
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
					"maXTouch: Self-Test OK\n");

		} else  {
			dev_err(&client->dev,
				"maXTouch: Self-Test Failed [%02x]:"
				"{%02x,%02x,%02x,%02x,%02x}\n",
				message[MXT_MSG_T25_STATUS],
				message[MXT_MSG_T25_STATUS + 0],
				message[MXT_MSG_T25_STATUS + 1],
				message[MXT_MSG_T25_STATUS + 2],
				message[MXT_MSG_T25_STATUS + 3],
				message[MXT_MSG_T25_STATUS + 4]
				);
		}
		break;

	case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving 2-touch gesture message\n");
		break;

	case MXT_SPT_CTECONFIG_T28:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"Receiving CTE message...\n");
		status = message[MXT_MSG_T28_STATUS];
		if (status & MXT_MSGB_T28_CHKERR)
			dev_err(&client->dev,
				"maXTouch: Power-Up CRC failure\n");

		break;
	default:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
				"maXTouch: Unknown message!\n");

		break;
	}
	return 0;
}

/* Processes messages when the interrupt line (CHG) is asserted. */
static void mxt_threaded_irq_handler(struct mxt_data *mxt)
{
	struct	i2c_client *client;

	/*note: changed message_length to 8 in ver0.9*/
	u8	message[MXT_MESSAGE_LENGTH];
	u16	message_length;
	u16	message_addr;
	u8	report_id;
	u8	object;
	int	error;
	int	i;

	client = mxt->client;
	message_addr = mxt->msg_proc_addr;
	message_length = mxt->message_size;

	if (debug >= DEBUG_TRACE)
		dev_info(&mxt->client->dev, "maXTouch worker active:\n");

	/* Read next message */
	mxt->message_counter++;
	mxt->read_fail_counter = 0;
	/* Reread on failure! */
	for (i = 1; i < I2C_RETRY_COUNT; i++) {
		/*note: changed message_length to 8 in ver0.9*/
		error = mxt_read_block(client,
			message_addr,
			MXT_MESSAGE_LENGTH,
			message);
		if (error >= 0)
			break;
		mxt->read_fail_counter++;
		pr_alert("mXT: message read failed!\n");
		/* Register read failed */
		dev_err(&client->dev,
			"Failure reading maxTouch device\n");
	}

#ifdef MXT_ERROR_WORKAROUND
	/*reset mxt touch ic if the i2c error occurs continuously*/
	if (mxt->read_fail_counter == I2C_RETRY_COUNT - 1) {
		mxt_force_reset(mxt);
		mxt->read_fail_counter = 0;
		return;
	}
#endif
	report_id = message[0];
	if (debug >= DEBUG_RAW) {
		pr_info("%s message [%08x]:",
		       REPORT_ID_TO_OBJECT_NAME(report_id),
		       mxt->message_counter
		);
		for (i = 0; i < message_length; i++) {
			pr_info("0x%02x ", message[i]);;
		}
		pr_info("\n");
	}

	if ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)) {

		for (i = 0; i < message_length; i++)
			mxt->last_message[i] = message[i];

		if (down_interruptible(&mxt->msg_sem)) {
			pr_warning("mxt_worker Interrupted "
				"while waiting for msg_sem!\n");
			return;
		}
		mxt->new_msgs = 1;
		up(&mxt->msg_sem);
		wake_up_interruptible(&mxt->msg_queue);
		/* Get type of object and process the message */
		object = mxt->rid_map[report_id].object;
		process_message(mxt, message, object);
	}
}

static irqreturn_t mxt_threaded_irq(int irq, void *_mxt)
{
	struct	mxt_data *mxt = _mxt;
	mxt->irq_counter++;
	mxt_threaded_irq_handler(mxt);
	return IRQ_HANDLED;
}

/* Function to write a block of data to any address on touch chip. */

#define I2C_PAYLOAD_SIZE 254

static ssize_t set_config(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	int i;

	u16 address;
	int whole_blocks;
	int last_block_size;

	struct i2c_client *client  = to_i2c_client(dev);

	address = *((u16 *) buf);
	address = cpu_to_be16(address);
	buf += 2;

	whole_blocks = (count - 2) / I2C_PAYLOAD_SIZE;
	last_block_size = (count - 2) % I2C_PAYLOAD_SIZE;

	for (i = 0; i < whole_blocks; i++) {
		mxt_write_block(client, address, I2C_PAYLOAD_SIZE, (u8 *) buf);
		address += I2C_PAYLOAD_SIZE;
		buf += I2C_PAYLOAD_SIZE;
	}

	mxt_write_block(client, address, last_block_size, (u8 *) buf);

	return count;

}

static ssize_t get_config(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	int i;
	struct i2c_client *client  = to_i2c_client(dev);
	struct mxt_data *mxt = i2c_get_clientdata(client);

	pr_warning("Reading %d bytes from current ap\n",
		mxt->bytes_to_read);
		
	if (0 == mxt->bytes_to_read)
		return 0;

	i = mxt_read_block_wo_addr(client, mxt->bytes_to_read, (u8 *) buf);

	return (ssize_t) i;

}

/*
 * Sets up a read from mXT chip. If we want to read config data from user space
 * we need to use this first to tell the address and byte count, then use
 * get_config to read the data.
 */

static ssize_t set_ap(struct device *dev,
		      struct device_attribute *attr,
		      const char *buf,
		      size_t count)
{

	int i;
	struct i2c_client *client;
	struct mxt_data *mxt;
	u16 ap;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	if (count < 3) {
		/* Error, ap needs to be two bytes, plus 1 for size! */
		pr_info("set_ap needs to arguments: address pointer "
		       "and data size");
		return -EIO;
	}

	ap = (u16) *((u16 *)buf);
	i = mxt_write_ap(client, ap);
	mxt->bytes_to_read = (u16) *(buf + 2);
	return count;

}


static ssize_t show_deltas(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	s16     *delta;
	s16     size, read_size;
	u16     diagnostics;
	u16     debug_diagnostics;
	char    *bufp;
	int     x, y;
	int     error;
	u16     *val;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	/* Allocate buffer for delta's */
	size = mxt->device_info.num_nodes * sizeof(__u16);
	if (mxt->delta == NULL) {
		mxt->delta = kzalloc(size, GFP_KERNEL);
		if (!mxt->delta) {
			sprintf(buf, "insufficient memory\n");
			return strlen(buf);
		}
	}

	if (mxt->object_table[MXT_GEN_COMMANDPROCESSOR_T6].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T6 not found\n");
		return 0;
	}
	diagnostics =  T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	if (mxt->object_table[MXT_DEBUG_DIAGNOSTICS_T37].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T37 not found\n");
		return 0;
	}
	debug_diagnostics = T37_REG(2);

	/* Configure T37 to show deltas */
	error = mxt_write_byte(client, diagnostics, MXT_CMD_T6_DELTAS_MODE);
	if (error)
		return error;

	delta = mxt->delta;

	while (size > 0) {
		read_size = size > 128 ? 128 : size;
		error = mxt_read_block(client,
				       debug_diagnostics,
				       read_size,
				       (__u8 *) delta);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"maXTouch: Error reading delta object\n");
		}
		delta += (read_size / 2);
		size -= read_size;
		/* Select next page */
		mxt_write_byte(client, diagnostics, MXT_CMD_T6_PAGE_UP);
	}

	bufp = buf;
	val  = (s16 *) mxt->delta;
	for (x = 0; x < mxt->device_info.x_size; x++) {
		for (y = 0; y < mxt->device_info.y_size; y++)
			bufp += sprintf(bufp, "%05d  ",
					(s16) le16_to_cpu(*val++));
		bufp -= 2;	/* No spaces at the end */
		bufp += sprintf(bufp, "\n");
	}
	bufp += sprintf(bufp, "\n");
	return strlen(buf);
}


static ssize_t show_references(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	s16   *reference;
	s16   size, read_size;
	u16   diagnostics;
	u16   debug_diagnostics;
	char  *bufp;
	int   x, y;
	int   error;
	u16   *val;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	/* Allocate buffer for reference's */
	size = mxt->device_info.num_nodes * sizeof(u16);
	if (mxt->reference == NULL) {
		mxt->reference = kzalloc(size, GFP_KERNEL);
		if (!mxt->reference) {
			sprintf(buf, "insufficient memory\n");
			return strlen(buf);
		}
	}

	if (mxt->object_table[MXT_GEN_COMMANDPROCESSOR_T6].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T6 not found\n");
		return 0;
	}
	diagnostics =  T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	if (mxt->object_table[MXT_DEBUG_DIAGNOSTICS_T37].type == 0) {
		dev_err(&client->dev, "maXTouch: Object T37 not found\n");
		return 0;
	}
	debug_diagnostics = T37_REG(2);

	/* Configure T37 to show references */
	mxt_write_byte(client, diagnostics, MXT_CMD_T6_REFERENCES_MODE);
	/* Should check for error */
	reference = mxt->reference;
	while (size > 0) {
		read_size = size > 128 ? 128 : size;
		error = mxt_read_block(client,
				       debug_diagnostics,
				       read_size,
				       (__u8 *) reference);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"maXTouch: Error reading reference object\n");
		}
		reference += (read_size / 2);
		size -= read_size;
		/* Select next page */
		mxt_write_byte(client, diagnostics, MXT_CMD_T6_PAGE_UP);
	}

	bufp = buf;
	val  = (u16 *) mxt->reference;

	for (x = 0; x < mxt->device_info.x_size; x++) {
		for (y = 0; y < mxt->device_info.y_size; y++)
			bufp += sprintf(bufp, "%05d  ", le16_to_cpu(*val++));
		bufp -= 2; /* No spaces at the end */
		bufp += sprintf(bufp, "\n");
	}
	bufp += sprintf(bufp, "\n");
	return strlen(buf);
}

static ssize_t show_device_info(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	char *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	bufp = buf;
	bufp += sprintf(bufp,
			"Family:\t\t\t[0x%02x] %s\n",
			mxt->device_info.family_id,
			mxt->device_info.family
			);
	bufp += sprintf(bufp,
			"Variant:\t\t[0x%02x] %s\n",
			mxt->device_info.variant_id,
			mxt->device_info.variant
			);
	bufp += sprintf(bufp,
			"Firmware version:\t[%d.%d], build 0x%02X\n",
			mxt->device_info.major,
			mxt->device_info.minor,
			mxt->device_info.build
			);
	bufp += sprintf(bufp,
			"%d Sensor nodes:\t[X=%d, Y=%d]\n",
			mxt->device_info.num_nodes,
			mxt->device_info.x_size,
			mxt->device_info.y_size
			);
	bufp += sprintf(bufp,
			"Reported resolution:\t[X=%d, Y=%d]\n",
			mxt->pdata->max_x+1,
			mxt->pdata->max_y+1
			);
	return strlen(buf);
}

static ssize_t show_stat(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	char *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	bufp = buf;
	bufp += sprintf(bufp,
			"Interrupts:\t[VALID=%d ; INVALID=%d]\n",
			mxt->valid_irq_counter,
			mxt->invalid_irq_counter
			);
	bufp += sprintf(bufp, "Messages:\t[%d]\n", mxt->message_counter);
	bufp += sprintf(bufp, "Read Failures:\t[%d]\n", mxt->read_fail_counter);
	return strlen(buf);
}

static ssize_t show_object_info(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client	*client;
	struct mxt_data		*mxt;
	char			*bufp;
	struct mxt_object	*object_table;
	int			i;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	object_table = mxt->object_table;

	bufp = buf;

	bufp += sprintf(bufp, "maXTouch: %d Objects\n",
			mxt->device_info.num_objs);

	for (i = 0; i < MXT_MAX_OBJECT_TYPES; i++) {
		if (object_table[i].type != 0) {
			bufp += sprintf(bufp,
					"Type:\t\t[%d]: %s\n",
					object_table[i].type,
					object_type_name[i]);
			bufp += sprintf(bufp,
					"Address:\t0x%04X\n",
					object_table[i].chip_addr);
			bufp += sprintf(bufp,
					"Size:\t\t%d Bytes\n",
					object_table[i].size);
			bufp += sprintf(bufp,
					"Instances:\t%d\n",
					object_table[i].instances
				);
			bufp += sprintf(bufp,
					"Report Id's:\t%d\n\n",
					object_table[i].num_report_ids);
		}
	}
	return strlen(buf);
}

static ssize_t show_messages(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	struct mxt_object *object_table;
	int   i;
	__u8  *message;
	__u16 message_len;
	__u16 message_addr;

	char  *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	object_table = mxt->object_table;

	bufp = buf;

	message = kmalloc(mxt->message_size, GFP_KERNEL);
	if (message == NULL) {
		pr_warning("Error allocating memory!\n");
		return -ENOMEM;
	}

	message_addr = mxt->msg_proc_addr;
	message_len = mxt->message_size;
	bufp += sprintf(bufp,
			"Reading Message Window [0x%04x]\n",
			message_addr);

	/* Acquire the lock. */
	if (down_interruptible(&mxt->msg_sem)) {
		pr_info("mxt: Interrupted while waiting for mutex!\n");
		kfree(message);
		return -ERESTARTSYS;
	}

	while (mxt->new_msgs == 0) {
		/* Release the lock. */
		up(&mxt->msg_sem);
		if (wait_event_interruptible(mxt->msg_queue, mxt->new_msgs)) {
			pr_info("mxt: Interrupted while waiting for new msg!\n");
			kfree(message);
			return -ERESTARTSYS;
		}

		/* Acquire the lock. */
		if (down_interruptible(&mxt->msg_sem)) {
			pr_info("mxt: Interrupted while waiting for mutex!\n");
			kfree(message);
			return -ERESTARTSYS;
		}

	}

	for (i = 0; i < mxt->message_size; i++)
		message[i] = mxt->last_message[i];

	mxt->new_msgs = 0;

	/* Release the lock. */
	up(&mxt->msg_sem);

	for (i = 0; i < message_len; i++)
		bufp += sprintf(bufp, "0x%02x ", message[i]);
	bufp--;
	bufp += sprintf(bufp, "\t%s\n", REPORT_ID_TO_OBJECT_NAME(message[0]));

	kfree(message);
	return strlen(buf);
}


static ssize_t show_report_id(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct i2c_client    *client;
	struct mxt_data      *mxt;
	struct report_id_map *report_id;
	int                  i;
	int                  object;
	char                 *bufp;

	client    = to_i2c_client(dev);
	mxt       = i2c_get_clientdata(client);
	report_id = mxt->rid_map;

	bufp = buf;
	for (i = 0 ; i < mxt->report_id_count ; i++) {
		object = report_id[i].object;
		bufp += sprintf(bufp, "Report Id [%03d], object [%03d], "
				"instance [%03d]:\t%s\n",
				i,
				object,
				report_id[i].instance,
				object_type_name[object]);
	}
	return strlen(buf);
}

static ssize_t set_debug(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	int state;

	sscanf(buf, "%d", &state);
	if (state == 0 || state == 1) {
		if (state) {
			debug = DEBUG_TRACE;
			pr_info("touch info enabled");
		} else {
			debug = DEBUG_INFO;
			pr_info("touch info disabled");
		}
	} else {
		return -EINVAL;
	}
	return count;
}

static ssize_t show_firmware_dev(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	u8 val[7];

	mxt_read_block(mxt->client, MXT_ADDR_INFO_BLOCK, 7, (u8 *)val);
	mxt->device_info.major = ((val[2] >> 4) & 0x0F);
	mxt->device_info.minor = (val[2] & 0x0F);
	mxt->device_info.build	= val[3];

	return snprintf(buf, PAGE_SIZE,
		"ATM_%d.%dx%d\n",
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build);
}

static ssize_t store_firmware(struct device *dev,
						struct device_attribute *attr,
						const char *buf,
						size_t count)
{
	int state;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (sscanf(buf, "%i", &state) != 1 || (state < 0 || state > 1))
		return -EINVAL;
	/*prevents the system from entering suspend during updating*/
	wake_lock(&mxt->wakelock);
	disable_irq(mxt->client->irq);

	mxt_load_firmware(dev, MXT1386_FIRMWARE);

	enable_irq(mxt->client->irq);
	wake_unlock(&mxt->wakelock);

	return count;
}

static ssize_t show_firmware_bin(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int ver[2];
	mxt_check_firmware(dev, ver);

	return snprintf(buf, PAGE_SIZE,
		"ATM_%d.%dx%d\n", ver[0]/16, ver[0]%16, ver[1]);
}

static int chk_obj(u8 type)
{
	switch (type) {
/*	case	MXT_GEN_MESSAGEPROCESSOR_T5:*/
/*	case	MXT_GEN_COMMANDPROCESSOR_T6:*/
	case	MXT_GEN_POWERCONFIG_T7:
	case	MXT_GEN_ACQUIRECONFIG_T8:
	case	MXT_TOUCH_MULTITOUCHSCREEN_T9:
/*	case	MXT_TOUCH_KEYARRAY_T15:*/
/*	case	MXT_SPT_COMMSCONFIG_T18:*/
	case	MXT_PROCG_NOISESUPPRESSION_T22:
/*	case	MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:*/
/*	case	MXT_SPT_SELFTEST_T25:*/
/*	case	MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:*/
	case	MXT_SPT_CTECONFIG_T28:
/*	case	MXT_DEBUG_DIAGNOSTICS_T37:*/
/*	case	MXT_USER_INFO_T38:*/
/*	case	MXT_GEN_EXTENSION_T39:*/
	case	MXT_PROCI_GRIPSUPPRESSION_T40:
	case	MXT_PROCI_PALMSUPPRESSION_T41:
/*	case	MXT_SPT_DIGITIZER_T43:*/
/*	case	MXT_MESSAGECOUNT_T44:*/
		return 0;
	default:
		return -1;
	}
}

static ssize_t show_object(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
/*	struct qt602240_data *data = dev_get_drvdata(dev);*/
/*	struct qt602240_object *object;*/
	struct mxt_data *mxt;
	struct mxt_object	*object_table;

	int count = 0;
	int i, j;
	u8 val;

	mxt = dev_get_drvdata(dev);
	object_table = mxt->object_table;

	for (i = 0; i < mxt->device_info.num_objs; i++) {
		u8 obj_type = object_table[i].type;

		if (chk_obj(obj_type))
			continue;

		count += sprintf(buf + count, "%s: %d bytes\n",
			object_type_name[obj_type], object_table[i].size);
		for (j = 0; j < object_table[i].size; j++) {
			mxt_read_byte(mxt->client,
				MXT_BASE_ADDR(obj_type)+(u16)j,
				&val);
			count += sprintf(buf + count,
				"  Byte %2d: 0x%02x (%d)\n",
				j, val, val);
		}

		count += sprintf(buf + count, "\n");
	}

#ifdef MXT_TUNNING_ENABLE
	backup_to_nv(mxt);
#endif

	return count;
}

static ssize_t store_object(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
/*	struct qt602240_data *data = dev_get_drvdata(dev);*/
/*	struct qt602240_object *object;*/
	struct mxt_data *mxt;
/*	struct mxt_object	*object_table;*/

	unsigned int type, offset, val;
	u16	chip_addr;
	int ret;

	mxt = dev_get_drvdata(dev);

	if ((sscanf(buf, "%u %u %u", &type, &offset, &val) != 3) ||
					(type >= MXT_MAX_OBJECT_TYPES)) {
		pr_err("Invalid values");
		return -EINVAL;
	}

	pr_info("Object type: %u, Offset: %u, Value: %u\n", type, offset, val);

	chip_addr = get_object_address(type, 0, mxt->object_table,
				mxt->device_info.num_objs);
	if (chip_addr == 0) {
		pr_err("Invalid object type(%d)!", type);
		return -EIO;
	}

	ret = mxt_write_byte(mxt->client, chip_addr+(u16)offset, (u8)val);
	if (ret < 0)
		return ret;

	return count;
}

/* Register sysfs files */
static DEVICE_ATTR(deltas,      S_IRUGO, show_deltas,      NULL);
static DEVICE_ATTR(references,  S_IRUGO, show_references,  NULL);
static DEVICE_ATTR(device_info, S_IRUGO, show_device_info, NULL);
static DEVICE_ATTR(object_info, S_IRUGO, show_object_info, NULL);
static DEVICE_ATTR(messages,    S_IRUGO, show_messages,    NULL);
static DEVICE_ATTR(report_id,   S_IRUGO, show_report_id,   NULL);
static DEVICE_ATTR(stat,        S_IRUGO, show_stat,        NULL);
static DEVICE_ATTR(config,      S_IWUSR|S_IRUGO, get_config, set_config);
static DEVICE_ATTR(ap,          S_IWUSR, NULL,             set_ap);
static DEVICE_ATTR(debug, S_IWUSR, NULL, set_debug);
static DEVICE_ATTR(fw_dev, S_IWUSR|S_IRUGO, show_firmware_dev, store_firmware);
static DEVICE_ATTR(fw_bin, S_IRUGO, show_firmware_bin, NULL);
static DEVICE_ATTR(object, S_IWUSR|S_IRUGO, show_object, store_object);

#ifdef ENABLE_NOISE_TEST_MODE
static void set_mxt_update_exe(struct work_struct *work)
{
	struct	mxt_data *mxt;
	int ret, cnt;;
	mxt = container_of(work, struct mxt_data, firmup_dwork.work);
	pr_info("set_mxt_update_exe\n");

	/*wake_lock(&mxt->wakelock); */
	disable_irq(mxt->client->irq); /*disable interrupt*/
	ret = mxt_load_firmware(&mxt->client->dev, MXT1386_FIRMWARE);
	enable_irq(mxt->client->irq);  /*enable interrupt*/
	/*wake_unlock(&mxt->wakelock);*/

	if (ret >= 0) {
		for (cnt = 10; cnt > 0; cnt--) {
			if (mxt->firm_normal_status_ack == 1) {
				/* firmware update success*/
				mxt->firm_status_data = 2;
				pr_info("[sec_touch]Reprogram done : Firmware update Success~~~~~~~~~~\n");
				break;
			} else {
				pr_info("[sec_touch]Reprogram done , but not yet normal status : 3s delay needed\n");
				msleep(3000);/*3s delay*/
			}
		}
		if (cnt == 0) {
			/* firmware update Fail */
			mxt->firm_status_data = 3;
			pr_info("[sec_touch]Reprogram done : Firmware update Fail ~~~~~~~~~~\n");
		}
	} else {
		/* firmware update Fail*/
		mxt->firm_status_data = 3;
		pr_info("[sec_touch]Reprogram done : Firmware update Fail~~~~~~~~~~\n");
	}
	mxt->firm_normal_status_ack = 0;
}
static ssize_t set_mxt_update_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{

	int count;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	pr_info("touch firmware update\n");
	/*start firmware updating*/
	mxt->firm_status_data = 1;

	cancel_delayed_work(&mxt->firmup_dwork);
	schedule_delayed_work(&mxt->firmup_dwork, 0);
	if (mxt->firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "OK\n");

	return count;
}
/*Current(Panel) Version*/
static ssize_t set_mxt_firm_version_read_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{

	struct mxt_data *mxt = dev_get_drvdata(dev);
	int error, cnt;
	u8 val[7];
	u8 fw_current_version;

	for (cnt = 10; cnt > 0; cnt--) {
		error = mxt_read_block(mxt->client, MXT_ADDR_INFO_BLOCK,
								7, (u8 *)val);
		if (error < 0) {
			pr_info("Atmel touch version read fail , it will try 2s later");
			msleep(2000);
		} else {
			break;
		}
	}
	if (cnt == 0) {
		pr_err("set_mxt_firm_version_show failed!!!");
		fw_current_version = 0;
	}
	mxt->device_info.major = ((val[2] >> 4) & 0x0F);
	mxt->device_info.minor = (val[2] & 0x0F);
	mxt->device_info.build	= val[3];
	fw_current_version = val[2];
	pr_info("Atmel %s Firmware version [%d.%d](%d) Build %d\n",
		mxt224_variant,
		mxt->device_info.major,
		mxt->device_info.minor,
		fw_current_version,
		mxt->device_info.build);
	return sprintf(buf, "%02d\n", fw_current_version);
}

/*Last(Phone) Version*/
static ssize_t set_mxt_firm_version_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	u8 fw_latest_version;
	fw_latest_version = firmware_latest[0];
	pr_info("Atmel Last firmware version is %d\n", fw_latest_version);
	return sprintf(buf, "%02d\n", fw_latest_version);
}
static ssize_t set_mxt_firm_status_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{

	int count;
	struct mxt_data *mxt = dev_get_drvdata(dev);
	pr_info("Enter firmware_status_show by Factory command\n");
	if (mxt->firm_status_data == 1)
		count = sprintf(buf, "Downloading\n");
	else if (mxt->firm_status_data == 2)
		count = sprintf(buf, "PASS\n");
	else if (mxt->firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "PASS\n");
	return count;

}

static ssize_t key_threshold_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	if (mxt->set_mode_for_ta)
		return sprintf(buf, "%d\n", mxt->pdata->tchthr_for_ta_connect);
	else
		return sprintf(buf, "%d\n", mxt->pdata->touchscreen_config.tchthr);
}

static ssize_t key_threshold_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t size)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	int i;
	if (sscanf(buf, "%d", &i) == 1) {
		/* prevents the system from entering suspend during updating*/
		wake_lock(&mxt->wakelock);
		disable_irq(mxt->client->irq);
		mxt->pdata->touchscreen_config.tchthr = i;/*basically,48*/
		mxt_multitouch_config(mxt);
		/* backup to nv memory */
		backup_to_nv(mxt);
		/* forces a reset of the chipset */
		reset_chip(mxt, RESET_TO_NORMAL);
		msleep(250);
		enable_irq(mxt->client->irq);
		wake_unlock(&mxt->wakelock);
		pr_info("[TSP] threshold is changed to %d\n", i);
	} else {
		pr_info("[sec_touch] threshold write error\n");
	}
	return size;
}

static uint8_t read_uint16_t(u16 Address, u16 *Data, struct mxt_data *mxt)
{
	uint8_t status;
	uint8_t temp[2];

	status = mxt_read_block(mxt->client, Address, 2, temp);
	*Data = ((uint16_t)temp[1]<<8) + (uint16_t)temp[0];

	return status;
}

static int  read_all_data(struct mxt_data *mxt)
{
	int status = -1;
	int try_cnt = 0;
	u8 mode = 0;
	u8 read_page = 0;
	u8 max_page = 0;
	u8 read_point = 0;
	u8 max_line = 0;
	u8 numofch = 0;
	u8 dbg_data[128];
	u16 max_val =0;
	u16 min_val = MIN_REF_VALUE;
	u16	diagnostics;
	u16 diagnostic_addr;
	u16 ref_val;

	diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);
	diagnostics = T6_REG(MXT_ADR_T6_DIAGNOSTICS);

	/*Page Num Clear*/
	mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_CTE_MODE);
	msleep(20);
	mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_REFERENCES_MODE);
	msleep(20);

	/* check the mode */
	do {
		mxt_read_byte(mxt->client, diagnostic_addr, &mode);
		if (MXT_CMD_T6_REFERENCES_MODE == mode )
			break;
		try_cnt++;
		msleep(20);
	}while (try_cnt < 5);

	max_page = MAX_CHANNEL / 64;
	max_line = (mxt->pdata->touchscreen_config.xsize * 14) >> 6;

	do {
		if ((read_page%8)<max_line)
			numofch = 64 * 2;
		else if ((read_page%8) == max_line)
			numofch = ((mxt->pdata->touchscreen_config.xsize * 14)
				- (max_line * 64)) * 2;
		else {
			read_page++;
			mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_PAGE_UP);
			msleep(20);
			continue;
		}

		mxt_read_byte(mxt->client, diagnostic_addr+1, &mode);
		if (mode == read_page) {
			mxt_read_block(mxt->client, diagnostic_addr+2, numofch, dbg_data);
			for (read_point = 0; read_point < numofch; read_point += 2) {
				ref_val = (u16)dbg_data[read_point] |(u16)dbg_data[read_point+1] <<8;
				if ((ref_val > MAX_REF_VALUE) ||(ref_val < MIN_REF_VALUE)) {
					//pr_info("[TSP] page : %u, node : %u, ref : %u\n",
						//mode, read_point, ref_val);
					status = 1;
					goto eof;
				}
				if (ref_val > max_val)
					max_val = ref_val;
				else if (ref_val < min_val)
					min_val = ref_val;
			}

			if (read_page == max_page) {
				status = 0;
				goto eof;
			}

			read_page++;
			mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_PAGE_UP);
			msleep(20);

		} else {
			//pr_info("[TSP] page : %u, try_cnt : %d\n", read_page, try_cnt);
			try_cnt++;
			msleep(50);
		}
	}while (try_cnt<10);
eof:
	pr_info("[TSP] diff %d\n", max_val - min_val);
	pr_info("[TSP] diff %d\n", status);

	return status;
}

static ssize_t all_refer_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	int status = 0;
	int try_cnt = 0;

	do {
		status = read_all_data(mxt);
		if (-1 != status)
			break;
		try_cnt++;
	} while (try_cnt < 5);

	if (-1 == status)
		status = 1;

	return sprintf(buf, "%u\n", status);
}

static int  find_channel(struct mxt_data *mxt)
{
	int status = 0;
	int try_cnt = 0;
	u8 mode = 0;
	u8 read_page = 0;
	u8 max_page = 0;
	u8 read_point = 0;
	u8 max_line = 0;
	u8 numofch = 0;
	u8 dbg_data[128];
	u16 max_val =0;
	u16	diagnostics;
	u16 diagnostic_addr;
	u16 delta_val;

	diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);
	diagnostics = T6_REG(MXT_ADR_T6_DIAGNOSTICS);

	/*Page Num Clear*/
	mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_CTE_MODE);
	msleep(10);
	mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_DELTAS_MODE);
	msleep(10);

	/* check the mode */
	do {
		mxt_read_byte(mxt->client, diagnostic_addr, &mode);
		if (MXT_CMD_T6_DELTAS_MODE == mode )
			break;
		try_cnt++;
		msleep(10);
	}while (try_cnt < 5);

	max_page = MAX_CHANNEL / 64;
	max_line = (mxt->pdata->touchscreen_config.xsize * 14) >> 6;

	do {
		if ((read_page%8)<max_line)
			numofch = 64 * 2;
		else if ((read_page%8) == max_line)
			numofch = ((mxt->pdata->touchscreen_config.xsize * 14)
				- (max_line * 64)) * 2;
		else {
			read_page++;
			mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_PAGE_UP);
			msleep(10);
			continue;
		}

		mxt_read_byte(mxt->client, diagnostic_addr+1, &mode);
		if (mode == read_page) {
			mxt_read_block(mxt->client, diagnostic_addr+2, numofch, dbg_data);
			for (read_point = 0; read_point < numofch; read_point += 2) {
				delta_val = (u16)dbg_data[read_point] |(u16)dbg_data[read_point+1] <<8;
				if (delta_val > 32767)
					delta_val = 65535 - delta_val;
				if (delta_val > max_val) {
					max_val = delta_val;
					status = (read_point -2)/2 + (read_page *64);
				}
			}

			if (read_page == max_page)
				break;

			read_page++;
			mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_PAGE_UP);
			msleep(10);

		} else {
			//pr_info("[TSP] page : %u, try_cnt : %d\n", read_page, try_cnt);
			try_cnt++;
			msleep(10);
		}
	}while (try_cnt<10);

	return status;
}

static ssize_t find_channel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int status = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = find_channel(mxt);

	return sprintf(buf, "%u\n", status);
}

static int  read_dbg_data(u8 dbg_mode , u16 node, u16 *dbg_data, struct mxt_data *mxt)
{
	int status = 0;
	int try_cnt = 0;
	u8 mode = 0;
	u8 page = 0;
	u8 read_page =0;
	u16 read_point;
	u16	diagnostics;
	u16 diagnostic_addr;

	diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);
	diagnostics = T6_REG(MXT_ADR_T6_DIAGNOSTICS);

	read_page = node / 64;
	node %= 64;
	read_point = (node * 2) + 2;

	/*Page Num Clear*/
	mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_CTE_MODE);
	msleep(10);
	mxt_write_byte(mxt->client, diagnostics, dbg_mode);
	msleep(10);

	do {
		mxt_read_byte(mxt->client, diagnostic_addr, &mode);
		if (mode == dbg_mode)
			break;
		msleep(10);
	} while (try_cnt < 5);

	do {
		if (page == read_page)
			break;

		mxt_read_byte(mxt->client, diagnostic_addr+1, &mode);

		if (mode == page)
			page++;
		else {
			//pr_info("[TSP] try_cnt : %d\n", try_cnt);
			try_cnt++;
			msleep(10);
			continue;
		}

		mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_PAGE_UP);
		msleep(10);

	} while (try_cnt < 10);
	status = read_uint16_t(diagnostic_addr + read_point, dbg_data, mxt);

	return status;
}

static ssize_t set_refer0_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
		test_node[0], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer1_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
		test_node[1], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer2_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
		test_node[2], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}


static ssize_t set_refer3_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
		test_node[3], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}


static ssize_t set_refer4_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
		test_node[4], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_delta0_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE,
		test_node[0], &qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta1_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE,
		test_node[1], &qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta2_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE,
		test_node[2], &qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta3_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE,
		test_node[3], &qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta4_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE,
		test_node[4], &qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t show_threshold(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	u8 val;

	mxt_read_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_TCHTHR, &val);
	return sprintf(buf,"%d\n", val);
}

static ssize_t set_suppression_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	u8 val;

	mxt_read_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCI_PALMSUPPRESSION_T41),
			&val);
	return sprintf(buf,"%d\n", val);
}

static ssize_t set_suppression_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t size)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	int i;
	if (sscanf(buf, "%d", &i) == 1) {
		if (0x81 == i) {
			mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCI_PALMSUPPRESSION_T41),
				0x0);
			pr_info("[TSP] the palm suppression field is off\n");
		} else {
			mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCI_PALMSUPPRESSION_T41),
				mxt->pdata->palmsupression_config.ctrl);
			pr_info("[TSP] the palm suppression field is on\n");

}
	} else {
		pr_info("[TSP] sysfs write error\n");
	}
	return size;
}

/* noise test file */
static DEVICE_ATTR(tsp_firm_update, S_IRUGO, set_mxt_update_show, NULL);
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO,	set_mxt_firm_status_show, NULL);
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR, key_threshold_show, key_threshold_store);
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO,	set_mxt_firm_version_show, NULL);
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO, set_mxt_firm_version_read_show, NULL);
static DEVICE_ATTR(set_all_refer, S_IRUGO, all_refer_show, NULL);
static DEVICE_ATTR(set_refer0, S_IRUGO, set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO, set_delta0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO, set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO, set_delta1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO, set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO, set_delta2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO, set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO, set_delta3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO, set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO, set_delta4_mode_show, NULL);
static DEVICE_ATTR(set_threshould, S_IRUGO, show_threshold, NULL);
static DEVICE_ATTR(delta_channel, S_IRUGO, find_channel_show, NULL);
static DEVICE_ATTR(set_suppression,  S_IRUGO | S_IWUSR, set_suppression_show, set_suppression_store);
#endif

static struct attribute *maxTouch_attributes[] = {
	&dev_attr_deltas.attr,
	&dev_attr_references.attr,
	&dev_attr_device_info.attr,
	&dev_attr_object_info.attr,
	&dev_attr_messages.attr,
	&dev_attr_report_id.attr,
	&dev_attr_stat.attr,
	&dev_attr_config.attr,
	&dev_attr_ap.attr,
	&dev_attr_debug.attr,
	&dev_attr_fw_dev.attr,
	&dev_attr_fw_bin.attr,
	&dev_attr_object.attr,
	NULL,
};

static struct attribute_group maxtouch_attr_group = {
	.attrs = maxTouch_attributes,
};

#ifdef ENABLE_NOISE_TEST_MODE
static struct attribute *maxTouch_facotry_attributes[] = {
	&dev_attr_tsp_firm_update.attr,
	&dev_attr_tsp_firm_update_status.attr,
	&dev_attr_tsp_threshold.attr,
	&dev_attr_tsp_firm_version_phone.attr,
	&dev_attr_tsp_firm_version_panel.attr,
	&dev_attr_set_all_refer.attr,
	&dev_attr_set_refer0.attr,
	&dev_attr_set_delta0.attr,
	&dev_attr_set_refer1.attr,
	&dev_attr_set_delta1.attr,
	&dev_attr_set_refer2.attr,
	&dev_attr_set_delta2.attr,
	&dev_attr_set_refer3.attr,
	&dev_attr_set_delta3.attr,
	&dev_attr_set_refer4.attr,
	&dev_attr_set_delta4.attr,
	&dev_attr_set_threshould.attr,
	&dev_attr_delta_channel.attr,
	&dev_attr_set_suppression.attr,
	NULL,
};

static struct attribute_group maxtouch_factory_attr_group = {
	.attrs = maxTouch_facotry_attributes,
};
#endif

/******************************************************************************/
/* Initialization of driver                                                   */
/******************************************************************************/

static int __devinit mxt_identify(struct i2c_client *client,
				  struct mxt_data *mxt)
{
	u8 buf[7];
	int error;
	int identified;

	identified = 0;

retry_i2c:
	/* Read Device info to check if chip is valid */
	error = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, 7, (u8 *) buf);

	if (error < 0) {
		mxt->read_fail_counter++;
	if (mxt->read_fail_counter == 1) {
		pr_info("Warning: To wake up touch-ic in deep sleep, retry i2c communication!");
		msleep(30);
		goto retry_i2c;
	}
		dev_err(&client->dev, "Failure accessing maXTouch device\n");
		return -EIO;
	}

	mxt->device_info.family_id  = buf[0];
	mxt->device_info.variant_id = buf[1];
	mxt->device_info.major	    = ((buf[2] >> 4) & 0x0F);
	mxt->device_info.minor      = (buf[2] & 0x0F);
	mxt->device_info.build	    = buf[3];
	mxt->device_info.x_size	    = buf[4];
	mxt->device_info.y_size	    = buf[5];
	mxt->device_info.num_objs   = buf[6];
	mxt->device_info.num_nodes  = mxt->device_info.x_size *
				      mxt->device_info.y_size;

	/* Check Family Info */
	if (mxt->device_info.family_id == MAXTOUCH_FAMILYID) {
		strcpy(mxt->device_info.family, maxtouch_family);
	} else {
		dev_err(&client->dev,
			"maXTouch Family ID [0x%x] not supported\n",
			mxt->device_info.family_id);
		identified = -ENXIO;
	}

	/* Check Variant Info */
	if ((mxt->device_info.variant_id == MXT224_CAL_VARIANTID) ||
	    (mxt->device_info.variant_id == MXT224_UNCAL_VARIANTID)) {
		strcpy(mxt->device_info.variant, mxt224_variant);
	} else {
		dev_err(&client->dev,
			"maXTouch Variant ID [0x%x] not supported\n",
			mxt->device_info.variant_id);
		identified = -ENXIO;
	}

	dev_info(
		&client->dev,
		"Atmel %s.%s Firmware version [%d.%d] Build %d\n",
		mxt->device_info.family,
		mxt->device_info.variant,
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build
	);
	dev_info(
		&client->dev,
		"Atmel %s.%s Configuration [X: %d] x [Y: %d]\n",
		mxt->device_info.family,
		mxt->device_info.variant,
		mxt->device_info.x_size,
		mxt->device_info.y_size
	);
	dev_info(
		&client->dev,
		"number of objects: %d\n",
		mxt->device_info.num_objs
	);

	return identified;
}

/*
 * Reads the object table from maXTouch chip to get object data like
 * address, size, report id.
 */
static int __devinit mxt_read_object_table(struct i2c_client *client,
					   struct mxt_data *mxt)
{
	u16	report_id_count;
	u8	buf[MXT_OBJECT_TABLE_ELEMENT_SIZE];
	u8	object_type;
	u16	object_address;
	u16	object_size;
	u8	object_instances;
	u8	object_report_ids;
	u16	object_info_address;
	u32	crc;
	u32     crc_calculated;
	int	i;
	int	error;

	u8	object_instance;
	u8	object_report_id;
	u8	report_id;
	int     first_report_id;

	struct mxt_object *object_table;

	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver get configuration\n");

	object_table = kzalloc(sizeof(struct mxt_object) *
			       mxt->device_info.num_objs,
			       GFP_KERNEL);
	if (object_table == NULL) {
		pr_warning("maXTouch: Memory allocation failed!\n");
		return -ENOMEM;
	}

	mxt->object_table = object_table;

	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver Memory allocated\n");

	object_info_address = MXT_ADDR_OBJECT_TABLE;

	report_id_count = 0;
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		if (debug >= DEBUG_TRACE)
			pr_info("Reading maXTouch at [0x%04x]: ",
			       object_info_address);
		error = mxt_read_block(client, object_info_address,
				MXT_OBJECT_TABLE_ELEMENT_SIZE, (u8 *)buf);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"maXTouch Object %d could not be read\n", i);
			return -EIO;
		}
		object_type		=  buf[0];
		object_address		= (buf[2] << 8) + buf[1];
		object_size		=  buf[3] + 1;
		object_instances	=  buf[4] + 1;
		object_report_ids	=  buf[5];
		if (debug >= DEBUG_TRACE)
			pr_info("Type=%03d, Address=0x%04x, "
			       "Size=0x%02x, %d instances, %d report id's\n",
			       object_type,
			       object_address,
			       object_size,
			       object_instances,
			       object_report_ids
		);

		if (object_type > MXT_MAX_OBJECT_TYPES) {
			/* Unknown object type */
			dev_err(&client->dev,
				"maXTouch object type [%d] not recognized\n",
				object_type);
			return -ENXIO;

		}

		/* Save frequently needed info. */
		if (object_type == MXT_GEN_MESSAGEPROCESSOR_T5) {
			mxt->msg_proc_addr = object_address;
			/*mxt->message_size = object_size;*/
			/*note: changed message_length to 8 in ver0.9*/
			mxt->message_size = MXT_MESSAGE_LENGTH;
		}

		object_table[i].type            = object_type;
		object_table[i].chip_addr       = object_address;
		object_table[i].size            = object_size;
		object_table[i].instances       = object_instances;
		object_table[i].num_report_ids  = object_report_ids;
		report_id_count += object_instances * object_report_ids;

		object_info_address += MXT_OBJECT_TABLE_ELEMENT_SIZE;
	}

	mxt->rid_map =
		kzalloc(sizeof(struct report_id_map) * (report_id_count + 1),
			/* allocate for report_id 0, even if not used */
			GFP_KERNEL);
	if (mxt->rid_map == NULL) {
		pr_warning("maXTouch: Can't allocate memory!\n");
		return -ENOMEM;
	}

	mxt->last_message = kzalloc(mxt->message_size, GFP_KERNEL);
	if (mxt->last_message == NULL) {
		pr_warning("maXTouch: Can't allocate memory!\n");
		return -ENOMEM;
	}


	mxt->report_id_count = report_id_count;
	if (report_id_count > 254) {	/* 0 & 255 are reserved */
			dev_err(&client->dev,
				"Too many maXTouch report id's [%d]\n",
				report_id_count);
			return -ENXIO;
	}

	/* Create a mapping from report id to object type */
	report_id = 1; /* Start from 1, 0 is reserved. */

	/* Create table associating report id's with objects & instances */
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		for (object_instance = 0;
		     object_instance < object_table[i].instances;
		     object_instance++){
			first_report_id = report_id;
			for (object_report_id = 0;
			     object_report_id < object_table[i].num_report_ids;
			     object_report_id++) {
				mxt->rid_map[report_id].object =
					object_table[i].type;
				mxt->rid_map[report_id].instance =
					object_instance;
				mxt->rid_map[report_id].first_rid =
					first_report_id;
				report_id++;
			}
		}
	}

	/* Read 3 byte CRC */
	error = mxt_read_block(client, object_info_address, 3, buf);
	if (error < 0) {
		mxt->read_fail_counter++;
		dev_err(&client->dev, "Error reading CRC\n");
	}

	crc = (buf[2] << 16) | (buf[1] << 8) | buf[0];

	if (calculate_infoblock_crc(mxt, &crc_calculated) < 0)
		pr_warning("Error while calculating CRC!\n");

	if (debug >= DEBUG_TRACE) {
		pr_info("Reported info block CRC = 0x%6X\n\n", crc);
		pr_info("Calculated info block CRC = 0x%6X\n\n",
		       crc_calculated);
	}

	if (crc == crc_calculated) {
		mxt->info_block_crc = crc;
	} else {
		mxt->info_block_crc = 0;
		pr_warning("maXTouch: info block CRC invalid!\n");
	}


	mxt->delta	= NULL;
	mxt->reference	= NULL;
	mxt->cte	= NULL;

	if (debug >= DEBUG_VERBOSE) {

		dev_info(&client->dev, "maXTouch: %d Objects\n",
				mxt->device_info.num_objs);

		for (i = 0; i < mxt->device_info.num_objs; i++) {
			dev_info(&client->dev, "Type:\t\t\t[%d]: %s\n",
				 object_table[i].type,
				 object_type_name[object_table[i].type]);
			dev_info(&client->dev, "\tAddress:\t0x%04X\n",
				 object_table[i].chip_addr);
			dev_info(&client->dev, "\tSize:\t\t%d Bytes\n",
				 object_table[i].size);
			dev_info(&client->dev, "\tInstances:\t%d\n",
				 object_table[i].instances);
			dev_info(&client->dev, "\tReport Id's:\t%d\n",
				 object_table[i].num_report_ids);
		}
	}
	return 0;
}

u8 mxt_valid_interrupt(void)
{
	return 1;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *h)
{
#ifndef MXT_SLEEP_POWEROFF
	u8 cmd_sleep[2] = {0};
	u16 addr;
#endif
	struct	mxt_data *mxt = container_of(h, struct mxt_data, early_suspend);

	pr_info("%s has been called!\n", __func__);
#if defined(MXT_FACTORY_TEST)
	/*start firmware updating : not yet finished*/
	while (mxt->firm_status_data == 1) {
		pr_info("mxt firmware is Downloading : mxt suspend must be delayed!");
		msleep(1000);
	}
#endif
	disable_irq(mxt->client->irq);
	mxt->enabled = false;

	resume_flag = 1;
	
	/* cancel and wait for all works to stop so they don't try to
	 * communicate with the controller after we turn it off
	 */
#ifdef MXT_CALIBRATE_WORKAROUND
	cancel_delayed_work_sync(&mxt->calibrate_dwork);
#endif
	cancel_work_sync(&mxt->ta_work);
#ifdef MXT_SLEEP_POWEROFF
	if (mxt->pdata->suspend_platform_hw != NULL)
		mxt->pdata->suspend_platform_hw();
#else
	/*
	  * a setting of zeros to IDLEACQINT and ACTVACQINT
	  * forces the chip set to enter Deep Sleep mode.
	  */
	addr = get_object_address(MXT_GEN_POWERCONFIG_T7, 0,
			mxt->object_table, mxt->device_info.num_objs);
	pr_info("addr: 0x%02x, buf[0]=0x%x, buf[1]=0x%x",
				addr, cmd_sleep[0], cmd_sleep[1]);
	mxt_write_block(mxt->client, addr, 2, (u8 *)cmd_sleep);
#endif
	mxt_forced_release(mxt);
}

static void mxt_late_resume(struct early_suspend *h)
{
#ifndef MXT_SLEEP_POWEROFF
	int cnt;
#endif
	struct	mxt_data *mxt = container_of(h, struct mxt_data, early_suspend);

	pr_info("%s has been called!\n", __func__);
#ifdef MXT_SLEEP_POWEROFF
	if (mxt->pdata->resume_platform_hw != NULL)
		mxt->pdata->resume_platform_hw();
#else
	for (cnt = 10; cnt > 0; cnt--) {
		if (mxt_power_config(mxt) < 0)
			continue;
		if (reset_chip(mxt, RESET_TO_NORMAL) == 0)/* soft reset*/
			break;
	}
	if (cnt == 0)
		pr_err("%s : reset_chip failed!!!\n", __func__);
	/*typical atmel spec. value is 250ms,
	but it sometimes fails to recover so it needs more*/
	msleep(300);
#endif
	resume_flag = 1;
	if (mxt->set_mode_for_ta && !work_pending(&mxt->ta_work))
		schedule_work(&mxt->ta_work);
	mxt->enabled = true;
	mxt->pdata->fherr_cnt = 0;
	enable_irq(mxt->client->irq);
#ifdef MXT_CALIBRATE_WORKAROUND
	schedule_delayed_work(&mxt->calibrate_dwork, msecs_to_jiffies(4000));
#endif
}
#endif


static int __devinit mxt_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct mxt_data          *mxt;
	struct mxt_platform_data *pdata;
	struct input_dev         *input;
	struct device *test_dev;
	struct device *tsp_dev;
	int error;
	int i;

	pr_info("mXT1386: mxt_probe\n");

	if (client == NULL) {
		pr_err("maXTouch: client == NULL\n");
		return -EINVAL;
	} else if (client->adapter == NULL) {
		pr_err("maXTouch: client->adapter == NULL\n");
		return -EINVAL;
	} else if (&client->dev == NULL) {
		pr_err("maXTouch: client->dev == NULL\n");
		return -EINVAL;
	} else if (&client->adapter->dev == NULL) {
		pr_err("maXTouch: client->adapter->dev == NULL\n");
		return -EINVAL;
	} else if (id == NULL) {
		pr_err("maXTouch: id == NULL\n");
		return	-EINVAL;
	}
	error = 0xff;
	if (debug >= DEBUG_INFO) {
		pr_info("maXTouch driver\n");
		pr_info("\t \"%s\"\n",		client->name);
		pr_info("\taddr:\t0x%04x\n",	client->addr);
		pr_info("\tirq:\t%d\n",	client->irq);
		pr_info("\tflags:\t0x%04x\n",	client->flags);
		pr_info("\tadapter:\"%s\"\n",	client->adapter->name);
		pr_info("\tdevice:\t\"%s\"\n",	client->dev.init_name);
	}
	if (debug >= DEBUG_TRACE)
		pr_info("Parameters OK\n");;
	/* Allocate structure - we need it to identify device */
	mxt = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	if (!mxt) {
		dev_err(&client->dev, "insufficient memory\n");
		error = -ENOMEM;
		goto err_mxt_alloc;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&client->dev, "error allocating input device\n");
		error = -ENOMEM;
		goto err_input_dev_alloc;
	}

	/* Initialize Platform data */
	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		dev_err(&client->dev, "platform data is required!\n");
		goto err_pdata;
	}
	if (debug >= DEBUG_TRACE)
		pr_info("Platform OK: pdata = %p\n", pdata);

	mxt->pdata = pdata;
	mxt->client = client;
	mxt->input  = input;
#if defined(MXT_FACTORY_TEST)
	mxt->firm_status_data = 0;
	mxt->firm_normal_status_ack = 0;
#endif
	mxt->read_fail_counter = 0;
	mxt->message_counter   = 0;

	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver identifying chip\n");


	if (mxt_identify(client, mxt) < 0) {
		dev_err(&client->dev, "Chip could not be identified\n");
		goto err_identify;
	}
	/* Chip is valid and active. */
	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver allocating input device\n");

#ifdef MXT_CALIBRATE_WORKAROUND
	INIT_DELAYED_WORK(&mxt->calibrate_dwork, mxt_calibrate_worker);
#endif
	INIT_WORK(&mxt->ta_work, mxt_ta_worker);
#ifdef MXT_FACTORY_TEST
	INIT_DELAYED_WORK(&mxt->firmup_dwork, set_mxt_update_exe);
#endif

	/* Register callbacks */
	/* To inform tsp , charger connection status*/
	mxt->callbacks.inform_charger = mxt_inform_charger_connection;
	if (mxt->pdata->register_cb)
		mxt->pdata->register_cb(&mxt->callbacks);

	init_waitqueue_head(&mxt->msg_queue);
	init_MUTEX(&mxt->msg_sem);

	spin_lock_init(&mxt->lock);


	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver creating device name\n");

	snprintf(
		mxt->phys_name,
		sizeof(mxt->phys_name),
		"%s/input0",
		dev_name(&client->dev)
	);
#ifdef CONFIG_SAMSUNG_INPUT
	input->name = "sec_touchscreen";
#else
	input->name = "Atmel_maXTouch_Touchscreen_controller";
#endif
	input->phys = mxt->phys_name;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	if (debug >= DEBUG_INFO) {
		pr_info("maXTouch name: \"%s\"\n", input->name);
		pr_info("maXTouch phys: \"%s\"\n", input->phys);
		pr_info("maXTouch driver setting abs parameters\n");
	}
	__set_bit(BTN_TOUCH, input->keybit);
	__set_bit(EV_ABS, input->evbit);

	/* multi touch */
	input_set_abs_params(input, ABS_MT_POSITION_X, 0,
						mxt->pdata->max_x, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0,
						mxt->pdata->max_y, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0,
							MXT_MAX_NUM_TOUCHES-1,
							0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 30, 0, 0);
	__set_bit(EV_SYN, input->evbit);
	__set_bit(EV_KEY, input->evbit);

	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver setting client data\n");

	i2c_set_clientdata(client, mxt);
	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver setting drv data\n");

	input_set_drvdata(input, mxt);

	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver input register device\n");

	error = input_register_device(mxt->input);
	if (error < 0) {
		dev_err(&client->dev,
			"Failed to register input device\n");
		goto err_register_device;
	}
	if (debug >= DEBUG_TRACE)
		pr_info("maXTouch driver allocate interrupt\n");

	error = mxt_read_object_table(client, mxt);
	if (error < 0)
		goto err_read_ot;

#ifndef MXT_TUNNING_ENABLE
	/* pre-set configuration before soft reset */
	error = mxt_config_settings(mxt);
	if (error < 0)
		goto err_after_read_ot;
#endif
	for (i = 0; i < MXT_MAX_NUM_TOUCHES ; i++)
		mtouch_info[i].pressure = -1;

	/* Allocate the interrupt */
	mxt->irq = client->irq;
	mxt->valid_irq_counter = 0;
	mxt->invalid_irq_counter = 0;
	mxt->irq_counter = 0;
	if (mxt->irq) {
		/* Try to request IRQ with falling edge first. This is
		 * not always supported. If it fails, try with any edge. */

		error = request_threaded_irq(mxt->irq,
				NULL,
				mxt_threaded_irq,
				IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				client->dev.driver->name,
				mxt);
		if (error < 0) {
			error = request_threaded_irq(mxt->irq,
					NULL,
					mxt_threaded_irq,
					IRQF_DISABLED,
					client->dev.driver->name,
					mxt);
		}

		if (error < 0) {
			dev_err(&client->dev,
				"failed to allocate irq %d\n", mxt->irq);
			goto err_irq;
		}
	}

	if (debug > DEBUG_INFO)
		dev_info(&client->dev, "touchscreen, irq %d\n", mxt->irq);

#ifdef CONFIG_HAS_EARLYSUSPEND
	mxt->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	mxt->early_suspend.suspend = mxt_early_suspend;
	mxt->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&mxt->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	tsp_dev  = device_create(sec_class, NULL, 0, mxt, "sec_touch");
	if (IS_ERR(tsp_dev)) {
		pr_err("Failed to create device for the sysfs\n");
		error = -ENODEV;
		goto err_sysfs_create_group;
	}

	error = sysfs_create_group(&tsp_dev->kobj, &maxtouch_attr_group);
	if (error) {
		pr_err("Failed to create sysfs group\n");
		goto err_sysfs_create_group;
	}

#ifdef ENABLE_NOISE_TEST_MODE
	test_dev = device_create(sec_class, NULL, 0, mxt, "qt602240_noise_test");
	if (IS_ERR(test_dev)) {
		pr_err("Failed to create device for the factory test\n");
		error = -ENODEV;
		goto err_sysfs_create_group;
	}

	error = sysfs_create_group(&test_dev->kobj, &maxtouch_factory_attr_group);
	if (error) {
		pr_err("Failed to create sysfs group for the factory test\n");
		goto err_sysfs_create_group;
	}
#endif

	wake_lock_init(&mxt->wakelock, WAKE_LOCK_SUSPEND, "touch");
#ifdef MXT_CALIBRATE_WORKAROUND
	schedule_delayed_work(&mxt->calibrate_dwork, msecs_to_jiffies(10000));
#endif
	pr_warning("mxt probe ok\n");
	mxt->enabled = true;
	return 0;

err_sysfs_create_group:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mxt->early_suspend);
#endif
	if (mxt->irq)
		free_irq(mxt->irq, mxt);
err_irq:
err_after_read_ot:
	if (mxt != NULL) {
		kfree(mxt->rid_map);
		kfree(mxt->delta);
		kfree(mxt->reference);
		kfree(mxt->cte);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}
err_read_ot:
	input_unregister_device(mxt->input);
	input = NULL;
err_register_device:
	cancel_work_sync(&mxt->ta_work);
#ifdef MXT_CALIBRATE_WORKAROUND
	cancel_delayed_work_sync(&mxt->calibrate_dwork);
#endif
err_identify:
err_pdata:
	if (input)
		input_free_device(input);
err_input_dev_alloc:
	if (mxt->pdata->exit_platform_hw != NULL)
		mxt->pdata->exit_platform_hw();
	kfree(mxt);
err_mxt_alloc:
	return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	/* Close down sysfs entries */
	sysfs_remove_group(&client->dev.kobj, &maxtouch_attr_group);

#ifdef CONFIG_HAS_EARLYSUSPEND
	wake_lock_destroy(&mxt->wakelock);
	unregister_early_suspend(&mxt->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	/* Release IRQ so no queue will be scheduled */
	if (mxt->irq)
		free_irq(mxt->irq, mxt);

	/* Should dealloc deltas, references, CTE structures, if allocated */
	if (mxt != NULL) {
		kfree(mxt->rid_map);
		kfree(mxt->delta);
		kfree(mxt->reference);
		kfree(mxt->cte);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}

	input_unregister_device(mxt->input);
	cancel_work_sync(&mxt->ta_work);
#ifdef MXT_CALIBRATE_WORKAROUND
	cancel_delayed_work_sync(&mxt->calibrate_dwork);
#endif
	if (mxt->pdata->exit_platform_hw != NULL)
		mxt->pdata->exit_platform_hw();
	kfree(mxt);

	i2c_set_clientdata(client, NULL);
	if (debug >= DEBUG_TRACE)
		dev_info(&client->dev, "Touchscreen unregistered\n");

	return 0;
}

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static int mxt_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev))
		enable_irq_wake(mxt->irq);

	return 0;
}

static int mxt_resume(struct i2c_client *client)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev))
		disable_irq_wake(mxt->irq);

	return 0;
}
#else
#define mxt_suspend NULL
#define mxt_resume NULL
#endif

static const struct i2c_device_id mxt_idtable[] = {
	{"sec_touch", 0,},
	{ }
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "sec_touch",
		.owner  = THIS_MODULE,
	},

	.id_table	= mxt_idtable,
	.probe		= mxt_probe,
	.remove		= __devexit_p(mxt_remove),
	.suspend	= mxt_suspend,
	.resume		= mxt_resume,

};

static int __init mxt_init(void)
{
	int err;
	err = i2c_add_driver(&mxt_driver);
	if (err)
		pr_warning("Adding mXT1386 driver failed "
		       "(errno = %d)\n", err);
	else
		pr_info("Successfully added driver %s\n",
		       mxt_driver.driver.name);
	return err;
}

static void __exit mxt_cleanup(void)
{
	i2c_del_driver(&mxt_driver);
}


module_init(mxt_init);
module_exit(mxt_cleanup);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Atmel mXT1386 Touchscreen Controller");

MODULE_LICENSE("GPL");
