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
#include "atl_chain.h"
#include "dbc_assert.h"
#include "tlsf.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
DBC_MODULE_NAME("ATL_CHAIN");

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
 ** @brief  Notify chain about step completion
 **         This function should be called from AT command callbacks
 ** @param  none
 ** @return none
 ******************************************************************************/
static void atl_chain_step_cb(const bool result, const void* const ctx, const void* const data) 
{
  DBC_REQUIRE(101, ctx);
  const atl_chain_t *chain = (atl_chain_t*)ctx;
  chain_step_t* const step = &chain->steps[chain->current_step];
  if(step->state == ATL_CHAIN_STEP_RUNNING) 
  {
    step->state = result ? ATL_CHAIN_STEP_SUCCESS : ATL_CHAIN_STEP_ERROR;
    step->execution_count++;
    ATL_DEBUG("[INFO] Step '%s' completed with %s\n", step->name, result ? "SUCCESS" : "ERROR");
    if(result) step->was_executed_successfully = true;
    if(step->action.func.cb) step->action.func.cb(result, ctx, data);
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
  ATL_DEBUG("[ERROR] Step '%s' not found!\n", step_name);
  return UINT32_MAX;
}

/*******************************************************************************
 ** @brief  Execute jump to target step
 ** @param  none
 ** @retval none
 *******************************************************************************/
static bool atl_chain_execute_step_jump(atl_chain_t* const chain, const char* const target_name) 
{
  if(!target_name) // NULL target means go to next step
  {
    chain->current_step++;
    return true;
  }
  uint32_t target_index = find_step_index_by_name(chain, target_name); // Find target step by name
  if (target_index != UINT32_MAX) 
  {
    chain->current_step = target_index;
    return true;
  }
  return false;
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
  chain->loop_counter = 0;
  chain->loop_stack_ptr = 0;
  for(uint32_t i = 0; i < chain->step_count; i++) //Reset all steps to initial state
  {
    chain->steps[i].state = ATL_CHAIN_STEP_IDLE;
    chain->steps[i].execution_count = 0;
    chain->steps[i].was_executed_successfully = false;
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

  if(chain->current_step >= chain->step_count) // Check if chain completed
  { 
    chain->is_running = false;
    ATL_DEBUG("[INFO] Chain '%s' completed\n", chain->name);
    return false;
  }
  chain_step_t *step = &chain->steps[chain->current_step];

  DBC_ASSERT(502, step);

  switch(step->type) 
  {
    case ATL_CHAIN_STEP_FUNCTION: 
    {
      switch(step->state) 
      {
            case ATL_CHAIN_STEP_IDLE: ATL_DEBUG("[INFO] Starting step '%s'\n", step->name); // Start executing the function
                                      step->state = ATL_CHAIN_STEP_RUNNING;
                                      if(!step->action.func.function(step->action.func.cb, step->action.func.param, step->action.func.ctx))
                                      {
                                        ATL_DEBUG("[ERROR] Failed to start step '%s'\n", step->name);
                                        step->state = ATL_CHAIN_STEP_ERROR;
                                        step->execution_count++;
                                      }
                                      break; 
            
         case ATL_CHAIN_STEP_RUNNING: // Waiting for callback - do nothing this cycle
                                      break;
            
         case ATL_CHAIN_STEP_SUCCESS: step->was_executed_successfully = true; // Function completed successfull
                                      if(!execute_step_jump(chain, step->action.func.success_target)) // Jump to success target
                                      {
                                        chain->is_running = false;
                                        return false;
                                      }
                                      break;
            
           case ATL_CHAIN_STEP_ERROR: ATL_DEBUG("[ERROR] Step '%s' failed (attempt %u/%u)\n", step->name, step->execution_count, step->action.func.max_retries);  // Function completed with error
                                      if(step->execution_count <= step->action.func.max_retries) // Check if we should retry
                                      { 
                                        step->state = ATL_CHAIN_STEP_IDLE;
                                        ATL_DEBUG("[INFO] Retrying '%s' after delay\n", step->name);
                                      } 
                                      else 
                                      {
                                        if(!execute_step_jump(chain, step->action.func.error_target)) 
                                        {
                                          chain->is_running = false;
                                          return false;
                                        }
                                      }
                                      break;
      }
      break;
    }

    case ATL_CHAIN_STEP_CONDITION: 
    {
      if(step->action.cond.condition) // Conditions are executed synchronously
      {
        bool condition_result = step->action.cond.condition();
        ATL_DEBUG("[INFO] Condition '%s': %s\n", step->name, condition_result ? "true" : "false");
        const char *target = condition_result ? step->action.cond.true_target : step->action.cond.false_target;  // Jump based on condition result
        if(!execute_step_jump(chain, target)) 
        {
          chain->is_running = false;
          return false;
        }
      } 
      else 
      {
        ATL_DEBUG("[INFO] Condition '%s' has no function\n", step->name);
        chain->is_running = false;
        return false;
      }
      break;
    }
          
    case ATL_CHAIN_STEP_LOOP_START: 
    {
      if(chain->loop_stack_ptr < chain->loop_stack_size) // Start of loop - push current position to stack
      {
        chain->loop_stack[chain->loop_stack_ptr] = chain->current_step;
        chain->loop_stack_ptr++;
        chain->loop_counter = 0;
        ATL_DEBUG("[INFO] Loop start, iterations: %u\n", step->action.loop_count);
        chain->current_step++;
      } 
      else 
      {
        ATL_DEBUG("[ERROR] Loop stack overflow!\n");
        chain->is_running = false;
        return false;
      }
      break;
    }
          
    case ATL_CHAIN_STEP_LOOP_END: 
    {
      if(chain->loop_stack_ptr > 0) // End of loop - check if we should continue looping
      {
        uint32_t loop_start_step = chain->loop_stack[chain->loop_stack_ptr - 1];
        chain_step_t *loop_start = &chain->steps[loop_start_step];
        chain->loop_counter++;
        if(loop_start->action.loop_count == 0 || chain->loop_counter < loop_start->action.loop_count) // 0 = infinite loop, otherwise check iteration count
        {
          chain->current_step = loop_start_step;
          ATL_DEBUG("[INFO] Loop iteration %u", chain->loop_counter);
        } 
        else 
        {
          chain->loop_stack_ptr--;
          ATL_DEBUG("[INFO] Loop completed after %u iterations\n", chain->loop_counter);
          chain->current_step++;
        }
      }
      else 
      {
        ATL_DEBUG("[ERROR] Loop end without start!\n");
        chain->is_running = false;
        return false;
      }
      break;
    }
          
    case ATL_CHAIN_STEP_DELAY: 
    {
      ATL_DEBUG("[INFO] Delay %u ms\n", step->action.delay.value - (atl_get_cur_time() -step->action.delay.start));
      if(!step->action.delay.start) step->action.delay.start = atl_get_cur_time();
      if(step->action.delay.start +atl_get_cur_time() >= step->action.delay.value) chain->current_step++;
      break;
    }
          
    default:
      ATL_DEBUG("[ERROR] Unknown step type: %d\n", step->type);
      chain->is_running = false;
      return false;
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
  uint32_t required_stack_size = calculate_max_loop_depth(steps, step_count);
  
  // Allocate memory for chain structure
  atl_chain_t *chain = (atl_chain_t*)tlsf_malloc(atl_get_init().atl_tlsf, sizeof(atl_chain_t));
  if(!chain) 
  {
    return NULL;
  }
  
  // Allocate memory for step copy (ensures steps survive function return)
  chain_step_t *steps_copy = (chain_step_t*)tlsf_malloc(atl_get_init().atl_tlsf, step_count * sizeof(chain_step_t));
  if(!steps_copy) 
  {
    tlsf_free(atl_get_init().atl_tlsf, (chain));
    return NULL;
  }
  
  // Copy steps to heap
  memcpy(steps_copy, steps, step_count * sizeof(chain_step_t));
  for(uint16_t i = 0; i < step_count; ++i) {
    if(steps_copy->type == ATL_CHAIN_STEP_FUNCTION)  steps_copy[i].action.func.ctx = chain;
  }
  
  // Initialize chain structure
  memset(chain, 0, sizeof(atl_chain_t));
  chain->name = name;
  chain->steps = steps_copy;
  chain->step_count = step_count;
  chain->loop_stack_size = required_stack_size; // Dynamic size based on actual needs
  
  // Allocate loop stack with calculated size
  chain->loop_stack = (uint32_t*)tlsf_malloc(atl_get_init().atl_tlsf, chain->loop_stack_size * sizeof(uint32_t));
  if(!chain->loop_stack) 
  {
    tlsf_free(atl_get_init().atl_tlsf, (chain->steps));
    tlsf_free(atl_get_init().atl_tlsf, (chain));
    return NULL;
  }
  
  ATL_DEBUG("[INFO] Created chain '%s' with %u steps, loop stack: %u\n", name, step_count, required_stack_size);
  return chain;
}


/*******************************************************************************
 ** @brief  Destroy chain and tlsf_free(atl_get_init().atl_tlsf,  all resources
 ** @param  chain Chain to destroy
 ** @retval none
 *******************************************************************************/
void atl_chain_destroy(const atl_chain_t* const chain) 
{
  DBC_REQUIRE(701, chain);
  DBC_REQUIRE(702, atl_get_init().init);
  if(chain->steps) tlsf_free(atl_get_init().atl_tlsf, chain->steps);
  if(chain->loop_stack) tlsf_free(atl_get_init().atl_tlsf, chain->loop_stack);
  tlsf_free(atl_get_init().atl_tlsf, chain);
  ATL_DEBUG("[INFO] Chain destroyed\n");
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
  ATL_DEBUG("[INFO]  Chain '%s' started\n", chain->name);
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
  ATL_DEBUG("[INFO] Chain '%s' stopped\n", chain->name);
}

/*******************************************************************************
 ** @brief Reset chain state (steps, counters, etc.)
 ** @param chain Chain to reset
 ** @retval none
 *******************************************************************************/
void atl_chain_reset(const atl_chain_t* const chain) 
{
  DBC_REQUIRE(1101, chain);
  atl_chain_reset_state(chain);
  ATL_DEBUG("[INFO] Chain '%s' reset\n", chain->name);
}

/*******************************************************************************
 ** @brief Execute one step of the chain (non-blocking)
 ** @param chain Chain to execute
 ** @return true if chain should continue, false if completed or error
 *******************************************************************************/
bool atl_chain_run(const atl_chain_t* const chain) 
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
inline bool atl_chain_is_running(const atl_chain_t* const chain) 
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
inline uint32_t atl_chain_get_current_step(const atl_chain_t* const chain) 
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
inline const char* atl_chain_get_current_step_name(const atl_chain_t* const chain) 
{
  DBC_REQUIRE(1401, chain);
  DBC_REQUIRE(1402, chain->current_step < chain->step_count);
  const char* res = chain->steps[chain->current_step].name;
  return res;
}

/*******************************************************************************
 ** @brief Print chain execution statistics
 ** @param chain Chain to print stats for
 ** @retval none
 *******************************************************************************/
void atl_chain_print_stats(const atl_chain_t* const chain) 
{
  DBC_REQUIRE(1501, chain);
  ATL_DEBUG("\n=== ATL Chain '%s' Statistics ===\n", chain->name);
  ATL_DEBUG("Current step: %u/%u (%s)\n", chain->current_step, chain->step_count, atl_chain_get_current_step_name(chain) ?: "unknown");
  ATL_DEBUG("Running: %s\n", chain->is_running ? "yes" : "no");
  ATL_DEBUG("Loop stack depth: %u\n", chain->loop_stack_ptr);
  for(uint32_t i = 0; i < chain->step_count; i++) 
  {
    chain_step_t *step = &chain->steps[i];
    if (step->type == ATL_CHAIN_STEP_FUNCTION) 
    {
      const char *state_str = "IDLE";
      switch (step->state) 
      {
        case ATL_CHAIN_STEP_RUNNING:    state_str = "RUNNING";    break;
        case ATL_CHAIN_STEP_SUCCESS:    state_str = "SUCCESS";    break;
        case ATL_CHAIN_STEP_ERROR:      state_str = "ERROR";      break;
        case ATL_CHAIN_STEP_RETRY_WAIT: state_str = "RETRY_WAIT"; break;
        default: break;
      }
      ATL_DEBUG(" %-20s: state=%-10s attempts=%u\n", step->name, state_str, step->execution_count);
    }
  }
  ATL_DEBUG("==================================\n\n");
}