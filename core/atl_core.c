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
#include "dbc_assert.h"
#include "ringslice.h"
#include "tlsf.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_CORE");

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
static atl_entity_queue_t atl_entity_queue = {0};         //entity queue
static atl_urc_t atl_urc_queue[ATL_URC_QUEUE_SIZE] = {0}; //urc queue
static atl_init_t atl_init_struct = {0};                         //init struct
static uint8_t atl_mem_pool[ATL_MEMORY_POOL_SIZE] = {0};  //memory pool
static uint32_t atl_time = 0;

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Function to parce the RX ring buffer for proc with existing entities in queue.
 **         Should be called each time when the new data comes to RX ring buffer.
 ** @param  entity  current proc entity
 ** @param  item    current proc item
 ** @return 1 - success parce, 
 **         0 - some issues occurs or ERROR result of cmd
 **        -1 - issue
 ******************************************************************************/
static int atl_cmd_ring_parcer(const atl_entity_t* const entity, const atl_item_t* const item)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(101, atl_init_struct.init);
  DBC_REQUIRE(102, item);
  DBC_REQUIRE(103, entity);

  ATL_DEBUG("[INFO] READ: req='%s', answ='%s'\n", item->req, item->answ.prefix ? item->answ.prefix : "NULL");

  int res = 0;
  ringslice_t rs_me = {0};
  ringslice_t rs_req = {0};
  ringslice_t rs_res = {0}; 
  ringslice_t rs_data = {0};
  
  rs_me = ringslice_initializer(atl_init_struct.ring, atl_init_struct.ring_len, *atl_init_struct.ring_head, *atl_init_struct.ring_tail);
  DBC_ASSERT(104, !ringslice_is_empty(&rs_me));

  atl_parcer_process_urcs(&rs_me); // Process URCs first

  atl_parcer_find_rs_req(&rs_me, &rs_req, item->req); // Find request and response in buffer
  atl_parcer_find_rs_res(&rs_req, &rs_res);
  atl_parcer_find_rs_data(&rs_me, &rs_req, &rs_res, &rs_data); // Extract data section

  res = atl_parcer_post_proc(&rs_me, &rs_req, &rs_res, &rs_data, item, entity); // Proc data
  ATL_CRITICAL_EXIT
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
        ATL_DEBUG("[INFO] Found URC: %s", atl_urc_queue[i].prefix);
        atl_urc_queue[i].cb(rs_urc);
      }
    }
  }
}

/** 
 * @brief Find request ring slice 
 */
static void atl_parcer_find_rs_req(const ringslice_t* const me, ringslice_t* const rs_req, const char* const req)
{
  DBC_REQUIRE(106, me); 
  DBC_REQUIRE(107, rs_req); 

  if(!req) return;

  const uint8_t req_len = strlen(req) - 1; // Request echo returns only CR, so ignore LF

  *rs_req = ringslice_strstr(me, req);

  if(!ringslice_is_empty(rs_req)) *rs_req = ringslice_subslice(rs_req, 0, req_len);
}

/** 
 * @brief Find result ring slice 
 */
