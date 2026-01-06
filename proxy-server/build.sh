#FLAGS:
# DNS_DEBUG
# C_MEMORY_DEBUG
# NO_TRACERT
# PING

object_files=""
flags="-std=gnu89 -pedantic -Wall -Wextra "
gcc Callisto/c_mem_debug.c -c $@ $flags && object_files+="c_mem_debug.o "
gcc Callisto/c_defer.c -c $@ $flags && object_files+="c_defer.o "

gcc cjson/cJSON.c -c $@ $flags && object_files+="cJSON.o "

gcc traceroute.c -c $@ $flags && object_files+="traceroute.o "
gcc dnslookup.c -c $@ $flags && object_files+="dnslookup.o "
gcc proxy.c -c $@ $flags && object_files+="proxy.o "

gcc main.c -c $@ $flags && object_files+="main.o "

gcc $object_files -o test $@ $flags -lm -lpthread

rm $object_files
