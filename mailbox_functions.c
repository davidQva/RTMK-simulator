#include "kernel_functions.h"
#include "mailbox_functions.h"

mailbox *create_mailbox(uint nMaxMessages, uint nDataSize)
{
  mailbox *newbox;
  newbox = (mailbox *)calloc(1, sizeof(mailbox));
  if (newbox == NULL)
  {
    return NULL;
  }
  newbox->pHead = NULL;
  newbox->pTail = NULL;

  newbox->nDataSize = nDataSize;
  newbox->nMaxMessages = nMaxMessages;

  return newbox;
}

exception remove_mailbox(mailbox *mBox)
{
  if (mBox->pHead == NULL && mBox->pTail == NULL)
  {
    free(mBox);
    return OK;
  }
  else
    return NOT_EMPTY;
}

//Inserts in waitinglist tail
exception insert_in_waiting(listobj *blockedTask)
{
  if (WaitingList->pHead == NULL)
  {
    WaitingList->pHead = blockedTask;
    WaitingList->pTail = blockedTask;
    return OK;
  }
  else
  {
    WaitingList->pTail->pNext = blockedTask;
    blockedTask->pPrevious = WaitingList->pTail;
    WaitingList->pTail = blockedTask;
    return OK;
  }
}

//This function exctracts the task from the waiting list head
listobj *extract_from_waitinglist(listobj *tmpObj)
{
  if (WaitingList->pHead == WaitingList->pTail)
  {
    tmpObj->pPrevious = NULL;
    tmpObj->pNext = NULL;
    WaitingList->pHead = NULL;
    WaitingList->pTail = NULL;
    return tmpObj;
  }
  if (tmpObj != NULL)
  {
    if (WaitingList->pHead == tmpObj)
    {
      listobj *remove_head;
      remove_head = WaitingList->pHead;
      WaitingList->pHead = WaitingList->pHead->pNext;
      WaitingList->pHead->pPrevious = NULL;
      return tmpObj;
    }
    tmpObj->pPrevious->pNext = tmpObj->pNext;
    tmpObj->pNext->pPrevious = tmpObj->pPrevious;
    tmpObj->pPrevious = NULL;
    tmpObj->pNext = NULL;
  }
  return tmpObj;
}

exception send_wait(mailbox *mBox, void *pData)
{
  msg *newMsg;
  isr_off();

  if (mBox->nBlockedMsg < 0) // if receiving task is waiting, copy data to msg and move task to readylist
  {
    memcpy(mBox->pHead->pData, pData, mBox->nDataSize);
    msg *tempHead = extract_mailbox_msg(mBox);
    PreviousTask = ReadyList->pHead->pTask;
    insert(ReadyList, extract_from_waitinglist(tempHead->pBlock));
    NextTask = ReadyList->pHead->pTask;
    free(tempHead->pData);
    free(tempHead);
  }
  else // No receiving task, creat msg and add to mailbox
  {
    newMsg = (msg *)calloc(1, sizeof(msg));
    newMsg->pData = (void *)calloc(1, sizeof(mBox->nDataSize));
    newMsg->pData = pData;
    newMsg->Status = SENDER;
    if (mBox->nMessages >= mBox->nMaxMessages)
    { // if mailbox is full, remove head and free
      msg *tempHead = extract_mailbox_msg(mBox);
      free(tempHead->pData);
      free(tempHead);
    }
    insert_mailbox_msg(mBox, newMsg);
    mBox->nBlockedMsg++;
    PreviousTask = ReadyList->pHead->pTask;
    insert_in_waiting(extract_listhead(ReadyList));
    newMsg->pBlock = WaitingList->pTail;
    NextTask = ReadyList->pHead->pTask;
  }

  SwitchContext(); // If Readylist is changed after insert, this is where process stops/starts
  uint dead = deadline();
  if (dead < Ticks)
  { // if deadline reach when pc is here, remove msg
    isr_off();
    mBox->nBlockedMsg--;
    msg *temp = extract_mailbox_msg(mBox);
    free(temp->pData);
    free(temp);
    isr_on();
    return DEADLINE_REACHED;
  }
  else
    return OK;
}

