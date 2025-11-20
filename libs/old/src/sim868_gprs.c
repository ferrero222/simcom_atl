/*******************************************************************************
 *                                 v1.0                                        *
 *                       Procedures for GPRS data                              *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "sim868_gprs.h"
#include "sim868_gnss.h"
#include "sim868_gsm.h"
#include "sim868_driver.h"
#include "sim868_crc.h"
#include <stdio.h>

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
sim_gsm_gprs_struct sim_gsm_gprs;

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
uint8_t sim_gprs_case_proccess(uint8_t* res, uint8_t* phase, uint8_t error_phase)
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
 ** \brief  Procedure for initialization of GPRS
 ** \param  apn: pointer to char array of APN 
 ** \param  usr: pointer to char array of user name of provider network
 ** \param  pass: pointer to char array of paasword to provider network
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/
uint8_t sim_gprs_start_init(char *apn, char *usr, char *pass)
{
  static uint8_t phase = 0;
  sim_gsm_data.state = phase;
  char temp[1000] = {0};
  char data[100] = {0};
  uint8_t res = SIM_UNKNOWN_STATUS;
  switch(phase)
  {
//******************************************************************************   
// Start init
//******************************************************************************
    case SIM_GPRS_SI_CHECK_SIM_CARD:      //(0) check sim card pass status (CPIN)
         res = sim_gsm_proc_at("AT+CPIN?\r", "+CPIN: READY", data, SIM_AT1, 50, 10); 
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_SET_IMEI:            //(1) set imei
       //res = sim_gsm_proc_at("AT+SIMEI=862059061965409\r", NULL, data, SIM_AT2, 15, 10);
         res = sim_gsm_proc_at("AT+GSN\r", NULL, data, SIM_AT1, 15, 10); //temp
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_REQ_IMEI:            //(2) Request IMEI data
         res = sim_gsm_proc_at("AT+GSN\r", NULL, data, SIM_AT1, 15, 10);
         if(res == SIM_PROC_DONE) memcpy(sim_gsm_gprs.imei, data, sizeof(sim_gsm_gprs.imei));
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_REQ_ID:              //(3) request ID data
         res = sim_gsm_proc_at("AT+GMM\r", NULL, data, SIM_AT1, 15, 10);
         if(res == SIM_PROC_DONE) memcpy(sim_gsm_gprs.id, data, sizeof(sim_gsm_gprs.id));
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_REQ_REV:             //(4) request revision data
         res = sim_gsm_proc_at("AT+GMR\r", NULL, data, SIM_AT1, 15, 10);
         if(res == SIM_PROC_DONE) memcpy(sim_gsm_gprs.rev, data, sizeof(sim_gsm_gprs.rev));
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
//******************************************************************************   
// GPRS init
//******************************************************************************
    case SIM_GPRS_SI_CHECK_SIGNAL_LVL:    //(5) check for correct signal lvl
         res = sim_gsm_proc_at("AT+CSQ\r", NULL, NULL, SIM_AT1, 50, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_CHECK_CONN_TO_GSM:   //(6) check connection to GSM network 
         res = sim_gsm_proc_at("AT+CREG?\r", "+CREG: 0,1", data, SIM_AT1, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_SET_CGATT:           //(7)Set CGATT connection
         res = sim_gsm_proc_at("AT+CGATT=1\r", NULL, NULL, SIM_AT2, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_CHECK_ACC_TO_GPRS:  //(8)Check access to GPSR
         res = sim_gsm_proc_at("AT+CGATT?\r", "+CGATT: 1", data, SIM_AT1, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_SET_COMMAND_MODE:   //(9) Set command mode (CLIPMODE=0)
         res = sim_gsm_proc_at("AT+CIPMODE=0\r", NULL, NULL, SIM_AT2, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
//******************************************************************************   
// Connection settings config
//******************************************************************************
    case SIM_GPRS_SI_SET_SINGLE_SOKET:   //(10) Set single soket (CIPMUX=0)
         res = sim_gsm_proc_at("AT+CIPMUX=0\r", NULL, NULL, SIM_AT2, 50, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_CHECK_CONN_STAT1:   //(11) Check current connection status 
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: IP INITIAL", data, SIM_AT3, 25, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_SET_CONTEXT:        //(12) Set context settings (CSTT=APN,USR,PASS)
         sprintf(temp, "AT+CSTT=\"%s\",\"%s\",\"%s\"\r", apn, usr, pass); //"AT+CSTT="internet.beeline.ru","beeline","beeline"\r";
         res = sim_gsm_proc_at(temp, NULL, NULL, SIM_AT2, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_CHECK_CONN_STAT2:   //(13) Check current connection status 
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: IP START", data, SIM_AT3, 25, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_CONTEX_ACTIV:       //(14) Context activation after success status(CIICR)
         res = sim_gsm_proc_at("AT+CIICR\r", NULL, NULL, SIM_AT2, 80, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_CHECK_CONN_STAT3:   //(15) Check current connection status 
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: IP GPRSACT", data, SIM_AT3, 25, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_REQ_SELF_IP:        //(16) Req for self IP to confirm new status (CIFSR)
         res = sim_gsm_proc_at("AT+CIFSR\r", NULL, data, SIM_AT4, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
//******************************************************************************   
// Recieving data config
//******************************************************************************   
    case SIM_GPRS_SI_SHOW_IP_HEAD:       //(17) Set to show destination in header of recieving data(CIPSRIP=1)
         res = sim_gsm_proc_at("AT+CIPHEAD=1\r", NULL, NULL, SIM_AT2, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_SHOW_DEST_HEAD:     //(18) Set to show destination in header of recieving data(CIPSRIP=1)
         res = sim_gsm_proc_at("AT+CIPSRIP=1\r", NULL, NULL, SIM_AT2, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SI_CHECK_SIM_CARD);
    case SIM_GPRS_SI_SHOWP_PROT_TYPE:    //(19) Set to show protocol type in header of recieving data(CIPSHOWTP=1)
         res = sim_gsm_proc_at("AT+CIPSHOWTP=1\r", NULL, NULL, SIM_AT2, 15, 10);
         if(res == SIM_PROC_DONE) phase = 0;
         if(res == SIM_NO_RPT || res == SIM_BUSY_MODULE || res == SIM_UNKNOWN_STATUS) phase = 0; 
         return res;
  }
  return SIM_UNKNOWN_STATUS;
}
  
/*******************************************************************************
 ** \brief  Procedure for geting connection to remote server through GPRS with user settings
 ** \param  mode: pointer to char array of connection mode
 ** \param  ip: pointer to char array of remote server IP
 ** \param  port: pointer to char array of remote server port
 ** \retval Procedure return status, which can be:
 **         @arg (ERROR) SIM_NO_RPT: Resend repeats is 0. Module in not answering. 
 **         @arg (ERROR) SIM_UNKNOWN_STATUS: Procedure cant execute for some reason. 
 **         @arg (ERROR) SIM_BUSY_MODULE: Trying to send new data, when another is still sending
 **         @arg (NOT DONE) SIM_INCORRECT_DATA: Gotten message cant parce correctly. 
 **         @arg (NOT DONE) SIM_NO_CORRECT_ANSWER: No correct answer in buffer yet.
 **         @arg (NOT DONE) SIM_PROC_WORKING: Procedure is in correct proccess.
 **         @arg (DONE) SIM_PROC_DONE: Procedure was done fully correctly.
 ******************************************************************************/
