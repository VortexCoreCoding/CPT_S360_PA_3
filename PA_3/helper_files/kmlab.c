#define LINUX

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h> // for copy_from_user
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
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

static LIST_HEAD(process_head);

static struct proc_dir_entry *kmlab_dir;
static struct proc_dir_entry *status_dir; 

// The buffer used to store characters for proc file r/w
static char procfs_buffer[MAX_PROCFS_SIZE];

// The current size of the buffer (len of the str in buffer)
static unsigned long procfs_buffer_size = 0;

// Timer used for wakeup function
static struct timer_list update_timer;

// Loops through the list of nodes and increases the cpu time by the cpu use calculator in kmlab_given.h
static void update_list_cpu_time(struct list_head* p_head) {
   struct list_head* p_cur;
   struct proc_node* e;
   list_for_each(p_cur, p_head) {
      e = list_entry(p_cur, struct proc_node, list);
      if (get_cpu_use(e->pid, &e->cpu_time)) {
         pr_info("Error in get_cpu_use() function\n");
      }
   }
}

static void cpu_time_work_handler(struct work_struct *task) {
   update_list_cpu_time(&process_head);
}

DECLARE_WORK(work_cpu_time, cpu_time_work_handler);

// Loops through the node list and prints pid and cpu time
static void print_list(struct list_head* p_head) {
   struct list_head* p_cur;
   struct proc_node* e;
   list_for_each(p_cur, p_head) {
      e = list_entry(p_cur, struct proc_node, list);
      pr_info("PID: %u -> CPU Time: %lu\n", e->pid, e->cpu_time);
   }
}


// zero allocates space for the new node (erroring if not enough mem)
// and then instantiates it with the pid and adds it to the list
static int add_process(unsigned int pid) {
   proc_node *new_node = kzalloc(sizeof(*new_node), GFP_KERNEL);
   if (!new_node) {
      return -ENOMEM;
   }
   new_node->cpu_time = 0;
   new_node->pid = pid;
   list_add(&new_node->list, &process_head);
   print_list(&process_head);
   return 0;
}


static void wakeup(struct timer_list *t) {
   pr_info("Timer up!\n");
   // queue_work(&wq, &work_cpu_time);
   if (schedule_work(&work_cpu_time)) {
      pr_info("Error: Duplicate work not added to queue\n");
   }
   mod_timer(t, jiffies + msecs_to_jiffies(5000)); // reset for loop
}

// from linux kernel module tutorial book
// reads data from procfile and returns to user
// interfaces with fopen userspace function
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
// writes data from usrbuffer to the file
// interfaces with fopen userspace function
static ssize_t procfile_write(struct file *file, const char __user *usr_buffer,
                              size_t write_size, loff_t *offset) {
   unsigned int pid = 0;
   if (*offset >= MAX_PROCFS_SIZE) {
      return -EFBIG; // files is too large error code
   }
   if (*offset + write_size >= MAX_PROCFS_SIZE) {
      write_size = MAX_PROCFS_SIZE - *offset;
   }
   if (copy_from_user(procfs_buffer + *offset, usr_buffer, write_size)) {
      return -EFAULT;
   }
   *offset += write_size;

   // from the usr buffer, read in the pid, add the process to the list
   if (kstrtouint(usr_buffer, 10, &pid) == 0) {
      add_process(pid);
   }

   return write_size;
}

static const struct proc_ops status_access_ops = {
   .proc_read = procfile_read,
   .proc_write = procfile_write,
};

// Searches through the list for a matching pid and removes it, freeing the node
static int remove_process(struct list_head* p_head, unsigned int pid) {
   proc_node *node;
   proc_node *next;

   list_for_each_entry_safe(node, next, p_head, list) {
      if (node->pid == pid) {
         list_del(&node->list);
         kfree(node);
         return 0;
      }
   }
   return -ESRCH;
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
      proc_remove(kmlab_dir);
      kmlab_dir = NULL;
      return -ENOMEM;
   }

   timer_setup(&update_timer, wakeup, 0);
   mod_timer(&update_timer, jiffies + msecs_to_jiffies(5000));

   pr_info("KMLAB MODULE LOADED\n");
   return 0;   
}

// kmlab_exit - Called when the module is unloaded
void __exit kmlab_exit(void)
{
   proc_node *node, *next;
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif

   // printk("Module unloaded successfully");
   if (status_dir) {
      proc_remove(status_dir);
      status_dir = NULL;
   }
   if (kmlab_dir) {
      proc_remove(kmlab_dir);
      kmlab_dir = NULL;
   }

   list_for_each_entry_safe(node, next, &process_head, list) {
      list_del(&node->list);
      kfree(node);
   }

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);
