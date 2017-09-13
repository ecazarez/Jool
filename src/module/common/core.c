#include "core.h"

#include "config.h"
#include "handling_hairpinning.h"
#include "xlator.h"
#include "translation_state.h"
#include "rfc6145/core.h"
#include "nat64/compute_outgoing_tuple.h"
#include "nat64/determine_incoming_tuple.h"
#include "nat64/filtering_and_updating.h"
#include "nat64/fragment_db.h"
#include "send_packet.h"

static verdict core_common(struct xlation *state)
{
	verdict result;

	if (state->jool.type == XLATOR_NAT64) {
		result = determine_in_tuple(state);
		if (result != VERDICT_CONTINUE)
			return result;
		result = filtering_and_updating(state);
		if (result != VERDICT_CONTINUE)
			return result;
		result = compute_out_tuple(state);
		if (result != VERDICT_CONTINUE)
			return result;
	}

	result = translating_the_packet(state);
	if (result != VERDICT_CONTINUE)
		return result;

	if (is_hairpin(state)) {
		result = handling_hairpinning(state);
		kfree_skb(state->out.skb); /* Put this inside of hh()? */
	} else {
		result = sendpkt_send(state);
		/* sendpkt_send() releases out's skb regardless of verdict. */
	}
	if (result != VERDICT_CONTINUE)
		return result;

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
	return VERDICT_STOLEN;
}

verdict core_4to6(struct xlation *state, struct sk_buff *skb)
{
	struct iphdr *hdr = ip_hdr(skb);

	/* TODO this field is prolly redundat (ip link set jool0 down). */
	if (!state->jool.global->cfg.enabled)
		return VERDICT_DROP;

	log_debug("===============================================");
	log_debug("Got IPv4 packet: %pI4->%pI4", &hdr->saddr, &hdr->daddr);

	/* Reminder: This function might change pointers. */
	if (pkt_init_ipv4(&state->in, skb, state->jool.type) != 0)
		return VERDICT_DROP;

	/*
	if (xlat_is_nat64(&state)) {
		if (ip_defrag(jool->ns, skb, DEFRAG4_JOOL_USER))
			return VERDICT_STOLEN;
	}
	*/

	return core_common(state);
}

verdict core_6to4(struct xlation *state, struct sk_buff *skb)
{
	struct ipv6hdr *hdr = ipv6_hdr(skb);

	snapshot_record(&state->in.debug.shot1, skb);

	if (!state->jool.global->cfg.enabled)
		return VERDICT_DROP;

	log_debug("===============================================");
	log_debug("Got IPv6 packet: %pI6c->%pI6c", &hdr->saddr, &hdr->daddr);

	/* Reminder: This function might change pointers. */
	if (pkt_init_ipv6(&state->in, skb, state->jool.type) != 0)
		return VERDICT_DROP;

	snapshot_record(&state->in.debug.shot2, skb);

	/*
	 * TODO this is going to be a project.
	 *
	 * There are a few things that have always bothered me about the
	 * kernel's fragment reassembly code. Off the top of my head:
	 *
	 * - While the ip_defrag() API seems reasonably well-engineered in that
	 *   it seems designed to be reusable, the IPv6 defrag seems to be a
	 *   single-purpose gimmic. Perhaps as a result of only being used once,
	 *   its API seems to not be self-contained.
	 * - I don't think it's quite "fully well-engineered" because it relies
	 *   on an integer called "users", and there is no way for a kernel
	 *   module to acquire a 100% guaranteed unique value for this number.
	 *   This appears to be an unsolvable problem.
	 * - For some reason, the code sometimes reassembles in frags instead of
	 *   frag_list. I used to think I had this figured out, but later
	 *   experience revealed that I don't. I don't even see any usage of
	 *   frags in the code, so this baffles me to no end.
	 *   This essentially means that I haven't managed to nail defrag usage
	 *   correctly. Even the current most mature version of Jool likely has
	 *   quirks (issue #231).
	 * - Jool doesn't actually need the reassembly step. It just needs a
	 *   list with the fragments. They don't even need to be in the right
	 *   order. This makes fragment handling likely an order of complexity
	 *   slower than it needs to be.
	 * - They are miserably documented and poorly coded pieces of shit.
	 *   Seriously; I don't mind gotos in C, but the kernel devs seem to
	 *   outright have a fetish for them.
	 * - The kernel's official fragment representation is a bit of a pain to
	 *   work with.
	 *
	 * So I'm honestly wondering if it wouldn't be better to just roll-out
	 * my own defrag and call it a day. It would certainly be easier, but I
	 * would lose the official defrags' maturity.
	 */

	/*
	if (xlat_is_nat64(&state)) {
		result = fragdb_handle(state.jool.nat64.frag, &state.in);
		if (result != VERDICT_CONTINUE)
			return result;
	}
	*/

	return core_common(state);
}
