/*******************************************************************************
 *                              ATL Example                         26.11.2025 *
 *                                 v1.0                                        *
 *       This example is showing how to catch incoming sms URC, get msg from   *
 *       it and send back as echo.                                             *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ddl.h"
#include "boot.h"
#include "proc.h"
#include "timers.h"
#include "sim_proc.h"
#include "hc32f460_utility.h"
#include "atl_core.h"
#include "atl_mdl_general.h"
#include "atl_mdl_sms.h"

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void atl_sms_urc_cb(const ringslice_t urc_slice);
static void atl_sms_read_cb(const bool result, void* const ctx, const void* const data);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Static methods for sms
 ** @param  None
 ** @return None
 ******************************************************************************/
static atl_urc_queue_t test_urc_sms = {"+CMTI:", atl_sms_urc_cb};

/* Get sms */
static void atl_sms_urc_cb(const ringslice_t urc_slice)
{
  atl_mdl_sms_msg_t sms = {0};
  ringslice_scanf(&urc_slice, "+CMTI:%*[^,],%d\x0d", &sms.index);
  if(0 != sms.index) atl_mdl_sms_read(atl_sms_read_cb, &sms, NULL);
}

/* Send echo sms */
static void atl_sms_read_cb(const bool result, void* const ctx, const void* const data)
{
  if(!result) return;
  atl_mdl_sms_msg_t* sms = (atl_mdl_sms_msg_t*)data;
  atl_mdl_sms_send_text(NULL, sms, NULL);
}

/*******************************************************************************
 ** \brief  Main function of project
 ** \param  None
 ** \retval None
 ******************************************************************************/ 
void main(void)
{
  atl_boot(); //init hardware, pins, uart, clock and etc.
  atl_init(my_printf, gsm_proc_send_data, (atl_ring_buffer_t*)&uart_gsm_ctx.rx_buf); //atl lib init
  atl_mdl_modem_init(NULL, NULL, NULL);
  atl_urc_enqueue(&test_urc_sms);
  while(1)
  {
    atl_timers_proc(); //proc programm timers (10ms included inside of it)
  }
}






