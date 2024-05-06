/*Access kernel-level code not usually available to userspace programs*/
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

/*
This code includes necessary libraries for the message_slot module:
*/
#include <linux/kernel.h> /*kernel.h for basic types and macros*/
#include <linux/module.h> /*module.h for module operations*/
#include <linux/fs.h> /*fs.h for file system operations*/
#include <linux/uaccess.h> /*uaccess.h for user-kernel space data transfer*/
#include <linux/string.h> /*string.h for string operations*/
#include <linux/errno.h> /*errno.h for system error numbers*/
#include <linux/slab.h> /*slab.h for memory allocation*/
#include "message_slot.h" /*message_slot.h for specific functionality of the message_slot module*/

MODULE_LICENSE("GPL"); /*GNU General Public License*/
/*A kernel implementing the messege slot IPC(inter process communication) mechanism*/

/*single message channel struct*/
typedef struct single_message_channel {
    unsigned int message_channel_ID; /*The message channel ID*/
    char message[MAX_ZISE_BUFFER]; /*The message that the user wants to send*/
    unsigned int message_length; /*The length of the message*/
    struct single_message_channel *next; /*Pointer to the next message channel*/
} single_message_channel;

/*message_slot struct, a character device file that contains multiple message channels active concurrently*/
typedef struct message_slot {
    single_message_channel *head; /*Pointer to the first message channel*/
} message_slot;

/*data_file struct that contains the  minor and the channel id*/
typedef struct data_file {
    unsigned int minor; /*The minor number of the device file*/
    unsigned int channel_id; /*The channel ID of the message slot*/
} data_file;


/*save the number of messages that the device drive has already created*/
static int count_individual_message_slot = 0;

/*create message_slot array that can keep up to MAX_NUMBER_OF_MINOR_DEVICES as was writen in the assignment*/
static struct message_slot* message_slot_array[MAX_NUMBER_OF_MINOR_DEVICES];

/*
DEVICE FUNCTIONS
*/
/*
 Opens a device file, initializing a new message slot if necessary and message channel.
 Inputs: inode: The device file's inode, file: The open file's structure.
 Output: Returns 0 on success, -ENOMEM on memory allocation failure.
 */
static int device_open(struct inode *inode, struct file *file)
{
    int found;
    int  i;
    int current_minor;
    data_file *current_data_file;
    single_message_channel *temp_single_message_channel;
    single_message_channel *new_single_message_channel;
    temp_single_message_channel  = NULL;
    found = 0;
    current_minor = iminor(inode);
    /*Check if a message with this minor message hasn't already open*/
    if (count_individual_message_slot == 0)
    {
        /*first message that the device driver has created*/
        for (i = 0; i < MAX_NUMBER_OF_MINOR_DEVICES; i++)
        {
            /*allocate place for the array*/
            message_slot_array[i] = (message_slot*)kmalloc(sizeof(struct message_slot), GFP_KERNEL);
            if (message_slot_array[i] == NULL)
            {
                printk(KERN_ERR "message_slot: Failed to allocate memory for the message_slot array\n");
                return -ENOMEM;
            }
            /*initialize the message_slot array*/
            message_slot_array[i]->head = NULL;
        }
    }
    /*Make new single message slot*/
    new_single_message_channel = (single_message_channel*)kmalloc(sizeof(struct single_message_channel), GFP_KERNEL);
    if (new_single_message_channel == NULL)
    {
        /*If allocate memory fialed, print an error and exit*/
        printk(KERN_ERR "message_slot: Failed to allocate memory for the single_message_channel\n");
        return -ENOMEM;
    }
    /*initialize the single message channel*/
    new_single_message_channel->message_channel_ID = 0;
    new_single_message_channel->message_length = 0;
    new_single_message_channel->next = NULL;

    /*Put the single message channel in it place according to the message slot minor*/
    if (message_slot_array[current_minor]->head == NULL)
    {
        message_slot_array[current_minor]->head = new_single_message_channel;
    }
    else
    {
        temp_single_message_channel = message_slot_array[current_minor]->head;
        /*put the new message last*/
        while (temp_single_message_channel->next != NULL)
        {
            /*find place for the new message*/
            temp_single_message_channel = temp_single_message_channel->next;
        }
        temp_single_message_channel->next = new_single_message_channel;
    }
    /*update the number of messages that the device drive has already created*/
    count_individual_message_slot++;

    /*save the minor and the channel id in data_file struct*/
    current_data_file = (data_file*)kmalloc(sizeof(struct data_file), GFP_KERNEL);
    if (current_data_file == NULL)
    {
        /*If allocate memory fialed, print an error and exit*/
        printk(KERN_ERR "message_slot: Failed to allocate memory for the data_file\n");
        return -ENOMEM;
    }
    current_data_file->minor = current_minor;
    current_data_file->channel_id = 0; /*Will be updated in the device_ioctl function*/
    file -> private_data = (void*)current_data_file; /*save the data_file struct in the file's private_data*/

    return SUCCESS;
}

