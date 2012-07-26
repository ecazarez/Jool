#include "nf_nat64_ipv4_pool.h"

__be32 swap_endians(__be32 be)
{
    __be32 le = ((be & 0xFF) << 24)
                      | ((be & 0xFF00) << 8)
                      | ((be >> 8) & 0xFF00)
                      | (be >> 24);
    return le;
}

char *ip_address_to_string(__be32 ip)
{
    char *result = (char *)kmalloc((sizeof(char))*INET_ADDRSTRLEN, GFP_ATOMIC);
    
    sprintf(result, "%d.%d.%d.%d",
            (ip      ) & 0xFF,
            (ip >>  8) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 24) & 0xFF);
    
    return result;
}

struct transport_addr_struct
	*get_transport_addr(struct list_head *head, int *next_address,
						int *next_port, int *first_port, int *last_port)
{
    // if the list is empty
    if(list_empty(head) == 1){
        // and the next address is greater than the last address, return NULL
        if(*next_address > last_address){
            return NULL;
        }
        // get the next address
        else{
            struct transport_addr_struct *new_transport_addr = (struct transport_addr_struct *)kmalloc(sizeof(struct transport_addr_struct), GFP_ATOMIC);
            
            if(new_transport_addr != NULL){
                __be32 r = swap_endians(*next_address);
                
                new_transport_addr->address = ip_address_to_string(r);
                
                new_transport_addr->port = (*next_port)++;
    
                if(*next_port > *last_port){
                    *next_port = *first_port;
                    (*next_address)++;
                }
    
                return new_transport_addr;
            
            }else{
                return NULL;
            }
        }
    }
    // is not empty
    else{
        // get the last address of the list
        struct list_head *prev = head->prev;
        struct transport_addr_struct *transport_addr = list_entry(prev, struct transport_addr_struct, list);
        list_del(prev);
        return transport_addr;
    }
}

struct transport_addr_struct *get_udp_transport_addr(void)
{
  return get_transport_addr(&free_udp_transport_addr, &next_udp_address,
							&next_udp_port, &first_udp_port, &last_udp_port);
}

struct transport_addr_struct *get_tcp_transport_addr(void)
{
  return get_transport_addr(&free_tcp_transport_addr, &next_tcp_address,
							&next_tcp_port, &first_tcp_port, &last_tcp_port);
}


void return_transport_addr(struct transport_addr_struct *transport_addr, struct list_head *head)
{
    INIT_LIST_HEAD(&transport_addr->list);
    list_add(&transport_addr->list, head);
}

void return_tcp_transport_addr(struct transport_addr_struct *transport_addr)
{
  return_transport_addr(transport_addr, &free_tcp_transport_addr);
}

void return_udp_transpsort_addr(struct transport_addr_struct *transport_addr)
{
  return_transport_addr(transport_addr, &free_udp_transport_addr);
}

//~ void init_pools(void)
void init_pools(struct config_struct *cs)
{
    __be32 r1,r2;
    char *add1;
    char *add2;

    //~ in4_pton(IPV4_DEF_POOL_FIRST, -1, (u8 *)&next_udp_address, '\x0', NULL);
    //~ in4_pton(IPV4_DEF_POOL_FIRST, -1, (u8 *)&next_tcp_address, '\x0', NULL);
	next_udp_address = (*cs).ipv4_pool_range_first.s_addr;
	next_tcp_address = (*cs).ipv4_pool_range_first.s_addr;

    next_udp_address = swap_endians(next_udp_address);
    next_tcp_address = swap_endians(next_tcp_address);

    //~ in4_pton(IPV4_DEF_POOL_LAST, -1, (u8 *)&last_address, '\x0', NULL);
    last_address = (*cs).ipv4_pool_range_last.s_addr;
    last_address = swap_endians(last_address);

    //~ first_tcp_port = IPV4_DEF_TCP_POOL_FIRST; // FIRST_PORT;
    //~ first_udp_port = IPV4_DEF_UDP_POOL_FIRST;
    //~ next_udp_port = first_udp_port;
    //~ next_tcp_port = first_tcp_port;
    //~ last_tcp_port = IPV4_DEF_TCP_POOL_LAST; // LAST_PORT;
	//~ last_udp_port = IPV4_DEF_UDP_POOL_LAST;
	first_tcp_port = (*cs).ipv4_tcp_port_first; // FIRST_PORT;
    first_udp_port = (*cs).ipv4_tcp_port_first;
    next_udp_port = first_udp_port;
    next_tcp_port = first_tcp_port;
    last_tcp_port = (*cs).ipv4_tcp_port_last; // LAST_PORT;
	last_udp_port = (*cs).ipv4_udp_port_last;
       
    r1 = swap_endians(next_udp_address);
    r2 = swap_endians(last_address);
    add1 = ip_address_to_string(r1);
    add2 = ip_address_to_string(r2);
    pr_debug("NAT64: Configuring IPv4 pool.");
    pr_debug("NAT64:	First address: %s - Last address: %s\n", add1, add2);
    pr_debug("NAT64:	First UDP port: %u - Last port: %u\n", first_udp_port, last_udp_port);
    pr_debug("NAT64:	First TCP port: %u - Last port: %u\n", first_tcp_port, last_tcp_port);

    INIT_LIST_HEAD(&free_udp_transport_addr);
    INIT_LIST_HEAD(&free_tcp_transport_addr);
}
