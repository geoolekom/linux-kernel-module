#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/seq_file.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/topology.h>
#include <linux/workqueue.h>

#define QUEUE_SIZE 8

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

static DEFINE_KFIFO(snap_fifo, struct snapshot, QUEUE_SIZE);

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

  u32 num = kfifo_put(&snap_fifo, s);
  if (num == 0) {
    pr_warn("%s: ring full, dropping", name);
  }
}

static int monitor_fn(void* data) {
  pr_info("Current kmonitor stats: pid=%d comm=%s\n", current->pid,
          current->comm);

  while (!kthread_should_stop()) {
    produce_snapshot("kmonitor");
    msleep(sleep_ms);
  }

  return 0;
}

static struct proc_dir_entry* monitor_entry;

static int monitor_show(struct seq_file* f, void*) {
  struct snapshot s;
  u32 num = kfifo_get(&snap_fifo, &s);
  if (num > 0) {
    seq_printf(
        f, "proc-log: %lld: CPU %d, %d tasks alive, %d Mb free, %d Mb total\n",
        s.ts, s.cpu, s.nr_procs, s.free_ram, s.total_ram);
  } else {
    seq_printf(f, "proc-log: nothing to consume\n");
  }
  return 0;
}

static int monitor_open(struct inode* i, struct file* f) {
  return single_open(f, monitor_show, NULL);
}

static const struct proc_ops monitor_proc_ops = {
    .proc_open = monitor_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static struct task_struct* monitor_task;

static int __init main_init(void) {
  pr_info("Current init stats: %d %s %d\n", current->pid, current->comm,
          current->tgid);

  monitor_entry = proc_create("monitor_info", 0444, NULL, &monitor_proc_ops);
  if (monitor_entry == NULL) {
    return -ENOMEM;
  }
  pr_info("monitor: /proc/monitor_info created\n");

  monitor_task = kthread_run(monitor_fn, NULL, "monitor");
  if (IS_ERR(monitor_task)) {
    pr_err("Failed to create monitor task: %ld\n", PTR_ERR(monitor_task));
    return PTR_ERR(monitor_task);
  }

  pr_info("Module loaded: sleep_ms=%d", sleep_ms);
  return 0;
}

static void __exit main_exit(void) {
  if (monitor_task) {
    kthread_stop(monitor_task);
    monitor_task = NULL;
  }
  proc_remove(monitor_entry);
  pr_info("monitor: /proc/monitor_info deleted\n");
  pr_info("Module unloaded");
  return;
}

module_init(main_init);
module_exit(main_exit);

MODULE_LICENSE("GPL");
