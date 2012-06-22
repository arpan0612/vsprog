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

#ifndef __MAL_H_INCLUDED__
#define __MAL_H_INCLUDED__

#include "app_type.h"

#include "../dal.h"

struct mal_capacity_t
{
	uint64_t block_size;
	uint64_t block_number;
};

struct mal_info_t
{
	struct mal_capacity_t capacity;
	void *extra;
	uint32_t erase_page_size;
	uint32_t write_page_size;
	uint32_t read_page_size;
	
	struct mal_driver_t *driver;
};

struct mal_t
{
	vsf_err_t (*init)(struct dal_info_t *param);
	vsf_err_t (*init_isready)(struct dal_info_t *param);
	vsf_err_t (*fini)(struct dal_info_t *param);
	vsf_err_t (*getinfo)(struct dal_info_t *param);
	vsf_err_t (*poll)(struct dal_info_t *param);
	
	vsf_err_t (*eraseall)(struct dal_info_t *param);
	vsf_err_t (*eraseblock)(struct dal_info_t *param, 
							uint64_t address, uint64_t count);
	vsf_err_t (*readblock)(struct dal_info_t *param, 
						uint64_t address, uint8_t *buff, uint64_t count);
	vsf_err_t (*writeblock)(struct dal_info_t *param, 
							uint64_t address, uint8_t *buff, uint64_t count);
	
	vsf_err_t (*eraseall_nb_start)(struct dal_info_t *param);
	vsf_err_t (*eraseall_nb_isready)(struct dal_info_t *param);
	vsf_err_t (*eraseall_nb_waitready)(struct dal_info_t *param);
	vsf_err_t (*eraseall_nb_end)(struct dal_info_t *param);
	
	vsf_err_t (*eraseblock_nb_start)(struct dal_info_t *param, 
										uint64_t address, uint64_t count);
	vsf_err_t (*eraseblock_nb)(struct dal_info_t *param, 
								uint64_t address);
	vsf_err_t (*eraseblock_nb_isready)(struct dal_info_t *param, 
										uint64_t address);
	vsf_err_t (*eraseblock_nb_waitready)(struct dal_info_t *param, 
											uint64_t address);
	vsf_err_t (*eraseblock_nb_end)(struct dal_info_t *param);
	
	vsf_err_t (*readblock_nb_start)(struct dal_info_t *param, 
									uint64_t address, uint64_t count);
	vsf_err_t (*readblock_nb)(struct dal_info_t *param, 
								uint64_t address, uint8_t *buff);
	vsf_err_t (*readblock_nb_isready)(struct dal_info_t *param, 
										uint64_t address, uint8_t *buff);
	vsf_err_t (*readblock_nb_waitready)(struct dal_info_t *param, 
										uint64_t address, uint8_t *buff);
	vsf_err_t (*readblock_nb_end)(struct dal_info_t *param);
	
	vsf_err_t (*writeblock_nb_start)(struct dal_info_t *param, 
										uint64_t address, uint64_t count);
	vsf_err_t (*writeblock_nb)(struct dal_info_t *param, 
								uint64_t address, uint8_t *buff);
	vsf_err_t (*writeblock_nb_isready)(struct dal_info_t *param, 
										uint64_t address, uint8_t *buff);
	vsf_err_t (*writeblock_nb_waitready)(struct dal_info_t *param, 
										uint64_t address, uint8_t *buff);
	vsf_err_t (*writeblock_nb_end)(struct dal_info_t *param);
};

extern struct mal_t mal;
extern struct mal_driver_t mal_empty_drv;

#if DAL_EE93CX6_EN
#	include "../ee93cx6/ee93cx6_drv.h"
#endif
#if DAL_EE24CXX_EN
#	include "../ee24cxx/ee24cxx_drv.h"
#endif
#if DAL_DF25XX_EN
#	include "../df25xx/df25xx_drv.h"
#endif
#if DAL_DF45XX_EN
#	include "../df45xx/df45xx_drv.h"
#endif
#if DAL_SD_SPI_EN || DAL_SD_SDIO_EN
#	include "../sd/sd_common.h"
#endif
#if DAL_SD_SPI_EN
#	include "../sd/sd_spi_drv.h"
#endif
#if DAL_SD_SDIO_EN
#	include "../sd/sd_sdio_drv.h"
#endif
#if DAL_CFI_EN
#	include "../cfi/cfi_drv.h"
#endif
#if DAL_NAND_EN
#	include "../nand/nand_drv.h"
#endif

#endif	// __MAL_H_INCLUDED__

