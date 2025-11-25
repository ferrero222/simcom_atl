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
#include "atl_chain.h"
#include "dbc_assert.h"
#include "o1heap.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_CHAIN")

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static bool atl_chain_step_function_proc(atl_chain_t* chain, chain_step_t* const step);
static bool atl_chain_step_exec_proc(atl_chain_t* chain, chain_step_t* const step);
static bool atl_chain_step_loop_start_proc(atl_chain_t* chain, chain_step_t* const step);
static bool atl_chain_step_loop_end_proc(atl_chain_t* chain, chain_step_t* const step);
static bool atl_chain_step_delay_proc(atl_chain_t* chain, chain_step_t* const step);

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
 ** @brief  Notify chain about step completion
 **         This function should be called from AT command callbacks
 ** @param  none
 ** @return none
 ******************************************************************************/
static void atl_chain_step_cb(const bool result, void* const ctx, const void* const data) 
{
  DBC_REQUIRE(101, ctx);
  const atl_chain_t *chain = (atl_chain_t*)ctx;
  chain_step_t* const step = &chain->steps[chain->current_step];
  if(step->state == ATL_CHAIN_STEP_RUNNING) 
  {
    step->state = result ? ATL_CHAIN_STEP_SUCCESS : ATL_CHAIN_STEP_ERROR;
    step->execution_count++;
    ATL_DEBUG("[ATL][INFO] Step '%s' completed with %s", step->name, result ? "SUCCESS" : "ERROR");
    if(step->action.func.cb) step->action.func.cb(result, step->action.func.ctx, data);
  }
}

/******************************************************************************* 
 ** @brief  Calculate maximum loop nesting depth in chain
 ** @param  steps Array of steps
 ** @param  step_count Number of steps
 ** @return Maximum loop nesting depth
 *******************************************************************************/
static uint32_t atl_chain_cal_max_loop_depth(const chain_step_t* const steps, const uint32_t step_count) 
{
  DBC_REQUIRE(201, steps && step_count);
  uint32_t current_depth = 0;
  uint32_t max_depth = 0;
  for(uint32_t i = 0; i < step_count; i++) 
  {
    if(steps[i].type == ATL_CHAIN_STEP_LOOP_START) 
    {
      current_depth++;
      if(current_depth > max_depth) max_depth = current_depth;
    } 
    else if(steps[i].type == ATL_CHAIN_STEP_LOOP_END) 
    {
      if(current_depth > 0) current_depth--;
    }
  }
  return (max_depth > 0) ? (max_depth + 2) : 3;
}

/*******************************************************************************
 ** @brief  Find step index by name
 ** @param  none
 ** @retval none
 *******************************************************************************/
static uint32_t atl_chain_find_step_index_by_name(const atl_chain_t* const chain, const char* const step_name) 
{
  DBC_REQUIRE(301, chain);
  DBC_REQUIRE(302, step_name);
  for(uint32_t i = 0; i < chain->step_count; i++) 
  {
    if(chain->steps[i].name && strcmp(chain->steps[i].name, step_name) == 0) 
    {
      ATL_CRITICAL_EXIT
      return i;
    }
  }
  ATL_DEBUG("[ATL][ERROR] Step '%s' not found!", step_name);
  return UINT32_MAX;
}

/*******************************************************************************
 ** @brief  Find step index by name
 ** @param  none
 ** @retval none
 *******************************************************************************/
