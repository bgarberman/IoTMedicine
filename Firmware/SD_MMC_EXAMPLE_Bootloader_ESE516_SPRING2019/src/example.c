/*
*
* SAMW25 Bootloader
* ESE 516
* Spring 2019
*
* The Medicine Men
* Brett Garberman
* Matthew Hanna
*
*/


#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "SerialConsole/SerialConsole.h"
#include "sd_mmc_spi.h"
#include "crc32.h"

#define APP_START_ADDRESS  ((uint32_t)0xB000) 
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04)

struct usart_module cdc_uart_module;

static void jumpToApplication(void);
static void configure_nvm(void);
static void bootloader(void);
static void setupSD(void);
static void setFlashFlag(void);
static void setVersionNVM(int ver);
static void configure_port_pins(void);

int userAppCondition(void);
int hardwareCondition(void);
int flashFlag(void);

//INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";
char version_file[] = "0:version.txt";
char versionNVM_file[] = "0:versionNVM.txt";
char flag_file[] = "0:flag.txt";
char newFirmwareFileName[] = "app.bin";
TCHAR* firmwareVersionBuffer[1];
int firmwareVersion = 0;
int readFirmwareVersion;
int fileSize;
uint16_t remainingFileToBeRead = 0; 
uint16_t additional_bytes = 0; 
int fileSizeVer;
int loops;
int eraseCounter = 0;
int max_erased_row = 0;
int max_read_row = 0;
char out1[10];
char out2[10];
char out3[10];
char outTest[10];
char outError[20];
Ctrl_status status;
volatile FRESULT result;
volatile FRESULT readResult;
FRESULT res;
FRESULT res2;
FATFS fs;
FIL file_object;
FILINFO fno;
FRESULT fr;
enum status_code error_code;
uint32_t num_bytes_read = 0; 
uint32_t num_bytes_read2; 


int main(void){

	//INITIALIZE SYSTEM PERIPHERALS
	system_init();
	delay_init();
	InitializeSerialConsole();
	SerialConsoleWriteString("\x0C\n\r-- ESE516 - Start of project!\r\n");	
	system_interrupt_enable_global();
	sd_mmc_init();
	configure_nvm();
	configure_port_pins();
	irq_initialize_vectors();
	cpu_irq_enable();
	setupSD();
	dsu_crc32_init();


	if (userAppCondition() == 0) { //If there is no data at application start address
			setFlashFlag();
			bootloader();
			}
	else{
		if (flashFlag() == 0){ //If the device has never been flashed with a version number 
			setFlashFlag();
			}
		else{ //Continue to bootloader
		}
		bootloader();
	}
}


static void jumpToApplication(void) {

	void (*applicationCodeEntry)(void);
	__set_MSP(*(uint32_t *) APP_START_ADDRESS); /// Rebase stack pointer
	SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk); /// Rebase vector table
	applicationCodeEntry = /// Set pointer to application section
	(void (*)(void))(unsigned *)(*(unsigned *)(APP_START_RESET_VEC_ADDRESS));

	applicationCodeEntry();
}


static void configure_nvm(void){

	struct nvm_config config_nvm;
	nvm_get_config_defaults(&config_nvm);
	config_nvm.manual_page_write = false;
	nvm_set_config(&config_nvm);
}

void configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction  = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(PIN_PB23, &config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PA23, &config_port_pin);
}

void setupSD(void){
	do {
		status = sd_mmc_test_unit_ready(0);
		if (CTRL_FAIL == status) {
			SerialConsoleWriteString("Card install FAIL\n\r");
			SerialConsoleWriteString("Please unplug and re-plug the card.\n\r");
			while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
			}
		}
	} while (CTRL_GOOD != status);


	//Attempt to mount a FAT file system on the SD Card using FATFS
	SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
	memset(&fs, 0, sizeof(FATFS));
	res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
	if (FR_INVALID_DRIVE == res) {
		LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
	}
	SerialConsoleWriteString("[OK]\r\n");
}

