#include "message_slot.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> /*For file control options (e.g., O_RDONLY, O_WRONLY)*/     
#include <unistd.h> /*For POSIX operating system API (e.g., read, write, close)*/
#include <sys/ioctl.h> /*For I/O control device operations*/


/*A user space program to send a message*/
int main(int argc, char const *argv[])
{
    int file;
    const char *message_slot_file_path;
    unsigned int message_channel_ID;
    int dev_ioctl_ret;
    ssize_t read_ret;
    char* read_buffer[MAX_ZISE_BUFFER];
    ssize_t written_ret;

    /*Recive 2 command line arguments*/
    if (argc != 3)
    {
        /*Recived number of line arguments that diffrent from 2- an error*/
        fprintf(stderr, "Program gets 2 command line arguments\n");
        exit(1);
    }

    /*Get the line arguments*/
    message_slot_file_path = argv[1]; /*The path to the message slot device file*/
    message_channel_ID = strtol(argv[2], NULL, 10); /*The target message channel ID. Assume a non - negative int*/

    /*Open the specified slot device file*/
    file = open(message_slot_file_path, O_RDONLY);
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

    /*Main part- read to the specified message to the message slot file,
     without the terminal null char of the C string as part of the message*/
    read_ret = -1;
    /*read data from a file up to 128 bytes*/
    read_ret = read(file, read_buffer, MAX_ZISE_BUFFER);
    if (read_ret == -1)
    {
        /*An error occured while writing to the file*/
        perror("Error");
        exit(1);
    }
    if (read_ret > MAX_ZISE_BUFFER){
        /*The number of bytes read is greater than the buffer size*/
        fprintf(stderr, "The number of bytes read is greater than the buffer size\n");
        exit(1);
    }
    /*Close the file*/
    close(file);
    written_ret = -1;
    /*Write data to the standart output*/
    written_ret = write(STDOUT_FILENO, read_buffer, read_ret);
    if (written_ret == -1)
    {
        /*An error occured while writing to the file*/
        perror("Error");
        exit(1);
    }
    if (written_ret != read_ret){
        /*The number of bytes written is diffrent from the number of bytes in the message*/
        fprintf(stderr, "The number of bytes written is diffrent from the number of bytes in the message\n");
        exit(1);
    }

    exit(0);
}
