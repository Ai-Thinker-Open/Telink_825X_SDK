
#include "tl_common.h"
#include "drivers.h"
#include "drivers/8258/flash.h"
#include "drivers/8258/spi_i.h"
#include "drivers/8258/irq.h"
#include "drivers/8258/timer.h"
#include "drivers/8258/watchdog.h"

_attribute_ram_code_ static void flash_send_cmd(unsigned char cmd){
	mspi_high();
	sleep_us(1);
	mspi_low();
	mspi_write(cmd);
	mspi_wait();
}

/**
 * @brief     This function serves to send flash address.
 * @param[in] addr - the flash address.
 * @return    none
 */
_attribute_ram_code_ static void flash_send_addr(unsigned int addr){
	mspi_write((unsigned char)(addr>>16));
	mspi_wait();
	mspi_write((unsigned char)(addr>>8));
	mspi_wait();
	mspi_write((unsigned char)(addr));
	mspi_wait();
}
/**
 * @brief	  MAC id. Before reading UID of flash, you must read MID of flash. and then you can
 *            look up the related table to select the idcmd and read UID of flash
 * @param[in] buf - store MID of flash
 * @return    none.
 */
_attribute_ram_code_ void flash_read_mid(unsigned char *buf){
	unsigned char j = 0;
	unsigned char r = irq_disable();
	flash_send_cmd(FLASH_GET_JEDEC_ID);
	mspi_write(0x00);		/* dummy,  to issue clock */
	mspi_wait();
	mspi_ctrl_write(0x0a);	/* auto mode */
	mspi_wait();

	for(j = 0; j < 3; ++j){
		*buf++ = mspi_get();
		mspi_wait();
	}
	mspi_high();

	irq_restore(r);
}
/**
 * @brief	  UID. Before reading UID of flash, you must read MID of flash. and then you can
 *            look up the related table to select the idcmd and read UID of flash
 * @param[in] idcmd - get this value to look up the table based on MID of flash
 * @param[in] buf   - store UID of flash
 * @return    none.
 */
_attribute_ram_code_ void flash_read_uid(unsigned char idcmd,unsigned char *buf)
{
	unsigned char j = 0;
	unsigned char r = irq_disable();
	flash_send_cmd(idcmd);
	if(idcmd==FLASH_GD_PUYA_READ_UID_CMD)				//< GD/puya
	{
		flash_send_addr(0x00);
		mspi_write(0x00);		/* dummy,  to issue clock */
		mspi_wait();
	}
	else if (idcmd==FLASH_XTX_READ_UID_CMD)		//< XTX
	{
		flash_send_addr(0x80);
		mspi_write(0x00);		/* dummy,  to issue clock */
		mspi_wait();

	}
	mspi_write(0x00);			/* dummy,  to issue clock */
	mspi_wait();
	mspi_ctrl_write(0x0a);		/* auto mode */
	mspi_wait();

	for(j = 0; j < 16; ++j){
		*buf++ = mspi_get();
		mspi_wait();
	}
	mspi_high();
	irq_restore(r);
}
/**
 * @brief 		 This function serves to read flash mid and uid,and check the correctness of mid and uid.
 * @param[out]   flash_mid - Flash Manufacturer ID
 * @param[out]   flash_uid - Flash Unique ID
 * @return       0:error 1:ok

 */
_attribute_ram_code_ int flash_read_mid_uid_with_check( unsigned int *flash_mid ,unsigned char *flash_uid)
{
	  unsigned char no_uid[16]={0x51,0x01,0x51,0x01,0x51,0x01,0x51,0x01,0x51,0x01,0x51,0x01,0x51,0x01,0x51,0x01};
	  int i,f_cnt=0;
	  unsigned int mid;
	  flash_read_mid((unsigned char*)&mid);
	  mid = mid&0xffff;
	  *flash_mid  = mid;
	 //     	  		 CMD        MID
	 //  GD25LD40C 		 0x4b     0x60c8
	 //  GD25LD05C  	 0x4b 	  0x60c8
	 //  P25Q40L   		 0x4b     0x6085
	 //  MD25D40DGIG	 0x4b     0x4051
	  if( (mid == 0x60C8) || (mid == 0x6085) ||(mid == 0x4051)){
		  flash_read_uid(FLASH_GD_PUYA_READ_UID_CMD,(unsigned char *)flash_uid);
	  }else{
		  return 0;
	  }
	  for(i=0;i<16;i++){
		if(flash_uid[i]==no_uid[i]){
			f_cnt++;
		}
	  }
	  if(f_cnt==16){//no uid flash
			  return 0;

	  }else{
		  return  1;
	  }
}