int userAppCondition(void){
	char userAppCheckBuffer[NVMCTRL_PAGE_SIZE];
	do{
		error_code = nvm_read_buffer(APP_START_RESET_VEC_ADDRESS, userAppCheckBuffer, NVMCTRL_PAGE_SIZE);
	} while (error_code == STATUS_BUSY);
	if (userAppCheckBuffer[0] == (uint8_t)0xFF){ //No user application
		return 0;
	}
	else{ //User application exists
		return 1;
	}
	
}


int flashFlag(void){
	char flagBuffer[5];
	int flag;
	res = f_open(&file_object, "0:flag.txt", FA_READ);
	if (res != FR_OK) {
		SerialConsoleWriteString("Couldn't open flag file on SD.\r\n");
	}
	f_gets(flagBuffer, sizeof(flagBuffer), &file_object);
	f_close(&file_object);
	flag = atoi(flagBuffer);
	return flag;
}

void setFlashFlag(void){
		char flagBuffer[5];
		TCHAR* flag = "1";

		res = f_open(&file_object, "0:flag.txt", FA_WRITE);
		if (res != FR_OK) {
			SerialConsoleWriteString("Couldn't open flag file on SD.\r\n");
		}
		res = f_lseek(&file_object, 0);
		
		//res = f_puts(" ", &file_object);
		//res = f_lseek(&file_object, 0);
		res = f_puts(flag, &file_object);
		f_close(&file_object);
}

void setVersionNVM(int ver){
		char newVerBuffer[5];
		sprintf(newVerBuffer, "%d", ver);

		res = f_open(&file_object, "0:versionNVM.txt", FA_WRITE);
		if (res != FR_OK) {
			SerialConsoleWriteString("Couldn't open flag file on SD.\r\n");
		}
		res = f_lseek(&file_object, 0);
		
		res = f_puts(newVerBuffer, &file_object);
		f_close(&file_object);
}

