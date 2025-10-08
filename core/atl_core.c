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
static void atl_parcer_process_urcs(const ringslice_t me);
static ringslice_t atl_parcer_find_rs_req(const ringslice_t me, const char *req);
static ringslice_t atl_parcer_find_rs_res(const ringslice_t rs_req);
static ringslice_t atl_parcer_extract_rs_data(const ringslice_t me, const ringslice_t rs_req, const ringslice_t rs_res);
static int atl_parcer_proc_rs_data(const ringslice_t rs_data, const atl_item_t *item);
static int atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t *item);

/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static atl_entity_queue_t atl_entity_queue = {0};        //entity queue
static atl_urc_queue_t atl_urc_queue = {0};              //urc queue
static atl_init_t atl_init = {0};                        //init struct
static uint8_t atl_mem_pool[ATL_MEMORY_POOL_SIZE] = {0}; //memory pool

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** \brief  Function to parce the RX ring buffer for proc with existing entities in queue.
 **         Should be called each time when the new data comes to RX ring buffer.
 ** \param  item  current proc item
 ** \retval 1 - success parce, 
 **         0 - some issues occurs or ERROR result of cmd, 
 ******************************************************************************/
static int atl_cmd_ring_parcer(const atl_item_t* item)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(201, atl_user_init.init);
  DBC_REQUIRE(202, item);

  ATL_DEBUG("[INFO] READ: req='%s', answ='%s'\n", item->req, item->prefix ? item->prefix : "NULL");

  const ringslice_t rs_me = ringslice_initializer(atl_user_init->ring, atl_user_init->ring_len, 
                                                 *atl_user_init->ring_head, *atl_user_init->ring_tail);

  // Process URCs first
  atl_parcer_process_urcs(rs_me);

  // Find request and response in buffer
  ringslice_t rs_req = atl_parcer_find_rs_req(rs_me, item->req);
  ringslice_t rs_res = atl_parcer_find_rs_res(rs_req);
  if(ringslice_is_empty(res_slice)) return 0; // Error or none response

  // Extract data section
  ringslice_t data_slice = atl_parcer_extract_rs_data(rs_me, rs_req, rs_res);
  if(ringslice_is_empty(data_slice)) return 0;

  int res = atl_parcer_proc_rs_data(data_slice, item);
  ATL_CRITICAL_EXIT
  return res;
}

/** 
 * \brief Find and proc URC 
 */
static void atl_parcer_process_urcs(const ringslice_t me)
{
  DBC_REQUIRE(301, !ringslice_is_empty(me));
  for(uint8_t i = atl_urc_queue_t.urc_head; i < atl_urc_queue_t.urc_cnt; i = (i+1) % ATL_URC_QUEUE_SIZE) 
  {
    ringslice_t rs_urc = ringslice_strstr(me, atl_urc_queue_t[i].urc.prefix);
    if(!ringslice_is_empty(rs_urc) && atl_urc_queue_t[i].urc.cb) 
    {
      ATL_DEBUG("[INFO] Found URC: %s", atl_urc_queue_t[i].urc.prefix);
      atl_urc_queue_t[i].urc.cb(rs_urc);
    }
  }
}

/** 
 * \brief Find request ring slice 
 */
static ringslice_t atl_parcer_find_rs_req(const ringslice_t me, const char *req)
{
  DBC_REQUIRE(402, !ringslice_is_empty(me));
  DBC_REQUIRE(403, req);
  const uint8_t req_len = strlen(req) - 1; // Request echo returns only CR, so ignore LF
  ringslice_t rs_req = ringslice_strstr(me, req);
  if (!ringslice_is_empty(rs_req)) rs_req = ringslice_subslice(rs_req, 0, req_len);
  return rs_req;
}

/** 
 * \brief Find result ring slice 
 */
static ringslice_t atl_parcer_find_rs_res(const ringslice_t rs_req)
{
  const char* const CMD_OK    =  ATL_CMD_CRLF"OK"ATL_CMD_CRLF;
  const char* const CMD_ERROR =  ATL_CMD_CRLF"ERROR"ATL_CMD_CRLF;
  if(ringslice_is_empty(rs_req)) return(ringslice_t){0};

  ringslice_t rs_res_err = ringslice_strstr(rs_req, CMD_ERROR);
  if (!ringslice_is_empty(rs_res_err)) return rs_res_err;
  
  ringslice_t rs_res_ok = ringslice_strstr(rs_req, CMD_OK);
  if (!ringslice_is_empty(rs_res_ok)) return ringslice_subslice(rs_res_ok, 0, strlen(CMD_OK));
  
  return(ringslice_t){0};
}

