#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define MONITOR_BUF_ENTRIES 64

static int sleep_ms = 10000;

module_param(sleep_ms, int, 0644);
MODULE_PARM_DESC(sleep_ms, "Sleep time in milliseconds");

struct snapshot {
  ktime_t ts;
  u32 nr_procs;
  u32 cpu;
};

static void print_statistics(const char* name, struct snapshot* s) {
  struct task_struct* task;
  int count = 0;

  rcu_read_lock();
  for_each_process(task) { count++; }
  rcu_read_unlock();

  pr_info("%s: %lld: CPU %d, %d tasks alive, snapshot %px\n", name, ktime_get(),
          smp_processor_id(), count, s);

  if (s == NULL) {
    pr_warn("No snapshot!");
    return;
  }

  *s = (struct snapshot){
      .ts = ktime_get(),
      .nr_procs = count,
      .cpu = smp_processor_id(),
  };
  pr_info("%s: %lld: CPU %d, %d tasks alive\n", name, s->ts, s->cpu,
          s->nr_procs);
}

static int monitor_fn(void* data) {
  allow_signal(SIGINT);
  allow_signal(SIGTERM);

  pr_info("Current kmonitor stats: pid=%d comm=%s\n", current->pid,
          current->comm);

  struct snapshot* snapshots;
  int snap_head = 0;
  snapshots = kcalloc(MONITOR_BUF_ENTRIES, sizeof(*snapshots), GFP_KERNEL);
  if (snapshots == NULL) {
    return -ENOMEM;
  }

  while (!kthread_should_stop()) {
    unsigned long sleep_time_left = msleep_interruptible(sleep_ms);
    if (sleep_time_left != 0) {
      pr_info("kmonitor: interrupted, sleep time left: %lu", sleep_time_left);
      if (signal_pending(current)) {
        pr_info("kmonitor: interrupted by signal");
        sigset_t* pending = &current->signal->shared_pending.signal;

        if (sigismember(pending, SIGINT)) {
          pr_info("kmonitor: interrupted by SIGINT");

          print_statistics("kmonitor",
                           &snapshots[snap_head % MONITOR_BUF_ENTRIES]);
          snap_head++;
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

  kfree(snapshots);
  return 0;
}

static struct task_struct* monitor_task;

static void work_fn(struct work_struct* work) {
  for (int i = 0; i < 10; i++) {
    print_statistics("work_fn", NULL);
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
