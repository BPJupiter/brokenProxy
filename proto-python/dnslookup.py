import dns.message
import dns.query
import dns.rdatatype
import dns.resolver

import subprocess

from iocolour import *
from traceroute import traceroute

ROOT_IP_A = '198.41.0.4'
ROOT_IP_B = '170.247.170.2'
ROOT_IP_C = '192.33.4.12'
ROOT_IP_D = '199.7.91.13'
ROOT_IP_E = '192.203.230.10'
ROOT_IP_F = '192.5.5.241'
ROOT_IP_G = '192.112.36.4'
ROOT_IP_H = '198.97.190.53'
ROOT_IP_I = '192.36.148.17'
ROOT_IP_J = '192.58.128.30'
ROOT_IP_K = '193.0.14.129'
ROOT_IP_L = '199.7.83.42'
ROOT_IP_M = '202.12.27.33'

ROOT_IP_DICT = {
    "a": ROOT_IP_A,
    "b": ROOT_IP_B,
    "c": ROOT_IP_C,
    "d": ROOT_IP_D,
    "e": ROOT_IP_E,
    "f": ROOT_IP_F,
    "g": ROOT_IP_G,
    "h": ROOT_IP_H,
    "i": ROOT_IP_I,
    "j": ROOT_IP_J,
    "k": ROOT_IP_K,
    "l": ROOT_IP_L,
    "m": ROOT_IP_M
}

def dns_resolve(root_server, hostname):
    current_nameserver = ROOT_IP_DICT[root_server]
    query_name = dns.name.from_text(hostname)
    print(f"{DNS_TAG} Starting iterative resolution for: {hostname}")

    while True:
        # 1. Create a query message for an A record (IPv4 address)
        query = dns.message.make_query(query_name, dns.rdatatype.A)
        query.flags ^= dns.flags.RD

        # TODO: Query database to avoid excessive traceroutes

        # 1.9. Traceroute current nameserver
        tr_output = traceroute(current_nameserver)

        # TODO: Determine cable and whether or not to forward packet

        print(f"{DNS_TAG} Querying NS: {current_nameserver}")

        try:
            # 2. Send the query via UDP
            response = dns.query.udp(query, current_nameserver, timeout=5)
        except dns.exception.Timeout as e:
            print(f"{DNS_TAG} {TIMEOUT_TAG} Query timeout for {query_name} at {current_nameserver}")
            break
        except Exception as e:
            print(f"{DNS_TAG} {ERR_TAG} Query failed for {query_name} at {current_nameserver}: {e}")
            break

        # 3. Check the response sections

        # Check if we found the final answer
        if response.answer:
            print(f"{DNS_TAG} {bcolours.OKGREEN}[Success]{bcolours.ENDC} Found the answer")
            for rrset in response.answer:
                return str(rrset[0])
            break

        # If not in the answer section, look for referrals in the authority section
        elif response.authority:

            # Find a nameserver record in the authority section
            next_ns_name = None
            for rrset in response.authority:
                if rrset.rdtype == dns.rdatatype.NS:
                    next_ns_name = str(rrset[0])
                    print(f"{DNS_TAG} Recieved referral to TLD/Authoritative NS: {next_ns_name}")
                    break

            if next_ns_name:
                # Look for the IP address of the next nameserver in the additional section (glue records)
                found_ip = False
                for additional_rr in response.additional:
                    if additional_rr.name.to_text() == next_ns_name and additional_rr.rdtype == dns.rdatatype.A:
                        current_nameserver = str(additional_rr[0])
                        print(f"{DNS_TAG} Found glue record IP: {current_nameserver}")
                        found_ip = True
                        break
                if not found_ip:
                    print(f"{DNS_TAG} {bcolours.WARNING}No glue record found. Recursively resolve NS IP: {next_ns_name}{bcolours.ENDC}")
                    current_nameserver = dns_resolve(root_server, next_ns_name)
                    continue
            else:
                print(f"{DNS_TAG} {ERR_TAG} Authority records found, but no NS referral. Possible NXDOMAIN.")
                break
        else:
            print(f"{DNS_TAG} {ERR_TAG} No answwer or authority records found. Cannot resolve iteratively.")

    return None