/** 
 * \brief Find and data ring slice 
 */
static ringslice_t atl_parcer_extract_rs_data(const ringslice_t me, const ringslice_t rs_req, const ringslice_t rs_res)
{
    const uint8_t crlf_len = strlen(ATL_CMD_CRLF);
    ringslice_t rs_data = {0};

    if(ringslice_is_empty(rs_req) && ringslice_is_empty(rs_res)) 
    { 
      //No request/response, data is the entire buffer \r\nDATA\r\n
      rs_data = me;
    } 
    else if(!ringslice_is_empty(rs_req) && ringslice_is_empty(rs_res))
    {
      //Request but no response yet, data is after request REQ\r\r\nDATA\r\n
      rs_data = ringslice_subslice_after(rs_req, me, 0);
    } 
    else if(!ringslice_is_empty(rs_req) && !ringslice_is_empty(rs_res)) 
    {
      //Both request and response present
      ringslice_t after_req = ringslice_subslice_after(rs_req, me, strlen(ATL_CMD_CRLF"OK"ATL_CMD_CRLF));
      rs_data = ringslice_equals(after_req, rs_res) 
                ? ringslice_subslice_after(rs_res, me)  // REQ\r\r\nOK\r\n\r\nDATA\r\n
                : ringslice_subslice_gap(rs_req, rs_res); // REQ\r\r\nDATA\r\n\r\nOK\r\n
    }

    // Validate data slice format
    if(ringslice_len(rs_data) <= 2 *crlf_len) return(ringslice_t){0};
    if(ringslice_strcmp(ringslice_subslice(rs_data, 0, 2), ATL_CMD_CRLF)) return(ringslice_t){0};
    
    // Extract clean data (without surrounding CRLF)
    rs_data = ringslice_subslice_with_suffix(rs_data, crlf_len, ATL_CMD_CRLF);
    if(ringslice_is_empty(rs_data)) return (ringslice_t){0};
    
    return ringslice_subslice(rs_data, 0, ringslice_len(rs_data) - crlf_len);
}

/** 
 * \brief Proc data ring slice 
 */
static int atl_parcer_proc_rs_data(const ringslice_t rs_data, const atl_item_t *item)
{
  if(item->prefix && ringslice_strcmp(rs_data, item->prefix)) return 0; // Check prefix match
  switch (item->meta.type) // Process based on type
  {
    case 1: // Callback type
        if (item->answ_data.cb) 
        {
          item->answ_data.cb(rs_data);
          return 1;
        }
        break;
    case 0: // Format type  
        if (item->answ_data.format_data.format) 
        {
          return atl_cmd_sscanf(&rs_data, item);
        }
        break;
  }
  return item->prefix ? 0 : 1; //If no prefix and no processor, consider success
}

/** 
 * \brief SSCANF for ring buffer atl
 */
static int atl_cmd_sscanf(const ringslice_t* const rs_data, const atl_item_t *item) 
{
  DBC_REQUIRE(101, rs_data);
  DBC_REQUIRE(102, item);
  DBC_REQUIRE(103, item->meta.type == 0);  
  const char *format = item->answ_data.format_data.format;
  void **output_ptrs = item->answ_data.format_data.ptrs;
  
  int param_count = 0;
  while (output_ptrs[param_count] != NULL) {
    param_count++;
  }
  
  DBC_ASSERT(104, format);
  DBC_ASSERT(105, output_ptrs);
  switch (param_count){
    case 1: return ringslice_scanf(rs_data, format, output_ptrs[0]) == 1;
    case 2: return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1]) == 2;
    case 3: return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2]) == 3;
    case 4: return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3]) == 4;
    case 5: return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3], output_ptrs[4]) == 5;
    case 6: return ringslice_scanf(rs_data, format, output_ptrs[0], output_ptrs[1], output_ptrs[2], output_ptrs[3], output_ptrs[4], output_ptrs[5]) == 6;
    default: return 0;
  }
}

/*******************************************************************************
 ** \brief  DBC fault override
 ** \param  none
 ** \retval none
 ******************************************************************************/
