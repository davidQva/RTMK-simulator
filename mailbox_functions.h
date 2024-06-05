
msg* extract_mailbox_msg(mailbox* thislist);
exception insert_mailbox_msg(mailbox* in_list, msg* newMsg);
msg* create_msg(char* data);
exception remove_mailbox(mailbox* mBox);
mailbox* create_mailbox(uint nMessages, uint nDataSize);
listobj* extract_from_waitinglist(listobj* tmpObj);
exception insert_in_waiting(listobj* blockedTask);