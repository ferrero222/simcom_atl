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
#include "asc_mdl_tcp_server.h"
#include "asc_mdl_general.h"
#include "asc_mdl_tcp.h"
#include "dbc_assert.h"
#include "asc_chain.h"
#include <stdio.h>
#include <string.h>
#include "asc_port.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ASC_MDL_TCP_SERVER")

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
static void process_complete_packet(asc_context_t* const asc_ctx, asc_tcp_stream_ctx_t* stream_ctx, asc_stream_data_cb cb)
{
  uint8_t* payload = stream_ctx->buffer + stream_ctx->header_len;
  uint16_t total_packet_len = stream_ctx->header_len + stream_ctx->expected_len;
  ASC_DEBUG(asc_ctx, "[ASC][INFO] Found full TCP stream packet, len: %d", stream_ctx->expected_len);
  if(cb) cb(payload, stream_ctx->expected_len);
  // Remove processed packet from buffer
  stream_ctx->data_len -= total_packet_len;
  if(stream_ctx->data_len > 0) memmove(stream_ctx->buffer, stream_ctx->buffer + total_packet_len, stream_ctx->data_len);
  // Reset parsing state
  stream_ctx->expected_len = -1;
  stream_ctx->header_len = 0;
  stream_ctx->packet_in_progress = false;
}

/*******************************************************************************
 ** @brief  Initialize TCP stream context
 ** @param  ctx          Pointer to context
 ** @param  packet_size  Max packet size
 ** @return true if success, false otherwise
 ******************************************************************************/
bool asc_tcp_stream_ctx_init(asc_context_t* const asc_ctx, asc_tcp_stream_ctx_t* stream_ctx, uint16_t packet_size)
{
  DBC_REQUIRE(101, asc_get_init(asc_ctx).init);
  stream_ctx->buffer = (uint8_t*)asc_malloc(asc_ctx, packet_size);
  if (!stream_ctx->buffer) 
  {
    ASC_DEBUG(asc_ctx, "[ASC][ERROR] Failed to allocate stream buffer", NULL);
    return false;
  }
  stream_ctx->buffer_size = packet_size;
  stream_ctx->data_len = 0;
  stream_ctx->expected_len = -1;
  stream_ctx->header_len = 0;
  stream_ctx->packet_in_progress = false;
  return true;
}

/*******************************************************************************
 ** @brief  Cleanup TCP stream context
 ** @param  ctx          Pointer to context
 ******************************************************************************/
void asc_tcp_stream_ctx_cleanup(asc_context_t* const asc_ctx, asc_tcp_stream_ctx_t* stream_ctx)
{
  if (stream_ctx && stream_ctx->buffer) 
  {
    asc_free(asc_ctx, stream_ctx->buffer);
    stream_ctx->buffer = NULL;
    stream_ctx->buffer_size = 0;
    stream_ctx->data_len = 0;
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
bool asc_mld_tcp_server_stream_data_handler(asc_context_t* const asc_ctx, asc_tcp_stream_ctx_t* stream_ctx, uint8_t* data, uint16_t len, asc_stream_data_cb cb)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(201, asc_get_init(asc_ctx).init);
  DBC_REQUIRE(202, stream_ctx != NULL);
  DBC_REQUIRE(203, stream_ctx->buffer != NULL);
  // Check if new data fits in buffer
  if (stream_ctx->data_len + len > stream_ctx->buffer_size) 
  {
    ASC_DEBUG(asc_ctx, "[ASC][INFO] Stream buffer overflow, resetting", NULL);
    stream_ctx->data_len = 0;
    stream_ctx->expected_len = -1;
    stream_ctx->header_len = 0;
    stream_ctx->packet_in_progress = false;
    ASC_CRITICAL_EXIT
    return false;
  }
  // Append new data to buffer
  memcpy(stream_ctx->buffer + stream_ctx->data_len, data, len);
  stream_ctx->data_len += len;
  // Process all complete packets in buffer
  while (stream_ctx->data_len > 0) 
  {
    if (!stream_ctx->packet_in_progress) 
    {
      // Look for IPD header
      uint8_t* ipd_start = find_ipd_header(stream_ctx->buffer, stream_ctx->data_len);
      if (!ipd_start) 
      {
        // No header found, keep last few bytes that could be start of header
        const uint16_t keep_bytes = 4;
        if (stream_ctx->data_len > keep_bytes) 
        {
          memmove(stream_ctx->buffer, stream_ctx->buffer + stream_ctx->data_len - keep_bytes, keep_bytes);
          stream_ctx->data_len = keep_bytes;
        }
        break;
      }
      
      // Remove data before header
      uint16_t header_offset = (uint16_t)(ipd_start - stream_ctx->buffer);
      if (header_offset > 0) 
      {
        memmove(stream_ctx->buffer, ipd_start, stream_ctx->data_len - header_offset);
        stream_ctx->data_len -= header_offset;
      }
      
      // Parse header
      if (!parse_ipd_header(stream_ctx->buffer, stream_ctx->data_len, &stream_ctx->expected_len, &stream_ctx->header_len)) 
      {
        // Invalid header, skip 4 bytes ("+IPD") and continue
        memmove(stream_ctx->buffer, stream_ctx->buffer + 4, stream_ctx->data_len - 4);
        stream_ctx->data_len -= 4;
        ASC_DEBUG(asc_ctx, "[ASC][INFO] Invalid stream header, skipping", NULL);
        continue;
      }
      
      stream_ctx->packet_in_progress = true;
    }
  
    // Check if we have complete packet
    if (stream_ctx->packet_in_progress && stream_ctx->data_len >= stream_ctx->header_len + stream_ctx->expected_len)
    {
        process_complete_packet(asc_ctx, stream_ctx, cb);
    } 
    else 
    {
      // Incomplete packet, wait for more data
      ASC_DEBUG(asc_ctx, "[ASC][INFO] Waiting for full stream data", NULL);
      break;
    }
  }
  
  ASC_CRITICAL_EXIT
  return true;
}







