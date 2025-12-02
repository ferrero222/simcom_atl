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
#include "dbc_assert.h"
#include "asc_mdl_general.h"
#include "stdlib.h"
#include "asc_port.h"
   
/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ASC_CORE")

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static bool asc_raw_parcer(asc_context_t* const ctx, const ringslice_t rs_me, const asc_item_t* const item, const asc_entity_t* const entity);

static bool asc_simcom_parcer(asc_context_t* const ctx, const ringslice_t rs_me, const asc_item_t* const item, const asc_entity_t* const entity);
static void asc_simcom_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req);
static void asc_simcom_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res);
static void asc_simcom_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data);
static bool asc_simcom_parcer_post_proc(asc_context_t* const ctx, const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, 
                                        const ringslice_t* const rs_data, const asc_item_t* const item, const asc_entity_t* const entity);

static bool asc_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern);
static bool asc_cmd_sscanf(const ringslice_t* const rs_data, const asc_item_t* const item);

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
 ** @brief  Function to parce the RX ring buffer for proc with existing entities in queue.
 **         Using choosen parcer for item.
 ** @param  ctx     core context
 ** @param  entity  current proc entity
 ** @param  item    current proc item
 ** @return true  - success parce
 **         false - failure parce and handle / or no data in buff
 ******************************************************************************/
static bool asc_cmd_ring_parcer(asc_context_t* const ctx, const asc_entity_t* const entity, const asc_item_t* const item, const ringslice_t rs_me)
{
  DBC_REQUIRE(100, ctx);
  DBC_REQUIRE(101, ctx->init_struct.init);
  DBC_REQUIRE(102, item);
  DBC_REQUIRE(103, entity);

  bool res = false;
  
  if(item->answ.prefix && strncmp(item->answ.prefix, ASC_CMD_FORCE, strlen(ASC_CMD_FORCE)) == 0) res = true;

  if(ringslice_is_empty(&rs_me)) return 0; // no data

  if(!res)
  {
    switch(item->parce_type)
    {
      case ASC_PARCE_SIMCOM: res = asc_simcom_parcer(ctx, rs_me, item, entity); break;
      case ASC_PARCE_RAW:    res = asc_raw_parcer(ctx, rs_me, item, entity);    break;
      default: break;
    }
  }

  #ifndef ASC_TEST
  if(res > 0) asc_printf_from_ring(ctx, rs_me, "[RX]");
  #endif
  return res;
}

/*******************************************************************************
 ** @brief  Implementations for raw data parcer kind of: > RAW DATA
 ** @param  ctx     core context
 ** @param  rs_me   slice of origin buffer
 ** @param  entity  current proc entity
 ** @param  item    current proc item
 ** @return true - success parce
 **         false - failure parce and handle / or no data in buff
 ******************************************************************************/
static bool asc_raw_parcer(asc_context_t* const ctx, const ringslice_t rs_me, const asc_item_t* const item, const asc_entity_t* const entity)
{
  DBC_REQUIRE(117, ctx);
  DBC_REQUIRE(118, item);
  DBC_REQUIRE(119, entity);
  
  int res = true;  

  ringslice_cnt_t proced_data = 0;
  ringslice_t rs_data = rs_me;
  
  if(item->answ.prefix)
  {
    char* prefix = strncmp(item->answ.prefix, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE)) ? item->answ.prefix : item->answ.prefix +strlen(ASC_CMD_SAVE);
    res = asc_string_boolean_ops(&rs_data, prefix);
    if(item->answ.format) res = asc_cmd_sscanf(&rs_data, item);
    if(res) proced_data = (uint16_t)rs_data.last;
  }

  #ifndef ASC_TEST
  if(proced_data)
  {
    ctx->init_struct.rx_buff->tail = proced_data;
    ringslice_t tmp = ringslice_initializer(rs_me.buf, rs_me.buf_size, rs_me.first, proced_data);
    ctx->init_struct.rx_buff->count -= ringslice_len(&tmp);
  }
  #endif
  if(item->answ.cb) item->answ.cb(ringslice_initializer(rs_me.buf, rs_me.buf_size, rs_data.first, rs_me.last), res, entity->data);
  return res; 
}

/*******************************************************************************
 ** @brief  Implementations for simcom data parcer kind of: ECHO\r\r\nRES\r\nDATA\r\n
 ** @param  ctx     core context
 ** @param  rs_me   slice of origin buffer
 ** @param  entity  current proc entity
 ** @param  item    current proc item
 ** @return true - success parce
 **         false - failure parce and handle / or no data in buff
 ******************************************************************************/
