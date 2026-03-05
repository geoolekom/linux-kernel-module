#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

static int __init schedinfo_init(void) {
  struct task_struct *task = current;
  pr_info("schedinfo: %d %s %d\n", task->pid, task->comm, task->tgid);
  pr_info("CFS params: %llu %llu %lu %lu", task->se.vruntime,
          task->se.sum_exec_runtime, task->nvcsw, task->nivcsw);
  return 0;
}

static void __exit schedinfo_exit(void) {
  pr_info("schedinfo: exiting\n");
  return;
}

module_init(schedinfo_init);
module_exit(schedinfo_exit);

MODULE_LICENSE("GPL");