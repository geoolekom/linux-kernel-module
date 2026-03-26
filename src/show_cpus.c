#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/percpu.h>

#define MAX_COUNT 1000

static DEFINE_PER_CPU(unsigned long, counter);
static struct task_struct *task1, *task2;

static int increment_function(void *data) {
  while (!kthread_should_stop() && counter < MAX_COUNT) {
    pr_info("Counter pid=%d: %lu CPU: %d", current->pid, counter,
            smp_processor_id());
    this_cpu_inc(counter);
    msleep_interruptible(1000);
  }
  return 0;
}

static int __init cpus_init(void) {
  pr_info("Loading cpus!\n");
  task1 = kthread_run(increment_function, NULL, "inc1");
  if (IS_ERR(task1)) {
    pr_err("Failed to create inc1 task: %ld\n", PTR_ERR(task1));
    return PTR_ERR(task1);
  }
  task2 = kthread_run(increment_function, NULL, "inc2");
  if (IS_ERR(task2)) {
    pr_err("Failed to create inc2 task: %ld\n", PTR_ERR(task2));
    return PTR_ERR(task2);
  }
  return 0;
}

static void __exit cpus_exit(void) {
  pr_info("Unoading cpus!\n");
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

module_init(cpus_init);
module_exit(cpus_exit);

MODULE_LICENSE("GPL");
