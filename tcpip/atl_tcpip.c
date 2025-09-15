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
#include "atl_tcpip.h"
#include "atl_tcpip_cmd.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
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
 ** \brief  URC "Sms ready" callback handler for modem ready to sms.
 ** \param  p_rsp_str - ptr to data string of urc
 ** \param  len       - len of data
 ** \retval None
 ** \note   Use this function to catch "Sms ready" URC. Use @atl_urc_queue_append()
 **         at the runtime or @atl_urc_entity_queue at the start of programm to
 **         proc this URC. You can add your custom code here.
 ******************************************************************************/
bool atl_tcpip_init(const char* apn, const char* usr, const char* pass, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  atl_cmd_entity_t at_cmd_init[] =
  {
    {"AT+CPIN?"ATL_CMD_END, 11, NULL},
    {"AT+GSN"ATL_CMD_END, 11, NULL},
    {"AT+GMM"ATL_CMD_END, 15, NULL},
    {"AT+GMR"ATL_CMD_END, 15, NULL},
    {"AT+CSQ"ATL_CMD_END, 15, NULL},
    {"AT+CREG?"ATL_CMD_END, 15, NULL},
    {"AT+CGATT=1"ATL_CMD_END, 15, NULL},
    {"AT+CIPMODE=0"ATL_CMD_END, 15, NULL},
    {"AT+CIPMUX=0"ATL_CMD_END, 15, NULL},
    {"AT+CIPSTATUS"ATL_CMD_END, 15, NULL},
    {"AT+CSTT="ATL_CMD_END, 15, NULL},
    {"AT+CIPSTATUS"ATL_CMD_END, 15, NULL},
    {"AT+CIFSR"ATL_CMD_END, 15, NULL},
  };
  uint8_t p_at_cmd[40] = {0};
  uint8_t p_at_cmd_msg[161] = {0};
  if(NULL == number) return false;
  if(msg_len > 160) return false;
  if(false == atl_sms_ready_state) return false;
  ATL_DEBUG("atl_simcom_sms_text_send");    
  sprintf(p_at_cmd,"%s\"%s\"%s", at_cmd_init[2].request, number, ATL_CMD_END);
  at_cmd_init[2].request = p_at_cmd;
  at_cmd_init[2].cmd_len = strlen(p_at_cmd);
  sprintf(p_at_cmd_msg, "%s%s%s", at_cmd_init[3].request, msg, ATL_CMD_CTRL_Z);
  at_cmd_init[3].request = p_at_cmd_msg;
  at_cmd_init[3].cmd_len = strlen(p_at_cmd_msg);
  result = atl_cmd_queue_fun_append(at_cmd_init, sizeof(at_cmd_init) /sizeof(at_cmd_init[0]), p_result_cb);
  return true;
}



atl_cmd_queue_append((atl_cmd_entity_t){"AT+CPIN?", "+CPIN:READY", &data, ATL_PARC_AT2, 3, ATL_RSP_STEP+1, ATL_RSP_STEP+3, atl_cmd_proc_def});