static bool atl_chain_execute_step_prepare(atl_chain_t* const chain, const uint32_t target_index)
{
  if(target_index != chain->current_step) //check loops
  {
    if(target_index > chain->current_step) //forward dest
    {
      for(uint32_t i = chain->current_step; i < target_index; i++) 
      {
        if(chain->steps[i].type == ATL_CHAIN_STEP_LOOP_START)
        {
          if(chain->loop_stack_ptr >= chain->loop_stack_size) return false;
          chain->loop_stack[chain->loop_stack_ptr].start_step_index = i;
          chain->loop_stack[chain->loop_stack_ptr].iteration_count = 0;
          chain->loop_stack_ptr++;
          chain->steps[i].state = ATL_CHAIN_STEP_SUCCESS;
        }
        else if(chain->steps[i].type == ATL_CHAIN_STEP_LOOP_END)
        {
          if(!chain->loop_stack_ptr) return false;
          atl_loop_stack_item_t *current_loop = &chain->loop_stack[chain->loop_stack_ptr - 1];
          chain_step_t *loop_start = &chain->steps[current_loop->start_step_index];
          chain->loop_stack_ptr--;
          loop_start->state = ATL_CHAIN_STEP_IDLE;
          current_loop->iteration_count = 0;
        }
      }
    }
    else //backward dest
    {
      for(uint32_t i = chain->current_step; i > target_index; i--) 
      {
        if(chain->steps[i].type == ATL_CHAIN_STEP_LOOP_START)
        {
          if(chain->steps[i].state == ATL_CHAIN_STEP_IDLE) continue;
          if(!chain->loop_stack_ptr) return false;
          atl_loop_stack_item_t *current_loop = &chain->loop_stack[chain->loop_stack_ptr - 1];
          chain_step_t *loop_start = &chain->steps[current_loop->start_step_index];
          chain->loop_stack_ptr--;
          loop_start->state = ATL_CHAIN_STEP_IDLE;
          current_loop->iteration_count = 0;
        }
      }
      for(uint32_t i = 0; i < target_index; i++) 
      {
        if(chain->steps[i].type == ATL_CHAIN_STEP_LOOP_START)
        {
          if(chain->steps[i].state != ATL_CHAIN_STEP_IDLE) continue;
          if(chain->loop_stack_ptr >= chain->loop_stack_size) return false;
          chain->loop_stack[chain->loop_stack_ptr].start_step_index = i;
          chain->loop_stack[chain->loop_stack_ptr].iteration_count = 0;
          chain->loop_stack_ptr++;
          chain->steps[i].state = ATL_CHAIN_STEP_SUCCESS;
        }
      }
    } 
  }
  return true;
}

/*******************************************************************************
 ** @brief  Execute jump to target step
 ** @param  none
 ** @retval none
 *******************************************************************************/
static bool atl_chain_execute_step_jump(atl_chain_t* const chain, const char* const target_name) 
{
  if(!target_name || strcmp(target_name, "STOP") == 0) 
  {
    return false;
  }
  if(!target_name || strcmp(target_name, "NEXT") == 0) 
  {
    chain->current_step++;
    return true;
  }
  if(strcmp(target_name, "PREV") == 0) 
  {
    if(chain->current_step) chain->current_step--; //not first
    else return false;
    return true;
  }
  
  uint32_t target_index = atl_chain_find_step_index_by_name(chain, target_name);
  if(target_index == UINT32_MAX) return false;
    
  if(!atl_chain_execute_step_prepare(chain, target_index)) return false;

  chain->current_step = target_index;
  return true;
}

/*******************************************************************************
 ** @brief  Reset internal chain state
 ** @param  none
 ** @retval none
 *******************************************************************************/
static void atl_chain_reset_state(atl_chain_t* const chain) 
{
  DBC_REQUIRE(401, chain);
  chain->current_step = 0;
  chain->loop_stack_ptr = 0;
  for(uint32_t i = 0; i < chain->step_count; i++) //Reset all steps to initial state
  {
    chain->steps[i].state = ATL_CHAIN_STEP_IDLE;
    chain->steps[i].execution_count = 0;
  }
}

/*******************************************************************************
 ** @brief  Process one chain step based on current state
 ** @param  none
 ** @retval none
 *******************************************************************************/
