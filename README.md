RTOS Mailbox Simulation
This program simulates a mailbox communication system in a Real-Time Operating System (RTOS) environment. It consists of four tasks that communicate with each other using three mailboxes: charMbox, intMbox, and floatMbox.

Tasks
task_body_1:

Sends and receives characters ('a', 'b', 'c') to/from the charMbox.
Sends an integer (100) to the intMbox.
Attempts to send a float (3.14159) to the floatMbox, which should fail due to a full mailbox.
task_body_2:

Receives and verifies the integer (100) from the intMbox.
Receives and verifies another integer (500) from the intMbox.
Waits for a global flag (g3) to be set, indicating the failed send operation in task_body_1.
task_body_3:

Sends an integer (500) to the intMbox.
task_body_4:

Waits for a specified time (900 time units).
Receives and verifies an integer (255) from the intMbox.
Mailbox Communication Primitives
The program tests the following mailbox communication primitives:

send_wait: Sends data to a mailbox and blocks until the mailbox is available.
send_no_wait: Sends data to a mailbox without blocking.
receive_wait: Receives data from a mailbox and blocks until data is available.
receive_no_wait: Receives data from a mailbox without blocking.
Test Cases
The program includes various test cases to verify the correctness of the mailbox communication primitives. These test cases cover scenarios such as:

Sending and receiving different data types (characters, integers, and floats).
Handling full mailboxes (by attempting to send data to a full mailbox).
Verifying the order of data received from the mailboxes.
Failure Indication
If any test case fails, the program enters an infinite loop (while(1) {}) to indicate the failure. Global variables (g1 and g3) are also used to track the success or failure of certain operations.

Usage
This program is intended to be used as a simulation or testing environment for RTOS mailbox communication functionality. It can be compiled and executed on a target system or simulated environment that supports the required RTOS or kernel.