/*
Releases the device and frees associated resources.
Inputs: node: The device file's inode, file: The open file's structure.
Output: Returns SUCCESS (0) after successful release.
*/
static int device_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    return SUCCESS;
}

/*
Reads the last message written on the channel into the user's buffer.
Returns the number of bytes read on success, or an error code on failure.
 */
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{ 
    /*message related structs*/  
    struct message_slot *current_message_slot;
    struct single_message_channel *current_single_message_channel;
    struct single_message_channel *temp_single_message_channel;

    /*for the file->private_data related info*/
    unsigned int file_channel_id;
    unsigned int file_minor;

    int  i;

    /*Check if the file's private_data is valid*/
    if (file->private_data == NULL)
    {
        /*The file's private_data is not valid*/
        return -EINVAL;
    }
    /*save the file->private_data related info*/
    file_channel_id = ((data_file*)(file->private_data))->channel_id;
    file_minor = ((data_file*)(file->private_data))->minor;

    if (file_channel_id == 0)
    {
        /*The channel id is not valid*/
        return -EINVAL;
    }
    if (buffer == NULL)
    {
        /*The buffer is not valid*/
        return -EINVAL;
    }
    
    /*save the current message slot by it minor*/
    current_message_slot = (message_slot*)(message_slot_array[file_minor]);

    if(current_message_slot == NULL)
    {
        /*The minor number is not valid*/
        return -EINVAL;
    }
    if (current_message_slot->head == NULL)
    {
        /*The message slot is empty*/
        return -EINVAL;
    }
    temp_single_message_channel = current_message_slot->head;
    current_single_message_channel = NULL;
    /*find the wanted message*/
    while (temp_single_message_channel != NULL)
    {
        if (temp_single_message_channel->message_channel_ID == file_channel_id)
        {
            current_single_message_channel = temp_single_message_channel;
            break;
        }
        temp_single_message_channel = temp_single_message_channel->next;
    }
    if (current_single_message_channel == NULL)
    {
        /*The message channel was not found*/
        return -EWOULDBLOCK;
    }
    if (current_single_message_channel->message_length == 0 || current_single_message_channel->message == NULL)
    {
        /*The message is empty*/
        return -EWOULDBLOCK;
    }
    if (length < current_single_message_channel->message_length)
    {
        /*The buffer is too short*/
        return -ENOSPC;
    }

    /*
    Main part-copy the message to the buffer from the kernel space to the user space
    */
   for (i = 0; i < current_single_message_channel->message_length; i++)
   {
         if (put_user(current_single_message_channel->message[i], &buffer[i]) < 0)
         {
              /*If put user fialed, print an error and exit*/
              return -ENOMEM;
         }
   }
   return current_single_message_channel->message_length;
}

