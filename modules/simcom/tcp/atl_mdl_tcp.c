/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "atl_core.h"
#include "atl_mdl_tcp.h"
#include "dbc_assert.h"
#include <stdio.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_TCP")

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
 ** @brief  Function init GPRS
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_init(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  (void)param;
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,               "+COPS: 0", ATL_PARCE_SIMCOM, 5,  100, 1, 2, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+COPS=0"ATL_CMD_CRLF,                    NULL, ATL_PARCE_SIMCOM, 5,  100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CREG?"ATL_CMD_CRLF,  "+CREG: 0,1|+CREG: 0,5", ATL_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CGATT?"ATL_CMD_CRLF,             "+CGATT: 1", ATL_PARCE_SIMCOM, 30, 100, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to create and config socket. (SINGLE SOCKET)
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_config(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  (void)param;
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPMODE?"ATL_CMD_CRLF,        "+CIPMODE: 0", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,          "+CIPMUX: 0", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                 NULL, ATL_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,   "STATE: IP START", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,      NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP GPRSACT", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                    NULL, ATL_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP GPRSACT", ATL_PARCE_SIMCOM,  3, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,           ATL_CMD_FORCE, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,        "+CIPHEAD: 1", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,        "+CIPSRIP: 1", ATL_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,    "+CIPSHOWTP: 1", ATL_PARCE_SIMCOM,  1, 100, 1, 0, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,              NULL, ATL_PARCE_SIMCOM, 10, 100, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to connect socket.
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @atl_mdl_tcp_server_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_connect(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  DBC_REQUIRE(101, param);
  char cipstart[128] = {0}; 
  atl_mdl_tcp_server_t* tcp = (atl_mdl_tcp_server_t*)param;
  snprintf(cipstart, sizeof(cipstart), "%sAT+CIPSTART=\"%s\",\"%s\",\"%s\"%s", ATL_CMD_SAVE, tcp->mode, tcp->ip, tcp->port, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP STATUS|STATE: TCP CLOSED", ATL_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM(cipstart,                           "CONNECT OK|ALREADY CONNECT", ATL_PARCE_SIMCOM,  6, 500,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,                  "STATE: CONNECT OK", ATL_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPSEND?"ATL_CMD_CRLF,                           "+CIPSEND:", ATL_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ATL_NO_ARG),         
    ATL_ITEM("AT+CIPQSEND?"ATL_CMD_CRLF,                       "+CIPQSEND: 0", ATL_PARCE_SIMCOM,  1, 100,  1, 0, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM("AT+CIPQSEND=0"ATL_CMD_CRLF,                                NULL, ATL_PARCE_SIMCOM, 10, 100,  0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to connect socket.
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is ptr to char* data
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_send_recieve(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  DBC_REQUIRE(201, param);
  DBC_REQUIRE(202, atl_get_init().init);
  bool res = false;
  atl_mdl_tcp_data_t* tcp = (atl_mdl_tcp_data_t*)param;
  size_t size = strlen(ATL_CMD_SAVE) + strlen(tcp->data) + strlen(ATL_CMD_CTRL_Z) +1; 
  char cipsend[32] = {0}; 
  char* datacmd = (char*)atl_malloc(size);
  if(!datacmd) 
  { 
    atl_deinit(); 
    return false; 
  } 
  snprintf(cipsend, sizeof(cipsend), "%sAT+CIPSEND=%d%s", ATL_CMD_SAVE, (int)(size - strlen(ATL_CMD_SAVE) -1), ATL_CMD_CRLF); 
  snprintf(datacmd, size, "%s%s%s", ATL_CMD_SAVE, tcp->data, ATL_CMD_CTRL_Z); 
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: CONNECT OK",  ATL_PARCE_SIMCOM, 5, 100,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM(cipsend,                        "AT+CIPSEND=&>",     ATL_PARCE_RAW, 3, 500,  0, 1, NULL, NULL, ATL_NO_ARG),
    ATL_ITEM(datacmd,                              tcp->answ,     ATL_PARCE_RAW, 3, 500,  0, 1, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) res = false;
  else res = true;
  atl_free(datacmd);
  return res;
}

/*******************************************************************************
 ** @brief  Function to disconnect from socket
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_disconnect(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  (void)param;
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPCLOSE=1"ATL_CMD_CRLF, "CLOSE OK", ATL_PARCE_SIMCOM, 10, 100, 0, 0, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to deinit gprs
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_deinit(const atl_entity_cb_t cb, const void* const param, void* const ctx)
{
  (void)param;
  atl_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPSHUT"ATL_CMD_CRLF, "SHUT OK", ATL_PARCE_SIMCOM, 2, 100, 0, 1, NULL, NULL, ATL_NO_ARG),
  };
  if(!atl_entity_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}
