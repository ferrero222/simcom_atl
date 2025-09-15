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
#include "atl_sms.h"
#include "atl_sms_cmd.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static bool atl_sms_ready_state = false;

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
void atl_urc_sms_ready_cb(uint8_t* p_rsp_str, uint16_t len)
{
  ATL_DEBUG("atl_urc_sms_ready_cb()");    
  atl_sms_ready_state = true;
}

/*******************************************************************************
 ** \brief  URC "+CMTI" callback handler for new incoming sms
 ** \param  p_rsp_str - ptr to data string of urc
 ** \param  len       - len of data
 ** \retval None
 ** \note   Use this function to catch "+CMTI" URC. Use @atl_urc_queue_append()
 **         at the runtime or @atl_urc_entity_queue at the start of programm to
 **         proc this URC. You can add your custom code here.
 **         ATTENTION: this handle using @atl_sms_msg_read() to immediately read
 **         sms msg, if you dont want to or you want to get callback after this
 **         read msg, edit this code.
 ******************************************************************************/
void atl_urc_new_sms_ind_cb(uint8_t* p_rsp_str, uint16_t len)
{
	atl_cmd_rsp_t rsp_value = ATL_RSP_WAIT;
  uint8_t *rsp_str_table[] = {"+CMTI: "};
  int16_t  rsp_type = -1;
  uint8_t  sz_buffer[20] = 0;
  uint16_t index = 0;
  uint8_t  i = 0;
  uint8_t  *p = p_rsp_str;
  ATL_DEBUG("atl_urc_new_sms_ind_cb()");    
  while(p) 
  {
    while ( ATL_CMD_CR == *p || ATL_CMD_LF == *p)
    {
      p++;
    }
    if (!strncmp(rsp_str_table[0], p, strlen(rsp_str_table[0])))
    {
      rsp_type = 0;
      if (rsp_type == 0)
      {
        ATL_DEBUG("atl_urc_new_sms_ind_cb: urc (%s)", p);    
        p = (uint8_t*)strchr(p,',');
        p = p + 1;
        ATL_DEBUG("atl_urc_new_sms_ind_cb: urc1 (%s)", p);    
        index = atoi(p); 
        ATL_DEBUG("atl_urc_new_sms_ind_cb: index(%d)", index);    
      }
      break;
    }
  }
  if(0 != index) atl_sms_msg_read(index, NULL);
}

/*******************************************************************************
 ** \brief  URC "+CMT" callback handler for new incoming sms with data parse in pdu
 ** \param  p_rsp_str - ptr to data string of urc
 ** \param  len       - len of data
 ** \retval None
 ** \note   Use this function to catch "+CMT" URC. Use @atl_urc_queue_append()
 **         at the runtime or @atl_urc_entity_queue at the start of programm to
 **         proc this URC. You can add your custom code here.
 **         ATTENTION: This function also parce incoming URC data, you can edit 
 **         this code to get some of this data if you need them.
 ******************************************************************************/
void atl_urc_flash_new_sms_cb(uint8_t* p_rsp_str, uint16_t len)
{
	atl_cmd_rsp_t  rsp_value = ATL_RSP_WAIT;
  uint8_t *rsp_str_table[ ] = {"+CMT: "};
  int16_t  rsp_type = -1;
  uint8_t  sz_buffer[20]= 0;
  uint16_t index = 0;
  uint8_t  i = 0;
  uint8_t  *p = p_rsp_str;
  uint8_t  *ptr_temp = NULL;
  uint8_t  data[ATL_SMS_MAX_TPDU_SIZE * 2] = {0};
  ATL_DEBUG("atl_urc_flash_new_sms_cb()");    
  while(p) 
  {
    while ( ATL_CMD_CR == *p || ATL_CMD_LF == *p)
    {
      p++;
    }
    if (!strncmp(rsp_str_table[0], p, strlen(rsp_str_table[0])))
    {
      rsp_type =0;
      if (rsp_type == 0)
      {
        ATL_DEBUG("atl_urc_flash_new_sms_cb: urc (%s)", p);    
        p = (uint8_t*)strchr(p, ATL_CMD_LF);
        p = p + 1;
        ptr_temp = (uint8_t*)strchr(p, ATL_CMD_CR);
        memcpy(data, p, ptr_temp -p);
        ATL_DEBUG("atl_urc_flash_new_sms_cb: urc1 (%s)", p);    
        atl_sms_decode_pdu(data);
      }
      break;
    }
  }
}

