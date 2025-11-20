/*******************************************************************************
 *                                 v1.0                                        *
 *                           GSM main procedure                                *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "sim868_gsm.h"
#include "sim868_gnss.h"
#include "sim868_gprs.h"
#include "sim868_config.h"
#include "sim868_driver.h"
#include <stdio.h>
#include <string.h>
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
sim_gsm_data_struct sim_gsm_data;
uint32_t sim_gsm_uart_speed = 9600;

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
 ** \brief  Procedure for parcing input data from gsm module
 ** \param  at_comm: pointer to char array of full user AT command which should be send
 ** \param  dest_data: pointer to char array into which the response correct data is copied
 ** \param  answer: pointer to char array of expecting answer which will be compare with data in response
 ** \param  answ_type: type of answer message from gsm module. Using to understand how to parcing 
 **                   this answer message. This parameter can be one of the following values:
 **         @arg SIM_AT_NO: no parcing requared in the answer message
 **         @arg SIM_AT1: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
 **         @arg SIM_AT2: REQUEST\r\r\nOK\r\n
 **         @arg SIM_AT3: REQUEST\r\r\nOK\r\n\r\nDATA\r\n
 **         @arg SIM_AT4: REQUEST\r\r\nDATA\r\n
 **         @arg SIM_AT5: \rSENDEDDATA\r\r\nSEND OK\r\n
 **         @arg SIM_AT6: REQUEST\r
 **         @arg SIM_AT7: DATA\r 
 **         @arg SIM_AT8: GPRSHEADER\r\nTCP:DATA\r (GPRS full response)
 ** \retval 0(error), 1(done)
 ******************************************************************************/ 
