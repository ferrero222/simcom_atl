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
#include "atl_modem.h"
#include "atl_modem_cmd.h"

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
 ** \brief  Function to proc data depends on sms CPIN response types
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ******************************************************************************/
atl_cmd_rsp_t atl_cmd_proc_gsm_init_cpin(uint8_t* p_rsp_str)
{
  atl_cmd_rsp_t  rsp_value = ATL_RSP_WAIT;
  uint8_t *rsp_str_table[ ] = {"ERROR","+CPIN: READY","+CPIN: SIM PIN","+CPIN: "};
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
    p = (uint8_t*)strchr(p,0x0a);
  }
  switch (rsp_type)
  {
    case 0: rsp_value = ATL_RSP_ERROR; atl_exe_fun_cb(false); break; /* ERROR */
    case 1: rsp_value = ATL_RSP_STEP +3; break; /* READY */
    case 2: rsp_value = ATL_RSP_STEP +1; break; /* +CPIN: SIM PIN */
    default: break;
  }
  return rsp_value;
}

/*******************************************************************************
 ** \brief  Function to proc data depends on sms CREG response types
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ******************************************************************************/
atl_cmd_rsp_t atl_cmd_proc_gsm_init_creg(uint8_t* p_rsp_str)
{
  atl_cmd_rsp_t  rsp_value = ATL_RSP_WAIT;
  uint8_t *rsp_str_table[ ] = {"ERROR","+CREG: "};
  int16_t  rsp_type = -1;
  uint8_t  n = 0;
  uint8_t  stat = 0;
  uint8_t  i = 0;
  uint8_t  *p = p_rsp_str;
  static uint8_t count = 0;
  while(p) 
  {
    while ( ATL_CMD_CR == *p || ATL_CMD_LF == *p)
    {
      p++;
    }
    for (i = 0; i < sizeof(rsp_str_table) /sizeof(rsp_str_table[0]); i++)
    {
      if (!strncmp(rsp_str_table[i], p, strlen(rsp_str_table[i])))
      {
        rsp_type = i;
        if (rsp_type == 1) sscanf(p +strlen(rsp_str_table[rsp_type]), "%d, %d", &n, &stat);
        break;
      }
    }
    p = (uint8_t*)strchr(p,0x0a);
  }
  switch (rsp_type)
  {
    case 0:  /* ERROR */
          atl_exe_fun_cb(false);
          rsp_value = ATL_RSP_ERROR;
          break;
    case 1:  /* +CREG */
          if(1 == stat) /* registered */
          {                          
            rsp_value = ATL_RSP_STEP +2;
          }
          else if(2 == stat)
          {                    
            if(count == 3) /* searching */
            {
              ATL_DEBUG("at+creg? return ATL_RSP_ERROR");    
              rsp_value = ATL_RSP_ERROR;
            }
            else
            {
              rsp_value = ATL_RSP_STEP -1;
              count++;
            }
          }
          else
          {
            rsp_value = ATL_RSP_ERROR;
          }
          break;
    default:
          break;
  }
  ATL_DEBUG("at+creg? return %d", rsp_value);    
  return rsp_value;
}

/*******************************************************************************
 ** \brief  Function to proc data depends on sms CGATT response types
 ** \param  p_rsp_str - string to response for this at cmd
 ** \retval @atl_cmd_rsp_t
 ******************************************************************************/
atl_cmd_rsp_t atl_cmd_proc_gsm_init_cgatt(uint8_t* p_rsp_str)
{
  atl_cmd_rsp_t  rsp_value = ATL_RSP_WAIT;
  uint8_t *rsp_str_table[] = {"ERROR","+CGATT: 1","+CGATT: 0"};
  int16_t  rsp_type = -1;
  uint8_t  i = 0;
  uint8_t  *p = p_rsp_str;
  static uint8_t count = 0;
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
  switch (rsp_type)
  {
    case 0:  /* ERROR */
          atl_exe_fun_cb(false);
          rsp_value = ATL_RSP_ERROR;
          break;
    case 1:  /* attached */
          rsp_value  = ATL_RSP_FINISH;
          ATL_DEBUG("at+cgatt? return ATL_RSP_OK");    
          break;
    case 2:  /* detached */
          if(count == 5)
          {
            ATL_DEBUG("at+cgatt? return ATL_RSP_ERROR");    
            rsp_value = ATL_RSP_ERROR;
          }
          else
          {
            rsp_value  = ATL_RSP_STEP -1;
          }
          count++;
          break;
    default:
          rsp_value = ATL_RSP_WAIT;
          break;
  }
  return rsp_value;
}

