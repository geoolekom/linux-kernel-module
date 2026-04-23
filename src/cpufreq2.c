#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static int cpufreq_show(struct seq_file* f, void* v) {
  int cpu;
  for_each_online_cpu(cpu) {
    seq_printf(f, "%d %u kHz", cpu, cpufreq_quick_get(cpu));
  }
  return 0;
}

static int cpufreq_open(struct inode* i, struct file* f) {
  return single_open(f, cpufreq_show, NULL);
}

static const struct proc_ops cpufreq_proc_ops = {
    .proc_open = cpufreq_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init cpufreq2_init(void) { return 0; }

static void __exit cpufreq2_exit(void) {
  pr_info("Goodbye, kernel!\n");
  return;
}

module_init(cpufreq2_init);
module_exit(cpufreq2_exit);

MODULE_LICENSE("GPL");