uint8_t sim_gprs_connect(char *mode, char *ip, char *port)
{
  static uint8_t phase = 0;
  sim_gsm_data.state = phase;
  char temp[1000] = {0};
  char data[100] = {0};
  uint8_t res = SIM_UNKNOWN_STATUS;
  switch(phase)
  {  
//******************************************************************************   
// Try connection
//****************************************************************************** 
    case SIM_GPRS_SC_TRY_CONN_TO_SERV:      //(0) Try connection to the server(CIPSTART=MODE,IP,PORT)
         sprintf(temp, "AT+CIPSTART=\"%s\",\"%s\",\"%s\"\r", mode, ip, port); //AT+CIPSTART=\"TYPE\",\"IP\",\"PORT\"\r
         res = sim_gsm_proc_at(temp, "CONNECT OK, ALREADY CONNECT", data, SIM_AT3, 160, 4);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_TRY_CONN_TO_SERV);
    case SIM_GPRS_SC_CHECK_CONN_STAT1:      //(1) Check current connection status 
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: CONNECT OK", data, SIM_AT3, 25, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_TRY_CONN_TO_SERV);
//******************************************************************************   
// PREP AND SEND LOGIN
//****************************************************************************** 
    case SIM_GPRS_SC_CHECK_MAX_DATA_LEN:    //(2) Check max data lenght that can be send to remote (CIPSEND?)
         res = sim_gsm_proc_at("AT+CIPSEND?\r", NULL, data, SIM_AT1, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_CLOSE_CONNECTION);
    case SIM_GPRS_SC_CHECK_EXCH_MODE:       //(3) Check for normal exchange mode (CIPQSEND?, answer should be 0)
         res = sim_gsm_proc_at("AT+CIPQSEND?\r", NULL, data, SIM_AT1, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_CLOSE_CONNECTION);
    case SIM_GPRS_SC_CHECK_CONNS_STAT2:     //(4) Check current connection status 
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: CONNECT OK", data, SIM_AT3, 25, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_CLOSE_CONNECTION);
    case SIM_GPRS_SC_REQ_TO_SEND_DATA:      //(5) Req for send random amount of data (CIPSEND, should answer '>')
         sprintf(temp, "AT+CIPSEND=%d\r", 32);
         res = sim_gsm_proc_at(temp, NULL, NULL, SIM_AT6, 50, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_CLOSE_CONNECTION);
    case SIM_GPRS_SC_SEND_LOGIN:            //(6) Send login
         strcpy(temp, "#L#2.0;000000000000000;NA;FFFF\r\n");
         memcpy(temp+7, sim_gsm_gprs.imei, 15);
         sim_calc_crc16((uint8_t*)(temp+3), 23);
         temp[29] = sim_convert_half_byte_to_ascii((char)((sim_proc_crc.words.crc16_1 & 0x000F) >> 0));
         temp[28] = sim_convert_half_byte_to_ascii((char)((sim_proc_crc.words.crc16_1 & 0x00F0) >> 4));
         temp[27] = sim_convert_half_byte_to_ascii((char)((sim_proc_crc.words.crc16_1 & 0x0F00) >> 8));
         temp[26] = sim_convert_half_byte_to_ascii((char)((sim_proc_crc.words.crc16_1 & 0xF000) >> 12));
         res = sim_gsm_proc_at(temp, NULL, NULL, SIM_AT5, 50, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_CLOSE_CONNECTION);
    case SIM_GPRS_SC_ANSW_LISTEN:          //(7) Listen asnwer
         res = sim_gsm_proc_at(NULL, "#AL#1\r\n", data, SIM_AT8, 50, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_CLOSE_CONNECTION);
//******************************************************************************   
// Close connection   
//****************************************************************************** 
    case SIM_GPRS_SC_CLOSE_CONNECTION:    //(8) Close connection to server (CIPCLOSE)
         res = sim_gsm_proc_at("AT+CIPCLOSE\r", "CLOSE OK", data, SIM_AT5, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_TRY_CONN_TO_SERV);
    case SIM_GPRS_SC_CHECK_CONN_STAT3:    //(9) Check current connection status 
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: TCP CLOSED", data, SIM_AT3, 25, 10); 
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SC_TRY_CONN_TO_SERV);
  }
  return SIM_UNKNOWN_STATUS;
}

