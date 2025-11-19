/*******************************************************************************
 *                                 v1.0                                        *
 *                         GNSS main procedures                                *
 ******************************************************************************/
#ifndef __SIM868_GNSS_H
#define __SIM868_GNSS_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define SIM_GNSS_DATA_BUF_SIZE 500

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
  ** @brief Pesponse answer types definitions
**/ 
typedef enum
{
  SIM_NMEA_NO = 0x00,   // no
  SIM_NMEA1 =   0x01,   // $NMEAcomm,DATA\r\n
} sim_nmea_data_types; 

/**
  ** @brief Phases of ring buffer procces defiinition
**/    
typedef enum
{
  SIM_GNSS_RING_DATA_START =   0x00,  //check for the start symbol "$"
  SIM_GNSS_RING_DATA_COLLECT = 0x01,  //get data after "$" until end of message "\r\n"
  SIM_GNSS_RING_DATA_ENDING =  0x02,  //last proc after getting "\r\n"
  SIM_GNSS_RING_DATA_DONE =    0xFF,  //valid message was proccess
} sim_gnss_ring_data_proc_status; 

/**
  ** @brief Struct for GNSS timers
**/ 
typedef struct sim_gnss_timers_struct 
{
  uint32_t wait_pack_tim;       //timer of waiting answer from the module after sending request
  uint8_t wait_pack_flag;
  
  uint32_t first_pack_tim_1ms;  //timer of waiting first URC stream after power on and start module
  uint8_t first_pack_flag_1ms;
  
  uint32_t end_of_pack_tim_1ms; //timer of waiting for the end of correct message after geting a start symbol "$"
  uint8_t end_of_pack_flag_1ms;
} sim_gnss_timers_struct;

/**
  ** @brief Main struct for GNSS module
**/ 
typedef struct sim_gnss_data_struct 
{
  uint8_t data[SIM_GNSS_DATA_BUF_SIZE];  //buffer for valid messages from the ring buffer
  uint16_t next_index_to_write;          //next index to write in data buffer
  uint8_t rpt_counter;                   //repeat counter for no answer or incorrect answer  
  uint8_t ring_proc_phase;               //data ring buffer recieve procsess status @sim_gnssRingDataProcStatus
//-----------------------------
  sim_gnss_timers_struct gnss_timers;    //struct for timers
} sim_gnss_data_struct;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
extern sim_gnss_data_struct sim_gnss_data;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
uint8_t sim_xor_gnss_crc_check(void);
uint8_t gnss_nmea_data_parcer(char* nmea_comm, char* dest_data, char* answer, uint8_t answ_type);
uint8_t sim_gnss_proc_recieve_data(char* req, char* data, char* answer, uint8_t answ_type);
uint8_t sim_gnss_proc_send_data(char* data, uint32_t timer);
void sim_gnss_module_off(void);
void sim_proc_gnss_data(void);
void sim_change_gnss_uart_speed(uint32_t speed);
uint8_t sim_gnss_proc_nmea(char* request, char* answer, char* data, uint8_t parc_type, uint8_t wait_time, uint8_t rpt_cnt);
uint8_t sim_gnss_init(void);
uint8_t sim_gnss_get_urc(char* start, char* dest);
void sim_usart_gnss_start_tx(char* data);
void sim_usart_gnss_rx_irq_callback(void);
void sim_usart_gnss_tx_irq_callback(void);
void sim_usart_gnss_tx_cmplt_irq_callback(void);
#endif //__SIM868_GNSS_H