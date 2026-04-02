#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/vmalloc.h>

static int __init hello_init(void) {
  pr_info("Hello!\n");
  char* buf = kmalloc(16, GFP_KERNEL);
  char* vbuf = vmalloc(16);

  int i;
  for (i = 0; i < 16; i++) {
    pr_info("Addr %d %px", i, &buf[i]);
  }
  for (i = 0; i < 16; i++) {
    pr_info("Vaddr %d %px", i, &vbuf[i]);
  }
  kfree(buf);
  vfree(vbuf);
  return 0;
}

static void __exit hello_exit(void) {
  pr_info("Goodbye, kernel!\n");
  return;
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
