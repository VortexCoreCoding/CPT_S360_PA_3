#define LINUX

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h> // for copy_from_user
#include <linux/list.h>
#include <linux/seq_file.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include "kmlab_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Spinali"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 PA_3 - KM");

#define DEBUG 1
#define MAX_PROCFS_SIZE 1024

static DEFINE_MUTEX(list_access_lock);

// store per process data (you may modify it as needed)
typedef struct proc_node {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_node;

static LIST_HEAD(process_head);

static struct proc_dir_entry *kmlab_dir;
static struct proc_dir_entry *status_dir; 

// Timer used for wakeup function
static struct timer_list update_timer;

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
   return 0;
}

static void print_list(struct list_head *head) {
   proc_node *node;

   list_for_each_entry(node, head, list) {
      pr_info("PID: %u -> CPU Time: %lu\n", node->pid, node->cpu_time);
   }
}

// Loops through the list of nodes and increases the cpu time by the cpu use calculator in kmlab_given.h
static void update_list_cpu_time(struct list_head* p_head) {
   proc_node *node;
   proc_node *next;
   mutex_lock(&list_access_lock);
   list_for_each_entry_safe(node, next, p_head, list) {
      if (get_cpu_use(node->pid, &node->cpu_time)) {
         // remove a process if its id is not present or finished
         pr_info("Removed %u", node->pid);
         list_del(&node->list);
         kfree(node);
      }
   }
   mutex_unlock(&list_access_lock);
}

static void cpu_time_work_handler(struct work_struct *task) {
   update_list_cpu_time(&process_head);
}

DECLARE_WORK(work_cpu_time, cpu_time_work_handler);

static int procfile_show(struct seq_file *seq, void *data) {
   proc_node *node;

   mutex_lock(&list_access_lock);
   list_for_each_entry(node, &process_head, list) {
      seq_printf(seq, "PID: %u -> CPU Time: %lu\n", node->pid, node->cpu_time);
   }
   mutex_unlock(&list_access_lock);
   return 0;
}

static int procfile_open(struct inode *inode, struct file *file) {
   return single_open(file, procfile_show, NULL);
}

static void wakeup(struct timer_list *t) {
   // queue_work(&wq, &work_cpu_time);
   if (!schedule_work(&work_cpu_time)) {
      pr_info("Error: Duplicate work not added to queue\n");
   }
   mod_timer(t, jiffies + msecs_to_jiffies(5000)); // reset for loop
}

static ssize_t procfile_write(struct file *file, const char __user *usr_buffer,
                              size_t write_size, loff_t *offset) {
   unsigned int pid = 0;
   char kernel_buffer[16];

   if (!write_size || write_size >= sizeof(kernel_buffer)) {
      return -EINVAL;
   }
   if (copy_from_user(kernel_buffer, usr_buffer, write_size)) {
      return -EFAULT;
   }
   kernel_buffer[write_size] = '\0';

   if (kstrtouint(kernel_buffer, 10, &pid)) {
      return -EINVAL;
   }

   mutex_lock(&list_access_lock);
   if (add_process(pid)) {
      mutex_unlock(&list_access_lock);
      return -ENOMEM;
   }
      print_list(&process_head);
   pr_info("Added %u", pid);
   mutex_unlock(&list_access_lock);

   return write_size;
}

static const struct proc_ops status_access_ops = {
   .proc_open = procfile_open,
   .proc_read = seq_read,
   .proc_lseek = seq_lseek,
   .proc_release = single_release,
   .proc_write = procfile_write,
};


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

   del_timer_sync(&update_timer);
   cancel_work_sync(&work_cpu_time);
   mutex_lock(&list_access_lock);
   list_for_each_entry_safe(node, next, &process_head, list) {
      list_del(&node->list);
      kfree(node);
   }
   mutex_unlock(&list_access_lock);

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);
