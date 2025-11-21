/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "atl_core.h"
#include "dbc_assert.h"
#include "atl_mdl_general.h"
#include "stdlib.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_CORE")

#warning "TODO: EXEC; CHAIN выход из циклов в ошибки; одинаковые имена проверка; итерация цикла происходит только в end"

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void atl_parcer_process_urcs(const ringslice_t* me);
static void atl_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req);
static void atl_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res);
static void atl_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data);
static int atl_parcer_post_proc(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, 
                                const ringslice_t* const rs_data, const atl_item_t* const item, const atl_entity_t* const entity);
static int atl_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern);
static int atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t* const item);

/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static atl_entity_queue_t atl_entity_queue = {0}; //entity queue
static atl_urc_queue_t atl_urc_queue[ATL_URC_QUEUE_SIZE] = {0}; //urc queue
static atl_init_t atl_init_struct = {0}; //init struct
static uint8_t atl_mem_pool[ATL_MEMORY_POOL_SIZE] __attribute__((aligned(O1HEAP_ALIGNMENT)));
static uint32_t atl_time = 0;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Function to parce the RX ring buffer for proc with existing entities in queue.
 **         Should be called each time when the new data comes to RX ring buffer.
 ** @param  entity  current proc entity
 ** @param  item    current proc item
 ** @return 1 - success parce
 **         0 - failure parce and handle / or no data in buff
 **        -1 - ERROR result code in answer for this cmd
 ******************************************************************************/
static int atl_cmd_ring_parcer(const atl_entity_t* const entity, const atl_item_t* const item)
{
  DBC_REQUIRE(101, atl_init_struct.init);
  DBC_REQUIRE(102, item);
  DBC_REQUIRE(103, entity);

  int res = 0;
  ringslice_t rs_me = {0};
  ringslice_t rs_req = {0};
  ringslice_t rs_res = {0}; 
  ringslice_t rs_data = {0};
  
  if(item->answ.prefix && strncmp(item->answ.prefix, ATL_CMD_FORCE, strlen(ATL_CMD_FORCE)) == 0) 
  {
    ATL_CRITICAL_ENTER
    atl_init_struct.rx_buff->tail = atl_init_struct.rx_buff->head;
    atl_init_struct.rx_buff->count = 0;
    ATL_CRITICAL_EXIT
    return 1;
  }

  ATL_CRITICAL_ENTER
  rs_me = ringslice_initializer(atl_init_struct.rx_buff->buffer, atl_init_struct.rx_buff->size, atl_init_struct.rx_buff->tail, atl_init_struct.rx_buff->head);
  ATL_CRITICAL_EXIT

  if(ringslice_is_empty(&rs_me)) return 0;
   
  atl_parcer_process_urcs(&rs_me); // Process URCs first

  atl_parcer_find_rs_req(&rs_me, &rs_req, item->req); // Find request and response in buffer
  atl_parcer_find_rs_res(&rs_me, &rs_req, &rs_res);
  atl_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data); // Extract data section

  res = atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, item, entity); // Proc data

  #ifndef ATL_TEST
  if(res > 0) atl_printf_from_ring(rs_me, "[RX]");
  #endif

  return res;
}

/** 
 * @brief Find and proc URC 
 */
static void atl_parcer_process_urcs(const ringslice_t* me)
{
  DBC_REQUIRE(105, me); 

  for(uint8_t i = 0; i < ATL_URC_QUEUE_SIZE; ++i) 
  {
    if(atl_urc_queue[i].prefix)
    {
      ringslice_t rs_urc = ringslice_strstr(me, atl_urc_queue[i].prefix);
      if(!ringslice_is_empty(&rs_urc) && atl_urc_queue[i].cb) 
      {
        ATL_DEBUG("[ATL][INFO] Found URC: %s", atl_urc_queue[i].prefix);
        atl_urc_queue[i].cb(rs_urc);
      }
    }
  }
}

/** 
 * @brief Find request ring slice 
 */
static void atl_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* req)
{
  DBC_REQUIRE(106, me); 
  DBC_REQUIRE(107, rs_req); 

  if(!req) return;

  if(strncmp(req, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE)) == 0) req += strlen(ATL_CMD_SAVE);

  *rs_req = ringslice_strnstr(me, req, strlen(req) - 1); //Request echo returns only CR, so ignore LF
}

/** 
 * @brief Find result ring slice 
 */
