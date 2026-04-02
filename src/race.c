#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/percpu.h>

#define MAX_COUNT 1000

static DEFINE_PER_CPU(unsigned long, counter);
static struct task_struct *task1, *task2;

static int increment_function(void* data) {
  while (!kthread_should_stop() && counter < MAX_COUNT) {
    pr_info("Counter pid=%d: %lu", current->pid, counter);
    this_cpu_inc(counter);
  }
  return 0;
}

static int __init race_init(void) {
  pr_info("Loading race!\n");
  struct sysinfo si;
  si_meminfo(&si);
  pr_info("monitor: total=%lu MiB free=%lu MiB\n",
          si.totalram >> (20 - PAGE_SHIFT), si.freeram >> (20 - PAGE_SHIFT));

  task1 = kthread_run(increment_function, NULL, "inc1");
  task2 = kthread_run(increment_function, NULL, "inc2");
  return 0;
}

static void __exit race_exit(void) {
  pr_info("Undloading race!\n");
  if (task1) {
    kthread_stop(task1);
    task1 = NULL;
  }
  if (task2) {
    kthread_stop(task2);
    task2 = NULL;
  }
  return;
}

module_init(race_init);
module_exit(race_exit);

MODULE_LICENSE("GPL");
