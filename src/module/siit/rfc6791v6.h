#ifndef _JOOL_MOD_RFC6791V6_H
#define _JOOL_MOD_RFC6791V6_H

/**
 * @file
 * This is RFC 6791's pool of addresses.
 *
 * "The recommended approach to source selection is to use a single (or
 * small pool of) public IPv4 address as the source address of the
 * translated ICMP message and leverage the ICMP extension [RFC5837] to
 * include the IPv6 address as an Interface IP Address Sub-Object."
 *
 * The ICMP extension thing has not been implemented yet.
 */

#include "translation_state.h"

int rfc6791_find_v6(struct xlation *state, struct in6_addr *result);

#endif /* _JOOL_MOD_RFC6791V6_H */
