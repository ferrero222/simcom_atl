/*******************************************************************************
 *                                 v1.0                                        *
 *                           GSM main procedure                                *
 ******************************************************************************/
#ifndef __SIM868_GSM_H
#define __SIM868_GSM_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define SIM_GSM_DATA_BUFF_SIZE  500  

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
  ** @brief Pesponse answer types definitions
**/
typedef enum
{
  SIM_AT_NO = 0x00,  // no parcing requared in the answer message
  SIM_AT1 =   0x01,  // \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
  SIM_AT2 =   0x02,  // REQUEST\r\r\nOK\r\n
  SIM_AT3 =   0x03,  // REQUEST\r\r\nOK\r\n\r\nDATA\r\n
  SIM_AT4 =   0x04,  // REQUEST\r\r\nDATA\r\n
  SIM_AT5 =   0x05,  // \rSENDEDDATA\r\r\nSEND OK\r\n
  SIM_AT6 =   0x06,  // REQUEST\r
  SIM_AT7 =   0x07,  // DATA\r (just geting data)
  SIM_AT8 =   0x08,  // GPRSHEADER\r\nTCP:DATA\r (GPRS full response) 
} sim_at_data_types; 

/**
  ** @brief Phases of ring buffer procces defiinition
**/ 
typedef enum
{
  SIM_GSM_RING_DATA_START =  0x00, //started data
  SIM_GSM_RING_DATA_ENDING = 0x01, //last proc
  SIM_GSM_RING_DATA_DONE =   0xFF, //end of valid data
} sim_gsm_ring_data_proc_status; 

/**
  ** @brief Struct for GSM timers
**/ 
typedef struct sim_gsm_timers_struct 
{
  uint32_t first_pack_tim_1ms;  //timer of waiting first URC stream after power on and start module
  uint8_t first_pack_flag_1ms;
  
  uint32_t wait_pack_tim;  //timer of waiting answer from the module after sending request
  uint8_t wait_pack_flag;
} sim_gsm_timers_struct;

/**
  ** @brief Main struct for GSM module
**/
typedef struct sim_gsm_data_struct 
{
  uint8_t data[SIM_GSM_DATA_BUFF_SIZE];  //recieve valid data from ring buffer
  uint16_t next_index_to_write;         //next index to write in DataIn buffer 
  uint8_t rpt_counter;                  //repeat counter for no answer or incorrect answer  
  uint8_t ring_proc_phase;              //data ring buffer recieve process status @ref sim_ringProcPhaseEn
  uint8_t state;
//-----------------------------
  sim_gsm_timers_struct gsm_timers;
} sim_gsm_data_struct;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
extern sim_gsm_data_struct sim_gsm_data;

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
uint8_t sim_gsm_data_parcer(char* at_comm, char* dest_data, char* answer, uint8_t answ_type);
uint8_t sim_gsm_proc_recieve_data(char* request, char* data, char* answer, uint8_t answ_type);
uint8_t sim_gsm_proc_send_data(char* data, uint32_t timer);
void sim_proc_gsm_data(void);
uint8_t sim_gsm_init(void);
uint8_t sim_gsm_at_message_check(char* request, char* answer, char* data, uint8_t parc_type);
uint8_t sim_gsm_proc_at(char* request, char* answer, char* data, uint8_t parc_type, uint16_t wait_time, uint16_t rpt_cnt);
void sim_usart_gsm_start_tx(char* data);
void sim_usart_gsm_rx_irq_callback(void);
void sim_usart_gsm_tx_irq_callback(void);
void sim_usart_gsm_tx_cmplt_irq_callback(void);
uint8_t sim_gsm_restart(void);
#endif //__SIM868_GSM_H