static bool atl_chain_process_step(atl_chain_t* const chain) 
{
  DBC_REQUIRE(501, chain);
  bool res = true;
  if(chain->current_step >= chain->step_count) // Check if chain completed
  { 
    chain->is_running = false;
    ATL_DEBUG("[ATL][INFO] Chain '%s' completed", chain->name);
    res = false;
    return res;
  }
  chain_step_t *step = &chain->steps[chain->current_step];
  DBC_ASSERT(502, step);
  switch(step->type) 
  {
    case ATL_CHAIN_STEP_FUNCTION:   res = atl_chain_step_function_proc(chain, step);   break;
    case ATL_CHAIN_STEP_EXEC:       res = atl_chain_step_exec_proc(chain, step);       break;
    case ATL_CHAIN_STEP_LOOP_START: res = atl_chain_step_loop_start_proc(chain, step); break;
    case ATL_CHAIN_STEP_LOOP_END:   res = atl_chain_step_loop_end_proc(chain, step);   break;
    case ATL_CHAIN_STEP_DELAY:      res = atl_chain_step_delay_proc(chain, step);      break;
    default: 
      ATL_DEBUG("[ATL][ERROR] Unknown step type: %d", step->type);
      chain->is_running = false;
      return false;
  }
  return true;
}

/** 
 * @brief Chain step function proc
 */
static bool atl_chain_step_function_proc(atl_chain_t* chain, chain_step_t* const step)
{
  switch(step->state) 
  {
     case ATL_CHAIN_STEP_IDLE: 
          ATL_DEBUG("[ATL][INFO] Starting step '%s'", step->name); // Start executing the function
          step->state = ATL_CHAIN_STEP_RUNNING;
          if(!step->action.func.function(atl_chain_step_cb, step->action.func.param, chain))
          {
            ATL_DEBUG("[ATL][ERROR] Failed to start step '%s'", step->name);
            step->state = ATL_CHAIN_STEP_ERROR;
            step->execution_count++;
          }
          break; 
     case ATL_CHAIN_STEP_RUNNING: // Waiting for callback - do nothing this cycle
          break;
     case ATL_CHAIN_STEP_SUCCESS: 
          step->state = ATL_CHAIN_STEP_IDLE;
          step->execution_count = 0; 
          if(!atl_chain_execute_step_jump(chain, step->action.func.success_target)) // Jump to success target
          {
            chain->is_running = false;
            return false;
          }
          break;
     case ATL_CHAIN_STEP_ERROR: 
          ATL_DEBUG("[ATL][ERROR] Step '%s' failed (attempt %u/%u)", step->name, step->execution_count, step->action.func.max_retries);  // Function completed with error
          if(step->execution_count < step->action.func.max_retries) // Check if we should retry
          { 
            step->state = ATL_CHAIN_STEP_IDLE;
            ATL_DEBUG("[ATL][INFO] Retrying '%s'", step->name);
          } 
          else 
          {
            step->state = ATL_CHAIN_STEP_IDLE;
            step->execution_count = 0; 
            if(!atl_chain_execute_step_jump(chain, step->action.func.error_target)) 
            {
              chain->is_running = false;
              return false;
            }
          }
          break;
      default:
          chain->is_running = false;
          return false;
  }
  return true;
}

/** 
 * @brief Chain step exec proc
 */
static bool atl_chain_step_exec_proc(atl_chain_t* chain, chain_step_t* const step)
{
  if(step->action.exec.function) // Exec are executed synchronously
  {
    bool exec_result = step->action.exec.function();
    ATL_DEBUG("[ATL][INFO] Execution '%s': %s", step->name, exec_result ? "true" : "false");
    const char *target = exec_result ? step->action.exec.true_target : step->action.exec.false_target;  // Jump based on exec result
    if(!atl_chain_execute_step_jump(chain, target)) 
    {
      chain->is_running = false;
      return false;
    }
  } 
  else 
  {
    ATL_DEBUG("[ATL][INFO] Execution '%s' has no function", step->name);
    chain->is_running = false;
    return false;
  }  
  return true;
}

/** 
 * @brief Chain step loop start proc
 */
static bool atl_chain_step_loop_start_proc(atl_chain_t* chain, chain_step_t* const step)
{
  if(chain->loop_stack_ptr < chain->loop_stack_size) // Start of loop - push current position to stack
  {
    if(step->state == ATL_CHAIN_STEP_IDLE) 
    {
      chain->loop_stack[chain->loop_stack_ptr].start_step_index = chain->current_step;
      chain->loop_stack[chain->loop_stack_ptr].iteration_count = 0;
      chain->loop_stack_ptr++;
      step->state = ATL_CHAIN_STEP_SUCCESS;
      ATL_DEBUG("[ATL][INFO] Loop start, iterations: %u", step->action.loop_count);
    }
    chain->current_step++;
  } 
  else 
  {
    ATL_DEBUG("[ATL][ERROR] Loop stack overflow!", NULL);
    chain->is_running = false;
    return false;
  }
  return true;
}

