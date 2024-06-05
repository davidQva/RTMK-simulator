#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"

int Ticks; /* global sysTick counter */
int KernelMode; /* can be equal to either INIT or RUNNING (constants defined 3 * in “kernel_functions.h”)*/
list *ReadyList, *WaitingList, *TimerList;
TCB *PreviousTask, *NextTask, *Runnning; /* Pointers to previous and next running tasks */

//insert listobj in specific list
exception insert(list* in_list, listobj* newObj)
{      
    if( in_list->pHead == in_list->pTail && (in_list->pHead != NULL))
    {              
        in_list->pHead->pPrevious = newObj;
        newObj->pNext = in_list->pHead;
        in_list->pHead = newObj;  
        Runnning = NextTask;
        return OK;
    }  
  
    if( in_list->pHead == NULL )
    {
            in_list->pHead = newObj;
            in_list->pTail = newObj; 
            Runnning = NextTask;
            return OK;
    }       
  
    listobj* curr = in_list->pTail;
    
    while(curr != in_list->pHead && (newObj->pTask->Deadline < curr->pPrevious->pTask->Deadline))
    {
        curr = curr->pPrevious;
    }
    if(curr == in_list->pHead) 
    {
        in_list->pHead->pPrevious = newObj;
        newObj->pNext = in_list->pHead;
        in_list->pHead = newObj; 
        Runnning = NextTask;
        return OK;
    }
    else 
    {
        newObj->pNext = curr;
        newObj->pPrevious = curr->pPrevious;
        curr->pPrevious->pNext = newObj;       
        curr->pPrevious = newObj;   
        Runnning = NextTask;
        return OK;
    }
}

//creates Ready,Wating,Timerlist
list* generic_list(void) {
  
    list* newList;    
    newList = (list*)malloc(sizeof(list));
    if(newList == NULL)
    {
      return NULL;
    }
    
    newList->pHead       = NULL;
    newList->pTail       = NULL;
    
    return newList;
}

//is list empty? return TRUE, NOT empty return FALSE
bool emptyList(list* thislist)
{
  if(thislist->pHead == NULL && thislist->pTail == NULL)
  {
    return TRUE;
  }
  return NULL;
}

listobj* create_lobj(TCB* newtask)
{
   listobj* new_obj;
   new_obj = (listobj *) calloc (1, sizeof(listobj)); 
    if(new_obj == NULL)
     return FAIL;
    
    new_obj->pTask = newtask;
    new_obj->pPrevious = NULL;
    new_obj->pNext = NULL;
          
  return new_obj;
}


//idle func, allways in readylist tail
void idle()
{
  while(1){}
}

exception create_task(void (*taskBody)(), unsigned int deadline)
{
  TCB *new_tcb;
  new_tcb = (TCB *) calloc (1, sizeof(TCB));
  /* you must check if calloc was successful or not! */
  if(new_tcb == NULL){return FAIL;}

  new_tcb->PC = taskBody;
  new_tcb->SPSR = 0x21000000;
  new_tcb->Deadline = deadline;
  
  new_tcb->StackSeg [STACK_SIZE - 2] = 0x21000000;
  new_tcb->StackSeg [STACK_SIZE - 3] = (unsigned int) taskBody;
  new_tcb->SP = &(new_tcb->StackSeg [STACK_SIZE - 9]);
  
  if(KernelMode == INIT)
  {
    insert(ReadyList, create_lobj(new_tcb));
    return OK;
  }
  else
  {
    isr_off();
    PreviousTask = ReadyList->pHead->pTask;
    insert(ReadyList, create_lobj(new_tcb));
    NextTask = ReadyList->pHead->pTask;
    if(PreviousTask!=NextTask)
    {
      SwitchContext();
    }    
    else isr_on();
  }
  return OK;
}


exception init_kernel(void)
{
  Ticks = 0;
  
  ReadyList = generic_list(); 
  WaitingList = generic_list();
  TimerList = generic_list();      
  if(ReadyList == NULL || WaitingList == NULL || TimerList == NULL)
   return FAIL;
  
  create_task(idle,UINT_MAX);
  KernelMode = INIT;
  
   return OK;
}

//removes head-listobj from specific list and returns it, if empty return NULL
listobj* extract_listhead(list* thislist)
{
  listobj* remove_head;  
  
  if(thislist->pHead == NULL) // If empty list        
   return NULL;
  
    if(thislist->pHead==thislist->pTail)
    {
       remove_head = thislist->pHead;
       thislist->pHead = NULL;
       thislist->pTail = NULL;
       return remove_head;   
    }  
  
  remove_head = thislist->pHead;
  thislist->pHead = thislist->pHead->pNext;
  thislist->pHead->pPrevious = NULL;
  remove_head->pPrevious = NULL;
  remove_head->pNext = NULL;
  
   return remove_head;
}


void terminate (void)
{
  isr_off();
  listobj* leavingObj;
  
  PreviousTask = ReadyList->pHead->pTask;
  leavingObj = extract_listhead(ReadyList);  
  
  NextTask = ReadyList->pHead->pTask;
  switch_to_stack_of_next_task();
    
  free(leavingObj->pTask);  
  free(leavingObj);
  LoadContext_In_Terminate();  
}

void run (void)
{
  Ticks = 0;  
  KernelMode = RUNNING;
  NextTask = ReadyList->pHead->pTask;   
  LoadContext_In_Run();  
}

exception wait( uint nTicks )
{
  isr_off();
  PreviousTask = ReadyList->pHead->pTask;
  listobj* timeouttask;
  timeouttask = extract_listhead(ReadyList);
  timeouttask->nTCnt = Ticks+nTicks;
  insert(TimerList, timeouttask);
  NextTask = ReadyList->pHead->pTask;
  SwitchContext();
  if(timeouttask->pTask->Deadline<nTicks)
  {
    return DEADLINE_REACHED;
  }
  return OK;
}

void set_ticks( uint nTicks )
{
  Ticks = nTicks;
}

uint ticks( void )
{
  return Ticks;
}

uint deadline( void )
{
  return ReadyList->pHead->pTask->Deadline;
}

//changes running task deadline and reschedule
void set_deadline( uint deadline )
{
    listobj* setNewDeadline;
    
    isr_off();
    
    PreviousTask = ReadyList->pHead->pTask;
    
    setNewDeadline = extract_listhead(ReadyList);
    setNewDeadline->pTask->Deadline = deadline;    
    insert(ReadyList, setNewDeadline);
    
    NextTask = ReadyList->pHead->pTask;
    
    if(PreviousTask!=NextTask)
    {
      SwitchContext();
    }    
    else isr_on();
}


void TimerInt ()
{
  Ticks++;
  listobj* timerTimeup = NULL;  
 
  timerTimeup = TimerList->pHead; 
  
  //checks if Timerlist has ready Task-> timeup or deadline reached
  if((timerTimeup->nTCnt<Ticks && (emptyList(TimerList) == NULL)) || 
     (timerTimeup->pTask->Deadline<Ticks && (emptyList(TimerList) == NULL)))
  {
    PreviousTask = ReadyList->pHead->pTask;
    insert(ReadyList, extract_listhead(TimerList));
    NextTask = ReadyList->pHead->pTask;
  }  

  listobj* waitingTimeup = WaitingList->pHead; 
  
  //checks if Task in Waitinglist, head in list, has deadline reached
  if(waitingTimeup->pTask->Deadline<Ticks && (emptyList(WaitingList) == NULL))
  {    
    PreviousTask = ReadyList->pHead->pTask;
    insert(ReadyList, extract_listhead(WaitingList));
    NextTask = ReadyList->pHead->pTask;
  }     
}

