#include <linux/cpufreq.h>
#include <linux/errno.h>
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

static struct proc_dir_entry* cpufreq_entry;

static int __init cpufreq2_init(void) {
  cpufreq_entry = proc_create("cpufreq2_info", 0444, NULL, &cpufreq_proc_ops);
  if (cpufreq_entry == NULL) {
    return -ENOMEM;
  }
  pr_info("cpufreq_info: /proc/cpufreq2_info created\n");
  return 0;
}

static void __exit cpufreq2_exit(void) {
  proc_remove(cpufreq_entry);
  pr_info("cpufreq_info: /proc/cpufreq2_info deleted\n");
  return;
}

module_init(cpufreq2_init);
module_exit(cpufreq2_exit);

MODULE_LICENSE("GPL");