static bool asc_simcom_parcer(asc_context_t* const ctx, const ringslice_t rs_me, const asc_item_t* const item, const asc_entity_t* const entity)
{
  DBC_REQUIRE(120, ctx);
  DBC_REQUIRE(121, item);
  DBC_REQUIRE(122, entity);

  bool res = false;

  ringslice_t rs_req = {0};
  ringslice_t rs_res = {0}; 
  ringslice_t rs_data = {0};

  asc_simcom_parcer_find_rs_req(&rs_me, &rs_req, item->req); // Find request and response in buffer
  asc_simcom_parcer_find_rs_res(&rs_me, &rs_req, &rs_res);
  asc_simcom_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data); // Extract data section
  res = asc_simcom_parcer_post_proc(ctx, &rs_me, &rs_req, &rs_res, &rs_data, item, entity); // Proc data
  return res;
}

/** 
 * @brief Find req echo
 */
static void asc_simcom_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* req)
{
  DBC_REQUIRE(125, me); 
  DBC_REQUIRE(127, rs_req); 

  if(!req) return;

  if(strncmp(req, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE)) == 0) req += strlen(ASC_CMD_SAVE);

  *rs_req = ringslice_strnstr(me, req, strlen(req) - 1); //Request echo returns only CR, so ignore LF
}

/** 
 * @brief Find result ring slice 
 */
static void asc_simcom_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res)
{
  DBC_REQUIRE(129, rs_req); 
  DBC_REQUIRE(131, rs_res); 

  if(ringslice_is_empty(rs_req)) return;

  ringslice_t tmp = ringslice_subslice_after(me, rs_req, 0);

  if(ringslice_is_empty(&tmp)) return;

  *rs_res = ringslice_strstr(&tmp, ASC_CMD_ERROR);
  if(ringslice_is_empty(rs_res)) *rs_res = ringslice_strstr(&tmp, ASC_CMD_OK);
}

/** 
 * @brief Find and data ring slice 
 */
static void asc_simcom_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data)
{
  DBC_REQUIRE(134, me); 
  DBC_REQUIRE(135, rs_req);  
  DBC_REQUIRE(136, rs_res); 
  DBC_REQUIRE(137, rs_data);

  const uint8_t crlf_len = strlen(ASC_CMD_CRLF);

  if(ringslice_is_empty(rs_req) && ringslice_is_empty(rs_res)) //No request/response, data is the entire buffer \r\nDATA\r\n
  { 
    *rs_data = *me;
  } 
  else if(!ringslice_is_empty(rs_req) && ringslice_is_empty(rs_res)) //Request but no response yet, data is after request REQ\r\r\nDATA\r\n
  {
    *rs_data = ringslice_subslice_after(me, rs_req, 0);
  } 
  else if(!ringslice_is_empty(rs_req) && !ringslice_is_empty(rs_res)) //Both request and response present
  {
    ringslice_t after_req = ringslice_subslice_after(me, rs_req, strlen(ASC_CMD_CRLF"OK"ASC_CMD_CRLF));
    *rs_data = ringslice_subslice_equals(&after_req, rs_res) 
               ? ringslice_subslice_after(me, rs_res, 0) // REQ\r\r\nOK\r\n\r\nDATA\r\n
               : ringslice_subslice_gap(rs_req, rs_res); // REQ\r\r\nDATA\r\n\r\nOK\r\n
  }

  if((ringslice_len(rs_data) <= 2 *crlf_len) || (ringslice_strncmp(rs_data, ASC_CMD_CRLF, 2))) // If data more than 2bytes and first 2 bytes its CRLF
  {
    *rs_data = (ringslice_t){0};
    return;
  }
  *rs_data = ringslice_subslice_with_suffix(rs_data, crlf_len, ASC_CMD_CRLF); 
  if(ringslice_is_empty(rs_data)) return;
  *rs_data = ringslice_subslice(rs_data, crlf_len, ringslice_len(rs_data)- crlf_len); // Extract clean data (without surrounding CRLFx2)
}

/** 
 * @brief Proc all found slices 
 * @note [REQ][RES][DATA]
 *        NULL NULL NULL  - unhandle state
 *        REQ  NULL NULL  - unhandle state
 *        REQ  RES  NULL  - handle state
 *        REQ  NULL DATA  - handle state
 *        REQ  RES  DATA  - handle state
 *        NULL RES  NULL  - unhandle state
 *        NULL RES  DATA  - unhandle state
 *        NULL NULL DATA  - handle state
 * @return  false - failure / true - success
 */