/*******************************************************************************
 ** \brief  Function to set PDU or TEXT mode for sms
 ** \param  n_format_type - 0 - PDU, 1 - TEXT.
 ** \param  p_result_cb   - callback for the end of this process.
 ** \retval bool
 ******************************************************************************/
bool atl_sms_formatl_set(const uint8_t n_format_type, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  uint8_t p_at_cpin[20] = {0};
  atl_cmd_entity_t at_cmd_init[] = {{"AT+CMGF=", 0, NULL},};
  ATL_DEBUG("atl_simcom_sms_formatl_set");    
  /* set at cmd func's range,must first set */
  sprintf(p_at_cpin,"%s%d%s", at_cmd_init[0].request, n_format_type, ATL_CMD_END);
  at_cmd_init[0].request = p_at_cpin;
  at_cmd_init[0].cmd_len = strlen(p_at_cpin);
  result = atl_cmd_queue_fun_append(at_cmd_init, sizeof(at_cmd_init) / sizeof(at_cmd_init[0]), p_result_cb);
  ATL_DEBUG("atl_simcom_sms_formatl_set, result=%d", result);    
  return result;
}

/*******************************************************************************
 ** \brief  Function to set PDU or TEXT mode for sms
 ** \param  number      - RP SC address.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func
 ******************************************************************************/
bool atl_sms_sc_set(const char* number, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  uint8_t p_at_cpin[50] = {0};
  atl_cmd_entity_t at_cmd_init[] = {{"AT+CSCA=", 0, NULL},};
  if(false == atl_sms_ready_state) return false;
  ATL_DEBUG("atl_simcom_sms_sc_set");    
  /* set at cmd func's range,must first set */
  sprintf(p_at_cpin,"%s%s%s", at_cmd_init[0].request, number, ATL_CMD_END);
  at_cmd_init[0].request = p_at_cpin;
  at_cmd_init[0].cmd_len = strlen(p_at_cpin);
  result = atl_cmd_queue_fun_append(at_cmd_init, sizeof(at_cmd_init) / sizeof(at_cmd_init[0]), p_result_cb);
  ATL_DEBUG("simcom_sms_sc_setsimcom_sms_sc_set, result=%d", result);    
  return result;
}

/*******************************************************************************
 ** \brief  Function to send sms in TEXT mode
 ** \param  number      - phone number where sms will be send
 ** \param  msg         - sms message.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func
 ******************************************************************************/
