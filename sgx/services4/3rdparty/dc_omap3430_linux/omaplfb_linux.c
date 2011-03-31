/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif

#include <linux/version.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fs.h>

#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#if defined(LDM_PLATFORM)
#include <linux/platform_device.h>
#endif 

#include <asm/io.h>
#include <asm/ioctl.h>
#include <asm/types.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
#include <mach/display.h>
#else 
#include <asm/arch-omap/display.h>
#endif 

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "omaplfb.h"
#include "pvrmodule.h"

MODULE_SUPPORTED_DEVICE(DEVNAME);

#define unref__ __attribute__ ((unused)) 

#if defined(LDM_PLATFORM)
static volatile OMAP_BOOL bDeviceSuspended;
#endif

/* IOCTL commands. */
#define OMAPLFB_UPDATE		_IOW('L', 1, int)
#define OMAPLFB_FLIP_ENABLE	_IOW('L', 2, int)


static int mWidth = 0;
static int mHeight = 0;
static const struct file_operations omaplfb_fops;
static int OMAPLBFlip_Enable = 1;

void *OMAPLFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void OMAPLFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}


OMAP_ERROR OMAPLFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (OMAP_ERROR_INVALID_PARAMS);
	}

	
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (OMAP_OK);
}


#if defined(SYS_USING_INTERRUPTS)

#if defined(SUPPORT_OMAP3430_OMAPFB3)

static void OMAPLFBVSyncISR(void *arg, u32 mask)
{
	OMAPLFB_SWAPCHAIN *psSwapChain= (OMAPLFB_SWAPCHAIN *)arg;
	(void) OMAPLFBVSyncIHandler(psSwapChain);
}

static inline int OMAPLFBRegisterVSyncISR(OMAPLFB_SWAPCHAIN *psSwapChain)
{
	return (int)omap_dispc_register_isr(OMAPLFBVSyncISR, psSwapChain, DISPC_IRQ_VSYNC | DISPC_IRQ_EVSYNC_ODD | DISPC_IRQ_EVSYNC_EVEN);
}

static inline int OMAPLFBUnregisterVSyncISR(OMAPLFB_SWAPCHAIN *psSwapChain)
{
	return omap_dispc_unregister_isr(OMAPLFBVSyncISR, psSwapChain, DISPC_IRQ_VSYNC | DISPC_IRQ_EVSYNC_ODD | DISPC_IRQ_EVSYNC_EVEN);
}

#else 

static void OMAPLFBVSyncISR(void *arg, struct pt_regs unref__ *regs)
{
	OMAPLFB_SWAPCHAIN *psSwapChain= (OMAPLFB_SWAPCHAIN *)arg;
	(void) OMAPLFBVSyncIHandler(psSwapChain);
}

static inline int OMAPLFBRegisterVSyncISR(OMAPLFB_SWAPCHAIN *psSwapChain)
{
	return omap2_disp_register_isr(OMAPLFBVSyncISR, psSwapChain, DISPC_IRQ_VSYNC | DISPC_IRQ_EVSYNC_ODD | DISPC_IRQ_EVSYNC_EVEN);
}

static inline int OMAPLFBUnregisterVSyncISR(OMAPLFB_SWAPCHAIN *psSwapChain)
{
	return omap2_disp_unregister_isr(OMAPLFBVSyncISR);
}

#endif 

#endif 

void OMAPLFBEnableVSyncInterrupt(OMAPLFB_SWAPCHAIN *psSwapChain)
{
}

void OMAPLFBDisableVSyncInterrupt(OMAPLFB_SWAPCHAIN *psSwapChain)
{
}

OMAP_ERROR OMAPLFBInstallVSyncISR(OMAPLFB_SWAPCHAIN *psSwapChain)
{
#if defined(SYS_USING_INTERRUPTS)
	OMAPLFBDisableVSyncInterrupt(psSwapChain);

	if (OMAPLFBRegisterVSyncISR(psSwapChain))
	{
		printk(KERN_INFO DRIVER_PREFIX ": OMAPLFBInstallVSyncISR: Request OMAPLCD IRQ failed\n");
		return (OMAP_ERROR_INIT_FAILURE);
	}

#endif
	return (OMAP_OK);
}


OMAP_ERROR OMAPLFBUninstallVSyncISR (OMAPLFB_SWAPCHAIN *psSwapChain)
{
#if defined(SYS_USING_INTERRUPTS)
	OMAPLFBDisableVSyncInterrupt(psSwapChain);

	OMAPLFBUnregisterVSyncISR(psSwapChain);

#endif
	return (OMAP_OK);
}

void OMAPLFBEnableDisplayRegisterAccess(void)
{
#if !defined(SUPPORT_OMAP3430_OMAPFB3)
	omap2_disp_get_dss();
#endif
}

void OMAPLFBDisableDisplayRegisterAccess(void)
{
#if !defined(SUPPORT_OMAP3430_OMAPFB3)
	omap2_disp_put_dss();
#endif
}

