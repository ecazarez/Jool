#include "atomic_config.h"

#include "nl_common.h"
#include "nl_core.h"
#include "common/atomic_config.h"

int handle_atomconfig_request(struct xlator *jool, struct genl_info *info)
{
	struct request_hdr *hdr;
	size_t total_len;
	int error;

	if (verify_superpriv())
		return nlcore_respond(info, -EPERM);

	hdr = nla_data(info->attrs[ATTR_DATA]);
	total_len = nla_len(info->attrs[ATTR_DATA]);

	error = atomconfig_add(jool, hdr + 1, total_len - sizeof(*hdr));
	return nlcore_respond(info, error);
}
