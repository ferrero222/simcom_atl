/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)26.05.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "atl_core.h"
#include "atl_core_scheduler.h"
#include "atl_mdl_tcp.h"
#include "dbc_assert.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_TCP");

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Function init GPRS
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_init(atl_entity_cb_t cb)
{
  DBC_REQUIRE(101, atl_is_init());
  bool result = false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,               "+COPS: 0", NULL, 2,  15, 1, 2, NULL),
    ATL_ITEM("AT+COPS=0"ATL_CMD_CRLF,                    NULL, NULL, 2,  15, 0, 1, NULL),
    ATL_ITEM("AT+CREG?"ATL_CMD_CRLF,  "+CREG: 0,1|+CREG: 0,5", NULL, 5,  60, 0, 1, NULL),
    ATL_ITEM("AT+CGATT?"ATL_CMD_CRLF,             "+CGATT: 1", NULL, 10, 60, 0, 0, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to create and config socket. (SINGLE SOCKET)
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_config(atl_entity_cb_t cb)
{
  DBC_REQUIRE(201, atl_is_init());
  bool result = false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPMODE?"ATL_CMD_CRLF,                   "+CIPMODE: 0", NULL, 1, 15, 1, 2, NULL),
    ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                           NULL, NULL, 2, 15, 0, 1, NULL),
    ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,                     "+CIPMUX: 0", NULL, 1, 15, 1, 2, NULL),
    ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                            NULL, NULL, 2, 50, 0, 1, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,              "STATE: IP START", NULL, 1, 25, 1, 2, NULL),
    ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,                 NULL, NULL, 2, 15, 0, 1, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,            "STATE: IP GPRSACT", NULL, 1, 25, 1, 2, NULL),
    ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                               NULL, NULL, 4, 80, 0, 1, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,            "STATE: IP GPRSACT", NULL, 1, 25, 0, 1, NULL),
    ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,                               NULL, NULL, 1, 15, 1, 1, NULL),
    ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,                   "+CIPHEAD: 1", NULL, 1, 15, 1, 2, NULL),
    ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                           NULL, NULL, 2, 15, 0, 1, NULL),
    ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,                   "+CIPSRIP: 1", NULL, 1, 15, 1, 2, NULL),
    ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                           NULL, NULL, 2, 15, 0, 1, NULL),
    ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,               "+CIPSHOWTP: 1", NULL, 1, 15, 1, 2, NULL),
    ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 15, 0, 1, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to connect socket. Use @atl_mdl_gprs_socket_connect_vla
 ** \param  mode  TCP/UDP. 
 ** \param  ip    ip of server. 
 ** \param  port  port. 
 ** \param  cb    cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
static uint16_t atl_mdl_gprs_send_len = 0;

#define atl_mdl_gprs_socket_connect_vla(_mode, _ip, _port, _cb) \
  do { \
      char _cmd[128]; \
      snprintf(_cmd, sizeof(_cmd), "%sAT+CIPSTART=\"%s\",\"%s\",\"%s\"%s", ATL_CMD_SAVE, _mode, _ip, _port, ATL_CMD_CRLF); \
      atl_mdl_gprs_socket_connect(_cmd, _cb); \
  } while(0)

bool atl_mdl_gprs_socket_connect(char* cipstart, atl_entity_cb_t cb)
{
  DBC_REQUIRE(301, atl_is_init());
  bool result = false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,   "STATE: IP STATUS|STATE: TCP CLOSED",           NULL, 2, 25,  0, 1, NULL),
    ATL_ITEM("AT+CIPSTART=",                                       "CONNECT OK",           NULL, 5, 160, 0, 1, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,                    "STATE: CONNECT OK",           NULL, 2, 25,  0, 1, NULL),
    ATL_ITEM("AT+CIPSEND?"ATL_CMD_CRLF,                             "+CIPSEND:", "+CIPSEND: %u", 2, 15,  0, 1, NULL, &atl_mdl_gprs_send_len),         
    ATL_ITEM("AT+CIPQSEND?"ATL_CMD_CRLF,                         "+CIPQSEND: 0",           NULL, 1, 15,  0, 1, NULL),
    ATL_ITEM("AT+CIPQSEND=0"ATL_CMD_CRLF,                                  NULL,           NULL, 2, 15,  0, 1, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to connect socket. Use @atl_mdl_gprs_socket_send_recieve_vla
 ** \param  data  data to send. 
 ** \param  cb    cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
#define atl_mdl_gprs_socket_send_recieve_vla(_data, _cb) \
  do { \
      size_t _size = strlen(ATL_CMD_SAVE) + strlen(_data) + strlen(ATL_CMD_CTRL_Z) +1; \
      char _cmd1[64]; \
      char _cmd2[_size]; \
      snprintf(_cmd1, sizeof(_cmd1), "%sAT+CIPSEND=%d%s", ATL_CMD_SAVE, strlen(_data), ATL_CMD_CRLF); \
      snprintf(_cmd2, sizeof(_cmd2), "%s%s%s",            ATL_CMD_SAVE, _data, ATL_CMD_CRLF); \
      atl_mdl_gprs_socket_send_recieve(_cmd1, _cmd2, _cb); \
  } while(0)

bool atl_mdl_gprs_socket_send_recieve(char* cipsend, char* data, atl_entity_cb_t cb)
{
  DBC_REQUIRE(401, atl_is_init());
  bool result = false;
  if(atl_mdl_gprs_send_len && atl_mdl_gprs_send_len < strlen(data)) return false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: CONNECT OK", NULL, 2, 25,  0, 1, NULL),
    ATL_ITEM("AT+CIPSEND=",                              ">", NULL, 2, 25,  0, 1, NULL),
    ATL_ITEM("",                              "+IPD&SEND OK", NULL, 2, 25,  0, 1, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to disconnect from socket
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_disconnect(atl_entity_cb_t cb)
{
  DBC_REQUIRE(501, atl_is_init());
  bool result = false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPCLOSE=1"ATL_CMD_CRLF, "CLOSE OK", NULL, 2, 25,  0, 1, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}

/*******************************************************************************
 ** \brief  Function to deinit gprs
 ** \param  cb  cb when proc will be done. 
 ** \retval true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_deinit(atl_entity_cb_t cb)
{
  DBC_REQUIRE(601, atl_is_init());
  bool result = false;
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPSHUT"ATL_CMD_CRLF, "SHUT OK", NULL, 2, 25,  0, 1, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb)) return false;
  return true;
}
