/*******************************************************************************
 *                           ╔══╗╔════╗╔╗──                      (c)03.10.2025 *
 *                           ║╔╗║╚═╗╔═╝║║──                          v1.0.0    *
 *                           ║╚╝║──║║──║║──                                    *
 *                           ║╔╗║──║║──║║──                                    *
 *                           ║║║║──║║──║╚═╗                                    *
 *                           ╚╝╚╝──╚╝──╚══╝                                    *  
 ******************************************************************************/
#ifndef ATL_CHAIN_H
#define ATL_CHAIN_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "atl_core.h"

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @brief Create function step with success and error targets
 * @param name Step name
 * @param success_target Target step name on success (NULL for next step)
 * @param error_target Target step name on error
 * @param func Function to execute
 * @param retries Maximum retry attempts
 */
#define ATL_CHAIN(name_, success_target_, error_target_, func_, cb_, param_, meta_, retries_) \
{ \
  .type = ATL_CHAIN_STEP_FUNCTION, \
  .name = name_, \
  .action.func.function = func_, \
  .action.func.cb = cb_, \
  .action.func.param = param_, \
  .action.func.meta = meta_, \
  .action.func.success_target = success_target_, \
  .action.func.error_target = error_target_, \
  .action.func.max_retries = retries_, \
  .state = ATL_CHAIN_STEP_IDLE, \
  .execution_count = 0, \
}

/**
 * @brief Create exec step
 * @param name Step name
 * @param true_target Target step name when exec is true
 * @param false_target Target step name when exec is false
 * @param exec_func exec function
 */
#define ATL_CHAIN_EXEC(name_, true_target_, false_target_, exec_func_) \
{ \
  .type = ATL_CHAIN_STEP_EXEC, \
  .name = name_, \
  .action.exec.function = exec_func_, \
  .action.exec.true_target = true_target_, \
  .action.exec.false_target = false_target_, \
  .state = ATL_CHAIN_STEP_IDLE, \
  .execution_count = 0, \
}

/**
 * @brief Create loop start step
 * @param iterations Number of iterations (0 = infinite loop)
 */
#define ATL_CHAIN_LOOP_START(iterations_) \
{ \
  .type = ATL_CHAIN_STEP_LOOP_START, \
  .name = "LOOP_START", \
  .action.loop_count = iterations_, \
  .state = ATL_CHAIN_STEP_IDLE, \
  .execution_count = 0, \
}

/**
 * @brief Create loop end step
 */
#define ATL_CHAIN_LOOP_END \
{ \
  .type = ATL_CHAIN_STEP_LOOP_END, \
  .name = "LOOP_END", \
  .action.loop_count = 0, \
  .state = ATL_CHAIN_STEP_IDLE, \
  .execution_count = 0, \
}

/**
 * @brief Create delay step
 * @param ms Delay in milliseconds
 */
#define ATL_CHAIN_DELAY(ms_) \
{ \
  .type = ATL_CHAIN_STEP_DELAY, \
  .name = "DELAY", \
  .action.delay.start = 0, \
  .action.delay.value = ms_, \
  .state = ATL_CHAIN_STEP_IDLE, \
  .execution_count = 0, \
}

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/    
typedef uint8_t atl_step_exec_state_t;
enum {        
  ATL_CHAIN_STEP_IDLE,       // Step is not executing
  ATL_CHAIN_STEP_RUNNING,    // Function started, waiting for callback
  ATL_CHAIN_STEP_SUCCESS,    // Step completed successfully
  ATL_CHAIN_STEP_ERROR,      // Step completed with error
};

typedef uint8_t atl_chain_step_type_t;
enum {              
  ATL_CHAIN_STEP_FUNCTION,   // Function call with transitions
  ATL_CHAIN_STEP_EXEC,       // Exec jump
  ATL_CHAIN_STEP_LOOP_START, // Loop start
  ATL_CHAIN_STEP_LOOP_END,   // Loop end
  ATL_CHAIN_STEP_DELAY,      // Delay
};

typedef struct {
  uint32_t start_step_index;    // index of LOOP_START step
  uint32_t iteration_count;     // current loop iteration
} atl_loop_stack_item_t;

typedef struct {
  union
  {
    struct {
      bool (*function)(atl_context_t* const ctx, const atl_entity_cb_t cb, const void* const param, void* const meta); // Function to execute
      atl_entity_cb_t cb;          // Callback for function
      void* param;                 // Pass params to function
      void* meta;                  // meta for function executing
      const char *success_target;  // Target step on success
      const char *error_target;    // Target step on error
      uint8_t max_retries;         // Maximum retry attempts
    } func;   
    struct {   
      bool (*function)(void);      // exec function
      const char *true_target;     // Target step when true
      const char *false_target;    // Target step when false
    } exec;
    struct {   
      uint32_t start;              // Start moment
      uint32_t value;              // Delay in milliseconds
    } delay;    
    uint8_t loop_count;            // Loop iterations (0 = infinite)
  } action;      
  const char *name;                // Step name for identification
  atl_chain_step_type_t type;      // Step type           
  atl_step_exec_state_t state;     // Current execution state
  uint8_t execution_count;         // Number of execution attempts
} chain_step_t;

typedef struct atl_chain_t {
  const char *name;                   // Chain name
  chain_step_t *steps;                // Array of steps 
  uint32_t loop_stack_ptr;            // Loop stack pointer
  uint8_t step_count;                 // Number of steps
  uint8_t current_step;               // Current step index
  uint16_t loop_stack_size;           // Maximum loop stack size
  atl_loop_stack_item_t* loop_stack;  // Loop stack for nested loops
  atl_context_t* ctx;                 // Core context
  bool is_running;                    // Chain execution flag
} atl_chain_t;

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/*******************************************************************************
 ** @brief Create a new chain with copied steps (heap allocated)
 ** @param name       Chain name
 ** @param steps      Array of steps (will be copied)
 ** @param step_count Number of steps
 ** @param ctx        core context
 ** @return Pointer to created chain, NULL on error
 *******************************************************************************/
atl_chain_t* atl_chain_create(const char* const name, const chain_step_t* const steps, const uint32_t step_count, atl_context_t* const ctx);

/*******************************************************************************
 ** @brief  Destroy chain and all resources
 ** @param  chain Chain to destroy
 ** @retval none
 *******************************************************************************/
void atl_chain_destroy(atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Start chain execution
 ** @param chain Chain to start
 ** @return true if started successfully, false otherwise
 *******************************************************************************/
bool atl_chain_start(atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Stop chain execution
 ** @param chain Chain to stop
 ** @retval none
 *******************************************************************************/
void atl_chain_stop(atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Reset chain state (steps, counters, etc.)
 ** @param chain Chain to reset
 ** @retval none
 *******************************************************************************/
void atl_chain_reset(atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Execute one step of the chain (non-blocking)
 ** @param chain Chain to execute
 ** @return true if chain should continue, false if completed or error
 *******************************************************************************/
bool atl_chain_run(atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Check if chain is currently running
 ** @param chain Chain to check
 ** @return true if running, false otherwise
 *******************************************************************************/
bool atl_chain_is_running(const atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Get current step index
 ** @param chain Chain
 ** @return Current step index
 *******************************************************************************/
uint32_t atl_chain_get_current_step(const atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Get current step name
 ** @param chain Chain
 ** @return Current step name or NULL if not available
 *******************************************************************************/
const char* atl_chain_get_current_step_name(const atl_chain_t* const chain);

#endif // ATL_CHAIN_H