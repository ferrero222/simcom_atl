/*******************************************************************************
 *               ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──   (c)03.10.2025 *
 *               ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──       v1.0.0    *
 *               ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                 *
 *               ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                 *
 *               ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                 *
 *               ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                 *  
 ******************************************************************************/
#ifndef ATL_CHAIN_H
#define ATL_CHAIN_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
#define ATL_CHAIN(name, success_target, error_target, func, cb, param,  retries) \
    { ATL_CHAIN_STEP_FUNCTION, name, { func, cb, param, 0, success_target, error_target, retries }, ATL_CHAIN_STEP_IDLE, 0, 0, 0, false }

/**
 * @brief Create conditional step
 * @param name Step name
 * @param true_target Target step name when condition is true
 * @param false_target Target step name when condition is false
 * @param cond_func Condition check function
 */
#define ATL_CHAIN_COND(name, true_target, false_target, cond_func) \
    { ATL_CHAIN_STEP_CONDITION, name, .action.cond = { cond_func, true_target, false_target }, ATL_CHAIN_STEP_IDLE, 0, 0, 0, false }

/**
 * @brief Create loop start step
 * @param iterations Number of iterations (0 = infinite loop)
 */
#define ATL_CHAIN_LOOP_START(iterations) \
    { ATL_CHAIN_STEP_LOOP_START, "LOOP_START", .action.loop_count = iterations, ATL_CHAIN_STEP_IDLE, 0, 0, 0, false }

/**
 * @brief Create loop end step
 */
#define ATL_CHAIN_LOOP_END \
    { ATL_CHAIN_STEP_LOOP_END, "LOOP_END", .action.loop_count = 0, ATL_CHAIN_STEP_IDLE, 0, 0, 0, false }

/**
 * @brief Create delay step
 * @param ms Delay in milliseconds
 */
#define ATL_CHAIN_DELAY(ms) \
    { ATL_CHAIN_STEP_DELAY, "DELAY", .action.delay_ms = ms, ATL_CHAIN_STEP_IDLE, 0, 0, 0, false }

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/    
typedef enum {        
  ATL_CHAIN_STEP_IDLE,       // Step is not executing
  ATL_CHAIN_STEP_RUNNING,    // Function started, waiting for callback
  ATL_CHAIN_STEP_SUCCESS,    // Step completed successfully
  ATL_CHAIN_STEP_ERROR,      // Step completed with error
  ATL_CHAIN_STEP_RETRY_WAIT, // Waiting before retry attempt
} atl_step_exec_state_t;

typedef enum {              
  ATL_CHAIN_STEP_FUNCTION,   // Function call with transitions
  ATL_CHAIN_STEP_CONDITION,  // Conditional jump
  ATL_CHAIN_STEP_LOOP_START, // Loop start
  ATL_CHAIN_STEP_LOOP_END,   // Loop end
  ATL_CHAIN_STEP_DELAY,      // Delay
} atl_chain_step_type_t;

typedef struct {
  atl_chain_step_type_t type; // Step type
  const char *name;       // Step name for identification
  union 
  {
    struct {
      bool (*function)(atl_entity_cb_t cb, void* param, void* ctx); // Function to execute
      atl_entity_cb_t cb;             // Callback for function
      void* param;                    // Pass params to function
      void* ctx;                      // Context for function executing
      const char *success_target;     // Target step on success
      const char *error_target;       // Target step on error
      uint32_t max_retries;           // Maximum retry attempts
    } func;      
    struct {      
      bool (*condition)(void);        // Condition check function
      const char *true_target;        // Target step when true
      const char *false_target;       // Target step when false
    } cond; 
    struct {      
      uint32_t start;                 // Start moment
      uint32_t value;                 // Delay in milliseconds
    } delay;       
    uint32_t loop_count;              // Loop iterations (0 = infinite)
  } action;                    
  atl_step_exec_state_t state;   // Current execution state
  uint32_t execution_count;       // Number of execution attempts
  bool was_executed_successfully; // Flag if step ever succeeded
} chain_step_t;

typedef struct atl_chain_t {
  const char *name;         // Chain name
  chain_step_t *steps;      // Array of steps (heap allocated)
  uint32_t step_count;      // Number of steps
  uint32_t current_step;    // Current step index
  uint32_t loop_counter;    // Current loop iteration
  uint32_t *loop_stack;     // Loop stack for nested loops
  uint32_t loop_stack_ptr;  // Loop stack pointer
  uint32_t loop_stack_size; // Maximum loop stack size
  bool is_running;          // Chain execution flag
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
 ** @param cb         User callback for chain completion
 ** @return Pointer to created chain, NULL on error
 *******************************************************************************/
atl_chain_t* atl_chain_create(const char* const name, const chain_step_t* const steps, const uint32_t step_count);

/*******************************************************************************
 ** @brief  Destroy chain and tlsf_free(atl_get_init().atl_tlsf,  all resources
 ** @param  chain Chain to destroy
 ** @retval none
 *******************************************************************************/
void atl_chain_destroy(const atl_chain_t* const chain);

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
void atl_chain_reset(const atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Execute one step of the chain (non-blocking)
 ** @param chain Chain to execute
 ** @return true if chain should continue, false if completed or error
 *******************************************************************************/
bool atl_chain_run(const atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Check if chain is currently running
 ** @param chain Chain to check
 ** @return true if running, false otherwise
 *******************************************************************************/
inline bool atl_chain_is_running(const atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Get current step index
 ** @param chain Chain
 ** @return Current step index
 *******************************************************************************/
inline uint32_t atl_chain_get_current_step(const atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Get current step name
 ** @param chain Chain
 ** @return Current step name or NULL if not available
 *******************************************************************************/
inline const char* atl_chain_get_current_step_name(const atl_chain_t* const chain);

/*******************************************************************************
 ** @brief Print chain execution statistics
 ** @param chain Chain to print stats for
 ** @retval none
 *******************************************************************************/
void atl_chain_print_stats(const atl_chain_t* const chain);

#endif // ATL_CHAIN_H