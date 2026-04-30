#include <linux/init.h>
#include <linux/module.h>
#include <linux/net_namespace.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>

unsigned int hookfn(void* priv, struct sk_buff* skb,
                    const struct nf_hook_state* state) {
  pr_info("Package: len %u, head %s, data %s, end %u", skb->data_len, skb->head,
          skb->data, skb->end);
  return NF_ACCEPT;
}

static struct nf_hook_ops fw_hook_ops = {
    .hook = hookfn,
    .pf = NFPROTO_IPV4,
    .hooknum = NF_INET_LOCAL_OUT,
    .priority = NF_IP_PRI_LAST,
};

static int __init myfw_init(void) {
  pr_info("myfw: init\n");
  int err = nf_register_net_hook(&init_net, &fw_hook_ops);
  if (err) {
    pr_err("myfw: error\n");
    return err;
  }
  return 0;
}

static void __exit myfw_exit(void) {
  pr_info("myfw: exit\n");
  nf_unregister_net_hook(&init_net, &fw_hook_ops);
  return;
}

module_init(myfw_init);
module_exit(myfw_exit);

MODULE_LICENSE("GPL");
