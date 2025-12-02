/******************************************************************************
 *                              _    ____   ____                              *
 *                   ======    / \  / ___| / ___| ======       (c)03.10.2025  *
 *                   ======   / _ \ \___ \| |     ======           v1.0.0     *
 *                   ======  / ___ \ ___) | |___  ======                      *
 *                   ====== /_/   \_\____/ \____| ======                      *  
 *                                                                            *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "asc_core.h"
#include "asc_mdl_tcp.h"
#include "dbc_assert.h"
#include <stdio.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ASC_MDL_TCP")

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
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_gprs_init(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM("AT+COPS?"ASC_CMD_CRLF,               "+COPS: 0", ASC_PARCE_SIMCOM, 5,  100, 1, 2, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+COPS=0"ASC_CMD_CRLF,                    NULL, ASC_PARCE_SIMCOM, 5,  100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CREG?"ASC_CMD_CRLF,  "+CREG: 0,1|+CREG: 0,5", ASC_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CGATT?"ASC_CMD_CRLF,             "+CGATT: 1", ASC_PARCE_SIMCOM, 30, 100, 0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to create and config socket. (SINGLE SOCKET)
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_gprs_socket_config(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM("AT+CIPMODE?"ASC_CMD_CRLF,        "+CIPMODE: 0", ASC_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPMODE=0"ASC_CMD_CRLF,                NULL, ASC_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPMUX?"ASC_CMD_CRLF,          "+CIPMUX: 0", ASC_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPMUX=0"ASC_CMD_CRLF,                 NULL, ASC_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSTATUS"ASC_CMD_CRLF,   "STATE: IP START", ASC_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CSTT=\"\",\"\",\"\""ASC_CMD_CRLF,      NULL, ASC_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSTATUS"ASC_CMD_CRLF, "STATE: IP GPRSACT", ASC_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIICR"ASC_CMD_CRLF,                    NULL, ASC_PARCE_SIMCOM, 30, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSTATUS"ASC_CMD_CRLF, "STATE: IP GPRSACT", ASC_PARCE_SIMCOM,  3, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIFSR"ASC_CMD_CRLF,           ASC_CMD_FORCE, ASC_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPHEAD?"ASC_CMD_CRLF,        "+CIPHEAD: 1", ASC_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPHEAD=1"ASC_CMD_CRLF,                NULL, ASC_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSRIP?"ASC_CMD_CRLF,        "+CIPSRIP: 1", ASC_PARCE_SIMCOM,  1, 100, 1, 2, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSRIP=1"ASC_CMD_CRLF,                NULL, ASC_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSHOWTP?"ASC_CMD_CRLF,    "+CIPSHOWTP: 1", ASC_PARCE_SIMCOM,  1, 100, 1, 0, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSHOWTP=1"ASC_CMD_CRLF,              NULL, ASC_PARCE_SIMCOM, 10, 100, 0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to connect socket.
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is @asc_mdl_tcp_server_t
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_gprs_socket_connect(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(101, param);
  char cipstart[128] = {0}; 
  asc_mdl_tcp_server_t* tcp = (asc_mdl_tcp_server_t*)param;
  snprintf(cipstart, sizeof(cipstart), "%sAT+CIPSTART=\"%s\",\"%s\",\"%s\"%s", ASC_CMD_SAVE, tcp->mode, tcp->ip, tcp->port, ASC_CMD_CRLF); 
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  { 
    ASC_ITEM("AT+CIPSTATUS"ASC_CMD_CRLF, "STATE: IP STATUS|STATE: TCP CLOSED", ASC_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM(cipstart,                           "CONNECT OK|ALREADY CONNECT", ASC_PARCE_SIMCOM,  6, 500,  0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSTATUS"ASC_CMD_CRLF,                  "STATE: CONNECT OK", ASC_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPSEND?"ASC_CMD_CRLF,                           "+CIPSEND:", ASC_PARCE_SIMCOM, 10, 100,  0, 1, NULL, NULL, ASC_NO_ARG),         
    ASC_ITEM("AT+CIPQSEND?"ASC_CMD_CRLF,                       "+CIPQSEND: 0", ASC_PARCE_SIMCOM,  1, 100,  1, 0, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM("AT+CIPQSEND=0"ASC_CMD_CRLF,                                NULL, ASC_PARCE_SIMCOM, 10, 100,  0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to connect socket.
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is ptr to char* data
 **                Should exist only when this function is executing
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_gprs_socket_send_recieve(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  DBC_REQUIRE(201, param);
  DBC_REQUIRE(202, asc_get_init(ctx).init);
  bool res = false;
  asc_mdl_tcp_data_t* tcp = (asc_mdl_tcp_data_t*)param;
  size_t size = strlen(ASC_CMD_SAVE) + strlen(tcp->data) + strlen(ASC_CMD_CTRL_Z) +1; 
  char cipsend[32] = {0}; 
  char* datacmd = (char*)asc_malloc(ctx, size);
  if(!datacmd) 
  { 
    asc_deinit(ctx); 
    return false; 
  } 
  snprintf(cipsend, sizeof(cipsend), "%sAT+CIPSEND=%d%s", ASC_CMD_SAVE, (int)(size - strlen(ASC_CMD_SAVE) -1), ASC_CMD_CRLF); 
  snprintf(datacmd, size, "%s%s%s", ASC_CMD_SAVE, tcp->data, ASC_CMD_CTRL_Z); 
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM("AT+CIPSTATUS"ASC_CMD_CRLF, "STATE: CONNECT OK",  ASC_PARCE_SIMCOM, 5, 100,  0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM(cipsend,                        "AT+CIPSEND=&>",     ASC_PARCE_RAW, 3, 500,  0, 1, NULL, NULL, ASC_NO_ARG),
    ASC_ITEM(datacmd,                              tcp->answ,     ASC_PARCE_RAW, 3, 500,  0, 1, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) res = false;
  else res = true;
  asc_free(ctx, datacmd);
  return res;
}

/*******************************************************************************
 ** @brief  Function to disconnect from socket
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_gprs_socket_disconnect(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM("AT+CIPCLOSE=1"ASC_CMD_CRLF, "CLOSE OK", ASC_PARCE_SIMCOM, 10, 100, 0, 0, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to deinit gprs
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool asc_mdl_gprs_deinit(asc_context_t* const ctx, const asc_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  asc_item_t items[] = //[REQ][PREFIX][PARCE_TYPE][RPT][WAIT][STEPERROR][STEPOK][CB][FORMAT][...##VA_ARGS]
  {
    ASC_ITEM("AT+CIPSHUT"ASC_CMD_CRLF, "SHUT OK", ASC_PARCE_SIMCOM, 2, 100, 0, 1, NULL, NULL, ASC_NO_ARG),
  };
  if(!asc_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}
