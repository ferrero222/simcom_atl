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
#include "atl_mdl_tcp_server.h"
#include "atl_mdl_general.h"
#include "atl_mdl_tcp.h"
#include "dbc_assert.h"
#include "atl_chain.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_MDL_TCP_SERVER")

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
/*
  chain_step_t server_steps[] = 
  {    
    ATL_CHAIN("MODEM INIT",     "GPRS INIT",      "MODEM RESET", atl_mdl_modem_init,          NULL, NULL, 3),
    ATL_CHAIN("GPRS INIT",      "SOCKET CONFIG",  "MODEM RESET", atl_mdl_gprs_init,           NULL, NULL, 3),
    ATL_CHAIN("SOCKET CONFIG",  "SOCKET CONNECT", "GPRS DEINIT", atl_mdl_gprs_socket_config,  NULL, NULL, 3),
    ATL_CHAIN("SOCKET CONNECT", "MODEM RTD",      "GPRS DEINIT", atl_mdl_gprs_socket_connect, NULL, NULL, 3),

    ATL_CHAIN_LOOP_START(0), 
    ATL_CHAIN("MODEM RTD",          "SOCKET SEND RECIVE", "SOCKET DISCONNECT", atl_mdl_rtd, NULL, NULL, 3),
    ATL_CHAIN("SOCKET SEND RECIVE", "MODEM RTD",          "SOCKET DISCONNECT", atl_mdl_gprs_socket_send_recieve, NULL, NULL, 3),
    ATL_CHAIN_LOOP_END,

    ATL_CHAIN("SOCKET DISCONNECT", "SOCKET CONFIG", "MODEM RESET", atl_mdl_gprs_socket_disconnect, NULL, NULL, 3),
    ATL_CHAIN("GPRS DEINIT",       "GPRS INIT",     "MODEM RESET", atl_mdl_gprs_deinit, NULL, NULL, 3),
    ATL_CHAIN("MODEM RESET",       "GPRS INIT",     "MODEM RESET", atl_mdl_modem_reset, NULL, NULL, 3),
  };
*/

/*******************************************************************************
 ** @brief  Find IPD header in buffer
 ** @param  data         Buffer to search
 ** @param  len          Buffer length
 ** @return Pointer to start of IPD header or NULL if not found
 ******************************************************************************/
static uint8_t* find_ipd_header(uint8_t* data, uint16_t len)
{
  for (uint16_t i = 0; i + 3 < len; i++) 
  {
    if(data[i] == '+' && data[i+1] == 'I' && data[i+2] == 'P' && data[i+3] == 'D') return data + i;
  }
  return NULL;
}

/*******************************************************************************
 ** @brief  Parse IPD header
 ** @param  header_start Start of IPD header
 ** @param  data_len     Length of available data from header start
 ** @param  out_payload_len  Output parsed payload length
 ** @param  out_header_len   Output parsed header length
 ** @return true if header parsed successfully
 ******************************************************************************/
static bool parse_ipd_header(uint8_t* header_start, uint16_t data_len, int16_t* out_payload_len, uint16_t* out_header_len)
{
  if(data_len < 10) return false; // Minimum header length
  int payload_len = 0;
  char* colon_ptr = NULL;
  // Try to parse header format: "+IPD,<len>,TCP:"
  int ret = sscanf((char*)header_start, "+IPD,%d,TCP:", &payload_len);
  if(ret != 1 || payload_len <= 0) return false;
  // Find the colon to determine header length
  colon_ptr = strchr((char*)header_start, ':');
  if (!colon_ptr) return false;
  *out_payload_len = payload_len;
  *out_header_len = (uint16_t)(colon_ptr - (char*)header_start + 1);
  return true;
}

/*******************************************************************************
 ** @brief  Process complete packet
 ** @param  ctx          Stream context
 ** @param  cb           Callback for complete packets
 ******************************************************************************/
static void process_complete_packet(atl_tcp_stream_ctx_t* ctx, atl_stream_data_cb cb)
{
  uint8_t* payload = ctx->buffer + ctx->header_len;
  uint16_t total_packet_len = ctx->header_len + ctx->expected_len;
  ATL_DEBUG("[INFO] Found full TCP stream packet, len: %d", ctx->expected_len);
  if(cb) cb(payload, ctx->expected_len);
  // Remove processed packet from buffer
  ctx->data_len -= total_packet_len;
  if(ctx->data_len > 0) memmove(ctx->buffer, ctx->buffer + total_packet_len, ctx->data_len);
  // Reset parsing state
  ctx->expected_len = -1;
  ctx->header_len = 0;
  ctx->packet_in_progress = false;
}

