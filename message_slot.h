#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h> /* For I/O control device operations */
#define MAJOR_NUMBER 235
#define MAX_NUMBER_OF_MINOR_DEVICES 256
#define MAX_ZISE_BUFFER 128 /*Max size of buffer for message as mention in the assignment*/
#define DEVICE_FILE_NAME "message_slot"
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned int) /* Define ioctl command */
#define SUCCESS 0

#endif