uint8_t sim_gsm_data_parcer(char* at_comm, char* dest_data, char* answer, uint8_t answ_type)//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ERROR
{
  char resp_ok[] = "\r\nOK\r\n";
  char resp_error[] = "\r\nERROR\r\n";
  char data_resp_ok[] = "\r\nSEND OK\r\n";
  uint8_t temp_index = sim_gsm_data.next_index_to_write;
  uint8_t res = 1;
  switch(answ_type)
  {
///*****************************************************************************
    case SIM_AT_NO: 
         return res;
///*****************************************************************************
    case SIM_AT1: // \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
         {
           uint8_t temp_data_len = 0;
           if(memcmp(sim_gsm_data.data +temp_index -(strlen(resp_ok)), resp_ok, strlen(resp_ok)) == 0) 
           {
             temp_index -= strlen(resp_ok);
           }
           else if(memcmp(sim_gsm_data.data +temp_index -(strlen(resp_error)), resp_error, strlen(resp_error)) == 0) 
           {
             res = 0;
             temp_index -= strlen(resp_error);
           }
           else 
           {
             return 0;
           }
           uint8_t temp_R_counter = 0;
           while(temp_R_counter < 2)
           {
             if(sim_gsm_data.data[temp_index -1] == '\r') ++temp_R_counter;
             --temp_index;
             if(temp_index == 0) return 0;
             ++temp_data_len;
           }
           if(temp_R_counter != 2) return 0;
           temp_data_len -= 4;
           if(at_comm)
           {
             if(memcmp(sim_gsm_data.data +temp_index - strlen(at_comm), at_comm, strlen(at_comm)) != 0) return 0;
           }
           if(answer)
           {
             char temp[100] = {0};
             memcpy(temp, sim_gsm_data.data +temp_index +2, temp_data_len);
             temp[temp_data_len] = '\0';
             if(strstr(answer, temp) == NULL && strstr(temp, answer) == NULL) res = 0;
             else res = 1;
           }
           if(dest_data)
           {
             memcpy(dest_data, sim_gsm_data.data +temp_index +2, temp_data_len);
           }
           return res;
         }
///*****************************************************************************
    case SIM_AT2: // REQUEST\r\r\nOK\r\n
         {
           if(memcmp(sim_gsm_data.data +temp_index -(strlen(resp_ok)), resp_ok, strlen(resp_ok)) == 0) 
           {
             temp_index -= strlen(resp_ok);
           }
           else if(memcmp(sim_gsm_data.data +temp_index -(strlen(resp_error)), resp_error, strlen(resp_error)) == 0) 
           {
             res = 0;
             temp_index -= strlen(resp_error);
           }
           else 
           {
             return 0;
           }
           if(at_comm)
           {
             if(memcmp(sim_gsm_data.data +temp_index -strlen(at_comm), at_comm, strlen(at_comm)) != 0) return 0;
           }
           return res;
         }
///*****************************************************************************
    case SIM_AT3: // REQUEST\r\r\nOK\r\n\r\nDATA\r\n
         {
           uint8_t temp_data_len = 0;
           uint8_t temp_R_counter = 0;
           while(temp_R_counter < 2)
           {
             if(sim_gsm_data.data[temp_index -1] == '\r') ++temp_R_counter;
             --temp_index;
             if(temp_index == 0) return 0;
             ++temp_data_len;
           }
           temp_data_len -= 4;
           if(memcmp(sim_gsm_data.data +temp_index -(strlen(resp_ok)), resp_ok, strlen(resp_ok)) == 0) 
           {
             temp_index -= strlen(resp_ok);
           }
           else if(memcmp(sim_gsm_data.data +temp_index -(strlen(resp_error)), resp_error, strlen(resp_error)) == 0) 
           {
             res = 0;
             temp_index -= strlen(resp_error);
           }
           else 
           {
             return 0;
           }
           if(at_comm)
           {
             if(memcmp(sim_gsm_data.data +temp_index - strlen(at_comm), at_comm, strlen(at_comm)) != 0) return 0;
           }
           if(answer)
           {
             char temp[100] = {0};
             if(res)
             {
               memcpy(temp, sim_gsm_data.data +temp_index +(strlen(resp_ok)) +2, temp_data_len);
               if(strstr(answer, temp) == NULL && strstr(temp, answer) == NULL) res = 0;
               else res = 1;
             }
             else   
             {
               memcpy(temp, sim_gsm_data.data +temp_index +(strlen(resp_error)) +2, temp_data_len);
               if(strstr(answer, temp) == NULL && strstr(temp, answer) == NULL) res = 0;
               else res = 1;
             }
           }
           if(dest_data)
           {
             if(res) memcpy(dest_data, sim_gsm_data.data +temp_index +(strlen(resp_ok)) +2, temp_data_len);
             else    memcpy(dest_data, sim_gsm_data.data +temp_index +(strlen(resp_error)) +2, temp_data_len);
           }
           return res;
         }
///*****************************************************************************
    case SIM_AT4: // REQUEST\r\r\nDATA\r\n
         {
           uint8_t temp_data_len = 0;
           uint8_t temp_R_counter = 0;
           while(temp_R_counter < 2)
           {
             if(sim_gsm_data.data[temp_index -1] == '\r') ++temp_R_counter;
             --temp_index;
             if(temp_index == 0) return 0; 
             ++temp_data_len;
           }
           temp_data_len -= 4;
           if(at_comm)
           {
             if(memcmp(sim_gsm_data.data +temp_index -strlen(at_comm), at_comm, strlen(at_comm)) != 0) return 0;
           }
           if(answer)
           {
             char temp[100] = {0};
             memcpy(temp,  sim_gsm_data.data +temp_index +2, temp_data_len);
             if(strstr(answer, temp) == NULL && strstr(temp, answer) == NULL) res = 0;
           }
           if(dest_data) 
           {
             memcpy(dest_data, sim_gsm_data.data +temp_index +2, temp_data_len);
           }
           return res;
         }
///*****************************************************************************
    case SIM_AT5: // \rSENDEDDATA\r\nSEND OK\r\n
         {
           if(memcmp(sim_gsm_data.data +temp_index -(strlen(data_resp_ok)), data_resp_ok, strlen(data_resp_ok)) != 0) res = 0;
           temp_index -= strlen(data_resp_ok);
           if(at_comm)
           {
             if(memcmp(sim_gsm_data.data +temp_index -strlen(at_comm), at_comm, strlen(at_comm)) != 0) res = 0;
           }
           return res;
         }
///*****************************************************************************
    case SIM_AT6: // SENDEDDATA\r\r\n
         { 
           if(at_comm)
           {
             if(memcmp(sim_gsm_data.data +temp_index -strlen(at_comm) -2, at_comm, strlen(at_comm)) != 0) res = 0;
           }
           return res;
         }
///*****************************************************************************
    case SIM_AT7: // DATA\r
         { 
           if(answer)
           {
             char temp[200] = {0};
             memcpy(temp, sim_gsm_data.data, sim_gsm_data.next_index_to_write);
             if(strstr(answer, temp) == NULL && strstr(temp, answer) == NULL) res = 0;
           }
           if(dest_data) 
           {
             memcpy(dest_data, sim_gsm_data.data, sim_gsm_data.next_index_to_write);
           }
           return res;
         }
///*****************************************************************************
    case SIM_AT8: // GPRSHEADER\r\nTCP:DATA\r
         { 
           uint8_t temp_ind = sim_gsm_data.next_index_to_write;
           while(temp_ind)
           {
             if(sim_gsm_data.data[temp_ind] == ':') 
             {
               if(answer)
               {
                 char temp[200] = {0};
                 memcpy(temp, sim_gsm_data.data +temp_ind +1, sim_gsm_data.next_index_to_write - temp_ind);
                 if(strstr(temp, answer) == NULL) res = 0;
               }
               if(dest_data) memcpy(dest_data, sim_gsm_data.data +temp_ind +1, sim_gsm_data.next_index_to_write - temp_ind);
               break;
             }
             --temp_ind;
             if(!temp_ind) return 0;
           }
           return res;
         }
  }
  return 0;
}

