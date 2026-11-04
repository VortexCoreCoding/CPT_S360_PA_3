#define LINUX

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include "kmlab_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Spinali"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 PA_3 - KM");

#define DEBUG 1

// store per process data (you may modify it as needed)
typedef struct {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_list;

// Global variables as needed ...
struct proc_dir_entry *kmlab_dir;
struct proc_dir_entry *status_dir;

static ssize_t read_status(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset) {
   return 1;
}

static const struct proc_ops status_access_ops = {
   .proc_read = read_status,
};

// kmlab_init - Called when the module is loaded
int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif
   // Insert your code here ...
   // printk("Hello World! Module loaded successfully");
   kmlab_dir = proc_mkdir("kmlab", NULL);
   if (!kmlab_dir) {
    return -ENOMEM;
   }
   status_dir = proc_create("status", 0666, kmlab_dir, &status_access_ops); // TODO: make custom proc ops
   
   pr_info("KMLAB MODULE LOADED\n");
   return 0;   
}

// kmlab_exit - Called when the module is unloaded
void __exit kmlab_exit(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif
   // Insert your code here ...
   // printk("Module unloaded successfully");
   

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);
