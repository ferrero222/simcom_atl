/******************************************************************************
 *                              _    ____   ____                              *
 *                   ======    / \  / ___| / ___| ======       (c)03.10.2025  *
 *                   ======   / _ \ \___ \| |     ======           v1.0.0     *
 *                   ======  / ___ \ ___) | |___  ======                      *
 *                   ====== /_/   \_\____/ \____| ======                      *  
 *                                                                            *
 ******************************************************************************/
#ifndef __ASC_MDL_TCP_SERVER_H
#define __ASC_MDL_TCP_SERVER_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdint.h> 
#include <stdbool.h> 
#include <string.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
typedef void (*asc_stream_data_cb)(uint8_t* data, uint16_t len);

typedef struct {
  uint8_t* buffer;           // Accumulation buffer
  uint16_t buffer_size;      // Total buffer size
  uint16_t data_len;         // Current data length in buffer
  int16_t expected_len;      // Expected payload length (-1 if unknown)
  uint16_t header_len;       // Parsed header length
  bool packet_in_progress;   // Packet parsing in progress flag
} asc_tcp_stream_ctx_t;

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
 ** @brief  Initialize TCP stream context
 ** @param  ctx          Pointer to context
 ** @param  packet_size  Max packet size
 ** @return true if success, false otherwise
 ******************************************************************************/
bool asc_tcp_stream_ctx_init(asc_context_t* const asc_ctx,  asc_tcp_stream_ctx_t* stream_ctx, uint16_t packet_size);

/*******************************************************************************
 ** @brief  Cleanup TCP stream context
 ** @param  ctx          Pointer to context
 ******************************************************************************/
void asc_tcp_stream_ctx_cleanup(asc_context_t* const asc_ctx, asc_tcp_stream_ctx_t* stream_ctx);

/*******************************************************************************
 ** @brief  Handle TCP stream data
 ** @param  ctx          Stream context (per connection)
 ** @param  data         Pointer to incoming data
 ** @param  len          Data length
 ** @param  cb           Callback when full packet found
 ** @return true if data processed successfully
 ******************************************************************************/
bool asc_mld_tcp_server_stream_data_handler(asc_context_t* const asc_ctx,  asc_tcp_stream_ctx_t* stream_ctx, uint8_t* data, uint16_t len, asc_stream_data_cb cb);

 #endif //__ASC_MDL_TCP_SERVER_H 