/*******************************************************************************
 *                                 v1.0                                        *
 *                       Procedures for GPRS data                              *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "sim868_ble.h"
#include "sim868_gnss.h"
#include "sim868_gsm.h"
#include "sim868_driver.h"
#include "sim868_crc.h"
#include <stdio.h>
sim_ble_server_info_struct sim_ble_server_info;
sim_ble_client_info_struct sim_ble_client_info;
sim_ble_timers_struct sim_ble_timers;
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

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
 ** \brief  Procedure for gprs cases
 ** \param  res: pointer to at command res
 ** \param  phase: pointer to current phase of proccess
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/
uint8_t sim_ble_case_proccess(uint8_t* res, uint8_t* phase, uint8_t error_phase)
{
  if(*res == SIM_PROC_DONE) 
  {
    *res = SIM_PROC_WORKING;
    ++(*phase);
  }
  if(*res == SIM_NO_RPT || *res == SIM_BUSY_MODULE || *res == SIM_UNKNOWN_STATUS) *phase = error_phase; 
  return *res;
}

/*******************************************************************************
 ** \brief  Procedure for power on BLE
 ** \param  apn: pointer to char array of APN 
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/   
uint8_t sim_ble_init(void)
{
  static uint8_t phase = 0;
  //char temp[1000] = {0};
  char data[100] = {0};
  uint8_t res = SIM_UNKNOWN_STATUS;
  switch(phase)
  {     
    case 0: 
         res = sim_gsm_proc_at("AT+BTPOWER=1\r", NULL, NULL, SIM_AT2, 15, 10);
         return sim_ble_case_proccess(&res, &phase, 0);   
    case 1: 
         res = sim_gsm_proc_at("AT+BTSTATUS?\r", "+BTSTATUS: 5", data, SIM_AT1, 15, 10);
         if(res == SIM_PROC_DONE) phase = 0;
         if(res == SIM_NO_RPT || res == SIM_BUSY_MODULE || res == SIM_UNKNOWN_STATUS) phase = 0; 
         return res;
  }
  return res;
}

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Procedure for power off BLE
 ** \param  apn: pointer to char array of APN 
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/   
uint8_t sim_ble_deinit(void)
{
  static uint8_t phase = 0;
  //char temp[1000] = {0};
  char data[100] = {0};
  uint8_t res = SIM_UNKNOWN_STATUS;
  switch(phase)
  {     
    case 0: 
         res = sim_gsm_proc_at("AT+BTPOWER=0\r", NULL, NULL, SIM_AT2, 15, 10);
         return sim_ble_case_proccess(&res, &phase, 0);   
    case 1: 
         res = sim_gsm_proc_at("AT+BTSTATUS?\r", "+BTSTATUS: 0", data, SIM_AT1, 15, 10);
         if(res == SIM_PROC_DONE) phase = 0;
         if(res == SIM_NO_RPT || res == SIM_BUSY_MODULE || res == SIM_UNKNOWN_STATUS) phase = 0; 
         return res;
  }
  return res;
}

/*******************************************************************************
 ** \brief  Procedure for register server and start advert
 ** \param  advert_param: pointer to struct of advertising parameters @ref sim_ble_advert_param_struct
            Can be NULL if you want just default advertise without custom data.
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/
uint8_t sim_ble_server_init_and_advert(sim_ble_advert_param_struct* advert_param)
{
  static uint8_t phase = 0;
  sim_gsm_data.state = phase;
  char temp[1000] = {0};
  char data[50] = {0};
  uint8_t res = SIM_UNKNOWN_STATUS;
  switch(phase)
  {  
    case 0: 
         res = sim_gsm_proc_at("AT+BLESREG\r", NULL, data, SIM_AT1, 25, 10);
         if(res == SIM_PROC_DONE)
         {
           sim_ble_server_info.server_index = data[10];
           strcpy(sim_ble_server_info.user_id, &(data[12]));
           if(advert_param)
           {
             res = SIM_PROC_WORKING;
             phase = 1;
           }
           else  
           {
             phase = 0;
           }
         }
         if(res == SIM_NO_RPT || res == SIM_BUSY_MODULE || res == SIM_UNKNOWN_STATUS) phase = 0; 
         return res;
    case 1: 
         sprintf(temp, "AT+BLEADV=%c,%d,%d,%d,%d,\"%s\",\"%s\",\"%s\"\r", //AT+BLEADV=<server_index>,<scan_rsp>,<include_name>,<include_txpower>,<appearance>,<manufacturer_data>,<service_data>,<service_uuid>
                 sim_ble_server_info.server_index,
                 advert_param->scan_rsp,
                 advert_param->bt_name,
                 advert_param->tx_power,
                 advert_param->appearance,
                 advert_param->manufactured_data,
                 advert_param->service_data,
                 advert_param->service_uuid);
         res = sim_gsm_proc_at(temp, NULL, NULL, SIM_AT1, 15, 10);
         if(res == SIM_PROC_DONE || res == SIM_NO_RPT || res == SIM_BUSY_MODULE || res == SIM_UNKNOWN_STATUS) phase = 0; 
         return res;
  }
  return res;
}

/*******************************************************************************
 ** \brief  Procedure for register client and start scanning
 ** \param  scanned: user char buffer where scanned device info wil be place, size
 **                  should be enough for all scanned devices or buffer will be looped
 ** \param  timeForScan: value for timer of total scanning duration.
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/
uint8_t sim_ble_client_init_and_scan(char* scanned, uint16_t buf_size, uint16_t time_for_scan)
{
  static uint8_t phase = 0;
  static uint16_t buf_index = 0;
  sim_gsm_data.state = phase;
  char temp[1000] = {0};
  char data[100] = {0};
  uint8_t res = SIM_UNKNOWN_STATUS;
  switch(phase)
  {  
    case 0: 
         res = sim_gsm_proc_at("AT+BLECREG\r", NULL, data, SIM_AT1, 25, 10);
         if(res == SIM_PROC_DONE)
         {
           sim_ble_client_info.client_index = data[10];
           strcpy(sim_ble_client_info.user_id, &(data[12]));
         }
         return sim_ble_case_proccess(&res, &phase, 0);   
    case 1: 
         sprintf(temp, "AT+BLESCAN=%c,1\r", sim_ble_client_info.client_index); //AT+BLESCAN=<client_index>,<op>
         res = sim_gsm_proc_at(temp, NULL, NULL, SIM_AT2, 15, 10);
         if(res == SIM_PROC_DONE) sim_ble_timers.scann_timer = time_for_scan;
         return sim_ble_case_proccess(&res, &phase, 0);   
    case 2: 
         res = sim_gsm_proc_at(NULL, "+BLESCANRST", data, SIM_AT7, 15, 10);
         if(res == SIM_PROC_DONE)
         {
           strcpy(scanned +buf_index, data);
           buf_index += strlen(data);
           if(buf_index >= buf_size) buf_index = 0;
         }
         if(sim_ble_timers.scann_timer_flag)
         {
           sim_ble_timers.scann_timer_flag = 0;
           buf_index = 0;
           ++phase;
         }
         return SIM_PROC_WORKING;
    case 3: 
         sprintf(temp, "AT+BLESCAN=%c,0\r", sim_ble_client_info.client_index); //AT+BLESCAN=<client_index>,<op>
         res = sim_gsm_proc_at(temp, NULL, NULL, SIM_AT1, 15, 10);
         if(res == SIM_PROC_DONE) phase = 0; 
         if(res == SIM_NO_RPT || res == SIM_BUSY_MODULE || res == SIM_UNKNOWN_STATUS) phase = 0; 
         return res;
  }
  return res;
}