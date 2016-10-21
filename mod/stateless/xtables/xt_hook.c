#include <linux/module.h>
#include <linux/netfilter/x_tables.h>
#include "nat64/mod/common/core.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NIC Mexico");
MODULE_DESCRIPTION("Stateless IP/ICMP Translation (RFC 7915)");

struct target_info {
	struct xlator instance;
};

/**
 * Called when the kernel wants us to validate an entry the user is adding.
 */
static int create_rule(const struct xt_tgchk_param *param)
{
	struct target_info *info = param->targinfo;
	return jool_xlator_find_current(&info->instance);
}

/** "Convert Netfilter code to Xtables code." */
static unsigned int nfcode2xtcode(unsigned int nfcode)
{
	return (nfcode == NF_ACCEPT) ? XT_CONTINUE : nfcode;
}

/**
 * Called on every matched packet; marks the packet depending on its source
 * address.
 */
static unsigned int xlat_target6(struct sk_buff *skb,
		const struct xt_action_param *param)
{
	const struct target_info *info = param->targinfo;
	return nfcode2xtcode(jool_xlat6(&info->instance, skb));
}

static unsigned int xlat_target4(struct sk_buff *skb,
		const struct xt_action_param *param)
{
	const struct target_info *info = param->targinfo;
	return nfcode2xtcode(jool_xlat4(&info->instance, skb));
}

static void destroy_rule(const struct xt_tgdtor_param *param)
{
	struct target_info *info = param->targinfo;
	jool_xlator_put(&info->instance);
}

static struct xt_target targets[] __read_mostly = {
	{
		.name           = "jool_siit",
		.revision       = 0,
		.family         = NFPROTO_IPV6,
		.hooks          = 1 << NF_INET_PRE_ROUTING,
		.table          = "mangle",
		.checkentry     = create_rule,
		.target         = xlat_target6,
		.destroy        = destroy_rule,
		.targetsize     = sizeof(struct target_info),
		.me             = THIS_MODULE,
	},
	{
		.name           = "jool_siit",
		.revision       = 0,
		.family         = NFPROTO_IPV4,
		.hooks          = 1 << NF_INET_PRE_ROUTING,
		.table          = "mangle",
		.checkentry     = create_rule,
		.target         = xlat_target4,
		.destroy        = destroy_rule,
		.targetsize     = sizeof(struct target_info),
		.me             = THIS_MODULE,
	}
};

/**
 * Called when the user modprobes the module.
 * (Which normally happens when they append the first MARKSRCRANGE rule.)
 */
static int __init jool_siit_tg_init(void)
{
	int error;
	error = xt_register_targets(targets, ARRAY_SIZE(targets));
	return (error < 0) ? error : 0;
}

/**
 * Called when the user "modprobe -r"'s the module.
 */
static void __exit jool_siit_tg_exit(void)
{
	xt_unregister_targets(targets, ARRAY_SIZE(targets));
}

module_init(jool_siit_tg_init);
module_exit(jool_siit_tg_exit);
