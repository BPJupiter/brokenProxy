#FLAGS:
# DNS_DEBUG
# DNS_PROGRAM
# C_MEMORY_DEBUG
# ROOT_SERVER=[A...M]
# NO_TRACERT
# PING

object_files=""
gcc Callisto/c_mem_debug.c -c $@ && object_files+="c_mem_debug.o "
gcc Callisto/c_defer.c -c $@ && object_files+="c_defer.o "

gcc cjson/cJSON.c -c $@ && object_files+="cJSON.o "

gcc traceroute.c -c $@ && object_files+="traceroute.o "
gcc dnslookup.c -c $@ && object_files+="dnslookup.o "
gcc proxy.c -c $@ && object_files+="proxy.o "

gcc main.c -c $@ && object_files+="main.o "

gcc $object_files -o test $@ -lm -lpthread -g -Wall -Wextra

rm $object_files