/*
writes an non-empty message up to 128 bytes from the user's buffer to the channel.
Returns the number of bytes written on success, or an error code on failure.
 */

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    /*message related structs*/  
    struct single_message_channel *current_single_message_channel;
    struct single_message_channel *temp_single_message_channel;
    char* backup_message; /*save the previous message in case of an error*/  

    /*save the file->private_data related info*/
    unsigned int file_channel_id;
    unsigned int file_minor;
    int i;
    int is_head;
    is_head = 0;

    if (file->private_data == NULL || buffer == NULL)
    {
        return -EINVAL;
    }
    file_channel_id = ((data_file*)(file->private_data))->channel_id;
    file_minor = ((data_file*)(file->private_data))->minor;

    if (file_channel_id == 0)
    {
        return -EINVAL;
    }
    /*Check if we can get to the minor place in message_slot_array*/
    if(message_slot_array[file_minor] == NULL)
    {
        /*The minor number is not valid*/
        return -EINVAL;
    }
    /*save the head of the message_slot_array in position minor*/
    current_single_message_channel = message_slot_array[file_minor]->head;
    while (current_single_message_channel->next != NULL)
    {
        /*find the wanted message*/
        if (current_single_message_channel->message_channel_ID == file_channel_id)
        {
            /*The message channel was found!*/
            break;
        }
        current_single_message_channel = current_single_message_channel->next; /*move to the next single message channel*/
    }
    if (current_single_message_channel == NULL)
    {
        /*The message channel was not found*/
        return -EINVAL;
    }
    if (current_single_message_channel->message_channel_ID != file_channel_id) 
    {

        /*The message channel was not found so we make a new one*/
        temp_single_message_channel = (single_message_channel*)kmalloc(sizeof(struct single_message_channel), GFP_KERNEL);
        if (temp_single_message_channel == NULL)
        {
            /*If allocate memory fialed, print an error and exit*/
            return -ENOMEM;
        }
        /*initialize the single message channel*/
        temp_single_message_channel->next = NULL; /*The next single message channel is NULL*/
        temp_single_message_channel->message_length = 0; /*The message length is 0 for now, will be updated*/
        temp_single_message_channel->message_channel_ID = file_channel_id; /*update the message channel ID*/
        current_single_message_channel->next = temp_single_message_channel; /*put the new message last*/  
        current_single_message_channel = temp_single_message_channel; 
    }
    if (length > MAX_ZISE_BUFFER || length <= 0)
    {
        /*The message is too long or too short*/
        return -EMSGSIZE;
    }

    /*
    Main part- copy the message to the buffer from the user space to the kernel space
    */
   /*save the previous message in case of an error*/
    backup_message = (char*)kmalloc(length, GFP_KERNEL);
    if (backup_message == NULL)
    {
        /*If allocate memory fialed, print an error and exit*/
        return -ENOMEM;
    }
    /*copy the message to the backup message*/
    for(i = 0; i < length; ++i ) {
        if(get_user(backup_message[i], &buffer[i]) < 0)
        {
            /*If get user fialed, print an error and exit*/
            return -ENOMEM;
        }
    }

    /*copy the message to the single message channel*/
    memcpy(current_single_message_channel->message, backup_message, length); /*update the message*/
    kfree(backup_message); /*free the backup message*/
    current_single_message_channel->message_length = length; /*update the message length*/
    return length; /*return the length of the message*/
}

/*Takes a single unsighned int parameter that specifies non-zero channel id and sets the file descriptor's channel id to this value*/
static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param)
{
    if(ioctl_command_id != MSG_SLOT_CHANNEL)
    {
        /*ioctl command is not valid*/
        return -EINVAL;
    }
    if(ioctl_param == 0)
    {
        /*The channel id is not valid (supposed to be a non-zero number)*/
        return -EINVAL;
    }
    ((data_file*)(file->private_data))->channel_id = ioctl_param;
    return SUCCESS;
}


/*
DEVICE SETUP
*/
/*
This is the definition of the file_operations structure for the message_slot module.
 It specifies the functions that should be called for different operations on the device file:
*/
struct file_operations Fops = {
    .owner = THIS_MODULE, /*This sets the owner of the structure to the current module.*/
    .open = device_open, /*This function is called when the device file is opened.*/
    .release = device_release, /*This function is called when the device file is closed.*/
    .read = device_read, /*This function is called when data is read from the device file.*/
    .write = device_write, /*This function is called when data is written to the device file.*/
    .unlocked_ioctl = device_ioctl /*This function is called when the device file is open*/

};

/*Initialize the moudle - Register the device driver*/
static int __init message_slot_init(void)
{
    int result;
    result = -1;
    result = register_chrdev(MAJOR_NUMBER, DEVICE_FILE_NAME, &Fops);  /*Register the device driver*/

    if (result < 0)
    {
        /*Registration failed*/
        printk(KERN_ERR "message_slot: Failed to register the device driver\n");
        return result;
    }
    return SUCCESS;
}

/*Cleanup - unregister the device driver*/
static void __exit message_slot_cleanup(void)
{
    int i;
    single_message_channel *temp_single_message_channel;
    single_message_channel *next_single_message_channel;
    unregister_chrdev(MAJOR_NUMBER, DEVICE_FILE_NAME); /*Unregister the device driver*/

    /*free the message_slot_array*/
    for (i = 0; i < MAX_NUMBER_OF_MINOR_DEVICES; i++)
    {
        temp_single_message_channel = message_slot_array[i]->head;
        while (temp_single_message_channel != NULL)
        {
            /*free the single message channel*/
            next_single_message_channel = temp_single_message_channel->next; /*save the next single message channel*/
            kfree(temp_single_message_channel); /*free the single message channel*/
            temp_single_message_channel = next_single_message_channel; /*update the current single message channel*/
        }
        kfree(message_slot_array[i]); /*free the message_slot_array in place i*/ 
    }
    count_individual_message_slot = 0; /*update the number of messages that the device drive has already created to 0*/
}

/*Declare the init and cleanup functions*/
module_init(message_slot_init);
module_exit(message_slot_cleanup);
/*End of the module*/
