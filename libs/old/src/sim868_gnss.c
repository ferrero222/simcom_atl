/*******************************************************************************
 *                                 v1.0                                        *
 *                         GNSS main procedures                                *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "sim868_gnss.h"
#include "sim868_gsm.h"
#include "sim868_driver.h"
#include "sim868_config.h"
#include <string.h>
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
uint32_t sim_gnss_uart_speed = 9600;
sim_gnss_data_struct sim_gnss_data;

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
 ** \brief  Calculation GNSS data to check CRC using XOR summation
 ** \param  None
 ** \retval Result of checking(0 - wrong, 1 - good)
 ******************************************************************************/ 
uint8_t sim_xor_gnss_crc_check(void)
{
  uint8_t calccrc = 0;
  uint8_t rdycrcH = 0;
  uint8_t rdycrcL = 0;
  uint8_t rdycrc = 0;
  for(int i = 1; i != (sim_gnss_data.next_index_to_write -5); ++i)
  {
    calccrc ^= sim_gnss_data.data[i];
  }
  rdycrcH = sim_gnss_data.data[sim_gnss_data.next_index_to_write -4] - 0x30;
  if (rdycrcH > 9) rdycrcH -= 7;
  rdycrcL =  sim_gnss_data.data[sim_gnss_data.next_index_to_write -3] - 0x30;
  if (rdycrcL > 9) rdycrcL -= 7;
  rdycrc = ((rdycrcH & 0x0F) << 4) | (rdycrcL & 0x0F);
  if(calccrc != rdycrc) return 0;
  else                  return 1;
}

/*******************************************************************************
 ** \brief Procedure for parcing input data from gnss module
 ** \param nmea_comm: pointer to char array of full user NMEA command which should be send
 ** \param dest_data: pointer to char array into which the response correct data is copied
 ** \param answer: pointer to char array of expecting answer which will be compare with data in response
 ** \param answ_type: type of answer message from gnns module. Using to understand how to parcing 
 **                   this answer message. This parameter can be one of the following values:
 **        @arg SIM_NMEA_NO: no parcing requared in the answer message
 **        @arg SIM_NMEA1: answer will be looks like $nmea_comm,DATA\r\n 
 ** \retval 0(error), 1(done)
 ******************************************************************************/ 
uint8_t gnss_nmea_data_parcer(char* nmea_comm, char* dest_data, char* answer, uint8_t answ_type)
{
  uint8_t temp_index = sim_gsm_data.next_index_to_write;
  uint8_t res = 0;
  switch(answ_type)
  {
///*****************************************************************************
     case SIM_NMEA_NO:
          return res;
///*****************************************************************************
     case SIM_NMEA1: //$nmea_comm,DATA\r\n
        while(temp_index)
        {
          if(*(sim_gsm_data.data +temp_index) == '$') 
          {
            if(answer)
            {
              if(memcmp(answer, sim_gsm_data.data +temp_index + strlen(nmea_comm) +1, sim_gsm_data.next_index_to_write) == 0) res = 1;
            }
            else res = 1;
            if(dest_data) 
            {
              memcpy(dest_data, sim_gsm_data.data +temp_index + strlen(nmea_comm) +1, sim_gsm_data.next_index_to_write);
            }
          }
          --temp_index; 
        }
        return res;
///*****************************************************************************
  }
  return 0;
}


/*******************************************************************************
 ** \brief  Procedure for getting one of the URC streams from GNSS module
 ** \param  start: pointer to char array which store a header of expecting URC. 
 **                Example:"$GNRMC"
 ** \param  dest: pointer to char array where gotten data should be placed. 
 **               This should have enough lenght to put there whole expecting 
 **               URC data.
 ** \retval Procedure return status, which can be:
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten data have incorrect CRC
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: "start" param header doesnt match to current gotten data header 
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly
 ******************************************************************************/ 
uint8_t sim_gnss_get_urc(char* start, char* dest)
{
  uint8_t res = SIM_PROC_WORKING;
  if(sim_gnss_data.ring_proc_phase == SIM_GNSS_RING_DATA_DONE)
  {
    if(memcmp(start, sim_gnss_data.data, strlen(start)) == 0)
    {
      if(sim_xor_gnss_crc_check()) 
      {
        memcpy(dest, sim_gnss_data.data + strlen(start) +1, sim_gnss_data.next_index_to_write - (strlen(start) +1) -2);
        res = SIM_PROC_DONE;
      }
      else res = SIM_INCORRECT_DATA;
    }
    else res = SIM_NO_CORRECT_ANSWER;
    memset(sim_gnss_data.data, 0, SIM_GNSS_DATA_BUF_SIZE);
    sim_gnss_data.next_index_to_write = 0;
    sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_START;
  }
  return res;
}

