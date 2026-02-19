#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

static int log_level = 0;
static char *module_name = "hello";

module_param(log_level, int, 0644);
MODULE_PARM_DESC(log_level, "Log level");

module_param(module_name, charp, 0644);
MODULE_PARM_DESC(module_name, "Module name");

static int __init main_init(void) {
  pr_info("Module loaded %d %s", log_level, module_name);
  pr_info("Current init stats: %d %s %d\n", current->pid, current->comm,
          current->tgid);

  struct task_struct *task;
  int count = 0;
  rcu_read_lock();
  for_each_process(task) {
    pr_info("Process: %d %s %d\n", task->pid, task->comm, task->tgid);
    count++;
  }
  rcu_read_unlock();
  pr_info("Total processes: %d\n", count);
  return 0;
}

static void __exit main_exit(void) {
  pr_info("Module unloaded");
  pr_info("Current exit stats: %d %s %d\n", current->pid, current->comm,
          current->tgid);
  return;
}

module_init(main_init);
module_exit(main_exit);

MODULE_LICENSE("GPL");