static struct omap_overlay_manager* lcd_mgr = 0;
static struct omap_overlay*         omap_gfxoverlay = 0;
static struct omap_overlay_info     gfxoverlayinfo;

void GetGFXOverlay(int manager){
    struct omap_overlay*         overlayptr = 0;
    unsigned int                 i = 0;
    unsigned int                 numoverlays = 0;

    lcd_mgr = omap_dss_get_overlay_manager(manager ? OMAP_DSS_OVL_MGR_TV: OMAP_DSS_OVL_MGR_LCD);
    if(!lcd_mgr)
    {
        DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBSetDisplayInfo couldn't find lcd overlay manager\n"));
        return;
    }

    numoverlays = omap_dss_get_num_overlays();
    if( numoverlays == 0)
    {
        DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBSetDisplayInfo couldn't find any overlays\n"));
        return;
    }

    for( i = 0; i < numoverlays; i++ )
    {
        overlayptr = omap_dss_get_overlay(i);
        if( strncmp( overlayptr->name, "gfx", 3 ) == 0)
        {
            DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBSetDisplayInfo found GFX overlay\n"));
            omap_gfxoverlay = overlayptr;
            break;
        }
    }
    if( omap_gfxoverlay == 0 )
    {
        DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBSetDisplayInfo GFX overlay no found\n"));
        return;
    }

      gfxoverlayinfo = omap_gfxoverlay->info;
      mWidth  = gfxoverlayinfo.width;
      mHeight = gfxoverlayinfo.height;
}

void OMAPLFBCheckNativeSize(int w, int h, int manager)
{
printk("OMAPLFBCheckNativeSize %d %d -> %d %d, manager %d\n\r",mWidth, mHeight, w, h, manager);
	if ((mWidth != w) || (mHeight != h))
	{
		GetGFXOverlay(manager);
		printk("New gfxoverlayinfo %d %d\n\r",omap_gfxoverlay->info.width, omap_gfxoverlay->info.height);
		updateOverlay(w, h);
	}
}

void OMAPLFBFlip(OMAPLFB_SWAPCHAIN *psSwapChain, unsigned long aPhyAddr)
{
	if (OMAPLBFlip_Enable == 1)
	{
		if (lcd_mgr && lcd_mgr->display && omap_gfxoverlay) {
			if (lcd_mgr->display->dev)
			{
			OMAPLFB_DEVINFO	*psDevInfo =
				(OMAPLFB_DEVINFO *) psSwapChain->pvDevInfo;
	
				gfxoverlayinfo = omap_gfxoverlay->info;
	
			gfxoverlayinfo.paddr = aPhyAddr;
			gfxoverlayinfo.vaddr = aPhyAddr -
				psDevInfo->sFBInfo.sSysAddr.uiAddr +
				psDevInfo->sFBInfo.sCPUVAddr;
	
				omap_gfxoverlay->info = gfxoverlayinfo;
			lcd_mgr->apply(lcd_mgr);
				if(lcd_mgr->display->update)
			{	
					lcd_mgr->display->update(lcd_mgr->display, 0, 0,
							gfxoverlayinfo.width,
							gfxoverlayinfo.height);
				}
			}
		}
	}

}

#if defined(LDM_PLATFORM) && !defined(SGX_EARLYSUSPEND)


static void OMAPLFBCommonSuspend(void)
{
	if (bDeviceSuspended)
	{
		return;
	}

	OMAPLFBDriverSuspend();

	bDeviceSuspended = OMAP_TRUE;
}

static int OMAPLFBDriverSuspend_Entry(struct platform_device unref__ *pDevice, pm_message_t unref__ state)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBDriverSuspend_Entry\n"));

	OMAPLFBCommonSuspend();

	return 0;
}

static int OMAPLFBDriverResume_Entry(struct platform_device unref__ *pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBDriverResume_Entry\n"));

	OMAPLFBDriverResume();	

	bDeviceSuspended = OMAP_FALSE;

	return 0;
}

static IMG_VOID OMAPLFBDriverShutdown_Entry(struct platform_device unref__ *pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBDriverShutdown_Entry\n"));

	OMAPLFBCommonSuspend();
}
#else
static void OMAPLFBCommonSuspend(void)
{

	if (bDeviceSuspended)
	{
		return;
	}

	OMAPLFBDriverSuspend();

	bDeviceSuspended = OMAP_TRUE;

}

static void OMAPLFBDriverSuspend_Entry(struct early_suspend *h)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBDriverSuspend_Entry\n"));

	printk(KERN_WARNING DRIVER_PREFIX ": **** SUSPEND\n");
	
	OMAPLFBCommonSuspend();
}

static void OMAPLFBDriverResume_Entry(struct early_suspend *h)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBDriverResume_Entry\n"));

	printk(KERN_WARNING DRIVER_PREFIX ": **** RESUME\n");	

	OMAPLFBDriverResume();	

	bDeviceSuspended = OMAP_FALSE;
}
#endif

#if defined(LDM_PLATFORM)

