#include <linux/init.h>
#include <linux/module.h>

static int __init cpufreq_init(void) { return 0; }

static void __exit cpufreq_exit(void) {
  pr_info("Goodbye, kernel!\n");
  return;
}

module_init(cpufreq_init);
module_exit(cpufreq_exit);

MODULE_LICENSE("GPL");