static void atl_parcer_find_rs_res(const ringslice_t* const rs_req, ringslice_t* const rs_res)
{
  DBC_REQUIRE(108, rs_req); 
  DBC_REQUIRE(109, rs_res); 

  if(ringslice_is_empty(rs_req)) return;

  *rs_res = ringslice_strstr(rs_req, ATL_CMD_ERROR);
  if(!ringslice_is_empty(rs_res)) *rs_res = ringslice_subslice(rs_res, 0, strlen(ATL_CMD_ERROR));
  
  *rs_res = ringslice_strstr(rs_req, ATL_CMD_OK);
  if(!ringslice_is_empty(rs_res)) *rs_res = ringslice_subslice(rs_res, 0, strlen(ATL_CMD_OK));
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

  if(!ringslice_is_empty(rs_req) && ringslice_is_empty(rs_res)) //No request/response, data is the entire buffer \r\nDATA\r\n
  { 
    *rs_data = *me;
  } 
  else if(!ringslice_is_empty(rs_req) && ringslice_is_empty(rs_res)) //Request but no response yet, data is after request REQ\r\r\nDATA\r\n
  {
    *rs_data = ringslice_subslice_after(rs_req, me, 0);
  } 
  else if(!ringslice_is_empty(rs_req) && !ringslice_is_empty(rs_res)) //Both request and response present
  {
    ringslice_t after_req = ringslice_subslice_after(rs_req, me, strlen(ATL_CMD_CRLF"OK"ATL_CMD_CRLF));
    *rs_data = ringslice_equals(&after_req, rs_res) 
               ? ringslice_subslice_after(rs_res, me, 0)    // REQ\r\r\nOK\r\n\r\nDATA\r\n
               : ringslice_subslice_gap(rs_req, rs_res); // REQ\r\r\nDATA\r\n\r\nOK\r\n
  }

  if((ringslice_len(rs_data) <= 2 *crlf_len) || (ringslice_strncmp(&rs_data, ATL_CMD_CRLF, 2))) // Validate data slice format
  {
    *rs_data = (ringslice_t){0};
    return;
  }

  *rs_data = ringslice_subslice_with_suffix(rs_data, crlf_len, ATL_CMD_CRLF); // Extract clean data (without surrounding CRLF)
  if(ringslice_is_empty(rs_data)) return;
  
  *rs_data = ringslice_subslice(rs_data, 0, ringslice_len(rs_data) - crlf_len);
}

/** 
 * @brief Proc all found slices 
 * \note  NULL NULL NULL  - unhandle state
 *        REQ  NULL NULL  - unhandle state
 *        REQ  RES  NULL  - handle state
 *        REQ  RES  DATA  - handle state
 *        NULL RES  NULL  - unhandle state
 *        NULL RES  DATA  - unhandle state
 *        NULL NULL DATA  - handle state
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
  
  switch((rs_req_exist << 2) | (rs_res_exist << 1) | rs_data_exist) // Bitmask: REQ[bit2] RES[bit1] DATA[bit0]
  {
      case 0b110: // REQ RES NULL
           if(ringslice_strcmp(rs_res, ATL_CMD_ERROR) == 0) res = -1;
           else if(item->answ.prefix || item->answ.format)  res = 0;
           else                                             res = 1;
           break;
      case 0b111: // REQ RES DATA  
           if(ringslice_strcmp(rs_res, ATL_CMD_ERROR) == 0) break;
           // Intentional fall-through
      case 0b001: // NULL NULL DATA
           if(item->answ.prefix) res = atl_string_boolean_ops(rs_data, item->answ.prefix);
           if(item->answ.format) res = atl_cmd_sscanf(rs_data, item);
           break;
      default:
           res = -1; 
  }
  if(item->answ.cb) item->answ.cb(*rs_data, res, entity->data);
  return res; 
}

/** 
 * @brief Boolean operation for strings in ATL
 */
static atl_string_boolean_ops(const ringslice_t* const rs_data, const char* const pattern)
{
  DBC_REQUIRE(120, rs_data); 
  DBC_REQUIRE(121, pattern);

  const char* or_sep = strchr(pattern, '|'); //Find type of boolean operation
  const char* and_sep = strchr(pattern, '&');
  
  if (or_sep && and_sep) return 0; // Combination in not allowed
  
  const char sep = or_sep ? '|' : (and_sep ? '&' : '\0');
  const int require_all = (sep == '&'); // 1 for AND, 0 for OR

  for(const char* start = pattern; *start;) 
  {
    const char* end = strchr(start, sep);
    size_t length = end ? (end - start) : strlen(start);
    int match = (ringslice_strncmp(rs_data, start) == 0);
    if(require_all) 
    { 
      if (!match) return 0; //if only one desmatch ->exit
    } else 
    {
      if (match) return 1; //if only one match ->exit
    }
    start = end ? end + 1 : start + length;
  }
  return require_all ? 1 : 0;
}

/** 
 * @brief SSCANF for ring buffer atl
 */
