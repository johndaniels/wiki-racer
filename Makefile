all: race parse

race: race.c diskhash.c diskhash.h
	gcc -g --std=c99 -Wall -Wextra -I/usr/include/libxml2 race.c diskhash.c -lxml2 -o race

parse: parse.c diskhash.c diskhash.h linkparse.c linkparse.h
	gcc -g --std=c99 -Wall -Wextra -I/usr/include/libxml2 parse.c diskhash.c linkparse.c -lxml2 -o parse
