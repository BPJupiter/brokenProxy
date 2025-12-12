object_files=""
gcc Callisto/c_mem_debug.c -c $@ && object_files+="c_mem_debug.o "
gcc Callisto/c_defer.c -c $@ && object_files+="c_defer.o "

gcc dnslookup.c -c $@ && object_files+="dnslookup.o "

gcc $object_files -o test $@

rm $object_files