static int atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t* const item) 
{
  DBC_REQUIRE(122, rs_data); 
  DBC_REQUIRE(123, item);  
  const char *format = item->answ.format;
  void **output_ptrs = item->answ.ptrs;
  
  int param_count = 0;
  while (output_ptrs[param_count] != NULL) 
  {
    param_count++;
  }
  
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
 ** @brief  DBC fault override
 ** @param  none
 ** @return none
 ******************************************************************************/
__attribute__((weak)) DBC_NORETURN void DBC_fault_handler(char const* module, int label) 
{
  ATL_CRITICAL_EXIT //if handler trigerred inside of critical section
  ATL_DEBUG("[DBC FAULT]: module=%s, label=%d\n", module, label);
  while (1) 
  {
    /* Typically you would trigger a system reset or safe state here */
  }
}

/*******************************************************************************
 ** @brief  Weak function to enter into critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_enter(void) 
{ 
  /* disable irq if needed */
}

/*******************************************************************************
 ** @brief  Weak function to exit critical section
 ** @param  none
 ** @return none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_exit(void)  
{
   /* enable irq  if needed */ 
}

/*******************************************************************************
 ** @brief  Function to get lib init status
 ** @param  none
 ** @return true/false
 ******************************************************************************/
bool atl_is_init(void)  
{
  ATL_CRITICAL_ENTER
  return atl_init_struct.init;
  ATL_CRITICAL_EXIT
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
void atl_init(const atl_printf_t atl_printf, const atl_write_t atl_write, const uint8_t* const ring, 
              const uint16_t ring_len, const uint16_t* const ring_tail, const uint16_t* const ring_head)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(201, !atl_init_struct.init);
  DBC_REQUIRE(202, atl_printf);
  DBC_REQUIRE(203, atl_write);
  DBC_REQUIRE(204, ring);
  DBC_REQUIRE(205, ring_tail);
  DBC_REQUIRE(206, ring_head);
  ATL_DEBUG("[INFO] Initializing ATL library\n");
  atl_init_struct.atl_tlsf = tlsf_create_with_pool(atl_mem_pool, ATL_MEMORY_POOL_SIZE); //memory allocator init
  DBC_ASSERT(207, atl_init_struct.atl_tlsf);
  atl_init_struct.atl_write = atl_write;
  atl_init_struct.atl_printf = atl_printf;
  atl_init_struct.ring = ring;
  atl_init_struct.ring_len = ring_len;
  atl_init_struct.ring_tail = ring_tail;
  atl_init_struct.ring_head = ring_head;
  atl_init_struct.init = true;
  ATL_DEBUG("[INFO] ATL library initialized successfully\n");
  ATL_DEBUG("[INFO] Memory pool size: %d bytes\n", ATL_MEMORY_POOL_SIZE);
  ATL_DEBUG("[INFO] Entity queue size: %d\n", ATL_ENTITY_QUEUE_SIZE);
  DBC_ENSURE(208, atl_init_struct.init);
  atl_mdl_modem_init(NULL);
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
  ATL_DEBUG("[INFO] Deinitializing ATL library\n");
  tlsf_destroy(atl_init_struct.atl_tlsf);
  atl_init_struct.init = false;
  ATL_DEBUG("[INFO] ATL library deinitialized\n");
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
bool atl_entity_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t cb, uint16_t data_size, const void* const ctx)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(401, atl_init_struct.init);
  DBC_REQUIRE(402, item != NULL);
  DBC_REQUIRE(403, item_amount > 0);
  DBC_REQUIRE(404, item_amount <= ATL_MAX_ITEMS_PER_ENTITY);
  DBC_REQUIRE(405, cb != NULL);
  ATL_DEBUG("[INFO] Enqueueing entity with %d items\n", item_amount);
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_tail];
  if(atl_entity_queue.entity_cnt >= ATL_ENTITY_QUEUE_SIZE) goto error_exit;
  cur_entity->data = tlsf_malloc(atl_init_struct.atl_tlsf, data_size);
  if(!cur_entity->data && data_size) goto error_exit;
  memset(cur_entity->data, 0, data_size);
  cur_entity->item = tlsf_malloc(atl_init_struct.atl_tlsf, item_amount * sizeof(atl_item_t));
  if(!cur_entity->item) goto error_exit;
  for(int i = 0; i < item_amount; i++)
  {
    memcpy(&cur_entity->item[i], &item[i], sizeof(atl_item_t));
    if(item[i].answ.ptrs != NULL && data_size)
    {
      int ptr_count = 0;
      while(item[i].answ.ptrs[ptr_count] != NULL) 
      {
        ptr_count++;
      }
      cur_entity->item[i].answ.ptrs = tlsf_malloc(atl_init_struct.atl_tlsf, (ptr_count + 1) * sizeof(void*));
      if(!cur_entity->item[i].answ.ptrs) goto error_exit;
      for(int j = 0; j < ptr_count; j++)
      {
        size_t offset = (size_t)item[i].answ.ptrs[j];
        cur_entity->item[i].answ.ptrs[j] = (char*)cur_entity->data + offset;
        ATL_DEBUG("[INFO] Pointer[%d]: original=%p, offset=%zu, new=%p\n", j, item[i].answ.ptrs[j], offset, cur_entity->item[i].answ.ptrs[j]);
      }
      cur_entity->item[i].answ.ptrs[ptr_count] = NULL;
    }
    if(!strncmp(item[i].req, ATL_CMD_SAVE, strlen(ATL_CMD_SAVE)))
    {
      char* tmp_req = item[i].req + strlen(ATL_CMD_SAVE);
      uint8_t* new_mem = tlsf_malloc(atl_init_struct.atl_tlsf, strlen(tmp_req) + 1);
      if(!new_mem) goto error_exit;
      strcpy((char*)new_mem, tmp_req);
      cur_entity->item[i].req = (char*)new_mem;
    }
  }
  cur_entity->item_cnt = item_amount;
  cur_entity->cb = cb;
  cur_entity->ctx = ctx;
  atl_entity_queue.entity_tail = (atl_entity_queue.entity_tail + 1) % ATL_ENTITY_QUEUE_SIZE;
  ++atl_entity_queue.entity_cnt;
  ATL_DEBUG("[INFO] Entity enqueued successfully. Data at %p\n", cur_entity->data);
  ATL_CRITICAL_EXIT
  return true;

  error_exit:
    ATL_DEBUG("[ERROR] Queue failed\n"); 
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
  ATL_DEBUG("[INFO] Dequeueing entity\n");
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  uint8_t item_amount = cur_entity->item_cnt;
  if(atl_entity_queue.entity_cnt == 0)
  {
    ATL_DEBUG("[ERROR] Entity queue is already empty\n");
    ATL_CRITICAL_EXIT
    return false;
  }
  ATL_DEBUG("[INFO] Dequeueing entity with %d items\n", cur_entity->item_cnt);
  while(item_amount)
  {
    atl_item_t* item = &cur_entity->item[cur_entity->item_cnt -item_amount];
    if(item->req) tlsf_free(atl_init_struct.atl_tlsf, item->req); //free for each, there is check for ptr valid for free process
    if(item->answ.ptrs) tlsf_free(atl_init_struct.atl_tlsf, item->answ.ptrs);
    cur_entity->item[cur_entity->item_cnt -item_amount].req = NULL;
    --item_amount;
  }
  if(cur_entity->item) tlsf_free(atl_init_struct.atl_tlsf, cur_entity->item);
  cur_entity->item = NULL;
  atl_entity_queue.entity_head = (atl_entity_queue.entity_head +1) % ATL_ENTITY_QUEUE_SIZE;
  --atl_entity_queue.entity_cnt;
  ATL_DEBUG("[INFO] Entity dequeued. Queue count: %d\n", atl_entity_queue.entity_cnt);
  DBC_ENSURE(502, atl_entity_queue.entity_cnt >= 0);
  ATL_CRITICAL_EXIT
  return true;
}

