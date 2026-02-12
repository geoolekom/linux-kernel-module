#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static int log_level = 0;
static char *module_name = "hello";

module_param(log_level, int, 0644);
MODULE_PARM_DESC(log_level, "Log level");

module_param(module_name, charp, 0644);
MODULE_PARM_DESC(module_name, "Module name");

static int __init main_init(void) {
  pr_info("Module loaded %d %s", log_level, module_name);
  return 0;
}

static void __exit main_exit(void) {
  pr_info("Module unloaded");
  return;
}

module_init(main_init);
module_exit(main_exit);

MODULE_LICENSE("GPL");