/*******************************************************************************
 ** \brief  Send amount of data to remote server through GPRS
 ** \param  mode: pointer to char array of connection mode
 ** \param  ip: pointer to char array of remote server IP
 ** \param  port: pointer to char array of remote server port
 ** \param  atcomm: pointer to char array of full user request which should be send
 ** \param  answer: pointer to char array of expecting answer which will be compare with response for sending
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
 ******************************************************************************/
uint8_t sim_gprs_send_data(char *mode, char *ip, char *port, char* atcomm, char* answer, uint32_t wait_time, uint8_t rpt_cnt)
{
  static uint8_t phase = SIM_GPRS_SD_REQ_TO_SEND_DATA; //Start without connection. Reconncetion happens when error is occur
  sim_gsm_data.state = phase;
  uint8_t res = SIM_UNKNOWN_STATUS;
  char temp[1000] = {0};
  char data[100] = {0};
  switch(phase)
  {  
//******************************************************************************   
// Try connection
//****************************************************************************** 
    case SIM_GPRS_SD_TRY_CONN_TO_SERV:   //(0) Try to connection to the server(CIPSTART=MODE,IP,PORT)
         sprintf(temp, "AT+CIPSTART=\"%s\",\"%s\",\"%s\"\r", mode, ip, port);
         res = sim_gsm_proc_at(temp, "CONNECT OK, ALREADY CONNECT", data, SIM_AT3, 160, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SD_TRY_CONN_TO_SERV);
    case SIM_GPRS_SD_CHECK_CONN_STAT1:   //(1) Check current connection status 
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: CONNECT OK", data, SIM_AT3, 25, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SD_TRY_CONN_TO_SERV);
//******************************************************************************   
// Check connection status and send data
//****************************************************************************** 
    case SIM_GPRS_SD_REQ_TO_SEND_DATA:   //(2) Send amount of sending data
         sprintf(temp, "AT+CIPSEND=%d\r", strlen(atcomm));
         res = sim_gsm_proc_at(temp, NULL, NULL, SIM_AT6, 15, rpt_cnt);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SD_CLOSE_CONNECTION);
    case SIM_GPRS_SD_DATA_SENDING:       //(3) Sending data
         res = sim_gsm_proc_at(atcomm, NULL, NULL, SIM_AT5, 50, rpt_cnt);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SD_CLOSE_CONNECTION);
    case SIM_GPRS_SD_ANSW_LISTEN:        //(4) Listening for answer
         res = sim_gsm_proc_at(NULL, answer, data, SIM_AT8, wait_time, rpt_cnt);
         if(res == SIM_PROC_DONE) phase = SIM_GPRS_SD_CHECK_CONN_STAT1;
         if(res == SIM_NO_RPT) phase = SIM_GPRS_SD_CLOSE_CONNECTION; 
         return res;
//******************************************************************************   
// Close connection   
//******************************************************************************   
    case SIM_GPRS_SD_CLOSE_CONNECTION:   //(5) Close connection with server
         res = sim_gsm_proc_at("AT+CIPCLOSE\r", "CLOSE OK", data, SIM_AT5, 15, 10);
         return sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SD_TRY_CONN_TO_SERV);
    case SIM_GPRS_SD_CHECK_CONN_STAT2:   //(6) Check current status connection
         res = sim_gsm_proc_at("AT+CIPSTATUS\r", "STATE: TCP CLOSED", data, SIM_AT3, 25, 10); 
         res = sim_gprs_case_proccess(&res, &phase, SIM_GPRS_SD_TRY_CONN_TO_SERV);
         if(res == SIM_PROC_DONE) phase = SIM_GPRS_SD_TRY_CONN_TO_SERV;
         return res;
  }
  return res;
}