/*
 * Description:
 * Helper functions to support the tegra USB controller
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/fsl_devices.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <mach/usb_phy.h>

static struct tegra_usb_phy *phy;
static struct clk *udc_clk;
static struct clk *emc_clk;
static struct clk *sclk;
static void *udc_base;

struct usb_data {
	struct mutex lock;
	int flag;
};

static struct usb_data fsl_usb_data;

int fsl_udc_clk_init(struct platform_device *pdev)
{
	struct resource *res;
	int err;
	int instance;
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;
	
	mutex_init(&fsl_usb_data.lock);
	fsl_usb_data.flag=0;

	udc_clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(udc_clk)) {
		dev_err(&pdev->dev, "Can't get udc clock\n");
		return PTR_ERR(udc_clk);
	}

	clk_enable(udc_clk);

	emc_clk = clk_get(&pdev->dev, "emc");
	if (IS_ERR(emc_clk)) {
		dev_err(&pdev->dev, "Can't get emc clock\n");
		err = PTR_ERR(emc_clk);
		goto err_emc;
	}

	clk_enable(emc_clk);

	clk_set_rate(emc_clk, 400000000);

	sclk = clk_get(&pdev->dev, "sclk");
	if (IS_ERR(sclk)) {
		dev_err(&pdev->dev, "Can't get sclk clock\n");
		err = PTR_ERR(sclk);
		goto err_sclk;
	}	

	/* we have to remap the registers ourselves as fsl_udc does not
	 * export them for us.
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err = -ENXIO;
		goto err0;
	}
	udc_base = ioremap(res->start, resource_size(res));
	if (!udc_base) {
		err = -ENOMEM;
		goto err0;
	}

	instance = pdev->id;
	if (instance == -1)
		instance = 0;

	phy = tegra_usb_phy_open(instance, udc_base, pdata->phy_config,
						TEGRA_USB_PHY_MODE_DEVICE);
	if (IS_ERR(phy)) {
		dev_err(&pdev->dev, "Can't open phy\n");
		err = PTR_ERR(phy);
		goto err1;
	}

	tegra_usb_phy_power_on(phy, true);

	return 0;
err1:
	iounmap(udc_base);
err0:
	clk_put(sclk);
err_sclk:	
	clk_disable(emc_clk);
	clk_put(emc_clk);
err_emc:
	clk_disable(udc_clk);
	clk_put(udc_clk);
	return err;
}

void fsl_udc_lock_sclk(uint rate)
{
	printk(KERN_ERR "sclk rate lock %d\n", rate);
	clk_set_rate(sclk, rate);
	clk_enable(sclk);
}

void fsl_udc_unlock_sclk()
{
	printk(KERN_ERR "sclk rate unlock 80Mhz\n");
	clk_disable(sclk);
}

void fsl_udc_clk_finalize(struct platform_device *pdev)
{
	fsl_usb_data.flag=1;
}

void fsl_udc_clk_release(void)
{
	tegra_usb_phy_close(phy);

	iounmap(udc_base);

	clk_disable(udc_clk);
	clk_put(udc_clk);

	clk_disable(emc_clk);
	clk_put(emc_clk);
}

void fsl_udc_clk_suspend(bool is_dpd)
{
	mutex_lock(&fsl_usb_data.lock);
	printk("%s : %d, is_dpd : %d\n", __func__, __LINE__, is_dpd);			
	tegra_usb_phy_power_off(phy, is_dpd);
	clk_disable(udc_clk);
	clk_disable(emc_clk);
	mutex_unlock(&fsl_usb_data.lock);		
}

void fsl_udc_clk_resume(bool is_dpd)
{
	mutex_lock(&fsl_usb_data.lock);
	printk("%s : %d, is_dpd : %d\n", __func__, __LINE__, is_dpd);			
	clk_enable(emc_clk);
	clk_enable(udc_clk);
	tegra_usb_phy_power_on(phy, is_dpd);
	mutex_unlock(&fsl_usb_data.lock);	
}

void fsl_udc_dtd_prepare(void)
{
	/* When we are programming two DTDs very close to each other,
	 * the second DTD is being prefetched before it is actually written
	 * to DDR. To prevent this, we disable prefetcher before programming
	 * any new DTD and re-enable it before priming endpoint.
	 */
	tegra_usb_phy_memory_prefetch_off(phy);
}

void fsl_udc_ep_barrier(void)
{
	tegra_usb_phy_memory_prefetch_on(phy);
}

