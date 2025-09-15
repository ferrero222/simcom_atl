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
/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
 typedef struct atl_sms_cmgr_data_t{
  uint8_t number[40];
  uint8_t msg[161];
} atl_sms_cmgr_data_t;

static atl_sms_cmgr_data_t atl_sms_cmgr_data = {0};
static bool atl_sms_cmgr_data_f = false;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Function get read msg data
 ** \param  None
 ** \retval @atl_sms_cmgr_data_t
 ******************************************************************************/
atl_sms_cmgr_data_t* atl_sms_cmgr_data_get(void)
{
  if(!atl_sms_cmgr_data_f) return NULL;
  atl_sms_cmgr_data_f = false;
  return &atl_sms_cmgr_data;
}

/*******************************************************************************
 ** \brief  Function to proc data depends on sms CMGS response types
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ******************************************************************************/
atl_cmd_rsp_t atl_cmd_proc_sms_text_send_cmgs(uint8_t* p_rsp_str)
{
  atl_cmd_rsp_t  rsp_value = ATL_RSP_WAIT;
  uint8_t *rsp_str_table[ ] = {"ERROR", "> ", "OK"};
  int16_t  rsp_type = -1;
  uint8_t  i = 0;
  uint8_t  *p = p_rsp_str;
  while(p) 
  {
    while ( ATL_CMD_CR == *p || ATL_CMD_LF == *p)
    {
        p++;
    }
    for (i = 0; i < sizeof(rsp_str_table) / sizeof(rsp_str_table[0]); i++)
    {
      if (!strncmp(rsp_str_table[i], p, strlen(rsp_str_table[i])))
      {
        rsp_type = i;
        break;
      }
    }
    p = (uint8_t*)strchr(p,ATL_CMD_LF);
  }
  ATL_DEBUG("atl_cmd_cb_cmgs(), rsp_type(%d)",rsp_type);    
  switch (rsp_type)
  {
    case 0:  rsp_value = ATL_RSP_ERROR; atl_exe_fun_cb(false); break; /* ERROR */
    case 1:  rsp_value = ATL_RSP_CONTINUE; break; /* >  */
    case 2:  rsp_value = ATL_RSP_FINISH;   break;/* OK */
    default: rsp_value = ATL_RSP_WAIT;     break;
  }
  return rsp_value;
}

/*******************************************************************************
 ** \brief  Function to proc data depends on sms CMGR response types
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ******************************************************************************/
atl_cmd_rsp_t atl_cmd_proc_sms_text_send_cmgr(uint8_t* p_rsp_str)
{
  atl_cmd_rsp_t  rsp_value = ATL_RSP_WAIT;
  uint8_t *rsp_str_table[ ] = {"ERROR","+CMGR: "};
  int16_t rsp_type = -1;
  uint8_t i = 0;
  uint8_t *p = p_rsp_str;
  uint8_t *p_num_end = NULL;
  uint8_t *p_msg_end = NULL;
  uint16_t len = 0;
  while(p) 
  {
    while ( ATL_CMD_CR == *p || ATL_CMD_LF == *p)
    {
      p++;
    }
    for (i = 0; i < sizeof(rsp_str_table) / sizeof(rsp_str_table[0]); i++)
    {
        if (!strncmp(rsp_str_table[i], p, strlen(rsp_str_table[i])))
        {
          rsp_type = i;
          break;
        }
    }
    p = (uint8_t*)strchr(p, ATL_CMD_LF);
  }
  ATL_DEBUG("atl_cmd_cb_cmgr: rsp_type(%d)", rsp_type);    
  switch (rsp_type)
  {
    case 0: rsp_value = ATL_RSP_ERROR; atl_exe_fun_cb(false); break; /* ERROR */
    case 1:  /* +CMGR:  */
          rsp_value  = ATL_RSP_FINISH;
          p = (uint8_t*)strchr(p,',');
          p = p+2;
          p_num_end = (char*)strchr(p,',');
          len = p_num_end - p - 1;
          memcpy(atl_sms_cmgr_data.number, p, len);
          p = (uint8_t*)strchr(p, ATL_CMD_LF);
          p_msg_end = (uint8_t*)strchr(p, ATL_CMD_CR);
          len = p_msg_end - p;
          memcpy(atl_sms_cmgr_data.msg, p, len);
          atl_sms_cmgr_data_f = true;
          ATL_DEBUG("atl_cmd_cb_cmgr: number(%s),msg(%s)", atl_sms_cmgr_data.number, atl_sms_cmgr_data.msg);    
          break;
    default: rsp_value = ATL_RSP_WAIT; break;
  }
  return rsp_value;
}