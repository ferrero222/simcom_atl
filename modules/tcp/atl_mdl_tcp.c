/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)03.10.2025 *
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
static uint16_t atl_mdl_gprs_send_len = 0;

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
bool atl_mdl_gprs_init(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+COPS?"ATL_CMD_CRLF,               "+COPS: 0", NULL, 2,  150, 1, 2, NULL, NULL),
    ATL_ITEM("AT+COPS=0"ATL_CMD_CRLF,                    NULL, NULL, 2,  150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CREG?"ATL_CMD_CRLF,  "+CREG: 0,1|+CREG: 0,5", NULL, 5,  600, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CGATT?"ATL_CMD_CRLF,             "+CGATT: 1", NULL, 10, 600, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
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
bool atl_mdl_gprs_socket_config(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPMODE?"ATL_CMD_CRLF,                 "+CIPMODE: 0", NULL, 1, 150, 0, 2, NULL, NULL),
    ATL_ITEM("AT+CIPMODE=0"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPMUX?"ATL_CMD_CRLF,                   "+CIPMUX: 0", NULL, 1, 150, 0, 2, NULL, NULL),
    ATL_ITEM("AT+CIPMUX=0"ATL_CMD_CRLF,                          NULL, NULL, 2, 500, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,            "STATE: IP START", NULL, 1, 250, 0, 2, NULL, NULL),
    ATL_ITEM("AT+CSTT=\"\",\"\",\"\""ATL_CMD_CRLF,               NULL, NULL, 2, 150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 2, NULL, NULL),
    ATL_ITEM("AT+CIICR"ATL_CMD_CRLF,                             NULL, NULL, 4, 800, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,          "STATE: IP GPRSACT", NULL, 1, 250, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIFSR"ATL_CMD_CRLF,                             NULL, NULL, 1, 150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPHEAD?"ATL_CMD_CRLF,                 "+CIPHEAD: 1", NULL, 1, 150, 0, 2, NULL, NULL),
    ATL_ITEM("AT+CIPHEAD=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSRIP?"ATL_CMD_CRLF,                 "+CIPSRIP: 1", NULL, 1, 150, 0, 2, NULL, NULL),
    ATL_ITEM("AT+CIPSRIP=1"ATL_CMD_CRLF,                         NULL, NULL, 2, 150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSHOWTP?"ATL_CMD_CRLF,             "+CIPSHOWTP: 1", NULL, 1, 150, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSHOWTP=1"ATL_CMD_CRLF,                       NULL, NULL, 2, 150, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
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
bool atl_mdl_gprs_socket_connect(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(101, param);
  char cipstart[128] = {0}; 
  atl_mdl_tcp_server_t* tcp = (atl_mdl_tcp_server_t*)param;
  snprintf(cipstart, sizeof(cipstart), "%sAT+CIPSTART=\"%s\",\"%s\",\"%s\"%s", ATL_CMD_SAVE, tcp->mode, tcp->ip, tcp->port, ATL_CMD_CRLF); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  { 
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: IP STATUS|STATE: TCP CLOSED",           NULL, 2, 250,  0, 1, NULL, NULL),
    ATL_ITEM(cipstart,                                           "CONNECT OK",           NULL, 5, 1600, 0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF,                  "STATE: CONNECT OK",           NULL, 2, 250,  0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSEND?"ATL_CMD_CRLF,                           "+CIPSEND:", "+CIPSEND: %u", 2, 150,  0, 1, NULL, NULL, &atl_mdl_gprs_send_len),         
    ATL_ITEM("AT+CIPQSEND?"ATL_CMD_CRLF,                       "+CIPQSEND: 0",           NULL, 1, 150,  0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPQSEND=0"ATL_CMD_CRLF,                                NULL,           NULL, 2, 150,  0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
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
bool atl_mdl_gprs_socket_send_recieve(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  DBC_REQUIRE(201, param);
  atl_mdl_tcp_server_t* tcp = (atl_mdl_tcp_server_t*)param;
  if(atl_mdl_gprs_send_len && atl_mdl_gprs_send_len < strlen(tcp->data)) return false;
  size_t size = strlen(ATL_CMD_SAVE) + strlen(tcp->data) + strlen(ATL_CMD_CTRL_Z) +1; 
  char cipsend[64] = {0}; 
  char* datacmd = (char*)tlsf_malloc(atl_get_init().atl_tlsf, size);
  if(!datacmd) 
  {
    ATL_DEBUG("[ERROR] Failed to allocate memory\n"); 
    atl_deinit(); 
    return false; 
  } 
  snprintf(cipsend, sizeof(cipsend), "%sAT+CIPSEND=%d%s", ATL_CMD_SAVE, size - strlen(ATL_CMD_SAVE), ATL_CMD_CRLF); 
  snprintf(datacmd, size, "%s%s%s", ATL_CMD_SAVE, datacmd, ATL_CMD_CTRL_Z); 
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPSTATUS"ATL_CMD_CRLF, "STATE: CONNECT OK", NULL, 2, 250,  0, 1, NULL, NULL),
    ATL_ITEM("AT+CIPSEND=",                              ">", NULL, 2, 250,  0, 1, NULL, NULL),
    ATL_ITEM("",                              "+IPD&SEND OK", NULL, 2, 250,  0, 0, NULL, NULL),
  };
  tlsf_free(atl_get_init().atl_tlsf, datacmd);
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to disconnect from socket
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  ctx    Context of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool atl_mdl_gprs_socket_disconnect(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPCLOSE=1"ATL_CMD_CRLF, "CLOSE OK", NULL, 2, 250, 0, 0, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
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
bool atl_mdl_gprs_deinit(const atl_entity_cb_t cb, const void* const param, const void* const ctx)
{
  atl_item_t items[] = //[REQ][PREFIX][FORMAT][RPT][WAIT][STEPERROR][STEPOK][CB][...##VA_ARGS]
  {
    ATL_ITEM("AT+CIPSHUT"ATL_CMD_CRLF, "SHUT OK", NULL, 2, 25,  0, 1, NULL, NULL),
  };
  if(!atl_enqueue(items, sizeof(items)/sizeof(items[0]), cb, 0, ctx)) return false;
  return true;
}