/*******************************************************************************
 ** @brief  Initialize TCP stream context
 ** @param  ctx          Pointer to context
 ** @param  packet_size  Max packet size
 ** @return true if success, false otherwise
 ******************************************************************************/
bool atl_tcp_stream_ctx_init(atl_tcp_stream_ctx_t* ctx, uint16_t packet_size)
{
  DBC_REQUIRE(101, atl_get_init().init);
  ctx->buffer = (uint8_t*)atl_tlsf_malloc(packet_size);
  if (!ctx->buffer) 
  {
      ATL_DEBUG("[ERROR] Failed to allocate stream buffer", NULL);
      return false;
  }
  ctx->buffer_size = packet_size;
  ctx->data_len = 0;
  ctx->expected_len = -1;
  ctx->header_len = 0;
  ctx->packet_in_progress = false;
  return true;
}

/*******************************************************************************
 ** @brief  Cleanup TCP stream context
 ** @param  ctx          Pointer to context
 ******************************************************************************/
void atl_tcp_stream_ctx_cleanup(atl_tcp_stream_ctx_t* ctx)
{
  if (ctx && ctx->buffer) 
  {
    atl_tlsf_free(ctx->buffer, ctx->buffer_size);
    ctx->buffer = NULL;
    ctx->buffer_size = 0;
    ctx->data_len = 0;
  }
}

/*******************************************************************************
 ** @brief  Handle TCP stream data
 ** @param  ctx          Stream context (per connection)
 ** @param  data         Pointer to incoming data
 ** @param  len          Data length
 ** @param  cb           Callback when full packet found
 ** @return true if data processed successfully
 ******************************************************************************/
bool atl_mld_tcp_server_stream_data_handler(atl_tcp_stream_ctx_t* ctx, uint8_t* data, uint16_t len, atl_stream_data_cb cb)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(201, atl_get_init().init);
  DBC_REQUIRE(202, ctx != NULL);
  DBC_REQUIRE(203, ctx->buffer != NULL);
  // Check if new data fits in buffer
  if (ctx->data_len + len > ctx->buffer_size) 
  {
    ATL_DEBUG("[INFO] Stream buffer overflow, resetting", NULL);
    ctx->data_len = 0;
    ctx->expected_len = -1;
    ctx->header_len = 0;
    ctx->packet_in_progress = false;
    ATL_CRITICAL_EXIT
    return false;
  }
  // Append new data to buffer
  memcpy(ctx->buffer + ctx->data_len, data, len);
  ctx->data_len += len;
  // Process all complete packets in buffer
  while (ctx->data_len > 0) 
  {
    if (!ctx->packet_in_progress) 
    {
      // Look for IPD header
      uint8_t* ipd_start = find_ipd_header(ctx->buffer, ctx->data_len);
      if (!ipd_start) 
      {
        // No header found, keep last few bytes that could be start of header
        const uint16_t keep_bytes = 4;
        if (ctx->data_len > keep_bytes) 
        {
          memmove(ctx->buffer, ctx->buffer + ctx->data_len - keep_bytes, keep_bytes);
          ctx->data_len = keep_bytes;
        }
        break;
      }
      
      // Remove data before header
      uint16_t header_offset = (uint16_t)(ipd_start - ctx->buffer);
      if (header_offset > 0) 
      {
        memmove(ctx->buffer, ipd_start, ctx->data_len - header_offset);
        ctx->data_len -= header_offset;
      }
      
      // Parse header
      if (!parse_ipd_header(ctx->buffer, ctx->data_len, &ctx->expected_len, &ctx->header_len)) 
      {
        // Invalid header, skip 4 bytes ("+IPD") and continue
        memmove(ctx->buffer, ctx->buffer + 4, ctx->data_len - 4);
        ctx->data_len -= 4;
        ATL_DEBUG("[INFO] Invalid stream header, skipping", NULL);
        continue;
      }
      
      ctx->packet_in_progress = true;
    }
  
    // Check if we have complete packet
    if (ctx->packet_in_progress && ctx->data_len >= ctx->header_len + ctx->expected_len)
    {
        process_complete_packet(ctx, cb);
    } 
    else 
    {
      // Incomplete packet, wait for more data
      ATL_DEBUG("[INFO] Waiting for full stream data", NULL);
      break;
    }
  }
  
  ATL_CRITICAL_EXIT
  return true;
}