static struct platform_driver omaplfb_driver = {
	.driver = {
		.name		= DRVNAME,
	},
#if !defined(SGX_EARLYSUSPEND)
	.suspend	= OMAPLFBDriverSuspend_Entry,
	.resume		= OMAPLFBDriverResume_Entry,
	.shutdown	= OMAPLFBDriverShutdown_Entry,
#endif
};

#if defined(MODULE)

static void OMAPLFBDeviceRelease_Entry(struct device unref__ *pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBDriverRelease_Entry\n"));

	OMAPLFBCommonSuspend();
}

static struct platform_device omaplfb_device = {
	.name			= DEVNAME,
	.id				= -1,
	.dev 			= {
		.release		= OMAPLFBDeviceRelease_Entry
	}
};

#endif  

#endif	

#if defined(SGX_EARLYSUSPEND)
	OMAPLFB_DEVINFO *psDevInfo;
#endif

static int __init OMAPLFB_Init(void)
{
#if defined(LDM_PLATFORM)
	int error;
	int Major;
#endif
	if(OMAPLFBInit() != OMAP_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": OMAPLFB_Init: OMAPLFBInit failed\n");
		return -ENODEV;
	}

	GetGFXOverlay(0);

#if defined(LDM_PLATFORM)
	printk(KERN_WARNING DRIVER_PREFIX ": OMAPLFB_Init: **** REGISTER\n");
	if ((error = platform_driver_register(&omaplfb_driver)) != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": OMAPLFB_Init: Unable to register platform driver (%d)\n", error);

		goto ExitDeinit;
	}
	printk(KERN_WARNING DRIVER_PREFIX ": OMAPLFB_Init: **** OK\n");

#if defined(MODULE)
	if ((error = platform_device_register(&omaplfb_device)) != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": OMAPLFB_Init: Unable to register platform device (%d)\n", error);

		goto ExitDeinit;
	}
#endif

#if defined(SGX_EARLYSUSPEND)
	psDevInfo = NULL;
	psDevInfo = GetAnchorPtr();

	if (psDevInfo == NULL)
	{
		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFB_Init: Unable to get DevInfo anchor pointer\n"));
		goto ExitDeinit;
	}

	psDevInfo->sFBInfo.early_suspend.suspend = OMAPLFBDriverSuspend_Entry;
        psDevInfo->sFBInfo.early_suspend.resume = OMAPLFBDriverResume_Entry;
        psDevInfo->sFBInfo.early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
        register_early_suspend(&psDevInfo->sFBInfo.early_suspend);
#endif


	Major = register_chrdev(OMAPLFB_MAJOR,"omaplfb",&omaplfb_fops);
	if (Major < 0)
		printk("unable to get major  %d for fb devs!!!!\n", Major);
#endif 

	return 0;

#if defined(LDM_PLATFORM)
ExitDeinit:
	platform_driver_unregister(&omaplfb_driver);

#if defined(SGX_EARLYSUSPEND)
        unregister_early_suspend(&psDevInfo->sFBInfo.early_suspend);
#endif

	if(OMAPLFBDeinit() != OMAP_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": OMAPLFB_Init: OMAPLFBDeinit failed\n");
	}

	return -ENODEV;
#endif 
}

static int 
fb_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	 unsigned long arg)
{
	struct {
	int w;
	int h; 
	int manager; 
    	} param;
	
	switch (cmd) {
	case OMAPLFB_FLIP_ENABLE:
		{
			int enable;
			if (!copy_from_user(&enable, (void __user *)arg, sizeof(enable)))
			{
				if (enable != 0) OMAPLBFlip_Enable = 1;
				else OMAPLBFlip_Enable = 0;
			}
		}
		break;
	case OMAPLFB_UPDATE:
		if (!copy_from_user(&param, (void __user *)arg, sizeof(param)))
			OMAPLFBCheckNativeSize(param.w, param.h, param.manager);
		break;
	default: break;
	}
	return 0;
}

static IMG_VOID __exit OMAPLFB_Cleanup(IMG_VOID)
{    
	/* Unregister the device */
	 unregister_chrdev(OMAPLFB_MAJOR, "omaplfb");

#if defined (LDM_PLATFORM)
#if defined (MODULE)
	platform_device_unregister(&omaplfb_device);
#endif
	platform_driver_unregister(&omaplfb_driver);
#endif

#if defined(SGX_EARLYSUSPEND)
	unregister_early_suspend(&psDevInfo->sFBInfo.early_suspend);
#endif

	if(OMAPLFBDeinit() != OMAP_OK)
	{
 		printk(KERN_WARNING DRIVER_PREFIX ": OMAPLFB_Cleanup: OMAPLFBDeinit failed\n");
	}
}


static int fb_open(struct inode *inode, struct file *file)
{
	return 0;
}
	
static int fb_release(struct inode *inode, struct file *file)
{
	return 0;
}


static const struct file_operations omaplfb_fops = {
	.owner =	THIS_MODULE,
	.ioctl =	fb_ioctl,
	.open =		fb_open,
	.release =	fb_release,
};

module_init(OMAPLFB_Init);
module_exit(OMAPLFB_Cleanup);

