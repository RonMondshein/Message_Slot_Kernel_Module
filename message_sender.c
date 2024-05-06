#include "message_slot.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h> /*For file control options (e.g., O_RDONLY, O_WRONLY)*/     
#include <unistd.h> /*For POSIX operating system API (e.g., read, write, close)*/
#include <sys/ioctl.h> /*For I/O control device operations*/


/*A user space program to send a message*/
int main(int argc, char const *argv[])
{
    int file;
    const char *message_slot_file_path;/* Declare message_slot_file_path as a const pointer */
    unsigned int message_channel_ID;
    const char *message; /* Declare message as a const pointer */
    int dev_ioctl_ret;
    ssize_t message_length;
    ssize_t written_ret;
    /*Recive 3 command line arguments*/
    if (argc != 4)
    {
        /*Recived number of line arguments that diffrent from 3- an error*/
        fprintf(stderr, "Program gets 3 command line arguments\n");
        exit(1);
    }

    /*Get the line arguments*/
    message_slot_file_path = argv[1]; /*The path to the message slot device file*/
    message_channel_ID = strtol(argv[2], NULL, 10); /*The target message channel ID. Assume a non - negative int*/
    message = argv[3]; /*The messade to pass*/

    /*Open the specified slot device file*/
    file = open(message_slot_file_path, O_WRONLY);
    if (file < 0)
    {
        /*Couldn't open the file*/
        perror("Error");
        exit(1);
    }

    /*Non- zero value means an error*/
    dev_ioctl_ret = -1;
    /*Allows the user to send direct control commands to the device from the user space, return 0 on success*/
    dev_ioctl_ret = ioctl(file, MSG_SLOT_CHANNEL, message_channel_ID); /*Set the channel ID to message_channel_ID*/
    if (dev_ioctl_ret != 0)
    {
        /*Non- zero value was returned- an error is occured*/
        perror("Error");
        exit(1);
    }

    /*Main part- write to the specified message to the message slot file,
     without the terminal null char of the C string as part of the message*/
    written_ret = -1;
    /*message size*/
    message_length = strlen(message);
    /*Write data to a file*/
    written_ret = write(file, message, message_length);
    if (written_ret == -1)
    {
        /*An error occured while writing to the file*/
        perror("Error");
        exit(1);
    }
    if (written_ret != message_length){
        /*The number of bytes written is diffrent from the number of bytes in the message*/
        fprintf(stderr, "The number of bytes written is diffrent from the number of bytes in the message\n");
        exit(1);
    }

    /*Close the file*/
    close(file);
    exit(0);
}
