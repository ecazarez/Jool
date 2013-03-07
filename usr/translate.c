#include "nat64/usr/translate.h"
#include "nat64/comm/str_utils.h"
#include "nat64/usr/netlink.h"
#include <errno.h>


#define HDR_LEN sizeof(struct request_hdr)
#define PAYLOAD_LEN sizeof(union request_translate)

static int handle_display_response(struct nl_msg *msg, void *arg)
{
	struct response_hdr *hdr = nlmsg_data(nlmsg_hdr(msg));
	struct translate_config *conf = (struct translate_config *) (hdr + 1);

	__u16 *plateaus;
	int i;

	if (hdr->result_code != ERR_SUCCESS) {
		print_code_msg(hdr->result_code, NULL);
		return EINVAL;
	}

	printf("Override IPv6 traffic class (%s): %s\n", RESET_TCLASS_OPT,
			conf->reset_traffic_class ? "ON" : "OFF");
	printf("Override IPv4 type of service (%s): %s\n", RESET_TOS_OPT,
			conf->reset_tos ? "ON" : "OFF");
	printf("IPv4 type of service (%s): %u\n", NEW_TOS_OPT, conf->new_tos);
	printf("DF flag always on (%s): %s\n", DF_ALWAYS_ON_OPT, conf->df_always_on ? "ON" : "OFF");
	printf("Generate IPv4 identification (%s): %s\n", BUILD_IPV4_ID_OPT,
			conf->build_ipv4_id ? "ON" : "OFF");
	printf("Decrease MTU failure rate (%s): %s\n", LOWER_MTU_FAIL_OPT,
			conf->lower_mtu_fail ? "ON" : "OFF");
	printf("IPv6 next hop MTU (%s): %u\n", IPV6_NEXTHOP_MTU_OPT, conf->ipv6_nexthop_mtu);
	printf("IPv4 next hop MTU (%s): %u\n", IPV4_NEXTHOP_MTU_OPT, conf->ipv4_nexthop_mtu);
	printf("MTU plateaus (%s): ", MTU_PLATEAUS_OPT);

	plateaus = (__u16 *) (conf + 1);
	for (i = 0; i < conf->mtu_plateau_count; i++) {
		if (i + 1 != conf->mtu_plateau_count)
			printf("%u, ", plateaus[i]);
		else
			printf("%u\n", plateaus[i]);
	}

	printf("Packet reserved head room (%s): %u\n", SKB_HEAD_ROOM_OPT, conf->skb_head_room);
	printf("Packet reserved tail room (%s): %u\n", SKB_TAIL_ROOM_OPT, conf->skb_tail_room);

	return 0;
}

static int handle_update_response(struct nl_msg *msg, void *arg)
{
	struct response_hdr *hdr = nlmsg_data(nlmsg_hdr(msg));
	print_code_msg(hdr->result_code, "Value changed successfully.");
	return 0;
}

int translate_request(__u32 operation, struct translate_config *config)
{
	if (operation == 0) {
		struct request_hdr request;

		request.length = sizeof(request);
		request.mode = MODE_TRANSLATE;
		request.operation = 0;

		return netlink_request(&request, request.length, handle_display_response);
	} else {
		struct request_hdr *hdr;
		struct translate_config *payload_translate;
		__u16 *payload_mtus;
		__u16 request_len, mtus_len;
		int result;

		mtus_len = (operation & MTU_PLATEAUS_MASK)
				? (config->mtu_plateau_count * sizeof(*config->mtu_plateaus))
				: 0;
		request_len = sizeof(*hdr) + sizeof(*payload_translate) + mtus_len;
		hdr = malloc(request_len);
		if (!hdr)
			return ENOMEM;
		payload_translate = (struct translate_config *) (hdr + 1);
		payload_mtus = (__u16 *) (payload_translate + 1);

		hdr->length = request_len;
		hdr->mode = MODE_TRANSLATE;
		hdr->operation = operation;
		*payload_translate = *config;
		memcpy(payload_mtus, config->mtu_plateaus, mtus_len);

		result = netlink_request(hdr, request_len, handle_update_response);
		free(hdr);
		return result;
	}
}