static bool asc_simcom_parcer_post_proc(asc_context_t* const ctx, const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, 
                                        const ringslice_t* const rs_data, const asc_item_t* const item, const asc_entity_t* const entity)
{
  DBC_REQUIRE(139, ctx); 
  DBC_REQUIRE(140, me); 
  DBC_REQUIRE(141, rs_req);  
  DBC_REQUIRE(142, rs_res); 
  DBC_REQUIRE(143, rs_data);
  DBC_REQUIRE(144, item);
  DBC_REQUIRE(145, entity);
  
  int res = false;  

  bool rs_req_exist  = !ringslice_is_empty(rs_req);
  bool rs_res_exist  = !ringslice_is_empty(rs_res);
  bool rs_data_exist = !ringslice_is_empty(rs_data);

  ringslice_cnt_t proced_data = 0;
  
  char* prefix = item->answ.prefix;
  if(prefix && !strncmp(prefix, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE))) prefix += strlen(ASC_CMD_SAVE);

  switch((rs_req_exist << 2) | (rs_res_exist << 1) | rs_data_exist) // Bitmask: REQ[bit2] RES[bit1] DATA[bit0]
  {
      case 0x01: //0b001 - NULL NULL DATA (PREFIX)
           if(!prefix) break;
           res = asc_string_boolean_ops(rs_data, prefix);
           if(item->answ.format) res = asc_cmd_sscanf(rs_data, item);
           if(res) proced_data = (uint16_t)rs_data->last;
           break;
      case 0x05: //0b101 - REQ NULL DATA (REQ +PREFIX)
           if(!item->req || !prefix) break;
           res = asc_string_boolean_ops(rs_data, prefix);
           if(item->answ.format) res = asc_cmd_sscanf(rs_data, item);
           if(res) proced_data = (uint16_t)rs_data->last;
           break;
      case 0x06: //0b110 - REQ RES NULL (REQ, NO PREFIX, NO FORMAT)
           if(!item->req || ringslice_strcmp(rs_res, ASC_CMD_ERROR) == 0) res = false;
           else if(prefix || item->answ.format) res = false;
           else res = true;
           if(res) proced_data = (uint16_t)rs_res->last;
           break;
      case 0x07: //0b111 - REQ RES DATA (REQ)
           if(!item->req) break;
           else if(ringslice_strcmp(rs_res, ASC_CMD_ERROR) == 0) res = false;
           else res = true;
           if(res){
             if(prefix) res = asc_string_boolean_ops(rs_data, prefix);
             if(item->answ.format) res = asc_cmd_sscanf(rs_data, item);
           }
           #ifndef ASC_TEST
           if(ringslice_is_later_than(rs_res, rs_data)) proced_data = (uint16_t)rs_res->last;
           else                                         proced_data = (uint16_t)rs_data->last;
           #endif
           break;
      default: 
           break;
  }
  #ifndef ASC_TEST
  if(proced_data)
  {
    ctx->init_struct.rx_buff->tail = proced_data;
    ringslice_t tmp = ringslice_initializer(me->buf, me->buf_size, me->first, proced_data);
    ctx->init_struct.rx_buff->count -= ringslice_len(&tmp);
  }
  #endif
  if(item->answ.cb) 
  {
    ringslice_t cb_rs_data = rs_data_exist ? ringslice_initializer(me->buf, me->buf_size, rs_data->first, me->last) : (ringslice_t){0};
    item->answ.cb(cb_rs_data, res, entity->data);
  }
  return res; 
}

/*******************************************************************************
 ** @brief  Boolean operation for strings in ATL
 ** @param  rs_data  data ringslices
 ** @param  pattern  pattern to work
 ** @return true - success parce
 **         false/true
 ******************************************************************************/
static bool asc_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern)
{
  DBC_REQUIRE(150, rs_data); 
  DBC_REQUIRE(151, pattern);

  const char* or_sep = strchr(pattern, '|'); //Find type of boolean operation
  const char* and_sep = strchr(pattern, '&');
  
  if (or_sep && and_sep) return false; // Combination in not allowed
  
  const char sep = or_sep ? '|' : (and_sep ? '&' : '\0');
  const int require_all = (sep == '&'); // 1 for AND, 0 for OR

  for(const char* start = pattern; *start && *start != '\0';) 
  {
    const char* end = strchr(start, sep);
    size_t length = end ? (size_t)(end - start) : strlen(start);
    ringslice_t match = ringslice_strnstr(rs_data, start, length);
    if(require_all) 
    { 
      if (ringslice_is_empty(&match)) return false; //if only one desmatch ->exit
    } else 
    {
      if (!ringslice_is_empty(&match)) return true; //if only one match ->exit
    }
    start = end && *end != '\0' ? end + 1 : start + length;
  }
  return require_all ? true : false;
}

/** 
 * @brief SSCANF for ring buffer atl
 * @return true success/ false error
 */