/*******************************************************************************
 ** @brief  Function to append URC queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_enqueue(const atl_urc_t* const urc)  
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(601, urc);
  atl_urc_t* tmp = NULL;
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
    ATL_DEBUG("[ERROR] URC queue is full");
    ATL_CRITICAL_EXIT
    return false;
  }
  memcpy(tmp, urc, ATL_URC_SIZE);
  ATL_DEBUG("[INFO] URC enqueued successfully");
  ATL_CRITICAL_EXIT
  return true;
}

/*******************************************************************************
 ** @brief  Function to delete URC from queue
 ** @param  urc  ptr to your URC.
 ** @return ture/false
 ******************************************************************************/
bool atl_urc_dequeue(const atl_urc_t* const urc)  
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(701, urc);
  atl_urc_t* tmp = NULL;
  for(uint8_t i = 0; i <= ATL_URC_QUEUE_SIZE; ++i)
  {
    if(!atl_urc_queue[i].prefix) continue;
    if(strcmp(atl_urc_queue[i].prefix, urc->prefix) == 0)
    {
      memset(tmp, 0, ATL_URC_SIZE);
      ATL_DEBUG("[INFO] URC dequeued successfully");
      ATL_CRITICAL_EXIT
      return true;
    }
  }
  ATL_DEBUG("[INFO] URC dequeued fail");
  ATL_CRITICAL_EXIT
  return false;
}

