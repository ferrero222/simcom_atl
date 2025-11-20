/**
  ******************************************************************************
  * @file    sim868_driver.c
  * @version V0.0.1
  * @date    14-february-2024
  * @brief   This file contains all the functions prototypes for the USART 
  *          firmware library.
  ******************************************************************************
  */ 
#ifndef __SIM868_DRIVER_H
#define __SIM868_DRIVER_H

/*******************************************************************************
 * Include files(driver)
 ******************************************************************************/
#include <stdint.h>
#include "sim868_config.h"
#include "sim868_gnss.h"
#include "sim868_gsm.h"
#include "sim868_gprs.h"
#include "sim868_config.h"
#include "sim868_crc.h"
#include "sim868_ble.h"

/*******************************************************************************
 * Pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define SIM_TX_BUFF_SIZE        100
#define SIM_RX_BUFF_SIZE        1000
#define SIM_WAIT_PACK_TIM_DEF   25   //2.5sec waiting for the end of pack

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
  ** @brief status 
**/ 
typedef enum
{
  SIM_PROC_DONE =         0x00, //Complex multiproccess was done
  SIM_PROC_WORKING =      0x02, //Proccess is working
  SIM_RECIEVING_DATA =    0x12, //Module recieving eternal data  

  SIM_PROC_UNAVAILABLE =  0xF0, //Proccess is working
  SIM_BUSY_MODULE =       0xF1, //Module is busy
  SIM_NO_RPT =            0xF2, //No more repeats for request
  SIM_NO_CORRECT_ANSWER = 0xF3, //No correct eternal data was recieved for all repeats
  SIM_INCORRECT_DATA =    0xF4, //Incorrect eternal data was recievd
  SIM_MODULE_NO_ANSWER =  0xF5, //Module is not asnwering
  SIM_UNKNOWN_STATUS =    0xF6, //Unknown status
  SIM_WRONG_PARAM =       0xF7, //Wrong param
} sim_lib_module_status; 

/**
  ** @brief phases for ring buffers
**/ 
typedef enum
{
  SIM_TX_NONE =     0x00,  //no transimittion
  SIM_TX_PROCCESS = 0x01,  //transmittion in progress
  SIM_TX_DONE =     0x02,  //transmittion is done
} sim_tx_status;

/**
  ** @brief Struct for ring RX buffers
**/ 
typedef struct sim_ring_rx_buf_struct 
{
  uint8_t buffer[SIM_RX_BUFF_SIZE];
  uint16_t next_index_to_write;
  uint16_t next_index_to_read;
} sim_ring_rx_buff_struct;

/**
  ** @brief Struct for TX ring buffers
**/ 
typedef struct sim_ring_tx_buf_struct 
{
  uint8_t buffer[SIM_TX_BUFF_SIZE];
  uint16_t ind;
  uint16_t counter;
  uint8_t tx_status;
} sim_ring_tx_buff_struct;

/**
  ** @brief main struct for ring buffers
**/ 
typedef struct sim_ring_buffer_struct
{
  sim_ring_tx_buff_struct tx;
  sim_ring_rx_buff_struct rx;
}sim_ring_buffer_struct;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
extern uint8_t sim_uart_speed;
extern sim_ring_buffer_struct sim_usart1_gsm_ring_buff;
extern sim_ring_buffer_struct sim_usart_gnss_ring_buff;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
void sim_timer_proc(void);
void sim_drv_loop_proc(void);
char sim_convert_half_byte_to_ascii(char half_byte);
void sim_data_dec_to_ascii(int* data, char* accum, uint16_t* ascii_len);

#endif //__SIM868_DRIVER_H
