/*******************************************************************************
 *                                 v1.0                                        *
 *                       Procedures for GPRS data                              *
 ******************************************************************************/
#ifndef __SIM868_GPRS_H
#define __SIM868_GPRS_H
   
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h>
   
/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
   
/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
  ** @brief phases for gprs start init
**/ 
typedef enum
{
//***************************START*INIT*****************************************
  SIM_GPRS_SI_CHECK_SIM_CARD =     0,
  SIM_GPRS_SI_SET_IMEI =           1,
  SIM_GPRS_SI_REQ_IMEI =           2,
  SIM_GPRS_SI_REQ_ID =             3,
  SIM_GPRS_SI_REQ_REV =            4,        
//***************************GPRS*INIT******************************************
  SIM_GPRS_SI_CHECK_SIGNAL_LVL =   5,
  SIM_GPRS_SI_CHECK_CONN_TO_GSM =  6,
  SIM_GPRS_SI_SET_CGATT =          7,
  SIM_GPRS_SI_CHECK_ACC_TO_GPRS =  8,
  SIM_GPRS_SI_SET_COMMAND_MODE =   9,         
//***************************CONN*SETTINGS*CONFIG*******************************
  SIM_GPRS_SI_SET_SINGLE_SOKET =  10,
  SIM_GPRS_SI_CHECK_CONN_STAT1 =  11,
  SIM_GPRS_SI_SET_CONTEXT =       12,
  SIM_GPRS_SI_CHECK_CONN_STAT2 =  13,
  SIM_GPRS_SI_CONTEX_ACTIV =      14,
  SIM_GPRS_SI_CHECK_CONN_STAT3 =  15,
  SIM_GPRS_SI_REQ_SELF_IP =       16,        
//***************************RECIEVING*DATA*CONFIG******************************
  SIM_GPRS_SI_SHOW_IP_HEAD =      17,
  SIM_GPRS_SI_SHOW_DEST_HEAD =    18,
  SIM_GPRS_SI_SHOWP_PROT_TYPE =   19,      
} sim_gprs_start_init_reqs_list_nums;
  
/**
  ** @brief phases for gprs get connection
**/ 
typedef enum
{  
//***************************START*CONNECTION***********************************  
  SIM_GPRS_SC_TRY_CONN_TO_SERV =   0,
  SIM_GPRS_SC_CHECK_CONN_STAT1 =   1,   
//***************************PREP*AND*SEND*LOGIN********************************
  SIM_GPRS_SC_CHECK_MAX_DATA_LEN = 2,
  SIM_GPRS_SC_CHECK_EXCH_MODE =    3,
  SIM_GPRS_SC_CHECK_CONNS_STAT2 =  4,
  SIM_GPRS_SC_REQ_TO_SEND_DATA =   5,
  SIM_GPRS_SC_SEND_LOGIN =         6,
  SIM_GPRS_SC_ANSW_LISTEN =        7,      
////***************************CLOSE*CONNECTION*********************************
  SIM_GPRS_SC_CLOSE_CONNECTION =   8,
  SIM_GPRS_SC_CHECK_CONN_STAT3 =   9,
} sim_gprs_start_conn_reqs_list_nums;

/**
  ** @brief phases for gprs send data
**/ 
typedef enum
{
//***************************START*CONNECTION***********************************  
  SIM_GPRS_SD_TRY_CONN_TO_SERV =   0,
  SIM_GPRS_SD_CHECK_CONN_STAT1 =   1,
//***************************SEND*DATA******************************************
  SIM_GPRS_SD_REQ_TO_SEND_DATA =   2,
  SIM_GPRS_SD_DATA_SENDING =       3,
  SIM_GPRS_SD_ANSW_LISTEN =        4,        
////*************************CLOSE*CONNECTION***********************************
  SIM_GPRS_SD_CLOSE_CONNECTION =   5,
  SIM_GPRS_SD_CHECK_CONN_STAT2 =   6,
} sim_gprs_send_data_reqs_list_nums;

/**
  ** @brief phases for gprs get some data of SIM
**/ 
typedef struct gsm_gprs_struct
{
  uint8_t imei[15];   //IMEI
  uint8_t id[13];     //ID
  uint8_t rev[28];    //revision
} sim_gsm_gprs_struct;

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
void gpts_procgsm(void);
uint8_t sim_gprs_start_init(char *apn, char *usr, char *pass);
uint8_t sim_gprs_connect(char *mode, char *ip, char *port);
uint8_t sim_gprs_send_data(char *mode, char *ip, char *port, char* atcomm, char* answer, uint32_t wait_time, uint8_t rptCnt);

#endif //__SIM868_GPRS_H