void bootloader(void){
	
	SerialConsoleWriteString("ESE516 - BOOTLOADER running!\r\n");	
   int versionNVM;
   int versionSD;
   uint32_t* bytesRead = (uint32_t*)malloc(sizeof(uint32_t));
   
   char versionBufferSD[5];
   char versionBufferNVM[5];
   
   res = f_open(&file_object, (char const*)version_file, FA_READ);
   if (res != FR_OK) {
	   SerialConsoleWriteString("Couldn't open version file on SD.\r\n");
   }
   f_gets(versionBufferSD, sizeof(versionBufferSD), &file_object);
   f_close(&file_object);
   versionSD = atoi(versionBufferSD);
   
   res = f_open(&file_object, (char const*)versionNVM_file, FA_READ);
   if (res != FR_OK) {
	   SerialConsoleWriteString("Couldn't open version file on SD.\r\n");
   }
   f_gets(versionBufferNVM, sizeof(versionBufferNVM), &file_object);
   f_close(&file_object);
   versionNVM = atoi(versionBufferNVM);
   
   int print1 = sprintf(out1, "SD Firmware: %d\r\n", versionSD);
   int print2 = sprintf(out2, "NVM Firmware: %d\r\n", versionNVM);
   SerialConsoleWriteString(out1);
   SerialConsoleWriteString(out2);


	if (versionSD > versionNVM){
	
		//Open the new firmware file

		res = f_open(&file_object, "0:app.bin", FA_READ);
		if (res != FR_OK) {
			SerialConsoleWriteString("Failed to open new firmware file...\r\n");
			int print3 = sprintf(outError, "The result log is %d.\r\n", res);
			SerialConsoleWriteString(outError);

		}

		int fileSize = f_size(&file_object);
		int print3 = sprintf(out3, "The size of the firmware file is %d bytes.\r\n", fileSize);
		SerialConsoleWriteString(out3); 

		//Erase the old firmware
		
		max_erased_row = (fileSize / (NVMCTRL_ROW_PAGES * NVMCTRL_PAGE_SIZE)) + 1;
		max_read_row = (fileSize / NVMCTRL_PAGE_SIZE) + 1;
		
		for (eraseCounter = 0; eraseCounter < max_erased_row; eraseCounter++){
			do{
			error_code = nvm_erase_row(APP_START_ADDRESS + (NVMCTRL_ROW_PAGES * NVMCTRL_PAGE_SIZE * eraseCounter));
			} while (error_code == STATUS_BUSY);
		}

			
		//Load the new firmware
		
		uint32_t *crc_sd;
		uint32_t *crc_nvm;
		
		char transferBuffer[NVMCTRL_PAGE_SIZE];
		char readBackBuffer[NVMCTRL_PAGE_SIZE]; 
		delay_s(1);
		for (int read_counter = 0; read_counter < max_read_row; read_counter++){
			
			num_bytes_read = NVMCTRL_PAGE_SIZE * read_counter; //track number of bytes read in
			remainingFileToBeRead = fileSize - num_bytes_read; 
			
			//Handles the edge case where there are additional bytes to read with length less than that of an NVM Page
			if (remainingFileToBeRead < NVMCTRL_PAGE_SIZE && remainingFileToBeRead >= 0) { 
				//additional_bytes = remainingFileToBeRead; 
				result = f_read(&file_object, transferBuffer, remainingFileToBeRead, &num_bytes_read);
				dsu_crc32_cal(&transferBuffer, remainingFileToBeRead, &crc_sd); 
				
				//remainingFileToBeRead = remainingFileToBeRead - additional_bytes; 
				do 
				{
					error_code = nvm_write_buffer(APP_START_ADDRESS + NVMCTRL_PAGE_SIZE*read_counter, 
					transferBuffer, remainingFileToBeRead);
				} while (error_code	== STATUS_BUSY);
				
				do 
				{
					error_code = nvm_read_buffer(APP_START_ADDRESS + NVMCTRL_PAGE_SIZE*read_counter,
					transferBuffer, remainingFileToBeRead);
					dsu_crc32_cal(&transferBuffer, remainingFileToBeRead, &crc_nvm); 
				} while (error_code == STATUS_BUSY);
			}
			else { 
				result = f_read(&file_object, transferBuffer, NVMCTRL_PAGE_SIZE, bytesRead); 
			 

				dsu_crc32_cal(&transferBuffer, bytesRead, &crc_sd);

			if (result == 0){
				do{
					
					error_code = nvm_write_buffer(APP_START_ADDRESS + NVMCTRL_PAGE_SIZE*read_counter, transferBuffer, NVMCTRL_PAGE_SIZE);
				} while (error_code == STATUS_BUSY);
				
				do {
					error_code = nvm_read_buffer(APP_START_ADDRESS + NVMCTRL_PAGE_SIZE*read_counter, transferBuffer, NVMCTRL_PAGE_SIZE);
				} while (error_code == STATUS_BUSY);
				
				dsu_crc32_cal(&transferBuffer, bytesRead, &crc_nvm);
				
			} else{
				SerialConsoleWriteString("Error with result.\r\n");
			}	
		}
					}
		f_close(&file_object);
		char crcnvmout[100];
		char crcsdout[100];

		sprintf(crcsdout, "CRC SD: %d\r\n", crc_sd);
		SerialConsoleWriteString(crcsdout);		
		
		sprintf(crcnvmout, "CRC NVM: %d\r\n", crc_nvm);
		SerialConsoleWriteString(crcnvmout);
		
		delay_s(1);
		
		char update[1];
		sprintf(update, "%d", versionSD);

		res = f_open(&file_object, "0:versionNVM.txt", FA_WRITE);
		if (res != FR_OK) {
			SerialConsoleWriteString("Couldn't open version NVM file on SD.\r\n");
 		}

		res = f_puts(update, &file_object);
		f_close(&file_object);
		
		SerialConsoleWriteString("We updated the firmware!\r\n");

		delay_s(1); //Delay to allow text to print
			
		cpu_irq_disable();
		DeinitializeSerialConsole();
		sd_mmc_deinit();
		jumpToApplication();
		}

	else{
		SerialConsoleWriteString("Firmware Up-to-Date! Jumping to Application...\r\n");
		delay_s(1);
		cpu_irq_disable();
		DeinitializeSerialConsole();
		sd_mmc_deinit();
		jumpToApplication();	
	}
}