static void atl_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res)
{
  DBC_REQUIRE(108, rs_req); 
  DBC_REQUIRE(109, rs_res); 

  if(ringslice_is_empty(rs_req)) return;

  ringslice_t tmp = ringslice_subslice_after(me, rs_req, 0);

  if(ringslice_is_empty(&tmp)) return;

  *rs_res = ringslice_strstr(&tmp, ATL_CMD_ERROR);
  if(ringslice_is_empty(rs_res)) *rs_res = ringslice_strstr(&tmp, ATL_CMD_OK);
}

/** 
 * @brief Find and data ring slice 
 */
static void atl_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data)
{
  DBC_REQUIRE(110, me); 
  DBC_REQUIRE(111, rs_req);  
  DBC_REQUIRE(112, rs_res); 
  DBC_REQUIRE(113, rs_data);

  const uint8_t crlf_len = strlen(ATL_CMD_CRLF);

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
    ringslice_t after_req = ringslice_subslice_after(me, rs_req, strlen(ATL_CMD_CRLF"OK"ATL_CMD_CRLF));
    *rs_data = ringslice_subslice_equals(&after_req, rs_res) 
               ? ringslice_subslice_after(me, rs_res, 0) // REQ\r\r\nOK\r\n\r\nDATA\r\n
               : ringslice_subslice_gap(rs_req, rs_res); // REQ\r\r\nDATA\r\n\r\nOK\r\n
  }

  if((ringslice_len(rs_data) <= 2 *crlf_len) || (ringslice_strncmp(rs_data, ATL_CMD_CRLF, 2))) // If data more than 2bytes and first 2 bytes its CRLF
  {
    *rs_data = (ringslice_t){0};
    return;
  }
  *rs_data = ringslice_subslice_with_suffix(rs_data, crlf_len, ATL_CMD_CRLF); 
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
 * @return -1 answer with ERROR result / 0 - failure / 1 - success
 */
static int atl_parcer_post_proc(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, 
                                const ringslice_t* const rs_data, const atl_item_t* const item, const atl_entity_t* const entity)
{
  DBC_REQUIRE(114, me); 
  DBC_REQUIRE(115, rs_req);  
  DBC_REQUIRE(116, rs_res); 
  DBC_REQUIRE(117, rs_data);
  DBC_REQUIRE(118, item);
  DBC_REQUIRE(119, entity);
  
  int res = 0;  

  bool rs_req_exist  = !ringslice_is_empty(rs_req);
  bool rs_res_exist  = !ringslice_is_empty(rs_res);
  bool rs_data_exist = !ringslice_is_empty(rs_data);

  ringslice_cnt_t proced_data = 0;
  
  char* prefix = item->answ.prefix;
  if(prefix && !strncmp(prefix, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE))) prefix += strlen(ATL_CMD_SAVE);

  switch((rs_req_exist << 2) | (rs_res_exist << 1) | rs_data_exist) // Bitmask: REQ[bit2] RES[bit1] DATA[bit0]
  {
      case 0x06: //0b110 - REQ RES NULL (REQ, NO PREFIX, NO FORMAT)
           if(!item->req || ringslice_strcmp(rs_res, ATL_CMD_ERROR) == 0) res = -1;
           else if(prefix || item->answ.format) res = 0;
           else res = 1;
           if(res != 0) proced_data = (uint16_t)rs_res->last;
           break;
      case 0x07: //0b111 - REQ RES DATA (REQ)
           if(!item->req) break;
           else if(ringslice_strcmp(rs_res, ATL_CMD_ERROR) == 0) res = -1;
           else res = 1;
           if(res > 0){
             if(prefix) res = atl_string_boolean_ops(rs_data, prefix);
             if(item->answ.format) res = atl_cmd_sscanf(rs_data, item);
           }
           #ifndef ATL_TEST
           if(ringslice_is_later_than(rs_res, rs_data)) proced_data = (uint16_t)rs_res->last;
           else                                         proced_data = (uint16_t)rs_data->last;
           #endif
           break;
      case 0x01: //0b001 - NULL NULL DATA (PREFIX)
           if(!prefix) break;
           res = atl_string_boolean_ops(rs_data, prefix);
           if(item->answ.format) res = atl_cmd_sscanf(rs_data, item);
           if(res > 0) proced_data = (uint16_t)rs_data->last;
           break;
      case 0x05: //0b101 - REQ NULL DATA (REQ +PREFIX)
           if(!item->req || !prefix) break;
           res = atl_string_boolean_ops(rs_data, prefix);
           if(item->answ.format) res = atl_cmd_sscanf(rs_data, item);
           if(res > 0) proced_data = (uint16_t)rs_data->last;
           break;
      default: 
           break;
  }
  #ifndef ATL_TEST
  if(proced_data)
  {
    atl_init_struct.rx_buff->tail = proced_data;
    ringslice_t tmp = ringslice_initializer(me->buf, me->buf_size, me->first, proced_data);
    atl_init_struct.rx_buff->count -= ringslice_len(&tmp);
  }
  #endif
  if(item->answ.cb) 
  {
    ringslice_t cb_rs_data = rs_data_exist 
        ? ringslice_initializer(me->buf, me->buf_size, rs_data->first, me->last) 
        : (ringslice_t){0};
    item->answ.cb(ringslice_initializer(me->buf, me->buf_size, rs_data->first, me->last), res, entity->data);
  }
  return res; 
}

