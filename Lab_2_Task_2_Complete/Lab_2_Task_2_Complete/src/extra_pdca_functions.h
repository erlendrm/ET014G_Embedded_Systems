/*
 * extra_pdca_functions.h
 *
 * Created: 26.02.2016 16:36:59
 *  Author: Erlend
 */ 


#ifndef EXTRA_PDCA_FUNCTIONS_H_
#define EXTRA_PDCA_FUNCTIONS_H_

//! Writing using PDCA
//! @brief This function opens a SD_MMC memory in write mode at a given sector
//! address.
//!
//! NOTE: If page buffer > 512 bytes, page content is first loaded in buffer to
//! be partially updated by write_byte or write64 functions.
//!
//! @param  pos   Sector address
//!
//! @return bit
//!   The open succeeded      -> Ture
//!/
static Bool sd_mmc_spi_write_open_PDCA (uint32_t pos)
{
	uint32_t	gl_ptr_mem;
	uint8_t		r1;
	
	
	// Set the global memory ptr at a Byte address.
	gl_ptr_mem = pos << 9;                    // gl_ptr_mem = pos * 512

	// wait for MMC not busy
	if (false == sd_mmc_spi_wait_not_busy())
	return false;


	spi_selectChip(SD_MMC_SPI, SD_MMC_SPI_NPCS);          // select SD_MMC_SPI
	// issue command
	r1 = sd_mmc_spi_command(MMC_WRITE_BLOCK, gl_ptr_mem);

	// check for valid response
	if (r1 != 0x00)
	{
		spi_unselectChip(SD_MMC_SPI, SD_MMC_SPI_NPCS);  // unselect SD_MMC_SPI
		// print_dbg("\n****** could not open SD_MMC_SPI for single block read\n");
		// Trace("\n****** could not open SD_MMC_SPI for single block read\n");
		return false;
	}

	// send dummy
	spi_write(SD_MMC_SPI,0xFF);   // give clock again to end transaction

	// send data start token
	spi_write(SD_MMC_SPI,MMC_STARTBLOCK_WRITE);

	return true;   // Read done.
}


//! Stop PDCA transfer
//! @brief This function closes a PDCA write transfer
//! page programming.
//!
static Bool sd_mmc_spi_write_close_PDCA (void)
{
	uint16_t	i;
	uint8_t		r1;

	// load 16-bit CRC (ignored)
	spi_write(SD_MMC_SPI,0xFF);
	spi_write(SD_MMC_SPI,0xFF);

	// read data response token
	r1 = sd_mmc_spi_send_and_read(0xFF);
	if( (r1 & MMC_DR_MASK) != MMC_DR_ACCEPT)
	{
		spi_write(SD_MMC_SPI,0xFF);    // send dummy bytes
		spi_write(SD_MMC_SPI,0xFF);
		spi_unselectChip(SD_MMC_SPI, SD_MMC_SPI_NPCS);
		return false;
		//     return r1;             // return ERROR byte
	}

	spi_write(SD_MMC_SPI,0xFF);    // send dummy bytes
	spi_write(SD_MMC_SPI,0xFF);

	// release chip select
	spi_unselectChip(SD_MMC_SPI, SD_MMC_SPI_NPCS);  // unselect SD_MMC_SPI

	// wait card not busy after last programming operation
	i=0;
	while (false == sd_mmc_spi_wait_not_busy())
	{
		i++;
		if (i == 10)
		return false;
	}

	return true;                  // Write done
}

#endif /* EXTRA_PDCA_FUNCTIONS_H_ */