/** 
 * @brief Chain step loop end proc
 */
static bool atl_chain_step_loop_end_proc(atl_chain_t* chain, chain_step_t* const step)
{
  (void)step;
  if(chain->loop_stack_ptr > 0) // End of loop - check if we should continue looping
  {
    atl_loop_stack_item_t *current_loop = &chain->loop_stack[chain->loop_stack_ptr - 1];
    chain_step_t *loop_start = &chain->steps[current_loop->start_step_index];
    current_loop->iteration_count++;

    if(loop_start->action.loop_count == 0 || current_loop->iteration_count < loop_start->action.loop_count) // 0 = infinite loop, otherwise check iteration count
    {
      chain->current_step = current_loop->start_step_index;
      ATL_DEBUG("[ATL][INFO] Loop iteration %u", current_loop->iteration_count);
    } 
    else 
    {
      chain->loop_stack_ptr--;
      loop_start->state = ATL_CHAIN_STEP_IDLE;
      current_loop->iteration_count = 0;
      chain->current_step++;
      ATL_DEBUG("[ATL][INFO] Loop completed after %u iterations", current_loop->iteration_count);
    }
  }
  else 
  {
    ATL_DEBUG("[ATL][ERROR] Loop end without start!", NULL);
    chain->is_running = false;
    return false;
  }
  return true;
}

/** 
 * @brief Chain step delay proc
 */
static bool atl_chain_step_delay_proc(atl_chain_t* chain, chain_step_t* const step)
{
  #ifndef ATL_TEST
  if(!step->action.delay.start) 
  {
    ATL_DEBUG("[ATL][INFO] Chain step delay %d ms", step->action.delay.value);
    ATL_DEBUG("[ATL][INFO] Wait....", NULL);
    step->action.delay.start = atl_get_cur_time();
  }
  if(atl_get_cur_time() >= (step->action.delay.start + step->action.delay.value)) 
  #endif
  {
    step->state = ATL_CHAIN_STEP_IDLE;
    step->action.delay.start = 0;
    chain->current_step++;
  }
  return true;
}

/*******************************************************************************
 ** @brief Create a new chain with copied steps (heap allocated)
 ** @param name       Chain name
 ** @param steps      Array of steps (will be copied)
 ** @param step_count Number of steps
 ** @param cb         User callback for chain completion
 ** @return Pointer to created chain, NULL on error
 *******************************************************************************/
atl_chain_t* atl_chain_create(const char* const name, const chain_step_t* const steps, const uint32_t step_count) 
{
  DBC_REQUIRE(601, atl_get_init().init);
  DBC_REQUIRE(602, name);
  DBC_REQUIRE(603, steps);
  DBC_REQUIRE(604, step_count > 0);
    
  // Calculate required loop stack size based on actual loop nesting
  uint32_t required_stack_size = atl_chain_cal_max_loop_depth(steps, step_count);
  
  // Allocate memory for chain structure
  atl_chain_t *chain = (atl_chain_t*)atl_malloc(sizeof(atl_chain_t));
  if(!chain) 
  {
    return NULL;
  }
  
  // Allocate memory for step copy (ensures steps survive function return)
  chain_step_t *steps_copy = (chain_step_t*)atl_malloc(step_count * sizeof(chain_step_t));
  if(!steps_copy) 
  {
    atl_deinit();
    return NULL;
  }
  
  // Copy steps to heap
  memcpy(steps_copy, steps, step_count * sizeof(chain_step_t));
  
  // Initialize chain structure
  memset(chain, 0, sizeof(atl_chain_t));
  chain->name = name;
  chain->steps = steps_copy;
  chain->step_count = step_count;
  chain->loop_stack_size = required_stack_size; // Dynamic size based on actual needs
  
  // Allocate loop stack with calculated size
  chain->loop_stack = (atl_loop_stack_item_t*)atl_malloc(chain->loop_stack_size * sizeof(atl_loop_stack_item_t));  
  if(!chain->loop_stack) 
  {
    atl_deinit();
    return NULL;
  }

  memset(chain->loop_stack, 0, chain->loop_stack_size * sizeof(atl_loop_stack_item_t));

  ATL_DEBUG("[ATL][INFO] Created chain '%s' with %u steps, loop stack: %u, memory used: %d/%d", 
             name, step_count, required_stack_size, o1heapGetDiagnostics(atl_get_init().heap).allocated, o1heapGetDiagnostics(atl_get_init().heap).capacity);
  return chain;
}


