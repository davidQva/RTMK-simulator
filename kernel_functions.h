/*********************************************************/
/** Global variables and definitions                     */
/** Modified  on 27th Feb 2019                           */
/*********************************************************/

#include <stdlib.h>  /* for using the functions calloc, free */
#include <string.h>  /* for using the function memcpy        */
#include <limits.h>  /* for using the constant UINT_MAX      */

#define CONTEXT_SIZE    8   /*  for the 8 registers: r4 to r11   */ 
#define STACK_SIZE      100 /*  about enough space for the stack */

#define TRUE    1
#define FALSE   !TRUE

#define RUNNING 1
#define INIT    !RUNNING

#define FAIL    0
#define SUCCESS 1
#define OK              1

#define DEADLINE_REACHED        0
#define NOT_EMPTY               0

#define SENDER          +1
#define RECEIVER        -1


typedef int             exception;
typedef int             bool;
typedef unsigned int    uint;
typedef int 		action;

struct  l_obj;         // Forward declaration

// Task Control Block, TCB.  Modified on 24/02/2019
typedef struct
{      
        uint    *SP;
        uint    R4toR11[CONTEXT_SIZE];
        void    (*PC)();
        uint    SPSR;     
        uint    StackSeg[STACK_SIZE];
        uint    Deadline;
} TCB;


// Message items(meddelande)
typedef struct msgobj {
        char            *pData; // meddelande
        exception       Status;
        struct l_obj    *pBlock; // pointer till task adress
        struct msgobj   *pPrevious;
        struct msgobj   *pNext;
} msg;

// Mailbox structure
typedef struct {
        msg             *pHead;
        msg             *pTail;
        int             nDataSize; // datatyp storlek ex int,char
        int             nMaxMessages; // max antal meddelande
        int             nMessages;  // nuvarande antal meddelande, skickad med send_wait
        int             nBlockedMsg; // antal blockerade meddelande, ski med receive_wait
} mailbox;


// Generic list item (Task)
typedef struct l_obj {
         TCB            *pTask; // pointer till task (tcb)
         uint           nTCnt;
         msg            *pMessage; // pointer till meddelande
         struct l_obj   *pPrevious;
         struct l_obj   *pNext;
} listobj;


// Generic list
typedef struct _list {
         listobj        *pHead;
         listobj        *pTail;
} list;




// Function prototypes
extern int Ticks; /* global sysTick counter */
extern int KernelMode; /* can be equal to either INIT or RUNNING (constants defined 3 * in “kernel_functions.h”)*/
extern list *ReadyList, *WaitingList, *TimerList;
extern TCB *PreviousTask, *NextTask; /* Pointers to previous and next running tasks */

// Task administration
exception       init_kernel(void);
exception	create_task( void (* task_body)(), uint deadline);
void            terminate( void );
void            run( void );
exception insert( list* in_list, listobj* thisObj);
listobj* create_lobj(TCB* newtask);
listobj* extract_listhead(list* thislist);

// Communication
mailbox*	create_mailbox( uint nMessages, uint nDataSize );
int             no_messages( mailbox* mBox );

exception       send_wait( mailbox* mBox, void* pData );
exception       receive_wait( mailbox* mBox, void* pData );

exception	send_no_wait( mailbox* mBox, void* pData );
exception       deadline_status(mailbox* mBox, void* pData);
int             receive_no_wait( mailbox* mBox, void* pData );

//Extra
bool emptyList(list* thislist);
listobj* timeup_list_search(list* in_list);


// Timing
exception	wait( uint nTicks );
void            set_ticks( uint nTicks );
uint            ticks( void );
uint		deadline( void );
void            set_deadline( uint deadline );
void            TimerInt ();

//Interrupt and context switch
extern void     isr_off(void);
extern void     isr_on(void);

extern void     SwitchContext( void );	
                   /* Stores stack frame in stack of currently running task, and the
                    * remaining registers in its TCB
                    * Loads stack frame from stack of RunningTask, and the
                    * remaining registers from its TCB
                    */
                                        
extern void     LoadContext_In_Run( void );
                   /* To be used on the last line of the C function run() */

extern void     switch_to_stack_of_next_task( void );
                   /* To be used inside the C function terminate() */
                     
extern void     LoadContext_In_Terminate( void );
                   /* To be used on the last line of the C function terminate() */