/*******************************************************************************
 ** \brief  Starting tx data to gsm UART with busy control
 ** \param  data: Pointer char array of data which should be send.
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gsm_start_tx(char* data)
{
  if(sim_usart1_gsm_ring_buff.tx.tx_status == SIM_TX_PROCCESS) return;
  memcpy(sim_usart1_gsm_ring_buff.tx.buffer, data, strlen(data));
  sim_usart1_gsm_ring_buff.tx.counter = strlen(data);
  sim_usart1_gsm_ring_buff.tx.tx_status = SIM_TX_PROCCESS;
  SIM_USART_FUNCTCMD(SIM_USARTGSM_CH, SIM_USART_TX_EMPTY_INT, Enable);
  #ifdef SIM_USART_TX 
  SIM_USART_FUNCTCMD(SIM_USARTGSM_CH, SIM_USART_TX, Enable);
  #endif
}

/*******************************************************************************
 ** \brief  Rx callback for gnss UART
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gsm_rx_irq_callback(void) /* RX IRQ */
{ 
  sim_usart1_gsm_ring_buff.rx.buffer[sim_usart1_gsm_ring_buff.rx.next_index_to_write] = (uint8_t)SIM_USART_RECDATA(SIM_USARTGSM_CH);
  ++sim_usart1_gsm_ring_buff.rx.next_index_to_write;
  if(sim_usart1_gsm_ring_buff.rx.next_index_to_write >= SIM_RX_BUFF_SIZE) sim_usart1_gsm_ring_buff.rx.next_index_to_write = 0; 
}

/*******************************************************************************
 ** \brief  Tx callback for gnss UART
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gsm_tx_irq_callback(void) /* TX in proc IRQ */
{
  SIM_USART_SENDDATA(SIM_USARTGSM_CH, (uint16_t)(*(sim_usart1_gsm_ring_buff.tx.buffer +sim_usart1_gsm_ring_buff.tx.ind)));
  ++sim_usart1_gsm_ring_buff.tx.ind;
  --sim_usart1_gsm_ring_buff.tx.counter;
  if(!sim_usart1_gsm_ring_buff.tx.counter)
  {
    SIM_USART_FUNCTCMD(SIM_USARTGSM_CH, SIM_USART_TX_EMPTY_INT, Disable);
    SIM_USART_FUNCTCMD(SIM_USARTGSM_CH, SIM_USART_TX_CMPLT_INT, Enable);
  }
}

/*******************************************************************************
 ** \brief  Tx complete callback for gnss UART
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_usart_gsm_tx_cmplt_irq_callback(void) /* TX Complete IRQ */
{
  memset(sim_usart1_gsm_ring_buff.tx.buffer, 0, SIM_TX_BUFF_SIZE);
  sim_usart1_gsm_ring_buff.tx.ind = 0;
  sim_usart1_gsm_ring_buff.tx.counter = 0;
  sim_usart1_gsm_ring_buff.tx.tx_status = SIM_TX_DONE;
  SIM_USART_FUNCTCMD(SIM_USARTGSM_CH, SIM_USART_TX_CMPLT_INT, Disable);
  #ifdef SIM_USART_TX 
  SIM_USART_FUNCTCMD(SIM_USARTGSM_CH, SIM_USART_TX, Disable);
  #endif
}