__attribute__((weak)) DBC_NORETURN void DBC_fault_handler(char const * module, int label) 
{
  ATL_DEBUG("[DBC FAULT]: module=%s, label=%d\n", module, label);
  while (1) 
  {
    /* Typically you would trigger a system reset or safe state here */
  }
}

/*******************************************************************************
 ** \brief  DBC fault override
 ** \param  none
 ** \retval none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_enter(void) 
{ 
  /* disable irq if needed */ 
}

/*******************************************************************************
 ** \brief  DBC fault override
 ** \param  none
 ** \retval none
 ******************************************************************************/
__attribute__((weak)) void atl_crit_exit(void)  
{
   /* enable irq  if needed */ 
}

/*******************************************************************************
 ** \brief  DBC fault override
 ** \param  none
 ** \retval none
 ******************************************************************************/
bool atl_is_init(void)  
{
  return atl_user_init.init;
}


/*******************************************************************************
 ** \brief  Init atl lib  
 ** \param  atl_printf pointer to user func of printf
 ** \param  atl_write  pointer to user func of write to uart
 ** \param  ring            ptr to RX ring buffer
 ** \param  ring_len        lenght of RX ring buffer. 
 ** \param  ring_tail       tail of RX ring buffer. 
 ** \param  ring_head       head of RX ring buffer. 
 ** \retval none
 ******************************************************************************/
void atl_init(const atl_printf atl_printf, const atl_write atl_write, const uint8_t* ring, 
              const uint16_t ring_len, const uint16_t* const ring_tail, const uint16_t* const ring_head)
{
  ATL_CRITICAL_ENTER
  DBC_REQUIRE(100, !atl_user_init.init);
  DBC_REQUIRE(101, atl_printf != NULL);
  DBC_REQUIRE(102, atl_write != NULL);
  DBC_REQUIRE(103, ring != NULL);
  DBC_REQUIRE(104, ring_tail != NULL);
  DBC_REQUIRE(105, ring_head != NULL);
  ATL_DEBUG("[INFO] Initializing ATL library\n");
  atl_user_init.atl_tlsf = tlsf_create_with_pool(atl_mem_pool, ATL_MEMORY_POOL_SIZE); //memory allocator init
  DBC_ASSERT(106, atl_user_init.atl_tlsf);
  atl_user_init.atl_write = atl_write;
  atl_user_init.atl_printf = atl_printf;
  atl_user_init.ring = ring;
  atl_user_init.ring_len = ring_len;
  atl_user_init.ring_tail = ring_tail;
  atl_user_init.ring_head = ring_head;
  atl_user_init.init = true;
  ATL_DEBUG("[INFO] ATL library initialized successfully\n");
  ATL_DEBUG("[INFO] Memory pool size: %d bytes\n", ATL_MEMORY_POOL_SIZE);
  ATL_DEBUG("[INFO] Entity queue size: %d\n", ATL_ENTITY_QUEUE_SIZE);
  DBC_ENSURE(107, atl_user_init.init);
  ATL_CRITICAL_EXIT
}

/*******************************************************************************
 ** \brief  DeInit atl lib  
 ** \param  atl_printf pointer to user func of printf
 ** \param  atl_write  pointer to user func of write to uart
 ** \retval false: some error is ocur true: ok
 ******************************************************************************/
void atl_deinit(void)
{
  DBC_REQUIRE(200, atl_user_init.init);
  ATL_DEBUG("[INFO] Deinitializing ATL library\n");
  tlsf_destroy(atl_user_init.atl_tlsf);
  atl_user_init.init = false;
  ATL_DEBUG("[INFO] ATL library deinitialized\n");
  DBC_ENSURE(201, !atl_user_init.init);
}

/*******************************************************************************
 ** \brief  Function to append main queue with new group of at cmds
 ** \param  item         ptr to your group of at cmds.
 ** \param  item_amount  amount  of your at cms in group 
 ** \param  entity_cb    ur callback function for the whole group  ptr to your group of at cmds.
 ** \retval true: ok false: error while trying to append
 ******************************************************************************/
