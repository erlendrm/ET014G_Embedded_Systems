/*
 * erm_fat.h
 *
 * Created: 08.03.2016 17:57:56
 *  Author: Erlend
 */ 


#ifndef ERM_FAT_H_
#define ERM_FAT_H_

// Global variables
uint32_t	erm_current_file_counter = 1;


static void erm_fat_mount_sdmmc(void)
{
	// Select and mount the FAT partition on the SD/MMC
	nav_reset();
	nav_drive_set(0);
	nav_partition_mount();
}


static void erm_fat_create_file(char* file)
{	
	
	sprintf(file, "Measurement_%lu.txt", erm_current_file_counter);
	
	nav_filelist_reset();
	
	while (nav_filelist_findname(file, false))
	{
		erm_current_file_counter++;
		sprintf(file, "Measurement_%lu.txt", erm_current_file_counter);
	}
	
	// Create the file
	nav_file_create((FS_STRING) file);
	
}

static void erm_fat_file_append(char* file, uint32_t input)
{
	uint8_t	i;
	char	temp_string[8];
	
	// Set navigator to desired file
	if (!nav_setcwd((FS_STRING) file, true, true))
	{
		erm_cdc_println("error selecting sd/mmc");
	}
	else
	{
		// Open file
		file_open(FOPEN_MODE_APPEND);
		// Convert pot_value to a string
		sprintf(temp_string, "%lu\x03", input);
		// Write value to file
		i = 0;
		while ((int) temp_string[i] != 0x03)
		{
			file_putc((int) temp_string[i]);
			i++;
		}
		// Add CR and LF
		file_putc('\r');
		file_putc('\n');
		// Close file
		file_close();
	}
}


#endif /* ERM_FAT_H_ */