/*******************************************************************************************
 *                    ╔══╗╔══╗╔╗──╔╗╔══╗╔══╗╔╗──╔╗───╔══╗╔════╗╔╗──          (c)26.05.2025 *
 *                    ║╔═╝╚╗╔╝║║──║║║╔═╝║╔╗║║║──║║───║╔╗║╚═╗╔═╝║║──              v1.0.0    *
 *                    ║╚═╗─║║─║╚╗╔╝║║║──║║║║║╚╗╔╝║───║╚╝║──║║──║║──                        *
 *                    ╚═╗║─║║─║╔╗╔╗║║║──║║║║║╔╗╔╗║───║╔╗║──║║──║║──                        *
 *                    ╔═╝║╔╝╚╗║║╚╝║║║╚═╗║╚╝║║║╚╝║║───║║║║──║║──║╚═╗                        *
 *                    ╚══╝╚══╝╚╝──╚╝╚══╝╚══╝╚╝──╚╝───╚╝╚╝──╚╝──╚══╝                        *  
 ******************************************************************************************/
 *  LIB CONTAINS:                                                                          *
 * - atl.c and atl.h files which contains main lib function files that contain             *
 *   the main functions of the library (check atl.h)                                       *
 * - sms, http, ftp, tcpip folders that contain functions for ready-made                   *
 *   implementations based on the main files mentioned above.                              *
 *   (you can create your own implementation, more about this below )                      *
 *                                                                                         *
 *  BEFORE TO USE:                                                                         *
 * 1. Place @atl_read() function in the place where data                                   *
 *    will be constantly collected from the modem through uart.                            *
 *    This function should be called and pass parameters only when the data                *
 *    buffer has received new data from the modem and deleted the old ones.                *
 *    Function works only with new data.                                                   *
 * 2. Place @atl_timer() function in the place where it will be called every 1MS.          *
 * 3. Use @atl_init() one single time for library initialization                           *
 *                                                                                         *
 *  HOW TO USE:                                                                            *
 * 1. If you want to send only one single AT CMD, use @atl_cmd_queue_append()              *
 *    and pass @atl_cmd_entity_t to this function.                                         *
 *    Callback should be look like default @atl_cmd_cb_def(). You can handle               *
 *    different types of @rsp_str_table in this callback depends on the at cmd.            *
 *    Callback should return @atl_cmd_rsp_t                                                *
 * 2. If you need to send a bunch of AT CMD sequentially, use @atl_cmd_queue_fun_append()  *
 *    and pass array of @atl_cmd_entity_t to this function.                                *
 *    Also you need to pass a global callback which will be called after                   *
 *    success execution of all of this cmd`s.                                              *
 * 3. If you need to to catch some URC you can use @atl_urc_queue_append()                 *
 *    and pass the @urc_entity_t to this function.                                         *
 * 4. If no more need to catch some of urc use @atl_urc_queue_remove()                     *
 *    and pass @urc_entity_t                                                               *
 *    !See examples in ready-made implementations.                                         *
 *                                                                                         *
 *  NOTE:                                                                                  *
 * - If you want to use ready-made implementations read about them below.                  *
 * - The library does not create a buffer for data, but uses the one provided              *
 *   by the user.                                                                          *
 * - Using atl_heap for creation of queue, max size is 50 for at cmd and 10 for urc            *
 * - You can send "DELAY<TIMEINMS>" like a AT CMD and it will be parse like                *
 *   a delay between two of other at cmd in queue. See examples.                           *
 * - You can do a step back or step forward when executing bunch of at cmd.                *
 *   Just use ATL_RSP_STEP+(-)your_step in @atl_cmd_cb_def or your custom CB. See examples *
 * - You can initializate @atl_urc_entity_queue_t and @atl_cmd_entity_queue_t              *
 *   before start the programm to proc some of urc or at cmd immediately after start       *
 *   the programm.                                                                         *
 *                                                                                         *
 *  READY-MADE IMPLEMENTATIONS:                                                            *
 *  Based on atl.c and atl.h some implementations were prepared and you can use them.      *
 *  Each implementation has its own folder and main header where you can find functions    *
 *  for this implementation.                                                               *
 *  List of implementation:                                                                *
 *    -sms                                                                                 *
 *    -http                                                                                *
 *    -ftp                                                                                 *
 *    -tcpip                                                                               *
 *  Some of implementation required some preparations, all this described inside of        *
 *  function descriptions for each implementation, read carefully!                         *
 *                                                                                         *
 *  FOR EXPERIENCED USERS (LIB DEEP DESCRIPTION):                                          *
 *  The lib itself works in such a way that we can add one AT command at a time or         *
 * a group at once to the queue. When we add one AT command for the first time, the queue  * 
 * is empty and it sees this in the add function and sends itself a user event where it    *
 * starts a delayed start timer for executing the command and adds it to the queue.        *
 * If there is already something in the queue, it simply adds it, but does not set the     *
 * timer because it is probably already working on another command in the queue. When      *
 * the timer counts down, it looks to see if there is a command in the queue, if there     *
 * is, it checks if it is a delay command or not, sets a delay if necessary, otherwise     *
 * it simply writes this command to the uart. Then it simply waits for a response and      *
 * when it arrives, the response handler first checks for new data on the uart, and then   *
 * for the currently executable command, a callback is called with the transfer of data    *
 * there. Based on the callback execution result, the handler performs actions: either     *
 * removes the command from the queue and starts a timer for the potential next one,       *
 * or waits for the end of execution, or skips, etc. That is, the command is always        *
 * executed in the timer handler, which is set when the command is added to an empty       *
 * queue or when the next command is finished executing. There is also a second timer      *
 * that is started when the next command starts executing and is disabled when it is       *
 * finished, but if it has counted to the end and the command has not been executed,       *
 * it reinitializes the entire queue. Removing a command from the queue occurs by          *
 * simply shifting the execution index of the current one forward. If we add a whole       *
 * group of at commands, they are all one single entity in the queue and until they        *
 * are all executed, they will not be deleted in case it is necessary to return to         *
 * one of the previous commands in the list. Only two such groups can be added at a time.  *
 ******************************************************************************************/