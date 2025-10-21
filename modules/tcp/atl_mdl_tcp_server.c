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
#include "atl_mdl_general.h"
#include "atl_mdl_tcp.h"
#include "dbc_assert.h"
#include "atl_chain.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_TCP_SERVER");

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
 ** @brief  Function to cerate an instance of tcp server
 ** @param  None. 
 ** @return chain instance
 ******************************************************************************
atl_chain_t* atl_mld_tcp_server_create(atl_mdl_tcp_server_t* tcp)
{
  chain_step_t server_steps[] = 
  {    
    ATL_CHAIN("MODEM INIT",     "GPRS INIT",      "MODEM RESET", atl_mdl_modem_init,          NULL, NULL, 3),
    ATL_CHAIN("GPRS INIT",      "SOCKET CONFIG",  "MODEM RESET", atl_mdl_gprs_init,           NULL, NULL, 3),
    ATL_CHAIN("SOCKET CONFIG",  "SOCKET CONNECT", "GPRS DEINIT", atl_mdl_gprs_socket_config,  NULL, NULL, 3),
    ATL_CHAIN("SOCKET CONNECT", "MODEM RTD",      "GPRS DEINIT", atl_mdl_gprs_socket_connect, NULL, NULL, 3),

    ATL_CHAIN_LOOP_START(0), 
    ATL_CHAIN("MODEM RTD",          "SOCKET SEND RECIVE", "SOCKET DISCONNECT", atl_mdl_gprs_socket_connect, NULL, NULL, 3),
    ATL_CHAIN("SOCKET SEND RECIVE", "MODEM RTD",          "SOCKET DISCONNECT", atl_mdl_gprs_socket_connect, NULL, NULL, 3),
    ATL_CHAIN_LOOP_END,

    ATL_CHAIN("SOCKET DISCONNECT", "SOCKET CONFIG", "MODEM RESET", atl_mdl_gprs_socket_connect, NULL, NULL, 3),
    ATL_CHAIN("GPRS DEINIT",       "GPRS INIT",     "MODEM RESET", atl_mdl_gprs_socket_connect, NULL, NULL, 3),
    ATL_CHAIN("MODEM RESET",       "GPRS INIT",     "MODEM RESET", atl_mdl_gprs_socket_connect, NULL, NULL, 3),
  };
  atl_chain_t *chain = atl_chain_create("ServerTCP", server_steps, sizeof(server_steps)/sizeof(server_steps[0]), NULL);
  return chain;
}
*/

/*******************************************************************************
 ** @brief  Handler of tcp stream data
 ** @param  cb  cb when proc will be done. 
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
void atl_mld_tcp_server_stream_data_handler(uint8_t* data, uint16_t len)
{
  static uint8_t buffer[STREAM_DATA_BUFFER_SIZE];
  static uint16_t buffer_len = 0;      
  static int expected_data_len = -1;  
  static uint16_t header_len = 0;
  if(buffer_len + len > STREAM_DATA_BUFFER_SIZE)
  {
    ATL_DEBUG("[INFO] overwriting stream data",__LINE__ , __FUNCTION__);
    buffer_len = 0;
    expected_data_len = -1;
    header_len = 0;
    return;
  }
  memcpy(buffer + buffer_len, data, len);
  buffer_len += len;
  while(1) //main work with stream
  {
    if(expected_data_len < 0) //no waiting data
    {
      uint8_t* pos = NULL;
      uint16_t i = 0;
      for(i = 0; i + 8 < buffer_len; i++) 
      {
        if (buffer[i] == '+' && buffer[i+1]=='I' && buffer[i+2]=='P' && buffer[i+3]=='D') 
        {
          pos = buffer + i;
          break;
        }
      }
      if(!pos) //sequence wasnt found, delete trash and save last 4 bytes in case if that is start of "+IPD"
      {
        if(buffer_len > 4) 
        {
          memmove(buffer, buffer + buffer_len - 4, 4);
          buffer_len = 4;
        }
        break; //leave
      }
      { //trying to parce header and get data len
        int data_len = 0;
        uint16_t pos_index = pos - buffer;
        int ret = 0;
        if(pos_index > 0) //delete trash
        {
          memmove(buffer, pos, buffer_len - pos_index);
          buffer_len -= pos_index;
          pos = buffer;
        }
        ret = sscanf((char*)pos, "+IPD,%d,TCP:", &data_len);
        if(ret == 1 && data_len != 0)
        {
          char* p_colon = NULL;
          expected_data_len = data_len;
          header_len = (uint16_t)strlen("+IPD,") + 0; 
          p_colon = strchr((char*)pos, ':');
          if(p_colon)
          {
            header_len = (uint16_t)((uint8_t*)p_colon - pos + 1);
            continue;
          }
        } 
        { //error proc, we found bad +IPD, delete it and proc again
          memmove(buffer, buffer +4, 4);
          buffer_len -= 4;
          ATL_DEBUG("[INFO] incorrect stream header data",__LINE__ , __FUNCTION__);
        }
      }
    }
    if(expected_data_len >= 0)
    {
      if(buffer_len >= header_len +expected_data_len) //we got full data block, lets do CB and check for new stream block
      {
        uint8_t* payload = buffer +header_len;
        uint16_t total_len = 0;
        ATL_DEBUG("[INFO] found full correct stream tcp packet, len: %d", __LINE__ , __FUNCTION__, expected_data_len);    
        if(sm_ctx && sm_ctx->rx_cb) sm_ctx->rx_cb(payload, expected_data_len);
        total_len = header_len +expected_data_len;
        buffer_len -= total_len;
        if (buffer_len > 0)  memmove(buffer, buffer + total_len, buffer_len);
        expected_data_len = -1;
        header_len = 0;
      } 
      else //no data, leave
      {
        ATL_DEBUG("[INFO] wait full stream data",__LINE__ , __FUNCTION__);
        break;
      }
    }
  }
}