bool atl_entity_enqueue(const atl_item_t* const item, const uint8_t item_amount, const atl_entity_cb_t entity_cb)
{
  DBC_REQUIRE(300, atl_user_init.init);
  DBC_REQUIRE(301, item != NULL);
  DBC_REQUIRE(302, item_amount > 0);
  DBC_REQUIRE(303, item_amount <= ATL_MAX_ITEMS_PER_ENTITY);
  DBC_REQUIRE(304, entity_cb != NULL);
  ATL_DEBUG("[INFO] Enqueueing entity with %d items\n", item_amount);
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_tail];
  if(atl_entity_queue.entity_cnt >= ATL_ENTITY_QUEUE_SIZE) 
  {
    ATL_DEBUG("[ERROR] Entity queue is full (current: %d, max: %d)\n", atl_entity_queue.entity_cnt, ATL_ENTITY_QUEUE_SIZE);
    return false;
  }
  cur_entity->item_cnt = item_amount;
  cur_entity->cb = entity_cb;
  cur_entity->item = tlsf_malloc(atl_user_init.atl_tlsf, item_amount *ATL_ITEM_SIZE);
  if(!cur_entity->item) 
  {
    ATL_DEBUG("[ERROR] Failed to allocate memory for %d items\n", item_amount); 
    atl_deinit(); 
    return false; 
  } 
  ATL_DEBUG("[INFO] Allocated memory for %d items at 0x%p\n", item_amount, cur_entity->item);
  while(item_amount)
  {
    memcpy(&cur_entity->item[cur_entity->item_cnt -item_amount], &item[cur_entity->item_cnt -item_amount], ATL_ITEM_SIZE);
    ATL_DEBUG("[INFO] Item %d: request='%s', answer='%s'\n", i, item[i].request, item[i].answer ? item[i].answer : "NULL");
    if(!strncmp(item[cur_entity->item_cnt -item_amount].request, ATL_CMD_SAVE_PATTERN, strlen(ATL_CMD_SAVE_PATTERN)))
    {
      char* tmp_req = item[cur_entity->item_cnt -item_amount].request +strlen(ATL_CMD_SAVE_PATTERN);
      uint8_t* new_mem = tlsf_malloc(atl_user_init.atl_tlsf, strlen(tmp_req)+1); //+1 \0
      if(!new_mem) 
      {
        ATL_DEBUG("[ERROR] Failed to allocate memory for request pattern\n"); 
        atl_deinit(); 
        return false; 
      } 
      strcpy(new_mem, tmp_req);
      item[cur_entity->item_cnt -item_amount].request = new_mem;
      ATL_DEBUG("[INFO] Saved pattern for item %d: '%s'\n", i, new_mem);
    }
    --item_amount;
  }
  atl_entity_queue.entity_tail = (atl_entity_queue.entity_tail +1) % ATL_ENTITY_QUEUE_SIZE;
  ++atl_entity_queue.entity_cnt;
  ATL_DEBUG("[INFO] Entity enqueued successfully. Queue count: %d\n", atl_entity_queue.entity_cnt);
  DBC_ENSURE(305, atl_entity_queue.entity_cnt > 0);
  return true;
}

/*******************************************************************************
 ** \brief  Clear first entity from the queue 
 ** \param  none
 ** \retval false: some errors; true: ok
 ******************************************************************************/
bool atl_entity_dequeue(void)
{
  DBC_REQUIRE(400, atl_user_init.init);
  ATL_DEBUG("[INFO] Dequeueing entity\n");
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  uint8_t item_amount = cur_entity->item_cnt;
  if(atl_entity_queue.entity_cnt == 0)
  {
    ATL_DEBUG("[ERROR] Entity queue is already empty\n");
    return false;
  }
  ATL_DEBUG("[INFO] Dequeueing entity with %d items\n", cur_entity->item_cnt);
  while(item_amount)
  {
    char* tmp_req = cur_entity->item[cur_entity->item_cnt -item_amount].request;
    if(tmp_req) 
    {
      ATL_DEBUG("[INFO] Freeing request memory for item");
      tlsf_free(atl_user_init.atl_tlsf, tmp_req); //free for each, there is check for ptr valid for free process
    }
    cur_entity->item[cur_entity->item_cnt -item_amount].request = NULL;
    --item_amount;
  }
  if(cur_entity->item) 
  {
    ATL_DEBUG("[INFO] Freeing items array at 0x%p\n", cur_entity->item);
    tlsf_free(atl_user_init.atl_tlsf, cur_entity->item);
  }
  cur_entity->item = NULL;
  atl_entity_queue.entity_head = (atl_entity_queue.entity_head +1) % ATL_ENTITY_QUEUE_SIZE;
  --atl_entity_queue.entity_cnt;
  ATL_DEBUG("[INFO] Entity dequeued. Queue count: %d\n", atl_entity_queue.entity_cnt);
  DBC_ENSURE(401, atl_entity_queue.entity_cnt >= 0);
  return true;
}

