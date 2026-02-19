#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

static int __init hello_init(void) {
  char buffer[8192];
  memset(buffer, 0, sizeof(buffer));
  printk(KERN_WARNING "Value: %p\n", buffer);
  return 0;
}

static void __exit hello_exit(void) {
  pr_info("Goodbye, kernel!\n");
  return;
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
