#FLAGS:
# DNS_DEBUG
# DNS_PROGRAM
# C_MEMORY_DEBUG

object_files=""
flags="-D DNS_DEBUG -D DNS_PROGRAM -std=gnu89 -pedantic -Wall -Wextra "
gcc Callisto/c_mem_debug.c -c $@ $flags && object_files+="c_mem_debug.o "
gcc Callisto/c_defer.c -c $@ $flags && object_files+="c_defer.o "

gcc traceroute.c -c $@ $flags && object_files+="traceroute.o "
gcc dnslookup.c -c $@ $flags && object_files+="dnslookup.o "

#gcc proxy.c -c $@ && object_files+="proxy.o "

gcc $object_files $flags -o dnstest $@ -lm -lpthread -g -Wall -Wextra

rm $object_files