/*******************************************************************************
 ** \brief  Procedure for recieving data after sending at command
 ** \param  request: pointer to char array of full user request which should be send
 ** \param  data: pointer to char array into which the response correct data is copied
 ** \param  answer: pointer to char array of expecting answer which will be compare with data in response
 ** \param  answ_type: type of answer message from gnns module. Using to understand how to parcing 
 **                   this answer message. This parameter can be one of the following values:
 **         @arg SIM_AT_NO: no parcing requared in the answer message
 **         @arg SIM_AT1: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
 **         @arg SIM_AT2: REQUEST\r\r\nOK\r\n
 **         @arg SIM_AT3: REQUEST\r\r\nOK\r\n\r\nDATA\r\n
 **         @arg SIM_AT4: REQUEST\r\r\nDATA\r\n
 **         @arg SIM_AT5: \rSENDEDDATA\r\r\nSEND OK\r\n
 **         @arg SIM_AT6: REQUEST\r
 **         @arg SIM_AT7: DATA\r 
 **         @arg SIM_AT8: GPRSHEADER\r\nTCP:DATA\r (GPRS full response)
 ** \retval Procedure return status, which can be:
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet
 **         @arg (DONE) SIM_PROC_WORKING: Procedure is done, continues working correctly.
 ******************************************************************************/ 
uint8_t sim_gsm_proc_recieve_data(char* request, char* data, char* answer, uint8_t answ_type)
{
  if(answ_type == SIM_AT_NO) return SIM_PROC_WORKING;
  if(sim_gsm_data.ring_proc_phase != SIM_GSM_RING_DATA_DONE) return SIM_NO_CORRECT_ANSWER;
  sim_gsm_data.ring_proc_phase = SIM_GSM_RING_DATA_START;
  if(!sim_gsm_data_parcer(request, data, answer, answ_type)) return SIM_INCORRECT_DATA;
  else                                                       return SIM_PROC_WORKING;
}

/*******************************************************************************
 ** \brief Procedure for sending data to gsm module
 ** \param data: pointer to char array which should be send to module
 ** \param timer: time in 100ms for waiting correct answer after sending data
 **               before trying to resend data.
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (DONE) SIM_PROC_WORKING: Procedure is done, continues working correctly.
*******************************************************************************/ 
uint8_t sim_gsm_proc_send_data(char* data, uint32_t timer)
{
  if(sim_usart1_gsm_ring_buff.tx.tx_status == SIM_TX_PROCCESS) return SIM_BUSY_MODULE;
  if(sim_gsm_data.rpt_counter) 
  {
    --sim_gsm_data.rpt_counter;
    sim_gsm_data.gsm_timers.wait_pack_tim = timer;  
    if(data) sim_usart_gsm_start_tx(data);
    return SIM_PROC_WORKING;
  }
  else
  {
    return SIM_NO_RPT; 
  }
}