/*******************************************************************************/
/** AT DESCRIPTION
********************************************************************************
   0)Description: Request IMEI data
     Request: AT+GSN\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: 8603051068941334
     Max response time: unknown(1,5 sec is ok)

   1)Description: request ID data
     Request: AT+GMM\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: SIMCOM_SIM868
     Max response time: unknown(1,5 sec is ok)

   2)Description: request revision data
     Request: AT+GMR\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: Revision:1418B02SIM68E32_BLE_DS_TLS12\r
     Max response time: unknown(1,5 sec is ok)

   3)Description: check sim card pass status (CPIN)
     Request: AT+CPIN?\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: +CPIN: READY (MT is not pending for any password)
                 +CPIN: SIM PIN (MT is waiting SIM PIN to be given)
                 +CPIN: SIM PUK (MT is waiting for SIM PUK to be given)
                 +CPIN: PH_SIM PIN (ME is waiting for phone to SIM card (antitheft))
                 +CPIN: PH_SIM PUK (ME is waiting for SIM PUK (antitheft))
     Max response time: 5sec

   4)Description: check for correct signal lvl (CSQ)
     Request: AT+CSQ\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: +CSQ: 0,0
     Max response time: 5sec

   5)Description: check connection to GSM network (CREG?)
     Request: AT+CREG?\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: +CREG: X,0 (0-Disable network registration unsolicited result code
                             1-Enable network registration unsolicited result code
                             2-Enable network registration unsolicited result code with location information)
                 +CREG: 0,X (0-Not registered
                             1-Registered, home network
                             2-Not registered, but MT is currently searching a new operator
                             3-Registration denied 
                             4-Unknown
                             5-Registered, roaming 
     Max response time: unknown(1,5 sec is ok)

   6)Description: check access to GPSR (CGATT?)
     Request: AT+CGATT?\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: +CGATT: 0 (Detached)
                 +CGATT: 1 (Attached)
     Max response time: 10sec

   7)Description: set command mode (CLIPMODE=0)
     Request: AT+CIPMODE=X\r (0-normal mode, 1-transparent mode)
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: +CGATT: no data
     Max response time: unknown(1,5 sec is ok)

   8)Description: set single soket (CIPMUX=0)
     IPMUX=X\r (0-sinde IP connection, 1-multi IP connection)
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: unknown(1,5 sec is ok)

  9)(Description: check current connection status (CIPSTATUS - IP initial)
     Request: AT+CIPSTATUS\r
     Answer format: REQUEST\r\r\nOK\r\n\r\nDATA\r\n
     DATA types: STATE: IP INITIAL
                 STATE: IP START
                 STATE: IP CONFIG
                 STATE: IP GPRSACT
                 STATE: IP STATUS
                 STATE: IP CONNECTING 
                 STATE: CONNECT OK
                 STATE: TCP CLOSING
                 STATE: TCP CLOSED
                 STATE: PDP DEACT 
     Max response time: unknown(2,5 sec is ok)

  10)Description: set context settings (CSTT=APN,USR,PASS)
     Request: AT+CSTT=\"internet.beeline.ru\",\"beeline\",\"beeline\"\r 
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: unknown(1,5 sec is ok)

  11)Description: context activation after success status(CIICR)
     Request: AT+CIICR\r 
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: 85 sec

  12)Description: req for self IP to confirm new status (CIFSR)
     Request: AT+CIFSR\r 
     Answer format: REQUEST\r\r\nDATA\r\n
     DATA types: self IP
     Max response time: unknown(1,5 sec is ok)

  13)Description: set to show IP in header of recieving data(CIPHEAD=1)
     Request: AT+CIPHEAD=X\r (0-not add ip header, 1-add ip header)
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: unknown(1,5 sec is ok)

  14)Description: set to show destination in header of recieving data(CIPSRIP=1)
     Request: AT+CIPSRIP=X\r (0-do not show prompt, 1-show the prompt)
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: unknown(1,5 sec is ok)

  15)Description: set to show protocol type in header of recieving data(CIPSHOWTP=1)
     Request: AT+CIPSHOWTP=X\r (0-no display transfer prot, 1-display transfer prot)
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: unknown(1,5 sec is ok)

  16)Description: try to connection to the server(CIPSTART=MODE,IP,PORT)
     Request: AT+CIPSTART=\"TCP\",\"mt.vega-absolute.ru\",\"20155\"\r
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: 160 sec

  17)Description: try to connection to the server(CIPSTART=MODE,IP,PORT)
     Request: AT+CIPSEND?\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: size
     Max response time: unknown(1,5 sec is ok)

  18)Description: check for normal exchange mode (CIPQSEND?, answer should be 0)
     Request: AT+CIPQSEND?\r
     Answer format: \r\nREQUEST\r\r\nDATA\r\n\r\nOK\r\n
     DATA types: +CIPQSEND: 0 (normal mode)
                 +CIPQSEND: 1 (quick send mode)
     Max response time: unknown(1,5 sec is ok)

  19)Description: req for send random amount of data (CIPSEND, should answer '>')
     Request: AT+CIPSEND=71\r
     Answer format: return request in answer
     DATA types: no data
     Max response time: unknown(5 sec is ok, waiting for the ">" symbol)

  20)Description: send block of data, wait for recieve data
     Request: TESTBLOCKOFDATA
     Answer format: \rSENDEDDATA\r\r\nSEND OK\r\n
     DATA types: no data
     Max response time: unknown(1,5 sec is ok)

  21)Description: close connection to server (CIPCLOSE)
     Request: AT+CIPCLOSE\r
     Answer format: REQUEST\r\r\nDATA\r\n
     DATA types: CLOSE OK
                 ERROR
     Max response time: unknown(1,5 sec is ok)

  22)Description: set cgatt connection
     Request: AT+CGATT=1\r
     Answer format: REQUEST\r\r\nOK\r\n
     DATA types: no data
     Max response time: 10 sec

  23)Description: listening to the answer
     Request: no request
     Answer format: RECV FROM:<IP>\r\n\r\n+IPD,11,TCP:<ANSWER>
     DATA types: no data
     Max response time: --
********************************************************************************
*/