bool atl_sms_text_send(const char* number, const char* msg, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  atl_cmd_entity_t at_cmd_init[] =
  {
    {"AT+CMGF=1"ATL_CMD_END,       11, NULL},
    {"AT+CSCS=\"GSM\""ATL_CMD_END, 15, NULL},
    {"AT+CMGS=",                    0, atl_cmd_proc_sms_text_send_cmgs},
    {"",                            0, NULL},
  };
  uint8_t p_at_cmd[40] = {0};
  uint8_t p_at_cmd_msg[161] = {0};
  if(NULL == number) return false;
  if(strlen(msg) > 160) return false;
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

/*******************************************************************************
 ** \brief  Function to send sms in PDU mode
 ** \param  ptr_pdu     - ready-done pdu data.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func.
 ******************************************************************************/
bool atl_sms_pdu_send(const char* ptr_pdu, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  atl_cmd_entity_t at_cmd_init[] =
  {
    {"AT+CMGF=0"ATL_CMD_END, 11, NULL},
    {"AT+CMGS=",              0, atl_cmd_proc_sms_text_send_cmgs},
    {"",                      0, atl_cmd_proc_sms_text_send_cmgs},
  };
  uint8_t p_at_cmd[40] = {0};
  uint8_t p_at_cmd_msg[161] = {0};
  if(NULL == ptr_pdu) return false;
  if(false == atl_sms_ready_state) return false; //CHECK URC "SMS ready" and set flag if it ready to proc this function
  ATL_DEBUG("atl_simcom_sms_pdu_send");    
  sprintf(p_at_cmd,"%s%d%s",at_cmd_init[1].request, strlen(ptr_pdu), ATL_CMD_END);
  at_cmd_init[1].request = p_at_cmd;
  at_cmd_init[1].cmd_len = strlen(p_at_cmd);
  sprintf(p_at_cmd_msg, "%s%s%s", at_cmd_init[2].request, ptr_pdu, ATL_CMD_CTRL_Z);
  at_cmd_init[2].request = p_at_cmd_msg;
  at_cmd_init[2].cmd_len = strlen(p_at_cmd_msg);
  result = atl_cmd_queue_fun_append(at_cmd_init, sizeof(at_cmd_init) /sizeof(at_cmd_init[0]), p_result_cb);
  return true;
}

/*******************************************************************************
 ** \brief  Function to read sms message
 ** \param  index       - index of message.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func.
 **         ATTENTION: This function puts read msg data into @atl_sms_cmgr_data
 **         use @atl_sms_cmgr_data_get() to get this data once.
 ******************************************************************************/
bool atl_sms_msg_read(const uint16_t index, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  atl_cmd_entity_t at_cmd_init[] =
  {
    {"AT+CMGF=1"ATL_CMD_END, 11, NULL},
    {"AT+CMGR=",              0, atl_cmd_proc_sms_text_send_cmgr}
  };
  uint8_t p_at_cmd[40] = {0};
  if(false == atl_sms_ready_state) return false; //CHECK URC "SMS ready" and set flag if it ready to proc this function
  ATL_DEBUG("atl_simcom_sms_msg_read");    
  sprintf(p_at_cmd, "%s%d,%d%s", at_cmd_init[1].request, index, true,ATL_CMD_END);
  ATL_DEBUG("atl_simcom_sms_msg_read,at cmd(%s)", p_at_cmd);    
  at_cmd_init[1].request = p_at_cmd;
  at_cmd_init[1].cmd_len = strlen(p_at_cmd);
  result = atl_cmd_queue_fun_append(at_cmd_init, sizeof(at_cmd_init) / sizeof(at_cmd_init[0]), p_result_cb);
  ATL_DEBUG("atl_simcom_sms_msg_read, result= %d", result);    
  return true;
}

/*******************************************************************************
 ** \brief  Function to delete sms message
 ** \param  index       - index of message.
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ** \note   ATTENTION: this function checks for @atl_sms_ready_state. Use
 **         @atl_urc_sms_ready_cb to catch this URC and properly use this func.
 ******************************************************************************/
bool atl_sms_msg_delete(const uint16_t index, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  atl_cmd_entity_t at_cmd_init[] = {{"AT+CMGD=", 0, NULL},};
  uint8_t p_at_cmd[40] = {0};
  if(false == atl_sms_ready_state) return false; //CHECK URC "SMS ready" and set flag if it ready to proc this function
  ATL_DEBUG("atl_simcom_sms_msg_delete");    
  sprintf(p_at_cmd, "%s%d%s", at_cmd_init[0].request, index, ATL_CMD_END);
  at_cmd_init[0].request = p_at_cmd;
  at_cmd_init[0].cmd_len = strlen(p_at_cmd);
  result = atl_cmd_queue_fun_append(at_cmd_init, sizeof(at_cmd_init) / sizeof(at_cmd_init[0]), p_result_cb);
  ATL_DEBUG("atl_simcom_sms_msg_delete, result=%d", result);    
  return true;
}

/*******************************************************************************
 ** \brief  Function to set new SMS message inddication
 ** \param  mode - bfr  - check AT manual of CNMI
 ** \param  p_result_cb - callback for the end of this process.
 ** \retval bool
 ******************************************************************************/
bool atl_sms_cnmi_set(const uint8_t mode, const uint8_t mt, const uint8_t bm, const uint8_t ds, const uint8_t bfr, const atl_cmd_fun_rsp_cb p_result_cb)
{
  bool result = false;
  uint8_t p_at_cpin[40] = {0};
  atl_cmd_entity_t at_cmd_init[] = {{"AT+CNMI=", 0, NULL},};
  if(false == atl_sms_ready_state) return false; //CHECK URC "SMS ready" and set flag if it ready to proc this function
  ATL_DEBUG("atl_simcom_sms_cnmi_set");    
  sprintf(p_at_cpin, "%s%d,%d,%d,%d,%d%s", at_cmd_init[0].request, mode, mt, bm, ds, bfr, ATL_CMD_END);
  at_cmd_init[0].request = p_at_cpin;
  at_cmd_init[0].cmd_len = strlen(p_at_cpin);
  ATL_DEBUG("atl_simcom_sms_cnmi_set,(%s)", p_at_cpin);    
  result = atl_cmd_queue_fun_append(at_cmd_init, sizeof(at_cmd_init) / sizeof(at_cmd_init[0]), p_result_cb);
  ATL_DEBUG("atl_simcom_sms_cnmi_set,result=%d",result);    
  return result;
}