static bool asc_cmd_sscanf(const ringslice_t* const rs_data, const asc_item_t* const item) 
{
  DBC_REQUIRE(155, rs_data); 
  DBC_REQUIRE(156, item);  
  const char *format = item->answ.format;
  void **output_ptrs = item->answ.ptrs;
  
  int param_count = 0;
  while(output_ptrs[param_count] != ASC_NO_ARG) param_count++;
  
  DBC_ASSERT(157, format);
  DBC_ASSERT(158, output_ptrs);
  switch (param_count)
  {
    case 1:  return ringslice_scanf(rs_data, format, output_ptrs[0]) == 1;
    case 2:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1]) == 2;
    case 3:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2]) == 3;
    case 4:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3]) == 4;
    case 5:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3], output_ptrs[4]) == 5;
    case 6:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3], output_ptrs[4], output_ptrs[5]) == 6;
    default: return false;
  }
}

/*******************************************************************************
 ** @brief  Find and proc standart URC 
 ** @param  ctx     core context
 ** @param  me  slice of origin buffer
 ** @return None
 ******************************************************************************/
static void asc_process_urcs(asc_context_t* const ctx, const ringslice_t* me)
{
  DBC_REQUIRE(165, ctx);
  DBC_REQUIRE(166, me); 
  
  if(ringslice_is_empty(me)) return; // no data

  for(uint8_t i = 0; i < ASC_URC_QUEUE_SIZE; ++i) 
  {
    if(ctx->urc_queue[i].prefix)
    {
      ringslice_t rs_urc = ringslice_strstr(me, ctx->urc_queue[i].prefix);
      if(!ringslice_is_empty(&rs_urc) && ctx->urc_queue[i].cb) 
      {
        #ifndef ASC_TEST
        ctx->init_struct.rx_buff->tail = rs_urc.last;
        ringslice_t tmp = ringslice_initializer(me->buf, me->buf_size, me->first, rs_urc.last);
        ctx->init_struct.rx_buff->count -= ringslice_len(&tmp);
        #endif
        rs_urc = ringslice_initializer(rs_urc.buf, rs_urc.buf_size, rs_urc.first, me->last);
        ASC_DEBUG(ctx, "[ASC][INFO] Found URC: %s", ctx->urc_queue[i].prefix);
        ctx->urc_queue[i].cb(rs_urc);
      }
    }
  }
}

/*******************************************************************************
 ** @brief  Init atl lib  
 ** @param  ctx        core context
 ** @param  asc_printf pointer to user func of printf
 ** @param  asc_write  pointer to user func of write to uart
 ** @param  rx_buff    struct to ring buffer
 ** @return none
 ******************************************************************************/
void asc_init(asc_context_t* const ctx, const asc_printf_t asc_printf, const asc_write_t asc_write, asc_ring_buffer_t* rx_buff)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(200, ctx);
  if(ctx->init_struct.init) { ASC_CRITICAL_EXIT return; }
  DBC_REQUIRE(202, asc_printf);
  DBC_REQUIRE(203, asc_write);
  DBC_REQUIRE(204, rx_buff);
  ctx->init_struct.heap = o1heapInit(ctx->mem_pool, ASC_MEMORY_POOL_SIZE); //memory allocator init
  DBC_ASSERT(208, ctx->init_struct.heap);
  ctx->init_struct.asc_write = asc_write;
  ctx->init_struct.asc_printf = asc_printf;
  ctx->init_struct.rx_buff = rx_buff;
  ctx->init_struct.init = true;
  ASC_DEBUG(ctx, "[ASC][INFO] ATL library initialized successfully", NULL);
  ASC_DEBUG(ctx, "[ASC][INFO] Memory pool size: %d bytes", ASC_MEMORY_POOL_SIZE);
  ASC_DEBUG(ctx, "[ASC][INFO] Memory overhead: %d/%d", o1heapGetDiagnostics(ctx->init_struct.heap).allocated, o1heapGetDiagnostics(ctx->init_struct.heap).capacity);
  ASC_DEBUG(ctx, "[ASC][INFO] Entity queue size: %d", ASC_ENTITY_QUEUE_SIZE);
  DBC_ENSURE(209, ctx->init_struct.init);
  ASC_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  DeInit atl lib  
 ** @param  ctx core context
 ** @return none
 ******************************************************************************/
