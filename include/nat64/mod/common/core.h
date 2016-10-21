#ifndef _JOOL_MOD_CORE_H
#define _JOOL_MOD_CORE_H

/**
 * @file
 * The core is the packet handling's entry point.
 */

#include <linux/skbuff.h>
#include "nat64/mod/common/xlator.h"

unsigned int jool_xlat6(const struct xlator *jool, struct sk_buff *skb);
unsigned int jool_xlat4(const struct xlator *jool, struct sk_buff *skb);

int jool_xlator_find_current(struct xlator *result);
void jool_xlator_put(struct xlator *jool);

#endif /* _JOOL_MOD_CORE_H */