/*******************************************************************************
 ** \brief  Procedure for recieving data after sending at command
 ** \param  req: pointer to char array of full user request which should be send
 ** \param  data: pointer to char array into which the response correct data is copied
 ** \param  answer: pointer to char array of expecting answer which will be compare with data in response
 ** \param  answ_type: type of answer message from gnns module. Using to understand how to parcing 
 **                   this answer message. This parameter can be one of the following values:
 **         @arg SIM_NMEA_NO: no parcing requared in the answer message
 **         @arg sim_NMEA1: answer will be looks like $NMEAcomm,DATA\r\n   
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (ERROR) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet
 **         @arg (DONE) SIM_PROC_WORKING: Procedure is done, continues working correctly.
 ******************************************************************************/ 
uint8_t sim_gnss_proc_recieve_data(char* req, char* data, char* answer, uint8_t answ_type)
{
  if(answ_type == SIM_NMEA_NO) return SIM_PROC_WORKING;
  if(sim_gnss_data.ring_proc_phase != SIM_GNSS_RING_DATA_DONE) return SIM_NO_CORRECT_ANSWER;
  sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_START;
  if(!gnss_nmea_data_parcer(req, data, answer, answ_type)) return SIM_INCORRECT_DATA;
  else                                                     return SIM_PROC_WORKING;
}

/*******************************************************************************
 ** \brief Procedure for sending data to gnss module
 ** \param data: pointer to char array which should be send to module
 ** \param timer: time in 100ms for waiting correct answer after sending data
 **               before trying to resend data.
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (DONE) SIM_PROC_WORKING: Procedure is done, continues working correctly.
 ******************************************************************************/ 
uint8_t sim_gnss_proc_send_data(char* data, uint32_t timer)
{
  if(sim_usart_gnss_ring_buff.tx.tx_status == SIM_TX_PROCCESS) return SIM_BUSY_MODULE;
  if(sim_gnss_data.rpt_counter) 
  {
    --sim_gnss_data.rpt_counter;
    sim_gnss_data.gnss_timers.wait_pack_tim = timer;  
    sim_usart_gnss_start_tx(data);
    return SIM_PROC_WORKING;
  }
  else
  {
    return SIM_NO_RPT;
  }
}

/*******************************************************************************
 ** \brief  Procedure to work with gnss ring buffer. Getting correct meassages from there.
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_proc_gnss_data(void)
{
  if(sim_gnss_data.gnss_timers.end_of_pack_flag_1ms)
  {
    sim_gnss_data.gnss_timers.end_of_pack_flag_1ms = 0;
    memset(sim_gnss_data.data, 0, SIM_GNSS_DATA_BUF_SIZE);
    sim_gnss_data.next_index_to_write = 0;
    sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_START;
  }
  while(sim_usart_gnss_ring_buff.rx.next_index_to_read != sim_usart_gnss_ring_buff.rx.next_index_to_write)
  {
    switch(sim_gnss_data.ring_proc_phase) //data proc status
    {
//******************************************************************************/
      case SIM_GNSS_RING_DATA_START:
           if(sim_usart_gnss_ring_buff.rx.buffer[sim_usart_gnss_ring_buff.rx.next_index_to_read] == '$') 
           {
             sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_COLLECT;
             sim_gnss_data.data[sim_gnss_data.next_index_to_write] = sim_usart_gnss_ring_buff.rx.buffer[sim_usart_gnss_ring_buff.rx.next_index_to_read];
             sim_gnss_data.gnss_timers.end_of_pack_tim_1ms = SIM_WAIT_PACK_TIM_DEF;
             ++sim_gnss_data.next_index_to_write;
             if(sim_gnss_data.next_index_to_write >= SIM_GNSS_DATA_BUF_SIZE) sim_gnss_data.next_index_to_write = 0;
             ++sim_usart_gnss_ring_buff.rx.next_index_to_read;
             if(sim_usart_gnss_ring_buff.rx.next_index_to_read >= SIM_RX_BUFF_SIZE) sim_usart_gnss_ring_buff.rx.next_index_to_read = 0;
           }
           else
           {
             ++sim_usart_gnss_ring_buff.rx.next_index_to_read;
             if(sim_usart_gnss_ring_buff.rx.next_index_to_read >= SIM_RX_BUFF_SIZE) sim_usart_gnss_ring_buff.rx.next_index_to_read = 0;
           }
           break;