/** 
 * @brief Boolean operation for strings in ATL
 * @return 1 success/0 error
 */
static int atl_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern)
{
  DBC_REQUIRE(120, rs_data); 
  DBC_REQUIRE(121, pattern);

  const char* or_sep = strchr(pattern, '|'); //Find type of boolean operation
  const char* and_sep = strchr(pattern, '&');
  
  if (or_sep && and_sep) return 0; // Combination in not allowed
  
  const char sep = or_sep ? '|' : (and_sep ? '&' : '\0');
  const int require_all = (sep == '&'); // 1 for AND, 0 for OR

  for(const char* start = pattern; *start && *start != '\0';) 
  {
    const char* end = strchr(start, sep);
    size_t length = end ? (size_t)(end - start) : strlen(start);
    ringslice_t match = ringslice_strnstr(rs_data, start, length);
    if(require_all) 
    { 
      if (ringslice_is_empty(&match)) return 0; //if only one desmatch ->exit
    } else 
    {
      if (!ringslice_is_empty(&match)) return 1; //if only one match ->exit
    }
    start = end && *end != '\0' ? end + 1 : start + length;
  }
  return require_all ? 1 : 0;
}

/** 
 * @brief SSCANF for ring buffer atl
 * @return num of successfully sscanfed args
 */
static int atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t* const item) 
{
  DBC_REQUIRE(122, rs_data); 
  DBC_REQUIRE(123, item);  
  const char *format = item->answ.format;
  void **output_ptrs = item->answ.ptrs;
  
  int param_count = 0;
  while(output_ptrs[param_count] != ATL_NO_ARG) param_count++;
  
  DBC_ASSERT(124, format);
  DBC_ASSERT(125, output_ptrs);
  switch (param_count)
  {
    case 1:  return ringslice_scanf(rs_data, format, output_ptrs[0]) == 1;
    case 2:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1]) == 2;
    case 3:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2]) == 3;
    case 4:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3]) == 4;
    case 5:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3], output_ptrs[4]) == 5;
    case 6:  return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3], output_ptrs[4], output_ptrs[5]) == 6;
    default: return 0;
  }
}

/*******************************************************************************
 ** @brief  Init atl lib  
 ** @param  atl_printf pointer to user func of printf
 ** @param  atl_write  pointer to user func of write to uart
 ** @param  ring       ptr to RX ring buffer
 ** @param  ring_len   lenght of RX ring buffer. 
 ** @param  ring_tail  tail of RX ring buffer. 
 ** @param  ring_head  head of RX ring buffer. 
 ** @return none
 ******************************************************************************/
