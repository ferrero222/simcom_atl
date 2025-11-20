/**
  ******************************************************************************
  * @file    sim868_driver.c
  * @version V0.0.1
  * @date    14-february-2024
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the SIM868    
  *          ===================================================================
  *                                 How to use this driver
  *          ===================================================================
  *           1. Initializate CPU GPIO and USART`S to work with SIM chip. 
  *              Set USART IRQ callbacks with drv callbacks such is:
  *              @ref sim_usart_gnss_rx_irq_callback
  *              @ref sim_usart_gnss_tx_irq_callback
  *              @ref sim_usart_gnss_tx_cmplt_irq_callback 
  *              @ref sim_usart_gsm_rx_irq_callback
  *              @ref sim_usart_gsm_tx_irq_callback
  *              @ref sim_usart_gsm_tx_cmplt_irq_callback
  *              GSM UART speed is optional. GNSS should be 9600.
  *
  *           2. Provide access to control the user device hardware by 
  *              uncommenting and defining macros in sim868_config.h 
  *              "Pre-processor symbols/macros" depends on the 
  *              description and example of each macros. Link user headers
  *              in "Include files(user)":
  *              @ref uart_channels_macro
  *              @ref uart_interrupts_macro
  *              @ref uart_comm_funct_macro
  *              @ref pinout_function_macro
  *              @ref other_function_macro
  *              If definition have mark "OPTIONAL" in description, that means this definiton 
  *              may or may not be determined
  *
  *           3. Create and define macro of 100ms timer flag variable. Variable 
  *              is should be set when user app counts to 100ms. 
  *              @ref uart_pointers_macro 
  *
  *           3. Place sim_drv_loop_proc() into main loop cycle.
  *
  *           4. When using GSM module:
  *                  - Configure and start module using sim_gsm_init().
  *                  - Use sim_gsm_restart() for restart GSM module.
  *
  *                  - Use sim_gsm_proc_AT() to send one single command to module and get response.
  *
  *                  - Use sim_gprs_start_init() to initializate GPRS
  *                  - Use sim_gprs_connect() to set GPRS param and connect with remote server.
  *                  - Use sim_gprs_send_data() to send single message to remote server and get expected answer.
  *
  *                  - Use sim_ble_init() to power on BLE.
  *                  - Use sim_ble_deinit() to power off BLE.
  *                  - Use sim_ble_server_init_and_advert() to init GATT server and start advertising data(manufacture data).
  *                  - Use sim_ble_client_int_and_scan() for init GATT client and start scanning surrounding devices.
  *
  *              @note GPRS functions have ability to automaticly reconnect with server if some 
  *                    issue occurs. 
  *
  *           5. When using GNSS module:
  *                  - Configure and start module using sim_gnss_init().
  *                  - Use sim_gnss_get_urc() to get one of the NMEA URC streams.
  *                  - Use sim_gnss_proc_nmea() to send one single command to module.
  *        
  *
  * @note - If some function return ERROR status, this function will start proc again, if
  *         user didnt realize some errors handles.
  *       - You can check sim_usart1_gsm_ring_buff or sim_UsartGNSSingBuff ring buffers
  *         to see what kind of data was recieved or transfered last time. It can be useful if you 
  *         dont know what king of answer type you need to set while sending request to module
  *       - When working simultaneously with several applications using the same module port 
  *         (for example, simultaneously GPRS and BLE via one AT port), it is necessary to implement 
  *         consistent port access control for application data at the user level to avoid errors 
  *         and collisions in the operation of the each application(use custom mutex or switch(phase)).
  ******************************************************************************
  */ 
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "sim868_driver.h"
#include <string.h>
#include <stdio.h>
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
sim_ring_buffer_struct sim_usart1_gsm_ring_buff;
sim_ring_buffer_struct sim_usart_gnss_ring_buff;

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Procedure for work wit programm timers
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_timer_proc(void)
{
  if(!(*SIM_TIMER100MS_PTR)) return;
  *SIM_TIMER100MS_PTR = 0;
  if(sim_gsm_data.gsm_timers.wait_pack_tim)
  {
    if(--sim_gsm_data.gsm_timers.wait_pack_tim == 0) sim_gsm_data.gsm_timers.wait_pack_flag = 1;
  }  
//******************************************************************************
  if(sim_gnss_data.gnss_timers.wait_pack_tim)
  {
    if(--sim_gnss_data.gnss_timers.wait_pack_tim == 0) sim_gnss_data.gnss_timers.wait_pack_flag = 1;
  }  
  if(sim_gnss_data.gnss_timers.end_of_pack_tim_1ms) 
  {
    if(--sim_gnss_data.gnss_timers.end_of_pack_tim_1ms == 0) sim_gnss_data.gnss_timers.end_of_pack_flag_1ms = 1;
  }
  if(sim_gnss_data.gnss_timers.first_pack_tim_1ms)
  {
    if(--sim_gnss_data.gnss_timers.first_pack_tim_1ms == 0) sim_gnss_data.gnss_timers.first_pack_flag_1ms = 1;
  }
//******************************************************************************
  if(sim_ble_timers.scann_timer)
  {
    if(--sim_ble_timers.scann_timer == 0) sim_ble_timers.scann_timer_flag = 1;
  }
}

/*******************************************************************************
 ** \brief Convertation array of DEC data to ASCII format
 ** \param data: Pointer to data
 ** \param accum: temp array of 5 elements to calc, len of data(5 max)
 ** \param ascii_len: lenght of data
 ** \retval None
 ******************************************************************************/ 
void sim_data_dec_to_ascii(int* data, char* accum, uint16_t* ascii_len)
{
  sprintf(accum, "%d", *data);
  uint8_t temp_cnt = 0;
  while(temp_cnt < 5)
  {
    if(*(accum +temp_cnt) == 0x00) break;
    ++temp_cnt;
  }
  *ascii_len = temp_cnt;
}

/*******************************************************************************
 ** \brief Convertation half byte DEC to ASCII
 ** \param half_byte: 4bits of 8bits variable
 ** \retval half_byte ascii symbol
 ******************************************************************************/ 
char sim_convert_half_byte_to_ascii(char half_byte) 
{
  if (half_byte <= 9)                          return half_byte + '0'; 
  else if (half_byte >= 10 && half_byte <= 15) return half_byte - 10 + 'A'; 
  else                                         return '\0'; 
}

/*******************************************************************************
 ** \brief  Main Procedure for driver
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_drv_loop_proc(void)
{
  sim_timer_proc();
  sim_proc_gnss_data();
  sim_proc_gsm_data();
}