/*******************************************************************************
 ** @brief  Destroy chain and all resources
 ** @param  chain Chain to destroy
 ** @retval none
 *******************************************************************************/
void atl_chain_destroy(atl_chain_t* const chain) 
{
  DBC_REQUIRE(701, chain);
  DBC_REQUIRE(702, atl_get_init().init);
  if(chain->steps) atl_free(chain->steps);
  if(chain->loop_stack) atl_free(chain->loop_stack);
  atl_free(chain);
  ATL_DEBUG("[ATL][INFO] Chain destroyed, memory used: %d/%d", 
             o1heapGetDiagnostics(atl_get_init().heap).allocated, o1heapGetDiagnostics(atl_get_init().heap).capacity);
}

/*******************************************************************************
 ** @brief Start chain execution
 ** @param chain Chain to start
 ** @return true if started successfully, false otherwise
 *******************************************************************************/
bool atl_chain_start(atl_chain_t* const chain) 
{
  DBC_REQUIRE(801, chain);
  atl_chain_reset_state(chain);
  chain->is_running = true;
  ATL_DEBUG("[ATL][INFO] Chain '%s' started", chain->name);
  return true;
}

/*******************************************************************************
 ** @brief Stop chain execution
 ** @param chain Chain to stop
 ** @retval none
 *******************************************************************************/
void atl_chain_stop(atl_chain_t* const chain) 
{
  DBC_REQUIRE(1001, chain);
  chain->is_running = false;
  ATL_DEBUG("[ATL][INFO] Chain '%s' stopped", chain->name);
}

/*******************************************************************************
 ** @brief Reset chain state (steps, counters, etc.)
 ** @param chain Chain to reset
 ** @retval none
 *******************************************************************************/
void atl_chain_reset(atl_chain_t* const chain) 
{
  DBC_REQUIRE(1101, chain);
  atl_chain_reset_state(chain);
  ATL_DEBUG("[ATL][INFO] Chain '%s' reset", chain->name);
}

/*******************************************************************************
 ** @brief Execute one step of the chain (non-blocking)
 ** @param chain Chain to execute
 ** @return true if chain should continue, false if completed or error
 *******************************************************************************/
bool atl_chain_run(atl_chain_t* const chain) 
{
  DBC_REQUIRE(1201, chain);
  if(!chain->is_running) return false;    
  bool res = atl_chain_process_step(chain);
  return res;
}

/*******************************************************************************
 ** @brief Check if chain is currently running
 ** @param chain Chain to check
 ** @return true if running, false otherwise
 *******************************************************************************/
bool atl_chain_is_running(const atl_chain_t* const chain) 
{
  DBC_REQUIRE(1301, chain);
  bool res = chain->is_running;
  return res;
}

/*******************************************************************************
 ** @brief Get current step index
 ** @param chain Chain
 ** @return Current step index
 *******************************************************************************/
uint32_t atl_chain_get_current_step(const atl_chain_t* const chain) 
{
  DBC_REQUIRE(1401, chain);
  uint32_t res = chain->current_step;
  return res;
}

/*******************************************************************************
 ** @brief Get current step name
 ** @param chain Chain
 ** @return Current step name or NULL if not available
 *******************************************************************************/
const char* atl_chain_get_current_step_name(const atl_chain_t* const chain) 
{
  DBC_REQUIRE(1402, chain);
  DBC_REQUIRE(1403, chain->current_step < chain->step_count);
  const char* res = chain->steps[chain->current_step].name;
  return res;
}
