#include <linux/compiler.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define RING_BUF_SIZE 10

static int sleep_ms = 1000;

module_param(sleep_ms, int, 0644);
MODULE_PARM_DESC(sleep_ms, "Sleep time in milliseconds");

struct snapshot {
  ktime_t ts;
  u32 nr_procs;
  u32 cpu;
  u32 free_ram;
  u32 total_ram;
};

struct ring_buf {
  struct snapshot entries[RING_BUF_SIZE];
  u32 head;
  u32 tail;
};

static struct ring_buf* ring;

static bool ring_produce(struct ring_buf* r, const struct snapshot* s) {
  u32 head = r->head;
  u32 tail = READ_ONCE(r->tail);

  if (head - tail > RING_BUF_SIZE) {
    return false;
  }

  r->entries[head % RING_BUF_SIZE] = *s;
  smp_wmb();
  WRITE_ONCE(r->head, head + 1);
  return true;
}

static bool ring_consume(struct ring_buf* r, struct snapshot* s) {
  u32 head = READ_ONCE(r->head);
  u32 tail = r->tail;

  if (head == tail) {
    return false;
  }

  smp_rmb();
  *s = r->entries[tail % RING_BUF_SIZE];
  smp_mb();

  WRITE_ONCE(r->tail, tail + 1);
  return true;
}

static void produce_snapshot(const char* name) {
  struct task_struct* task;
  int count = 0;

  struct sysinfo si;
  si_meminfo(&si);

  rcu_read_lock();
  for_each_process(task) { count++; }
  rcu_read_unlock();

  struct snapshot s = {
      .ts = ktime_get(),
      .nr_procs = count,
      .cpu = smp_processor_id(),
      .total_ram = si.totalram >> (20 - PAGE_SHIFT),
      .free_ram = si.freeram >> (20 - PAGE_SHIFT),
  };

  bool produced = ring_produce(ring, &s);
  if (!produced) {
    pr_warn("%s: ring full, dropping", name);
  }
}

static int monitor_fn(void* data) {
  allow_signal(SIGINT);
  allow_signal(SIGTERM);

  pr_info("Current kmonitor stats: pid=%d comm=%s\n", current->pid,
          current->comm);

  while (!kthread_should_stop()) {
    unsigned long sleep_time_left = msleep_interruptible(sleep_ms);
    if (sleep_time_left != 0) {
      pr_info("kmonitor: interrupted, sleep time left: %lu", sleep_time_left);
      if (signal_pending(current)) {
        pr_info("kmonitor: interrupted by signal");
        sigset_t* pending = &current->signal->shared_pending.signal;

        if (sigismember(pending, SIGINT)) {
          pr_info("kmonitor: interrupted by SIGINT");

          produce_snapshot("kmonitor");
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

static int log_fn(void* data) {
  struct snapshot s;
  while (!kthread_should_stop()) {
    msleep_interruptible(1000);
    bool consumed = ring_consume(ring, &s);
    if (consumed) {
      pr_info("log: %lld: CPU %d, %d tasks alive, %d Mb free, %d Mb total\n",
              s.ts, s.cpu, s.nr_procs, s.free_ram, s.total_ram);
    } else {
      pr_info("log: nothing to consume");
    }
  }

  return 0;
}

static struct task_struct *monitor_task, *log_task;

static int __init main_init(void) {
  pr_info("Current init stats: %d %s %d\n", current->pid, current->comm,
          current->tgid);

  ring = kzalloc(sizeof(*ring), GFP_KERNEL);
  if (ring == NULL) {
    return -ENOMEM;
  }

  monitor_task = kthread_run(monitor_fn, NULL, "monitor");
  if (IS_ERR(monitor_task)) {
    pr_err("Failed to create monitor task: %ld\n", PTR_ERR(monitor_task));
    return PTR_ERR(monitor_task);
  }
  log_task = kthread_run(log_fn, NULL, "log");
  if (IS_ERR(log_task)) {
    pr_err("Failed to create log task: %ld\n", PTR_ERR(log_task));
    return PTR_ERR(log_task);
  }
  pr_info("Module loaded: sleep_ms=%d", sleep_ms);
  return 0;
}

static void __exit main_exit(void) {
  if (monitor_task) {
    kthread_stop(monitor_task);
    monitor_task = NULL;
  }
  if (log_task) {
    kthread_stop(log_task);
    log_task = NULL;
  }
  pr_info("Module unloaded");
  kfree(ring);
  return;
}

module_init(main_init);
module_exit(main_exit);

MODULE_LICENSE("GPL");