//******************************************************************************/
      case SIM_GNSS_RING_DATA_COLLECT:
           sim_gnss_data.data[sim_gnss_data.next_index_to_write] = sim_usart_gnss_ring_buff.rx.buffer[sim_usart_gnss_ring_buff.rx.next_index_to_read];
           ++sim_gnss_data.next_index_to_write;
           if(sim_gnss_data.next_index_to_write >= SIM_GNSS_DATA_BUF_SIZE) sim_gnss_data.next_index_to_write = 0;
           ++sim_usart_gnss_ring_buff.rx.next_index_to_read;
           if(sim_usart_gnss_ring_buff.rx.next_index_to_read >= SIM_RX_BUFF_SIZE) sim_usart_gnss_ring_buff.rx.next_index_to_read = 0;
           sim_gnss_data.gnss_timers.end_of_pack_tim_1ms = SIM_WAIT_PACK_TIM_DEF;
           if(sim_gnss_data.data[sim_gnss_data.next_index_to_write -1] == '\n' && sim_gnss_data.data[sim_gnss_data.next_index_to_write -2] == '\r') sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_ENDING;
           else break;
//******************************************************************************/
      case SIM_GNSS_RING_DATA_ENDING:
           sim_gnss_data.gnss_timers.first_pack_tim_1ms = 0;
           sim_gnss_data.gnss_timers.end_of_pack_tim_1ms = 0;
           sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_DONE;
           return;
//******************************************************************************/
      default: return;
    }
  }
}

/*******************************************************************************
 ** \brief  Starting tx data to gnss UART with busy control
 ** \param  data: Pointer char array of data which should be send.
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gnss_start_tx(char* data)
{
  if(sim_usart_gnss_ring_buff.tx.tx_status == SIM_TX_PROCCESS) return;
  memcpy(sim_usart_gnss_ring_buff.tx.buffer, data, strlen(data));
  sim_usart_gnss_ring_buff.tx.counter = strlen(data);
  sim_usart_gnss_ring_buff.tx.tx_status = SIM_TX_PROCCESS;
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_TX_EMPTY_INT, Enable);
  #ifdef SIM_USART_TX 
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_TX, Enable);
  #endif
}

/*******************************************************************************
 ** \brief  Rx callback for gnss UART
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gnss_rx_irq_callback(void) /* rx IRQ */
{ 
  sim_usart_gnss_ring_buff.rx.buffer[sim_usart_gnss_ring_buff.rx.next_index_to_write] = (uint8_t)SIM_USART_RECDATA(SIM_USARTGNSS_CH);
  ++sim_usart_gnss_ring_buff.rx.next_index_to_write;
  if(sim_usart_gnss_ring_buff.rx.next_index_to_write >= SIM_RX_BUFF_SIZE) sim_usart_gnss_ring_buff.rx.next_index_to_write = 0; 
}

/*******************************************************************************
 ** \brief  Tx callback for gnss UART
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gnss_tx_irq_callback(void) /* TX in proc IRQ */
{
  SIM_USART_SENDDATA(SIM_USARTGNSS_CH, (uint16_t)(*(sim_usart_gnss_ring_buff.tx.buffer +sim_usart_gnss_ring_buff.tx.ind)));
  ++sim_usart_gnss_ring_buff.tx.ind;
  --sim_usart_gnss_ring_buff.tx.counter;
  if(!sim_usart_gnss_ring_buff.tx.counter)
  {
    SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_TX_EMPTY_INT, Disable);
    SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_TX_CMPLT_INT, Enable);
  }
}

/*******************************************************************************
 ** \brief  Tx complete callback for gnss UART
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gnss_tx_cmplt_irq_callback(void) /* TX Complete IRQ */
{
  memset(sim_usart_gnss_ring_buff.tx.buffer, 0, SIM_TX_BUFF_SIZE);
  sim_usart_gnss_ring_buff.tx.ind = 0;
  sim_usart_gnss_ring_buff.tx.counter = 0;
  sim_usart_gnss_ring_buff.tx.tx_status = SIM_TX_DONE;
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_TX_CMPLT_INT, Disable);
  #ifdef SIM_USART_TX 
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_TX, Disable);
  #endif
}