/*******************************************************************************
 ** @brief  Function get time in 10ms. 
 ** @param  none
 ** @return none
 ******************************************************************************/
uint16_t atl_get_cur_time(void)
{
  ATL_CRITICAL_ENTER
  return atl_time;
  ATL_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  Function get init struct. 
 ** @param  none
 ** @return none
 ******************************************************************************/
atl_init_t atl_get_init(void)
{
  ATL_CRITICAL_ENTER
  return atl_init_struct;
  ATL_CRITICAL_EXIT
}

/*******************************************************************************
 ** @brief  Function to proc ATL core proccesses. Call it each 10ms
 ** @param  none
 ** @return none
 ******************************************************************************/
void atl_core_proc(void)
{
  ATL_CRITICAL_ENTER
  if(!atl_entity_queue.entity_cnt)
  {
    ATL_CRITICAL_EXIT; 
    return;
  }
  if(atl_time >= UINT32_MAX) atl_time = 0;
  else atl_time += 1;
  atl_entity_t* entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  atl_item_t* item = &entity->item[entity->item_id];
  if(entity->timer) --entity->timer;
  switch(entity->state)
  {
    case ATL_STATE_WRITE:
         ATL_DEBUG("[INFO] WRITE: '%s'\n", item->req);
         if(item->req) atl_init_struct.atl_write(item->req, strlen(item->req));
         entity->timer = item->meta.wait;
         entity->state = ATL_STATE_READ;
         break;
    case ATL_STATE_READ:
         if(atl_cmd_ring_parcer(entity, item)) 
         {
           ATL_DEBUG("[INFO] Command successful\n");
           if(entity->item_id >= entity->item_cnt -1) //that was the last one
           {
             if(entity->cb) entity->cb(true, entity->ctx, entity->data);
             if(entity->data) tlsf_free(atl_init_struct.atl_tlsf, entity->data);
             atl_dequeue();
           } 
           else 
           {
              if(item->meta.ok_step == 0 ||
                (item->meta.ok_step > 0 && entity->item_id + item->meta.ok_step >= entity->item_cnt - 1) ||
                (item->meta.ok_step < 0 && item->meta.ok_step > entity->item_id)) 
              {
                entity->item_id++;
              }
            }
         } 
         else if(!entity->timer && item->meta.rpt_cnt)
         {
           ATL_DEBUG("[INFO] Timeout, retries left: %d\n", item->meta.rpt_cnt - 1);
           --item->meta.rpt_cnt;
           if(!item->meta.rpt_cnt) 
           {
             ATL_DEBUG("[INFO] No rtries left\n");
             if(entity->item_id >= entity->item_cnt -1) //that was the last one
             {
               if(entity->cb) entity->cb(false, entity->ctx, entity->data);
               if(entity->data) tlsf_free(atl_init_struct.atl_tlsf, entity->data);
               atl_dequeue();
             } 
             else
             {
               if(item->meta.err_step == 0 ||
                 (item->meta.err_step > 0 && entity->item_id + item->meta.err_step >= entity->item_cnt - 1) ||
                 (item->meta.err_step < 0 && item->meta.err_step > entity->item_id)) 
               {
                 entity->item_id++;
               }
             }
           }
           entity->state = ATL_STATE_WRITE;
         }
         break;
    default: 
        ATL_DEBUG("[ERROR] Unknown state: %d\n", entity->state);
        ATL_CRITICAL_EXIT
        return;
  }
  ATL_CRITICAL_EXIT
}