/*******************************************************************************
 ** \brief  Procedure to work with gsm ring buffer. Getting correct meassages from there.
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void sim_proc_gsm_data(void)
{
  while(sim_usart1_gsm_ring_buff.rx.next_index_to_read != sim_usart1_gsm_ring_buff.rx.next_index_to_write)
  {
    switch(sim_gsm_data.ring_proc_phase) //data proc status
    {
///*****************************************************************************
      case SIM_GSM_RING_DATA_START:
           sim_gsm_data.data[sim_gsm_data.next_index_to_write] = sim_usart1_gsm_ring_buff.rx.buffer[sim_usart1_gsm_ring_buff.rx.next_index_to_read];
           ++sim_gsm_data.next_index_to_write;
           if(sim_gsm_data.next_index_to_write >= SIM_GSM_DATA_BUFF_SIZE) sim_gsm_data.next_index_to_write = 0;
           ++sim_usart1_gsm_ring_buff.rx.next_index_to_read;
           if(sim_usart1_gsm_ring_buff.rx.next_index_to_read >= SIM_RX_BUFF_SIZE) sim_usart1_gsm_ring_buff.rx.next_index_to_read = 0;
           if(sim_gsm_data.data[sim_gsm_data.next_index_to_write -1] == '\n' && sim_gsm_data.data[sim_gsm_data.next_index_to_write -2] == '\r') sim_gsm_data.ring_proc_phase = SIM_GSM_RING_DATA_ENDING;
           else break;
///*****************************************************************************
      case SIM_GSM_RING_DATA_ENDING:
           {
             sim_gsm_data.ring_proc_phase = SIM_GSM_RING_DATA_DONE;
             return;
           }
///*****************************************************************************
      default: return;
    } 
  }
}

/*******************************************************************************
 ** \brief  Procedure for start GSM module and autoband UART speed
 ** \param  None
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/   
uint8_t sim_gsm_init(void)
{
  static uint8_t phase = 0;
  uint8_t res = SIM_UNKNOWN_STATUS;
  switch(phase)
  {
    case 0: 
          SIM_RTSUART_RESET; //rts on
          SIM_CTSUART_TOGGLE; //cts on
          #ifdef SIM_GSMVBAT_RESET
          SIM_GSMVBAT_RESET; //GSM power on
          #endif
          ++phase;
          return SIM_PROC_WORKING;  
    case 1:
          SIM_PWRKEY_TOGGLE; //Start GSM module (get line in low lvl through open drain)
          ++phase;
          return SIM_PROC_WORKING; 
    case 2:
          if(SIM_STATUS_GET_BIT)
          {
            SIM_PWRKEY_RESET;
            ++phase;
          }
          return SIM_PROC_WORKING;  
    case 3:
          res = sim_gsm_proc_at("AT\r", NULL, NULL, SIM_AT2, 15, 10);
          if(res == SIM_PROC_DONE || res == SIM_NO_RPT || res == SIM_UNKNOWN_STATUS || res == SIM_BUSY_MODULE)  phase = 0;
          return res;
  }
  return res;
}

/*******************************************************************************
 ** \brief  Procedure for sending AT command to GSM module and get response answer
 ** \param  request: pointer to char array of full user request which should be send
 ** \param  answer: pointer to char array of expecting answer which will be compare with data in response
 **                 If you need to compare several strings at the same time, you can pass a pointer
 **                 to an array of strings separated by a comma
 ** \param  data: pointer to char array into which the response correct data is copied
 ** \param  parc_type: type of answer message from gsm module. Using to understand how to parcing 
 **                    this answer message. This parameter can be one of the following values:
 **         @arg SIM_AT_NO: no parcing requared in the answer message
 **         @arg SIM_AT1: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
 **         @arg SIM_AT2: REQUEST\r\r\nOK\r\n
 **         @arg SIM_AT3: REQUEST\r\r\nOK\r\n\r\nDATA\r\n
 **         @arg SIM_AT4: REQUEST\r\r\nDATA\r\n
 **         @arg SIM_AT5: \rSENDEDDATA\r\r\nSEND OK\r\n
 **         @arg SIM_AT6: REQUEST\r
 **         @arg SIM_AT7: DATA\r 
 **         @arg SIM_AT8: GPRSHEADER\r\nTCP:DATA\r (GPRS full response) 
 ** \param  wait_time: time in 100ms for waiting correct answer after sending data
 **                    before trying to resend data.
 ** \param  rpt_cnt: Amount of resend repeats if error occurs.
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ** \note - "request" param can be NULL if you want just to get any data without sending some.
 **         "data" param can be NULL if you don want to copy correct data from response answer
 **         if they are exist. "answer" param can be NULL if you dont need to compare response data
 **         with some of your expecting data.
 **       - you can see a lot of examples in sim868_gprs.c
 ******************************************************************************/ 
uint8_t sim_gsm_proc_at(char* request, char* answer, char* data, uint8_t parc_type, uint16_t wait_time, uint16_t rpt_cnt)
{
  static uint8_t phase = 0;
  uint8_t res = SIM_UNKNOWN_STATUS;
  if(sim_gsm_data.gsm_timers.wait_pack_flag)
  {
    memset(sim_gsm_data.data, 0, SIM_GSM_DATA_BUFF_SIZE);
    sim_gsm_data.next_index_to_write = 0;
    sim_gsm_data.ring_proc_phase = SIM_GSM_RING_DATA_START;
    phase = 1;
    sim_gsm_data.gsm_timers.wait_pack_flag = 0;
  }
  switch(phase)
  {
    case 0:
         sim_gsm_data.rpt_counter = rpt_cnt;
         ++phase;
    case 1:
         res = sim_gsm_proc_send_data(request, wait_time);
         if(res == SIM_NO_RPT) 
         {
           phase = 0;
           return res;
         }
         ++phase;
         return res;
    case 2:
         if(sim_usart1_gsm_ring_buff.tx.tx_status == SIM_TX_PROCCESS) return SIM_NO_CORRECT_ANSWER;
         res = sim_gsm_proc_recieve_data(request, data, answer, parc_type);
         if(res != SIM_PROC_WORKING) return res; 
         sim_gsm_data.gsm_timers.wait_pack_tim = 0;
         memset(sim_gsm_data.data, 0, SIM_GSM_DATA_BUFF_SIZE);
         sim_gsm_data.next_index_to_write = 0;
         phase = 0;
         res = SIM_PROC_DONE;
         return res;
  }
  return(res);
}

/*******************************************************************************
 ** \brief  Procedure for restart GSM module
 ** \param  None
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/   
uint8_t sim_gsm_restart(void)
{
  return(sim_gsm_proc_at("AT+CFUN=1,1\r", NULL, NULL, SIM_AT_NO, 15, 10));
}