exception receive_wait(mailbox *mBox, void *pData)
{
  isr_off();
  msg *newMsg;

  if (mBox->nMessages > 0)
  { // if msg is waiting for reciver then copy data, then remove and free msg from mailbox
    memcpy(pData, mBox->pHead->pData, mBox->nDataSize);
    msg *tempHead = extract_mailbox_msg(mBox);

    if (tempHead->pBlock != NULL)
    { // if msg also linked to Task move task to readylist
      PreviousTask = ReadyList->pHead->pTask;
      insert(ReadyList, extract_from_waitinglist(tempHead->pBlock));
      NextTask = ReadyList->pHead->pTask;
      mBox->nBlockedMsg--;
      free(tempHead->pData);
      free(tempHead);
    }
    else
    {
      free(tempHead->pData);
      free(tempHead);
      return OK;
    }
  }
  else // No msg where waiting, create new msg and add to mailbox, move running Task to waitinglist and link to msg
  {
    newMsg = (msg *)calloc(1, sizeof(msg));
    newMsg->pData = pData;
    newMsg->Status = RECEIVER;
    mBox->nBlockedMsg--;
    if (mBox->nMessages >= mBox->nMaxMessages)
    { // If mailbox is full remove head and free
      msg *tempHead = extract_mailbox_msg(mBox);
      free(tempHead->pData);
      free(tempHead);
    }
    insert_mailbox_msg(mBox, newMsg);
    PreviousTask = ReadyList->pHead->pTask;
    insert_in_waiting(extract_listhead(ReadyList));
    newMsg->pBlock = WaitingList->pTail;
    NextTask = ReadyList->pHead->pTask;
  }
  SwitchContext(); // If Readylist is changed after insert, this is where process stops/starts

  if ((WaitingList->pHead != NULL) && WaitingList->pHead->pTask->Deadline < Ticks)
  { // If Deadline is reached remove Data, msg and set deadline to DEADLINE_REACHED
    newMsg->pPrevious->pNext = newMsg->pNext;
    newMsg->pNext->pPrevious = newMsg->pPrevious;
    mBox->nMessages--;
    mBox->nBlockedMsg--;
    isr_off();
    free(newMsg->pData);
    free(newMsg);
    isr_on();
    return DEADLINE_REACHED;
  }
  else
    return OK;
}

exception send_no_wait(mailbox *mBox, void *pData)
{
  isr_off();

  if (mBox->nMessages < 0 && (mBox->pHead->Status == RECEIVER))
  { // If msg is wating for Data, copy Data, if msg also is linked with task move task to readylist
    memcpy(mBox->pHead->pData, pData, mBox->nDataSize);
    msg *tempHead = extract_mailbox_msg(mBox);
    PreviousTask = ReadyList->pHead->pTask;
    insert(ReadyList, extract_from_waitinglist(tempHead->pBlock));
    mBox->nBlockedMsg++;
    NextTask = ReadyList->pHead->pTask;
    free(tempHead->pData);
    free(tempHead);
    SwitchContext();
  }
  else
  { // No msg's where waiting for Data create new msg, copy Data and put in mailbox
    msg *newMsg;
    newMsg = (msg *)calloc(1, sizeof(msg));
    if (newMsg == NULL)
      return FAIL;
    newMsg->pData = (void *)calloc(1, sizeof(mBox->nDataSize));
    if (newMsg->pData == NULL)
      return FAIL;

    memcpy(newMsg->pData, pData, mBox->nDataSize);

    if (mBox->nMessages >= mBox->nMaxMessages)
    { // if mailbox full remove head and free
      msg *tempHead = extract_mailbox_msg(mBox);
      free(tempHead->pData);
      free(tempHead);
    }
    insert_mailbox_msg(mBox, newMsg);
    mBox->pTail->Status = SENDER;
    isr_on();
  }
  return OK;
}

exception receive_no_wait(mailbox *mBox, void *pData)
{ 
  if (mBox->nMessages > 0 && (mBox->pHead->Status == SENDER))
  { // If there is a send msg waiting copy data from msg remove msg form mailbox and free
    isr_off();
    memcpy(pData, mBox->pHead->pData, mBox->nDataSize);
    msg *tempHead = extract_mailbox_msg(mBox);
    if (tempHead->pBlock != NULL) // is the message linked to a waiting task, then move to readylist
    {
      PreviousTask = ReadyList->pHead->pTask;
      insert(ReadyList, extract_from_waitinglist(tempHead->pBlock));
      mBox->nBlockedMsg--;
      NextTask = ReadyList->pHead->pTask; 
      free(tempHead->pData);
      free(tempHead);
      SwitchContext();// If Readylist is changed after insert, this is where process stops/starts
    }
    else{
    free(tempHead->pData);
    free(tempHead);
    isr_on();
    }
  }
  else //No message was waiting
  {
    return FAIL;
  }
  return OK;
}

// removes head from mailbox and returns it, and deincrement messages
msg *extract_mailbox_msg(mailbox *thislist)
{
  msg *remove_head;

  if (thislist->pHead == NULL) // If empty list
    return NULL;

  if (thislist->pHead == thislist->pTail)
  {
    remove_head = thislist->pHead;
    thislist->pHead = NULL;
    thislist->pTail = NULL;
    thislist->nMessages--;
    return remove_head;
  }

  remove_head = thislist->pHead;
  thislist->pHead = thislist->pHead->pNext;
  thislist->pHead->pPrevious = NULL;
  thislist->nMessages--;

  return remove_head;
}

// Insert msg in mailbox tail, and increment nMessages
exception insert_mailbox_msg(mailbox *in_list, msg *newMsg)
{
  if (in_list->nMessages == 0)
  {
    in_list->pHead = newMsg;
    in_list->pTail = newMsg;
    in_list->nMessages++;
    return OK;
  }
  else
  {
    in_list->pTail->pNext = newMsg;
    newMsg->pPrevious = in_list->pTail;
    in_list->pTail = newMsg;
    in_list->nMessages++;
  }

  return OK;
}
