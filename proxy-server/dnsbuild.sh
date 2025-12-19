#FLAGS:
# DNS_DEBUG
# DNS_PROGRAM
# C_MEMORY_DEBUG

object_files=""
gcc Callisto/c_mem_debug.c -c $@ && object_files+="c_mem_debug.o "
gcc Callisto/c_defer.c -c $@ && object_files+="c_defer.o "

gcc traceroute.c -c $@ && object_files+="traceroute.o "
gcc dnslookup.c -c $@ && object_files+="dnslookup.o "

#gcc proxy.c -c $@ && object_files+="proxy.o "

gcc $object_files -o dnstest $@ -lm -lpthread -g -Wall -Wextra

rm $object_files
