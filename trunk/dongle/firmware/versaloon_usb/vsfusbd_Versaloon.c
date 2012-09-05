#include "app_cfg.h"
#include "app_type.h"

#include "vsf_usbd_cfg.h"
#include "stack/usb_device/vsf_usbd_const.h"
#include "stack/usb_device/vsf_usbd.h"
#include "stack/usb_device/vsf_usbd_drv_callback.h"

#include "vsfusbd_Versaloon.h"

#include "app_interfaces.h"
#include "USB_TO_XXX.h"

uint8_t buffer_out[USB_DATA_BUFF_SIZE], asyn_rx_buf[ASYN_DATA_BUFF_SIZE];
volatile uint32_t count_out = 0;
volatile uint32_t usb_ovf = 0;
volatile uint32_t cmd_len = 0;

volatile uint32_t rep_len = 0;

static vsf_err_t Versaloon_OUT_hanlder(void *p, uint8_t ep)
{
	struct vsfusbd_device_t *device = p;
	uint32_t pkg_len;
#if VSFUSBD_CFG_DBUFFER_EN
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	int8_t iface = config->ep_OUT_iface_map[ep];
	struct vsfusbd_Versaloon_param_t *param = NULL;
	
	if (iface < 0)
	{
		return VSFERR_FAIL;
	}
	param = (struct vsfusbd_Versaloon_param_t *)config->iface[iface].protocol_param;
	if (NULL == param)
	{
		return VSFERR_FAIL;
	}
#endif

	if(cmd_len & 0x80000000)
	{
		usb_ovf = 1;
		count_out = 0;
	}
	
#if VSFUSBD_CFG_DBUFFER_EN
	if (param->dbuffer_en)
	{
		device->drv->ep.switch_OUT_buffer(ep);
	}
#endif
	pkg_len = device->drv->ep.get_OUT_count(ep);
	device->drv->ep.read_OUT_buffer(ep, buffer_out + count_out, pkg_len);
	device->drv->ep.enable_OUT(ep);
	
	if(pkg_len)
	{
		if(!count_out)
		{
			// first package
			if(buffer_out[0] <= VERSALOON_COMMON_CMD_END)
			{
				// Common Commands
				if(buffer_out[0] == VERSALOON_WRITE_OFFLINE_DATA)
				{
					cmd_len = buffer_out[1] + ((uint16_t)buffer_out[2] << 8) + 7;
				}
				else
				{
					cmd_len = pkg_len;
				}
			}
#if USB_TO_XXX_EN
			else if(buffer_out[0] <= VERSALOON_USB_TO_XXX_CMD_END)
			{
				// USB_TO_XXX Support
				cmd_len = buffer_out[1] + ((uint16_t)buffer_out[2] << 8);
			}
#endif
		}
		count_out += pkg_len;
		
		// all data received?
		pkg_len = cmd_len;
		if(count_out >= pkg_len)
		{
			cmd_len |= 0x80000000;
		}
	}
	
	return VSFERR_NONE;
}

static vsf_err_t versaloon_usb_init(uint8_t iface, struct vsfusbd_device_t *device)
{
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_Versaloon_param_t *param = 
		(struct vsfusbd_Versaloon_param_t *)config->iface[iface].protocol_param;
	
	app_interfaces.delay.init();
#if POWER_SAMPLE_EN
	core_interfaces.adc.init(TVCC_ADC_PORT);
	core_interfaces.adc.config(TVCC_ADC_PORT, CORE_APB2_FREQ_HZ / 8, ADC_ALIGNRIGHT);
	core_interfaces.adc.config_channel(TVCC_ADC_PORT, TVCC_ADC_CHANNEL, 0xFF);
	core_interfaces.adc.calibrate(TVCC_ADC_PORT, TVCC_ADC_CHANNEL);
#endif
	
#if USB_TO_XXX_EN
	USB_TO_XXX_Init(asyn_rx_buf + 2048);
#endif
	
	if (
#if VSFUSBD_CFG_DBUFFER_EN
		(param->dbuffer_en && 
		(	device->drv->ep.set_IN_dbuffer(param->ep_in) || 
			device->drv->ep.set_OUT_dbuffer(param->ep_out))) || 
#endif
		device->drv->ep.set_OUT_handler(param->ep_out, Versaloon_OUT_hanlder))
	{
		return VSFERR_FAIL;
	}
	return VSFERR_NONE;
}

static vsf_err_t versaloon_poll(uint8_t iface, struct vsfusbd_device_t *device)
{
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_Versaloon_param_t *param = 
		(struct vsfusbd_Versaloon_param_t *)config->iface[iface].protocol_param;
	
	if(cmd_len & 0x80000000)
	{
		// A valid USB package has been received
		LED_USB_ON();
		
		ProcessCommand(&buffer_out[0], cmd_len & 0xFFFF);
		if(rep_len > 0)
		{
			// indicate reply data is valid
			rep_len |= 0x80000000;
		}
		else
		{
			// no data to send, set cmd_len to 0
			cmd_len = 0;
		}
		count_out = 0;				// set USB receive pointer to 0
		
		if(rep_len & 0x80000000)	// there is valid data to be sent to PC
		{
			struct vsfusbd_transact_t *transact =
											&vsfusbd_IN_transact[param->ep_in];
			struct vsf_buffer_t *buffer = &transact->tbuffer.buffer;
			
			buffer->buffer = buffer_out;
			buffer->size = rep_len & 0xFFFF;
			transact->pkt.in.zlp = true;
			transact->callback.callback = NULL;
			vsfusbd_ep_send(device, param->ep_in);
			
			// reset command length and reply length for next command
			cmd_len = 0;
			rep_len = 0;
		}
		
		LED_USB_OFF();
	}
	else
	{
#if POWER_OUT_EN
			app_interfaces.target_voltage.poll(0);
#endif
	}
	
	return VSFERR_NONE;
}

const struct vsfusbd_class_protocol_t vsfusbd_Versaloon_class = 
{
	NULL, NULL, NULL, 
	versaloon_usb_init,
	NULL, versaloon_poll
};
