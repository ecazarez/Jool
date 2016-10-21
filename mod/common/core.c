#include "nat64/mod/common/core.h"

#include "nat64/mod/common/config.h"
#include "nat64/mod/common/handling_hairpinning.h"
#include "nat64/mod/common/xlator.h"
#include "nat64/mod/common/translation_state.h"
#include "nat64/mod/common/rfc6145/core.h"
#include "nat64/mod/stateful/compute_outgoing_tuple.h"
#include "nat64/mod/stateful/determine_incoming_tuple.h"
#include "nat64/mod/stateful/filtering_and_updating.h"
#include "nat64/mod/stateful/fragment_db.h"
#include "nat64/mod/common/send_packet.h"

static verdict core_common(struct xlation *state)
{
	verdict result;

	if (xlat_is_nat64()) {
		result = determine_in_tuple(state);
		if (result != VERDICT_CONTINUE)
			goto end;
		result = filtering_and_updating(state);
		if (result != VERDICT_CONTINUE)
			goto end;
		result = compute_out_tuple(state);
		if (result != VERDICT_CONTINUE)
			goto end;
	}
	result = translating_the_packet(state);
	if (result != VERDICT_CONTINUE)
		goto end;

	if (is_hairpin(state)) {
		result = handling_hairpinning(state);
		kfree_skb(state->out.skb); /* Put this inside of hh()? */
	} else {
		result = sendpkt_send(state);
		/* sendpkt_send() releases out's skb regardless of verdict. */
	}

	if (result != VERDICT_CONTINUE)
		goto end;

	log_debug("Success.");
	/*
	 * The new packet was sent, so the original one can die; drop it.
	 *
	 * NF_DROP translates into an error (see nf_hook_slow()).
	 * Sending a replacing & translated version of the packet should not
	 * count as an error, so we free the incoming packet ourselves and
	 * return NF_STOLEN on success.
	 */
	kfree_skb(state->in.skb);
	result = VERDICT_STOLEN;
	/* Fall through. */

end:
	if (result == VERDICT_ACCEPT)
		log_debug("Returning the packet to the kernel.");
	return result;
}

unsigned int jool_xlat6(const struct xlator *jool, struct sk_buff *skb)
{
	struct xlation state;
	struct ipv6hdr *hdr;
	verdict result;

	if (!jool->global->cfg.enabled)
		return NF_ACCEPT;

	hdr = ipv6_hdr(skb);
	log_debug("===============================================");
	log_debug("Catching IPv6 packet: %pI6c->%pI6c", &hdr->saddr,
			&hdr->daddr);

	/*
	 * TODO optimize this copy away? be aware that turning state.jool into
	 * a pointer will result in like a bazillion dereferences.
	 */
	/* Reminder: This function might change pointers. */
	if (pkt_init_ipv6(&state.in, skb) != 0)
		return NF_DROP;
	state.jool = *jool;
	bib_session_init(&state.entries);

	if (xlat_is_nat64()) {
		result = fragdb_handle(state.jool.nat64.frag, &state.in);
		if (result != VERDICT_CONTINUE)
			return result;
	}

	return core_common(&state);
}
EXPORT_SYMBOL(jool_xlat6);

unsigned int jool_xlat4(const struct xlator *jool, struct sk_buff *skb)
{
	struct xlation state;
	struct iphdr *hdr;

	if (!jool->global->cfg.enabled)
		return NF_ACCEPT;

	hdr = ip_hdr(skb);
	log_debug("===============================================");
	log_debug("Catching IPv4 packet: %pI4->%pI4", &hdr->saddr, &hdr->daddr);

	/* Reminder: This function might change pointers. */
	if (pkt_init_ipv4(&state.in, skb) != 0)
		return NF_DROP;
	state.jool = *jool;
	bib_session_init(&state.entries);

	return core_common(&state);
}
EXPORT_SYMBOL(jool_xlat4);

int jool_xlator_find_current(struct xlator *result)
{
	return xlator_find_current(result);
}
EXPORT_SYMBOL(jool_xlator_find_current);

void jool_xlator_put(struct xlator *jool)
{
	xlator_put(jool);
}
EXPORT_SYMBOL(jool_xlator_put);
