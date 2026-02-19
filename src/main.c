#include <linux/dcache.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/path.h>

static int log_level = 0;
static char *module_name = "hello";

module_param(log_level, int, 0644);
MODULE_PARM_DESC(log_level, "Log level");

module_param(module_name, charp, 0644);
MODULE_PARM_DESC(module_name, "Module name");

static void log_file_access(struct file *filp) {
  struct dentry *dentry = filp->f_path.dentry;
  pr_info("File: %pd --- %pd4\n", dentry, dentry);
}

static int __init main_init(void) {
  __be32 ipv4_addr = in_aton("192.168.1.100");
  unsigned char mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  pr_info("IPv4: %pI4\n", &ipv4_addr);
  pr_info("MAC: %pM\n", mac);
  pr_info("Module loaded %d %s", log_level, module_name);
  pr_info("Current PID: %d\n", current->pid);
  return 0;
}

static void __exit main_exit(void) {
  pr_info("Module unloaded");
  return;
}

module_init(main_init);
module_exit(main_exit);

MODULE_LICENSE("GPL");