void atl_init(const atl_printf_t atl_printf, const atl_write_t atl_write, atl_ring_buffer_t* rx_buff)
{
  ATL_CRITICAL_ENTER
  if(atl_init_struct.init) 
  {
    ATL_CRITICAL_EXIT
    return;
  }
  DBC_REQUIRE(202, atl_printf);
  DBC_REQUIRE(203, atl_write);
  DBC_REQUIRE(204, rx_buff);
  atl_init_struct.heap = o1heapInit(atl_mem_pool, ATL_MEMORY_POOL_SIZE); //memory allocator init
  DBC_ASSERT(208, atl_init_struct.heap);
  atl_init_struct.atl_write = atl_write;
  atl_init_struct.atl_printf = atl_printf;
  atl_init_struct.rx_buff = rx_buff;
  atl_init_struct.init = true;
  ATL_DEBUG("[ATL][INFO] ATL library initialized successfully", NULL);
  ATL_DEBUG("[ATL][INFO] Memory pool size: %d bytes", ATL_MEMORY_POOL_SIZE);
  ATL_DEBUG("[ATL][INFO] Memory overhead: %d/%d", o1heapGetDiagnostics(atl_init_struct.heap).allocated, o1heapGetDiagnostics(atl_init_struct.heap).capacity);
  ATL_DEBUG("[ATL][INFO] Entity queue size: %d", ATL_ENTITY_QUEUE_SIZE);
  DBC_ENSURE(209, atl_init_struct.init);
  #ifndef ATL_TEST
    atl_mdl_modem_init(NULL, NULL, NULL);
  #endif
  ATL_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  DeInit atl lib  
 ** @param  atl_printf pointer to user func of printf
 ** @param  atl_write  pointer to user func of write to uart
 ** @return false: some error is ocur true: ok
 ******************************************************************************/
void atl_deinit(void)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(301, atl_init_struct.init);
  ATL_DEBUG("[ATL][INFO] Deinitializing ATL library", NULL);
  atl_init_struct.init = false;
  memset(&atl_entity_queue, 0, sizeof(atl_entity_queue_t));
  memset(atl_urc_queue, 0, sizeof(atl_urc_queue_t));
  ATL_DEBUG("[ATL][INFO] ATL library deinitialized", NULL);
  DBC_ENSURE(302, !atl_init_struct.init);
  ATL_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  Function to append main queue with new group of at cmds
 ** @param  item         ptr to your group of at cmds.
 ** @param  item_amount  amount  of your at cms in group 
 ** @param  cb           ur callback function for the whole group.
 ** @param  data_size    If you`r expecting some usefull data while execution pass here size of them.
 **                      And pass the ptr of this data to VA ARGS of each item with proper format to
 **                      get this data. If no need pass the 0.
 ** @param  ctx          Ptr to some context of execution. Will be called in CB. Can be NULL.
 ** @return true: ok false: error while trying to append
 ******************************************************************************/
bool atl_entity_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, void* const ctx)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(401, atl_init_struct.init);
  DBC_REQUIRE(402, item != NULL);
  DBC_REQUIRE(403, item_amount > 0);
  DBC_REQUIRE(404, item_amount <= ATL_MAX_ITEMS_PER_ENTITY);
  ATL_DEBUG("[ATL][INFO] Enqueueing entity with %d items", item_amount);
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  if(atl_entity_queue.entity_cnt >= ATL_ENTITY_QUEUE_SIZE) goto error_exit;
  cur_entity->data = atl_malloc(data_size);
  if(!cur_entity->data && data_size) goto error_exit;
  cur_entity->data_size = data_size;
  memset(cur_entity->data, 0, data_size);
  cur_entity->item = atl_malloc(item_amount * sizeof(atl_item_t));
  if(!cur_entity->item) goto error_exit;
  for(int i = 0; i < item_amount; i++)
  {
    if(!item[i].req && !item[i].answ.prefix) goto error_exit; //unhandle comb
    memcpy(&cur_entity->item[i], &item[i], sizeof(atl_item_t));
    if(item[i].answ.ptrs && item[i].answ.ptrs[0] != ATL_NO_ARG && item[i].answ.format && data_size)
    {
      int ptr_count = 0;
      while(item[i].answ.ptrs[ptr_count] != ATL_NO_ARG) ptr_count++;
      if(ptr_count > 6) goto error_exit; //no more then 6 ARGS
      cur_entity->item[i].answ.ptrs = atl_malloc((ptr_count + 1) * sizeof(void*));
      if(!cur_entity->item[i].answ.ptrs) goto error_exit;
      for(int j = 0; j < ptr_count; j++)
      {
        size_t offset = (size_t)item[i].answ.ptrs[j];
        cur_entity->item[i].answ.ptrs[j] = (char*)cur_entity->data + offset;
        ATL_DEBUG("[ATL][INFO] User data for item[%d] VAR_ARGS[%d]: offset=%zu, new=%d", i, j, offset, cur_entity->item[i].answ.ptrs[j]);
      }
      cur_entity->item[i].answ.ptrs[ptr_count] = ATL_NO_ARG;
    }
    else //delete stack address if no ptrs
    {
      cur_entity->item[i].answ.ptrs = NULL;
    }
    if(item[i].req && strncmp(item[i].req, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE)) == 0)
    {
      char* tmp_req = item[i].req;
      uint8_t* new_mem = atl_malloc(strlen(tmp_req) + 1);
      if(!new_mem) goto error_exit;
      strcpy((char*)new_mem, tmp_req);
      cur_entity->item[i].req = (char*)new_mem;
    }
    if(item[i].answ.prefix && strncmp(item[i].answ.prefix, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE)) == 0)
    {
      char* tmp_prefix = item[i].answ.prefix;
      uint8_t* new_mem = atl_malloc(strlen(tmp_prefix) + 1);
      if(!new_mem) goto error_exit;
      strcpy((char*)new_mem, tmp_prefix);
      cur_entity->item[i].answ.prefix = (char*)new_mem;
    }
  }
  cur_entity->item_cnt = item_amount;
  cur_entity->cb = cb;
  cur_entity->ctx = ctx;
  cur_entity->state = ATL_STATE_WRITE;
  atl_entity_queue.entity_head = (atl_entity_queue.entity_head + 1) % ATL_ENTITY_QUEUE_SIZE;
  ++atl_entity_queue.entity_cnt;
  ATL_DEBUG("[ATL][INFO] Entity enqueued. Queue count: %d. Memory used: %d/%d. User data at %d. ", 
             atl_entity_queue.entity_cnt, o1heapGetDiagnostics(atl_init_struct.heap).allocated, o1heapGetDiagnostics(atl_init_struct.heap).capacity, cur_entity->data);
  ATL_CRITICAL_EXIT
  return true;

  error_exit:
    ATL_DEBUG("[ATL][ERROR] Queue failed", NULL); 
    atl_deinit();        
    ATL_CRITICAL_EXIT
    return false; 
}

/*******************************************************************************
 ** @brief  Clear first entity from the queue 
 ** @param  none
 ** @return false: some errors; true: ok
 ******************************************************************************/
bool atl_entity_dequeue(void)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(501, atl_init_struct.init);
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_tail];
  uint8_t item_amount = cur_entity->item_cnt;
  if(atl_entity_queue.entity_cnt == 0)
  {
    ATL_DEBUG("[ERROR] Entity queue is already empty", NULL);
    ATL_CRITICAL_EXIT
    return false;
  }
  ATL_DEBUG("[ATL][INFO] Dequeueing entity with %d items", cur_entity->item_cnt);
  while(item_amount)
  {
    atl_item_t* item = &cur_entity->item[cur_entity->item_cnt -item_amount];
    if(item->req && strncmp(item->req, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE)) == 0) atl_free(item->req);
    if(item->answ.prefix && strncmp(item->answ.prefix, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE)) == 0) atl_free(item->answ.prefix);
    if(item->answ.ptrs && cur_entity->data)
    {
      uint8_t ptr_count = 0;
      while(item->answ.ptrs[ptr_count] != ATL_NO_ARG) ptr_count++;
      if(ptr_count) atl_free(item->answ.ptrs);
    }
    cur_entity->item[cur_entity->item_cnt -item_amount].req = NULL;
    --item_amount;
  }
  if(cur_entity->data && cur_entity->data_size) atl_free(cur_entity->data);
  if(cur_entity->item) atl_free(cur_entity->item);
  memset(cur_entity, 0, sizeof(atl_entity_t));  
  atl_entity_queue.entity_tail = (atl_entity_queue.entity_tail +1) % ATL_ENTITY_QUEUE_SIZE;
  --atl_entity_queue.entity_cnt;
  ATL_DEBUG("[ATL][INFO] Entity dequeued. Queue count: %d. Memory used: %d/%d", 
             atl_entity_queue.entity_cnt, o1heapGetDiagnostics(atl_init_struct.heap).allocated, o1heapGetDiagnostics(atl_init_struct.heap).capacity);
  ATL_CRITICAL_EXIT
  return true;
}

/*******************************************************************************
 ** @brief  Function to append URC queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_enqueue(const atl_urc_queue_t* const urc)  
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(601, urc);
  atl_urc_queue_t* tmp = NULL;
  for(uint8_t i = 0; i <= ATL_URC_QUEUE_SIZE; ++i)
  {
    if(!atl_urc_queue[i].prefix) 
    {
      tmp = &(atl_urc_queue[i]);
      break;
    }
  }
  if(!tmp) 
  {
    ATL_DEBUG("[ATL][ERROR] URC queue is full", NULL);
    ATL_CRITICAL_EXIT
    return false;
  }
  memcpy(tmp, urc, ATL_URC_SIZE);
  ATL_DEBUG("[ATL][INFO] URC enqueued successfully", NULL);
  ATL_CRITICAL_EXIT
  return true;
}

/*******************************************************************************
 ** @brief  Function to delete URC from queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_dequeue(char* prefix)  
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(701, prefix);
  for(uint8_t i = 0; i <= ATL_URC_QUEUE_SIZE; ++i)
  {
    if(!atl_urc_queue[i].prefix) continue;
    if(strcmp(atl_urc_queue[i].prefix, prefix) == 0)
    {
      memset(&atl_urc_queue[i], 0, ATL_URC_SIZE);
      ATL_DEBUG("[ATL][INFO] URC dequeued successfully", NULL);
      ATL_CRITICAL_EXIT
      return true;
    }
  }
  ATL_DEBUG("[ATL][INFO] URC dequeued fail", NULL);
  ATL_CRITICAL_EXIT
  return false;
}

/*******************************************************************************
 ** @brief  Function get init. 
 ** @param  none
 ** @return none
 ******************************************************************************/
atl_init_t atl_get_init(void)
{
  ATL_CRITICAL_ENTER
  atl_init_t res = atl_init_struct;
  ATL_CRITICAL_EXIT
  return res;
}

/*******************************************************************************
 ** @brief  Function get time in 10ms. 
 ** @param  none
 ** @return none
 ******************************************************************************/
uint32_t atl_get_cur_time(void)
{
  ATL_CRITICAL_ENTER
  uint32_t res = atl_time;
  ATL_CRITICAL_EXIT
  return res;
}

/*******************************************************************************
 ** @brief  Function to custom malloc. Handle the concurrent state
 ** @param  none
 ** @return none
 ******************************************************************************/
void* atl_malloc(size_t size)
{
  ATL_CRITICAL_ENTER
  if(!atl_init_struct.init) 
  {
    ATL_CRITICAL_EXIT
    return NULL;
  }
  void* res = o1heapAllocate(atl_init_struct.heap, size);
  ATL_CRITICAL_EXIT
  return res;
}

/*******************************************************************************
 ** @brief  Function to custom free. Handle the concurrent state
 ** @param  none
 ** @return none
 ******************************************************************************/
void atl_free(void* ptr)
{
  ATL_CRITICAL_ENTER
  if(!atl_init_struct.init) 
  {
    ATL_CRITICAL_EXIT
    return;
  }
  o1heapFree(atl_init_struct.heap, ptr);
  ATL_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  Function to proc ATL core proccesses. Call it each 10ms
 ** @param  none
 ** @return none
 ******************************************************************************/
/**  @brief Helper */
static void atl_proc_handle_cmd_result(atl_entity_t* const entity, atl_item_t* const item, const bool success) 
{
  int step = success ? item->meta.ok_step : item->meta.err_step;
  if (entity->item_id < entity->item_cnt - 1) // not last
  {
    if (step != 0 || (step > 0 && entity->item_id + step < entity->item_cnt) || (step < 0 && entity->item_id + step >= 0)) 
    {
      ATL_DEBUG("[ATL][INFO] Next cmd of entity", NULL);
      entity->item_id += (step == 0) ? 1 : step;
      return;
    }
  }  
  //Step outside of cmd range or last cmd or step == 0
  ATL_DEBUG("[ATL][INFO] End of entity", NULL);
  #ifndef ATL_TEST
  if(!success) //debug
  {
    uint16_t data_start = atl_init_struct.rx_buff->head < 250
                          ? atl_init_struct.rx_buff->size - (250 - atl_init_struct.rx_buff->head)
                          : atl_init_struct.rx_buff->head - 250;
    ringslice_t rs_me = ringslice_initializer(atl_init_struct.rx_buff->buffer, atl_init_struct.rx_buff->size, data_start, atl_init_struct.rx_buff->head);
    atl_printf_from_ring(rs_me, "Failed last 250 bytes of data: ");
  }
  #endif
  if(entity->cb) entity->cb(success, entity->ctx, entity->data);
  atl_entity_dequeue();
}

/** * @brief Function */
void atl_core_proc(void)
{
  ATL_CRITICAL_ENTER
  if(atl_time >= UINT32_MAX) atl_time = 0;
  else atl_time += 1;
  if(!atl_entity_queue.entity_cnt)
  {
    ATL_CRITICAL_EXIT
    return;
  }
  atl_entity_t* entity = &atl_entity_queue.entity[atl_entity_queue.entity_tail];
  ATL_CRITICAL_EXIT //we work with exclusive memory field for this entity bcs of ring buffer
  atl_item_t* item = &entity->item[entity->item_id];
  if(entity->timer) --entity->timer;
  switch(entity->state)
  {
    case ATL_STATE_WRITE:
         ATL_DEBUG("[ATL][INFO] [TX] %s", item->req);    
         if(item->req)
         {
           ATL_CRITICAL_ENTER
           if(strncmp(item->req, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE)) == 0) atl_init_struct.atl_write((uint8_t*)item->req +strlen(ATL_CMD_SAVE), strlen(item->req+strlen(ATL_CMD_SAVE)));
           else                                                            atl_init_struct.atl_write((uint8_t*)item->req, strlen(item->req));
           ATL_CRITICAL_EXIT
         }         
         entity->timer = item->meta.wait;
         entity->state = ATL_STATE_READ;
         break;
    case ATL_STATE_READ:
         if(atl_cmd_ring_parcer(entity, item) == 1) 
         {
           entity->state = ATL_STATE_WRITE;
           ATL_DEBUG("[ATL][INFO] Successful entity cmd %d/%d", entity->item_id+1, entity->item_cnt);
           atl_proc_handle_cmd_result(entity, item, true);  
         } 
         else if(!entity->timer && item->meta.rpt_cnt) 
         {
           entity->state = ATL_STATE_WRITE;
           ATL_DEBUG("[ATL][INFO] Timeout, retries left: %d", item->meta.rpt_cnt - 1);
           if(--item->meta.rpt_cnt == 0) 
           {
             ATL_CRITICAL_ENTER
             #ifndef ATL_TEST
             atl_init_struct.rx_buff->tail = atl_init_struct.rx_buff->head;
             atl_init_struct.rx_buff->count = 0;
             #endif
             ATL_CRITICAL_EXIT
             ATL_DEBUG("[ATL][INFO] Failure entity cmd %d/%d", entity->item_id+1, entity->item_cnt);
             atl_proc_handle_cmd_result(entity, item, false);  
           }
         }
         else
         {
           if(entity->timer == item->meta.wait/2) ATL_DEBUG("[ATL][INFO] Waiting......", NULL);
         }
         break;
    default: 
         ATL_DEBUG("[ATL][INFO] Unknown state: %d", entity->state);
         return;
  }
}

#ifdef ATL_TEST
/*******************************************************************************
 ** @brief  TEST implementations
 ** @param  none
 ** @return none
 ******************************************************************************/
void _atl_core_proc(void) { 
  atl_core_proc();
}

int _atl_cmd_ring_parcer(const atl_entity_t* const entity, const atl_item_t* const item) { 
  return atl_cmd_ring_parcer(entity,item);
}

void _atl_parcer_process_urcs(const ringslice_t* me) { 
  atl_parcer_process_urcs(me); 
}

void _atl_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req) { 
  atl_parcer_find_rs_req(me, rs_req, req); 
}

void _atl_parcer_find_rs_res(const ringslice_t* const me, const ringslice_t* const rs_req, ringslice_t* const rs_res) { 
  atl_parcer_find_rs_res(me, rs_req, rs_res); 
}

void _atl_parcer_find_rs_data(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, ringslice_t* const rs_data) { 
  atl_parcer_find_rs_data(me, rs_req, rs_res, rs_data); 
}

int _atl_parcer_post_proc(const ringslice_t* const me, const ringslice_t* const rs_req, const ringslice_t* const rs_res, 
                          const ringslice_t* const rs_data, const atl_item_t* const item, const atl_entity_t* const entity) { 
  return atl_parcer_post_proc(me, rs_req, rs_res, rs_data, item, entity); 
}

int _atl_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern) { 
  return atl_string_boolean_ops(rs_data, pattern); 
}

int _atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t* const item) {
  return atl_cmd_sscanf(rs_data, item);
}

atl_entity_queue_t* _atl_get_entity_queue(void) {
  return &atl_entity_queue;
}

atl_urc_queue_t* _atl_get_urc_queue(void) {
  return atl_urc_queue;
}

atl_init_t _atl_get_init(void) {
  return atl_get_init();
}

#endif
