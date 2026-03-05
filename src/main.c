#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>
#include <linux/workqueue.h>

static int sleep_ms = 10000;

module_param(sleep_ms, int, 0644);
MODULE_PARM_DESC(sleep_ms, "Sleep time in milliseconds");

static int monitor_fn(void *data) {
  allow_signal(SIGINT);
  allow_signal(SIGTERM);

  pr_info("Current kmonitor stats: pid=%d comm=%s\n", current->pid,
          current->comm);

  struct task_struct *task;
  int count;
  while (!kthread_should_stop()) {
    count = 0;
    unsigned long sleep_time_left = msleep_interruptible(sleep_ms);
    if (sleep_time_left != 0) {
      pr_info("kmonitor: interrupted, sleep time left: %lu", sleep_time_left);
      if (signal_pending(current)) {
        pr_info("kmonitor: interrupted by signal");
        sigset_t *pending = &current->signal->shared_pending.signal;

        if (sigismember(pending, SIGINT)) {
          pr_info("kmonitor: interrupted by SIGINT");

          rcu_read_lock();
          for_each_process(task) { count++; }
          rcu_read_unlock();
          pr_info("kmonitor: %d tasks alive\n", count);
          flush_signals(current);
          continue;
        }
        if (sigismember(pending, SIGTERM)) {
          pr_info("kmonitor: interrupted by SIGTERM");
          flush_signals(current);
          break;
        }
      }
    }
  }
  return 0;
}

static struct task_struct *monitor_task;

static void work_fn(struct work_struct *work) {
  int count;
  for (int i = 0; i < 10; i++) {
    struct task_struct *task;
    count = 0;
    rcu_read_lock();
    for_each_process(task) { count++; }
    rcu_read_unlock();
    pr_info("work_fn: %d tasks alive\n", count);
    msleep(sleep_ms);
  }
}

static DECLARE_WORK(work, work_fn);

static int __init main_init(void) {
  pr_info("Current init stats: %d %s %d\n", current->pid, current->comm,
          current->tgid);

  schedule_work(&work);
  monitor_task = kthread_run(monitor_fn, NULL, "monitor");
  if (IS_ERR(monitor_task)) {
    pr_err("Failed to create monitor task: %ld\n", PTR_ERR(monitor_task));
    return PTR_ERR(monitor_task);
  }
  pr_info("Module loaded: sleep_ms=%d", sleep_ms);
  return 0;
}

static void __exit main_exit(void) {
  cancel_work_sync(&work);
  if (monitor_task) {
    kthread_stop(monitor_task);
    monitor_task = NULL;
  }
  pr_info("Module unloaded");
  return;
}

module_init(main_init);
module_exit(main_exit);

MODULE_LICENSE("GPL");
