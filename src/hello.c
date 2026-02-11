#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void) {
  pr_info("Hello, kernel!\n");
  return 0;  // 0 = success, negative = error
}

static void __exit hello_exit(void) {
  pr_info("Goodbye, kernel!\n");
  return;
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