/*******************************************************************************
 ** \brief  DBC fault override
 ** \param  none
 ** \retval none
 ******************************************************************************/
bool atl_urc_enqueue(void)  
{
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_tail];
  if(atl_entity_queue.entity_cnt >= ATL_ENTITY_QUEUE_SIZE) 
  {
    ATL_DEBUG("[ERROR] Entity queue is full (current: %d, max: %d)\n", atl_entity_queue.entity_cnt, ATL_ENTITY_QUEUE_SIZE);
    return false;
  }
  memcpy(&cur_entity->item[cur_entity->item_cnt -item_amount], &item[cur_entity->item_cnt -item_amount], ATL_ITEM_SIZE);
  atl_entity_queue.entity_tail = (atl_entity_queue.entity_tail +1) % ATL_ENTITY_QUEUE_SIZE;
  ++atl_entity_queue.entity_cnt;
  ATL_DEBUG("[INFO] Entity enqueued successfully. Queue count: %d\n", atl_entity_queue.entity_cnt);
}

/*******************************************************************************
 ** \brief  DBC fault override
 ** \param  none
 ** \retval none
 ******************************************************************************/
bool atl_urc_dequeue(void)  
{
  atl_entity_t* cur_entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  uint8_t item_amount = cur_entity->item_cnt;
  if(atl_entity_queue.entity_cnt == 0)
  {
    ATL_DEBUG("[ERROR] Entity queue is already empty\n");
    return false;
  }
  return atl_user_init.init;
  atl_entity_queue.entity_head = (atl_entity_queue.entity_head +1) % ATL_ENTITY_QUEUE_SIZE;
  --atl_entity_queue.entity_cnt;
  ATL_DEBUG("[INFO] Entity dequeued. Queue count: %d\n", atl_entity_queue.entity_cnt);
  DBC_ENSURE(401, atl_entity_queue.entity_cnt >= 0);
  return true;
}


/*******************************************************************************
 ** \brief  Function to proc ATL core proccesses. 
 ** \param  none
 ** \retval none
 ******************************************************************************/
void atl_entity_proc(void)
{
  if(!atl_entity_queue.entity_cnt) return;
  atl_entity_t* entity = &atl_entity_queue.entity[atl_entity_queue.entity_head];
  atl_item_t* item = &entity->item[entity->item_id];
  if(entity->timer) --entity->timer;
  switch(entity->state)
  {
    case ATL_STATE_WRITE:
         ATL_DEBUG("[INFO]  WRITE: '%s'\n", item->request);
         if(item->request) atl_user_init.atl_write(item->request, strlen(item->request));
         entity->timer = item->wait;
         entity->state = ATL_STATE_READ;
         break;
    case ATL_STATE_READ:
         if(atl_cmd_ring_parcer(item->request, item->answer, item->data, item->data_size)) 
         {
           ATL_DEBUG("[INFO] Command successful\n");
           if(entity->item_id >= entity->item_cnt -1) //that was the last one
           {
             entity->cb(true);
             atl_dequeue();
           } 
           else 
           {
             ++entity->item_id;
           }
         } 
         else if(!entity->timer && item->rpt_cnt)
         {
           ATL_DEBUG("[INFO] Timeout, retries left: %d\n", item->rpt_cnt - 1);
           --item->rpt_cnt;
           if(!item->rpt_cnt) 
           {
             ATL_DEBUG("[INFO] No rtries left\n");
             if(item->request) 
             {
               if(entity->item_id >= (entity->item_cnt -1) && entity->cb) entity->cb(false);
               atl_dequeue();
             } 
             else 
             {
               if(entity->item_id >= (entity->item_cnt -1)) 
               {
                 if(entity->cb) entity->cb(true);
                 atl_dequeue();
               }
               else
               {
                 ++entity->item_id;
               }
             }
           }
           entity->state = ATL_STATE_WRITE;
         }
         break;
    default: 
        ATL_DEBUG("[ERROR] Unknown state: %d\n", entity->state);
        return;
  }
}

