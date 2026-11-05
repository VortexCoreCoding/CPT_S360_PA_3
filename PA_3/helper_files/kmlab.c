#define LINUX

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> // for copy_from_user
#include <linux/list.h>
#include "kmlab_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Spinali"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 PA_3 - KM");

#define DEBUG 1
#define MAX_PROCFS_SIZE 1024

// store per process data (you may modify it as needed)
typedef struct proc_node {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_node;

static proc_node *process_list;
static struct list_head process_head;

static struct proc_dir_entry *kmlab_dir;
static struct proc_dir_entry *status_dir; 

// The buffer used to store characters for proc file r/w
static char procfs_buffer[MAX_PROCFS_SIZE];

// The current size of the buffer (len of the str in buffer)
static unsigned long procfs_buffer_size = 0;

// from linux kernel module tutorial book
// reads data from procfile
static ssize_t procfile_read(struct file *file, char __user *usr_buffer,
                             size_t read_size, loff_t *offset) {
   if (*offset >= procfs_buffer_size) {
      return 0; // EOF symbol
   }
   if (*offset + read_size >= procfs_buffer_size) {
      read_size = procfs_buffer_size - *offset;
   }
   if (copy_to_user(usr_buffer, procfs_buffer + *offset, read_size)) {
      return -EFAULT; // bad address error
   }
   *offset += read_size; // update offset counter (current place in file)

   return read_size;
}

// from linux kernel module tutorial book
// overwrites current data in file
static ssize_t procfile_write(struct file *file, const char __user *usr_buffer,
                              size_t write_size, loff_t *offset) {
   if (*offset >= procfs_buffer_size) {
      return -EFBIG; // files is too large error code
   }
   if (*offset + write_size >= MAX_PROCFS_SIZE) {
      write_size = MAX_PROCFS_SIZE - *offset;
   }
   if (copy_from_user(procfs_buffer + *offset, usr_buffer, write_size)) {
      return -EFAULT;
   }
   *offset += write_size;

   return write_size;
}

static const struct proc_ops status_access_ops = {
   .proc_read = procfile_read,
   .proc_write = procfile_write,
};

// zero allocates space for the new node (erroring if not enough mem) and then instantiates it with the pid and adds it to the list
static int add_process(unsigned int pid) {
   proc_node *new_node = (proc_node*)kzalloc(sizeof(proc_node));
   if (!new_node) {
      return -ENOMEM;
   }
   new_node->cpu_time = 0;
   new_node->pid = pid;
   list_add(&new_node->list, &process_head);
   print_list(&process_head);
   return 0;
}

static void print_list(struct list_head* p_head) {
   struct list_head* p_cur;
   struct proc_node* e;
   list_for_each(p_cur, p_head) {
      e = list_entry(p_cur, struct proc_node, list);
      kprint("PID: %u-> CPU Time: %u\n", e->pid, e->cpu_time);
   }
}

static int remove_process(struct list_head* p_head, unsigned int pid) {
   if (!list_empty(&process_list->list)) {
      struct list_head* p_cur;
      struct proc_node* e;
      list_for_each(p_cur, p_head) {
         e = list_entry(p_cur, struct proc_node, list);
         if (e->pid == pid) break;
      }
      if (e->pid == pid) {
         list_del(p_cur);
         kfree(p_cur);
      }
   }
}

// kmlab_init - Called when the module is loaded
int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif

   // printk("Hello World! Module loaded successfully");
   kmlab_dir = proc_mkdir("kmlab", NULL);
   if (!kmlab_dir) {
      return -ENOMEM;
   }
   status_dir = proc_create("status", 0666, kmlab_dir, &status_access_ops);
   if (!status_dir) {
      return -ENOMEM;
   }
   process_list = (proc_node*)kmalloc(sizeof(proc_node));
   if (!process_list) {
      return -ENOMEM;
   }
   else {
      INIT_LIST_HEAD(&process_head);
   }

   pr_info("KMLAB MODULE LOADED\n");
   return 0;   
}

// kmlab_exit - Called when the module is unloaded
void __exit kmlab_exit(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif

   // printk("Module unloaded successfully");
   if (!status_dir) {
      proc_remove(status_dir);
   }
   if (!kmlab_dir) {
      proc_remove(kmlab_dir);
   }



   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);