void asc_deinit(asc_context_t* const ctx)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(300, ctx);
  DBC_REQUIRE(301, ctx->init_struct.init);
  ASC_DEBUG(ctx, "[ASC][INFO] Deinitializing ATL library", NULL);
  ctx->init_struct.init = false;
  memset(&ctx->entity_queue, 0, sizeof(asc_entity_queue_t));
  memset(ctx->urc_queue, 0, sizeof(asc_urc_queue_t));
  ASC_DEBUG(ctx, "[ASC][INFO] ATL library deinitialized", NULL);
  DBC_ENSURE(302, !ctx->init_struct.init);
  ASC_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  Function to append main queue with new group of at cmds
 ** @param  ctx          core context
 ** @param  item         ptr to your group of at cmds.
 ** @param  item_amount  amount  of your at cms in group 
 ** @param  cb           ur callback function for the whole group.
 ** @param  data_size    If you`r expecting some usefull data while execution pass here size of them.
 **                      And pass the ptr of this data to VA ARGS of each item with proper format to
 **                      get this data. If no need pass the 0.
 ** @param  meta         Ptr to some meta data of execution. Will be called in CB. Can be NULL.
 ** @return true: ok false: error while trying to append
 ******************************************************************************/
bool asc_entity_enqueue(asc_context_t* const ctx, const asc_item_t* const item, const uint8_t item_amount, const asc_entity_cb_t cb, uint16_t data_size, void* const meta)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(400, ctx);
  DBC_REQUIRE(401, ctx->init_struct.init);
  DBC_REQUIRE(402, item != NULL);
  DBC_REQUIRE(403, item_amount > 0);
  DBC_REQUIRE(404, item_amount <= ASC_MAX_ITEMS_PER_ENTITY);
  ASC_DEBUG(ctx, "[ASC][INFO] Enqueueing entity with %d items", item_amount);
  asc_entity_t* cur_entity = &ctx->entity_queue.entity[ctx->entity_queue.entity_head];
  if(ctx->entity_queue.entity_cnt >= ASC_ENTITY_QUEUE_SIZE) goto error_exit;
  cur_entity->data = asc_malloc(ctx, data_size);
  if(!cur_entity->data && data_size) goto error_exit;
  cur_entity->data_size = data_size;
  memset(cur_entity->data, 0, data_size);
  cur_entity->item = asc_malloc(ctx, item_amount * sizeof(asc_item_t));
  if(!cur_entity->item) goto error_exit;
  for(int i = 0; i < item_amount; i++)
  {
    if(!item[i].req && !item[i].answ.prefix) goto error_exit; //unhandle comb
    memcpy(&cur_entity->item[i], &item[i], sizeof(asc_item_t));
    if(item[i].answ.ptrs && item[i].answ.ptrs[0] != ASC_NO_ARG && item[i].answ.format && data_size)
    {
      int ptr_count = 0;
      while(item[i].answ.ptrs[ptr_count] != ASC_NO_ARG) ptr_count++;
      if(ptr_count > 6) goto error_exit; //no more then 6 ARGS
      cur_entity->item[i].answ.ptrs = asc_malloc(ctx, (ptr_count + 1) * sizeof(void*));
      if(!cur_entity->item[i].answ.ptrs) goto error_exit;
      for(int j = 0; j < ptr_count; j++)
      {
        size_t offset = (size_t)item[i].answ.ptrs[j];
        cur_entity->item[i].answ.ptrs[j] = (char*)cur_entity->data + offset;
        ASC_DEBUG(ctx, "[ASC][INFO] User data for item[%d] VAR_ARGS[%d]: offset=%zu, new=%d", i, j, offset, cur_entity->item[i].answ.ptrs[j]);
      }
      cur_entity->item[i].answ.ptrs[ptr_count] = ASC_NO_ARG;
    }
    else //delete stack address if no ptrs
    {
      cur_entity->item[i].answ.ptrs = NULL;
    }
    if(item[i].req && strncmp(item[i].req, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE)) == 0)
    {
      char* tmp_req = item[i].req;
      uint8_t* new_mem = asc_malloc(ctx, strlen(tmp_req) + 1);
      if(!new_mem) goto error_exit;
      strcpy((char*)new_mem, tmp_req);
      cur_entity->item[i].req = (char*)new_mem;
    }
    if(item[i].answ.prefix && strncmp(item[i].answ.prefix, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE)) == 0)
    {
      char* tmp_prefix = item[i].answ.prefix;
      uint8_t* new_mem = asc_malloc(ctx, strlen(tmp_prefix) + 1);
      if(!new_mem) goto error_exit;
      strcpy((char*)new_mem, tmp_prefix);
      cur_entity->item[i].answ.prefix = (char*)new_mem;
    }
  }
  cur_entity->item_cnt = item_amount;
  cur_entity->cb = cb;
  cur_entity->meta = meta;
  cur_entity->state = ASC_STATE_WRITE;
  ctx->entity_queue.entity_head = (ctx->entity_queue.entity_head + 1) % ASC_ENTITY_QUEUE_SIZE;
  ++ctx->entity_queue.entity_cnt;
  ASC_DEBUG(ctx, "[ASC][INFO] Entity enqueued. Queue count: %d. Memory used: %d/%d. User data at %d. ", 
             ctx->entity_queue.entity_cnt, o1heapGetDiagnostics(ctx->init_struct.heap).allocated, o1heapGetDiagnostics(ctx->init_struct.heap).capacity, cur_entity->data);
  ASC_CRITICAL_EXIT
  return true;

  error_exit:
    ASC_DEBUG(ctx, "[ASC][ERROR] Queue failed", NULL); 
    asc_deinit(ctx);        
    ASC_CRITICAL_EXIT
    return false; 
}

/*******************************************************************************
 ** @brief  Clear first entity from the queue 
 ** @param  ctx core context
 ** @return false: some errors; true: ok
 ******************************************************************************/
bool asc_entity_dequeue(asc_context_t* const ctx)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(500, ctx);
  DBC_REQUIRE(501, ctx->init_struct.init);
  asc_entity_t* cur_entity = &ctx->entity_queue.entity[ctx->entity_queue.entity_tail];
  uint8_t item_amount = cur_entity->item_cnt;
  if(ctx->entity_queue.entity_cnt == 0)
  {
    ASC_DEBUG(ctx, "[ASC][ERROR] Entity queue is already empty", NULL);
    ASC_CRITICAL_EXIT
    return false;
  }
  ASC_DEBUG(ctx, "[ASC][INFO] Dequeueing entity with %d items", cur_entity->item_cnt);
  while(item_amount)
  {
    asc_item_t* item = &cur_entity->item[cur_entity->item_cnt -item_amount];
    if(item->req && strncmp(item->req, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE)) == 0) asc_free(ctx, item->req);
    if(item->answ.prefix && strncmp(item->answ.prefix, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE)) == 0) asc_free(ctx, item->answ.prefix);
    if(item->answ.ptrs && cur_entity->data)
    {
      uint8_t ptr_count = 0;
      while(item->answ.ptrs[ptr_count] != ASC_NO_ARG) ptr_count++;
      if(ptr_count) asc_free(ctx, item->answ.ptrs);
    }
    cur_entity->item[cur_entity->item_cnt -item_amount].req = NULL;
    --item_amount;
  }
  if(cur_entity->data && cur_entity->data_size) asc_free(ctx, cur_entity->data);
  if(cur_entity->item) asc_free(ctx, cur_entity->item);
  memset(cur_entity, 0, sizeof(asc_entity_t));  
  ctx->entity_queue.entity_tail = (ctx->entity_queue.entity_tail +1) % ASC_ENTITY_QUEUE_SIZE;
  --ctx->entity_queue.entity_cnt;
  ASC_DEBUG(ctx, "[ASC][INFO] Entity dequeued. Queue count: %d. Memory used: %d/%d", 
             ctx->entity_queue.entity_cnt, o1heapGetDiagnostics(ctx->init_struct.heap).allocated, o1heapGetDiagnostics(ctx->init_struct.heap).capacity);
  ASC_CRITICAL_EXIT
  return true;
}

/*******************************************************************************
 ** @brief  Function to append URC queue
 ** @param  ctx  core context
 ** @param  urc  ptr to your URC.
 ** @return true/false
 ******************************************************************************/
bool asc_urc_enqueue(asc_context_t* const ctx, const asc_urc_queue_t* const urc)  
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(600, ctx);
  DBC_REQUIRE(601, urc);
  asc_urc_queue_t* tmp = NULL;
  for(uint8_t i = 0; i <= ASC_URC_QUEUE_SIZE; ++i)
  {
    if(!ctx->urc_queue[i].prefix) 
    {
      tmp = &(ctx->urc_queue[i]);
      break;
    }
  }
  if(!tmp) 
  {
    ASC_DEBUG(ctx, "[ASC][ERROR] URC queue is full", NULL);
    ASC_CRITICAL_EXIT
    return false;
  }
  memcpy(tmp, urc, ASC_URC_SIZE);
  ASC_DEBUG(ctx, "[ASC][INFO] URC enqueued successfully", NULL);
  ASC_CRITICAL_EXIT
  return true;
}

/*******************************************************************************
 ** @brief  Function to delete URC from queue
 ** @param  ctx  core context
 ** @param  prefix  prefix of your URC.
 ** @return true/false
 ******************************************************************************/
bool asc_urc_dequeue(asc_context_t* const ctx, char* prefix)  
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(700, ctx);
  DBC_REQUIRE(701, prefix);
  for(uint8_t i = 0; i <= ASC_URC_QUEUE_SIZE; ++i)
  {
    if(!ctx->urc_queue[i].prefix) continue;
    if(strcmp(ctx->urc_queue[i].prefix, prefix) == 0)
    {
      memset(&ctx->urc_queue[i], 0, ASC_URC_SIZE);
      ASC_DEBUG(ctx,"[ASC][INFO] URC dequeued successfully", NULL);
      ASC_CRITICAL_EXIT
      return true;
    }
  }
  ASC_DEBUG(ctx, "[ASC][INFO] URC dequeued fail", NULL);
  ASC_CRITICAL_EXIT
  return false;
}

/*******************************************************************************
 ** @brief  Function get init. 
 ** @param  ctx  core context
 ** @return @asc_init_t
 ******************************************************************************/
asc_init_t asc_get_init(asc_context_t* const ctx)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(750, ctx);
  asc_init_t res = ctx->init_struct;
  ASC_CRITICAL_EXIT
  return res;
}

/*******************************************************************************
 ** @brief  Function get time in 10ms. 
 ** @param  ctx  core context
 ** @return time
 ******************************************************************************/
uint32_t asc_get_cur_time(asc_context_t* const ctx)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(760, ctx);
  uint32_t res = ctx->time;
  ASC_CRITICAL_EXIT
  return res;
}

/*******************************************************************************
 ** @brief  Function to custom malloc. Handle the concurrent state
 ** @param  ctx  core context
 ** @param  size amount to alloc
 ** @return ptr to new memory
 ******************************************************************************/
void* asc_malloc(asc_context_t* const ctx, size_t size)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(770, ctx);
  if(!ctx->init_struct.init) { ASC_CRITICAL_EXIT return NULL; }
  void* res = o1heapAllocate(ctx->init_struct.heap, size);
  ASC_CRITICAL_EXIT
  return res;
}

/*******************************************************************************
 ** @brief  Function to custom free. Handle the concurrent state
 ** @param  ctx  core context
 ** @param  ptr  ptr to delete
 ** @return none
 ******************************************************************************/
void asc_free(asc_context_t* ctx, void* ptr)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(780, ctx);
  if(!ctx->init_struct.init) { ASC_CRITICAL_EXIT return; }
  o1heapFree(ctx->init_struct.heap, ptr);
  ASC_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  Helper for main proc function
 ** @param  none
 ** @return none
 ******************************************************************************/
/**  @brief Helper */
static void asc_proc_handle_cmd_result(asc_context_t* const ctx, asc_entity_t* const entity, asc_item_t* const item, const bool success) 
{
  DBC_REQUIRE(801, ctx);
  DBC_REQUIRE(802, entity);
  DBC_REQUIRE(803, item);
  int step = success ? item->meta.ok_step : item->meta.err_step;
  if(entity->item_id < entity->item_cnt - 1) // not last
  {
    if (step != 0 || (step > 0 && entity->item_id + step < entity->item_cnt) || (step < 0 && entity->item_id + step >= 0)) 
    {
      ASC_DEBUG(ctx, "[ASC][INFO] Next cmd of entity", NULL);
      entity->item_id += (step == 0) ? 1 : step;
      return;
    }
  }  
  //Step outside of cmd range or last cmd or step == 0
  ASC_DEBUG(ctx, "[ASC][INFO] End of entity", NULL);
  #ifndef ASC_TEST
  if(!success) //debug
  {
    uint16_t data_start = ctx->init_struct.rx_buff->head < 250
                          ? ctx->init_struct.rx_buff->size - (250 - ctx->init_struct.rx_buff->head)
                          : ctx->init_struct.rx_buff->head - 250;
    ringslice_t rs_me = ringslice_initializer(ctx->init_struct.rx_buff->buffer, ctx->init_struct.rx_buff->size, data_start, ctx->init_struct.rx_buff->head);
    asc_printf_from_ring(ctx, rs_me, "Failed last 250 bytes of data: ");
  }
  #endif
  if(entity->cb) entity->cb(success, entity->meta, entity->data);
  asc_entity_dequeue(ctx);
}

/** * @brief Function to proc ATL core proccesses. Call it each 10ms */
void asc_core_proc(asc_context_t* const ctx)
{
  ASC_CRITICAL_ENTER
  DBC_REQUIRE(901, ctx);
  if(ctx->time >= UINT32_MAX) ctx->time = 0;
  else ctx->time += 1;
  ringslice_t rs_me = ringslice_initializer(ctx->init_struct.rx_buff->buffer, ctx->init_struct.rx_buff->size, ctx->init_struct.rx_buff->tail, ctx->init_struct.rx_buff->head);
  if(ctx->time % ASC_URC_FREQ_CHECK == 0) asc_process_urcs(ctx, &rs_me); //check URC each 100ms
  if(!ctx->entity_queue.entity_cnt) { ASC_CRITICAL_EXIT return; }
  asc_entity_t* entity = &ctx->entity_queue.entity[ctx->entity_queue.entity_tail];
  ASC_CRITICAL_EXIT //we work with exclusive memory field for this entity bcs of ring buffer
  asc_item_t* item = &entity->item[entity->item_id];
  if(entity->timer) --entity->timer;
  switch(entity->state)
  {
    case ASC_STATE_WRITE:
         if(item->req)
         {
           ASC_CRITICAL_ENTER
           uint16_t offset = strncmp(item->req, ASC_CMD_SAVE, strlen(ASC_CMD_SAVE)) ? 0 : strlen(ASC_CMD_SAVE);
           ctx->init_struct.asc_write((uint8_t*)item->req +offset, strlen(item->req+offset));
           ASC_DEBUG(ctx, "[ASC][INFO] [TX] %s", item->req +offset);    
           ASC_CRITICAL_EXIT
         }         
         entity->timer = item->meta.wait;
         entity->state = ASC_STATE_READ;
         break;
    case ASC_STATE_READ:
         if(asc_cmd_ring_parcer(ctx, entity, item, rs_me) == 1) 
         {
           entity->state = ASC_STATE_WRITE;
           ASC_DEBUG(ctx, "[ASC][INFO] Successful entity cmd %d/%d", entity->item_id+1, entity->item_cnt);
           asc_proc_handle_cmd_result(ctx, entity, item, true);  
         } 
         else if(!entity->timer && item->meta.rpt_cnt) 
         {
           entity->state = ASC_STATE_WRITE;
           ASC_DEBUG(ctx, "[ASC][INFO] Timeout, retries left: %d", item->meta.rpt_cnt - 1);
           if(--item->meta.rpt_cnt == 0) 
           {
             ASC_CRITICAL_ENTER
             #ifndef ASC_TEST
             ctx->init_struct.rx_buff->tail = ctx->init_struct.rx_buff->head;
             ctx->init_struct.rx_buff->count = 0;
             #endif
             ASC_CRITICAL_EXIT
             ASC_DEBUG(ctx, "[ASC][INFO] Failure entity cmd %d/%d", entity->item_id+1, entity->item_cnt);
             asc_proc_handle_cmd_result(ctx, entity, item, false);  
           }
         }
         else
         {
           if(entity->timer == item->meta.wait/2) ASC_DEBUG(ctx, "[ASC][INFO] Waiting......", NULL);
         }
         break;
    default: 
         ASC_DEBUG(ctx, "[ASC][INFO] Unknown state: %d", entity->state);
         return;
  }
}

#ifdef ASC_TEST
/*******************************************************************************
 ** @brief  TEST implementations
 ** @param  none
 ** @return none
 ******************************************************************************/
void _asc_core_proc(asc_context_t* const ctx) { 
  asc_core_proc(ctx);
}

int _asc_cmd_ring_parcer(asc_context_t* const ctx, const asc_entity_t* const entity, const asc_item_t* const item, const ringslice_t rs_me) { 
  return asc_cmd_ring_parcer(ctx, entity,item, rs_me);
}

void _asc_simcom_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req) { 
  asc_simcom_parcer_find_rs_req(me, rs_req, req); 
}

void _asc_simcom_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res) { 
  asc_simcom_parcer_find_rs_res(me, rs_req, rs_res); 
}

void _asc_simcom_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data) { 
  asc_simcom_parcer_find_rs_data(me, rs_req, rs_res, rs_data); 
}

int _asc_simcom_parcer_post_proc(asc_context_t* const ctx, const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, 
                                 const ringslice_t* const rs_data, const asc_item_t* const item, const asc_entity_t* const entity) { 
  return asc_simcom_parcer_post_proc(ctx, me, rs_req, rs_res, rs_data, item, entity); 
}

int _asc_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern) { 
  return asc_string_boolean_ops(rs_data, pattern); 
}

int _asc_cmd_sscanf(const ringslice_t* const rs_data, const asc_item_t* const item) {
  return asc_cmd_sscanf(rs_data, item);
}

void _asc_process_urcs(asc_context_t* const ctx, const ringslice_t* me)
{
  asc_process_urcs(ctx, me);
}


asc_entity_queue_t* _asc_get_entity_queue(asc_context_t* const ctx) {
  return &ctx->entity_queue;
}

asc_urc_queue_t* _asc_get_urc_queue(asc_context_t* const ctx) {
  return ctx->urc_queue;
}

asc_init_t _asc_get_init(asc_context_t* const ctx) {
  return asc_get_init(ctx);
}

#endif
