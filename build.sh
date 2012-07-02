gcc -O2 --std=c99 -Wall -Wextra -I/usr/include/libxml2 parse.c diskhash.c linkparse.c -lxml2 -o parse
gcc -g --std=c99 -Wall -Wextra race.c diskhash.c -o race