/*******************************************************************************
 ** \brief  Procedure for sending NMEA message to GNSS module and get response answer
 ** \param  request: pointer to char array of full user request which should be send
 ** \param  answer: pointer to char array of expecting answer which will be compare with data in response
 ** \param  data: pointer to char array into which the response correct data is copied
 ** \param  parc_type: type of answer message from gnns module. Using to understand how to parcing 
 **                   this answer message. This parameter can be one of the following values:
 **         @arg sim_NMEA_no: no parcing requared in the answer message
 **         @arg sim_NMEA1: answer will be looks like $NMEAcomm,DATA\r\n   
 ** \param  wait_time: time in 100ms for waiting correct answer after sending data
 **                   before trying to resend data.
 ** \param  rpt_cnt: Amount of resend repeats if error occurs.
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering.
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending 
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ** \note "request" param can be NULL if you want just to get any data without sending some.
 **       "data" param can be NULL if you don want to copy correct data from response answer
 **       if they are exist. "answer" param can be NULL if you dont need to compare response data
 **       with some of your expecting data.
 ******************************************************************************/   
uint8_t sim_gnss_proc_nmea(char* request, char* answer, char* data, uint8_t parc_type, uint8_t wait_time, uint8_t rpt_cnt)
{
  static uint8_t phase = 0;
  uint8_t res = SIM_UNKNOWN_STATUS;
  if(sim_gnss_data.gnss_timers.wait_pack_flag)
  {
    memset(sim_gnss_data.data, 0, SIM_GNSS_DATA_BUF_SIZE);
    sim_gnss_data.next_index_to_write = 0;
    sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_START;
    phase = 1;
    sim_gnss_data.gnss_timers.wait_pack_flag = 0;
  }
  switch(phase)
  {
    case 0:
         sim_gnss_data.rpt_counter = rpt_cnt;
         ++phase;
    case 1:
         if(request) 
         {
           res = sim_gnss_proc_send_data(request, wait_time);
           if(res == SIM_NO_RPT) 
           {
             phase = 0;
             return res;
           }
         }
         else res = SIM_PROC_WORKING;
         ++phase;
         return res;
    case 2:
         if(sim_usart_gnss_ring_buff.tx.tx_status == SIM_TX_PROCCESS) return SIM_NO_CORRECT_ANSWER;    
         res = sim_gnss_proc_recieve_data(request, data, answer, parc_type);
         if(res != SIM_PROC_WORKING) return res; 
         sim_gnss_data.gnss_timers.wait_pack_tim = 0;
         memset(sim_gnss_data.data, 0, SIM_GNSS_DATA_BUF_SIZE);
         sim_gnss_data.next_index_to_write = 0;
         phase = 0;
         res = SIM_PROC_DONE;
         return res;
  }
  return(res);

}

/*******************************************************************************
 ** \brief  Procedure for change gnss uart speed
 ** \param  speed: speed of uart
 ** \retval None
 ******************************************************************************/   
void sim_change_gnss_uart_speed(uint32_t speed)
{
  #ifdef SIM_USART_RX
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_RX, SIM_FUNC_DISABLE);
  #endif
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_RX_INT, SIM_FUNC_DISABLE);
  if(SIM_USART_SETBAUDRATE(SIM_USARTGNSS_CH, 115200) != Ok)
  {
    while(1)
    {
    }
  }
  #ifdef SIM_USART_RX
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_RX, SIM_FUNC_ENABLE);
  #endif
  SIM_USART_FUNCTCMD(SIM_USARTGNSS_CH, SIM_USART_RX_INT, SIM_FUNC_ENABLE);
  sim_gnss_uart_speed = speed;
}

/*******************************************************************************
 ** \brief  Procedure for start GNSS module and set correct UART speed
 ** \param  None
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/   
uint8_t sim_gnss_init(void)
{
  static uint8_t phase = 0;
  switch(phase)
  {
    case 0: //power on
          #ifdef SIM_GPSVBAT_RESET
          SIM_GPSVBAT_RESET; //gps power on
          #endif
          ++phase;
          return SIM_PROC_WORKING;  
///*****************************************************************************
    case 1:
          SIM_GNSSEN_TOGGLE; //gnss module on
          sim_gnss_data.gnss_timers.first_pack_tim_1ms = SIM_WAIT_PACK_TIM_DEF;
          memset(sim_gnss_data.data, 0, SIM_GNSS_DATA_BUF_SIZE);
          sim_gnss_data.next_index_to_write = 0;
          sim_gnss_data.ring_proc_phase = SIM_GNSS_RING_DATA_START;
          sim_usart_gnss_ring_buff.rx.next_index_to_read = sim_usart_gnss_ring_buff.rx.next_index_to_write;
          phase = 0;
          return SIM_PROC_DONE;    
    }
  return SIM_UNKNOWN_STATUS;
}