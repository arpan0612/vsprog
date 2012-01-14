/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <ctype.h>

#include "app_cfg.h"
#include "app_type.h"
#include "app_io.h"
#include "app_err.h"
#include "app_log.h"

#include "vsprog.h"
#include "interfaces.h"
#include "target.h"
#include "scripts.h"

#include "stm32l1.h"
#include "stm32l1_internal.h"
#include "comisp.h"
#include "cm3.h"
#include "adi_v5p1.h"

#define CUR_TARGET_STRING			STM32L1_STRING

struct program_area_map_t stm32l1_program_area_map[] =
{
	{APPLICATION_CHAR, 1, 0, 0, 0, AREA_ATTR_EP | AREA_ATTR_EWR | AREA_ATTR_RAE | AREA_ATTR_RAW},
	{FUSE_CHAR, 0, 0, 0, 0, AREA_ATTR_EWR | AREA_ATTR_RAW},
	{UNIQUEID_CHAR, 0, 0, 0, 0, AREA_ATTR_R},
	{0, 0, 0, 0, 0, AREA_ATTR_NONE}
};

const struct program_mode_t stm32l1_program_mode[] =
{
	{'j', SET_FREQUENCY, IFS_JTAG_HL},
	{'s', "", IFS_SWD},
	{'i', USE_COMM, 0},
	{0, NULL, 0}
};

struct program_functions_t stm32l1_program_functions;

VSS_HANDLER(stm32l1_help)
{
	VSS_CHECK_ARGC(1);
	PRINTF("\
Usage of %s:\n\
  -C,  --comport <COMM_ATTRIBUTE>           set com port\n\
  -m,  --mode <MODE>                        set mode<j|s|i>\n\
  -x,  --execute <ADDRESS>                  execute program\n\
  -F,  --frequency <FREQUENCY>              set JTAG frequency, in KHz\n\n",
			CUR_TARGET_STRING);
	return VSFERR_NONE;
}

VSS_HANDLER(stm32l1_mode)
{
	uint8_t mode;
	
	VSS_CHECK_ARGC(2);
	mode = (uint8_t)strtoul(argv[1], NULL,0);
	switch (mode)
	{
	case STM32L1_JTAG:
	case STM32L1_SWD:
		stm32l1_program_area_map[0].attr |= AREA_ATTR_RNP;
		cm3_mode_offset = 0;
		vss_call_notifier(cm3_notifier, "chip", "cm3_stm32l1");
		memcpy(&stm32l1_program_functions, &cm3_program_functions,
				sizeof(stm32l1_program_functions));
		break;
	case STM32L1_ISP:
		stm32l1_program_area_map[0].attr &= ~AREA_ATTR_RNP;
		vss_call_notifier(comisp_notifier, "chip", "comisp_stm32l1");
		memcpy(&stm32l1_program_functions, &comisp_program_functions,
				sizeof(stm32l1_program_functions));
		break;
	}
	return VSFERR_NONE;
}

VSS_HANDLER(stm32l1_extra)
{
	char cmd[4] = "E ";
	
	VSS_CHECK_ARGC(1);
	cmd[2] = '0' + COMISP_STM32;
	cmd[3] = '\0';
	return vss_run_script(cmd);
}

const struct vss_cmd_t stm32l1_notifier[] =
{
	VSS_CMD(	"help",
				"print help information of current target for internal call",
				stm32l1_help,
				NULL),
	VSS_CMD(	"mode",
				"set programming mode of target for internal call",
				stm32l1_mode,
				NULL),
	VSS_CMD(	"extra",
				"print extra information for internal call",
				stm32l1_extra,
				NULL),
	VSS_CMD_END
};

uint16_t stm32l1_get_flash_size(uint32_t mcuid, uint32_t flash_sram_reg)
{
	uint16_t den = mcuid & STM32L1_DEN_MSK;
	uint16_t flash_size = flash_sram_reg & 0xFFFF;
	
	if (0xFFFF == flash_size)
	{
		switch (den)
		{
		case STM32L1_DEN_L15:
			return 128;
		default:
			return 1024;
		}
	}
	return flash_size;
}

void stm32l1_print_device(uint32_t mcuid)
{
	char rev_char = 0;
	uint16_t den, rev;
	
	den = mcuid & STM32L1_DEN_MSK;
	rev = (mcuid & STM32L1_REV_MSK) >> 16;
	switch (den)
	{
	case STM32L1_DEN_L15:
		LOG_INFO("STM32L1 type: L15 device");
		switch (rev)
		{
		case 0x1000:
			rev_char = 'A';
			break;
		case 0x1008:
			rev_char = 'Y';
			break;
		}
		break;
	default:
		LOG_INFO("STM32L1 type: unknown device(%08X)", mcuid);
		break;
	}
	if (rev_char != 0)
	{
		LOG_INFO("STM32L1 revision: %c", rev_char);
	}
}
