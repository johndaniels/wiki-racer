#include <stdio.h>
#include <stdlib.h>
#include "diskhash.h"

#define MAX_QUEUE_ITEMS 256000000

typedef struct _node_data {
    uint32_t prev;
    uint32_t distance;
} node_data_t;

typedef struct _queue_data {
    uint32_t current;
} queue_data_t;

queue_data_t *queue;
node_data_t *data;
size_t queue_front;
size_t queue_back;
uint32_t size;

size_t queue_size() {
    return queue_back - queue_front;
}

queue_data_t queue_pop() {
    if (queue_front == queue_back) {
        perror("QUEUE IS EMPTY");
        exit(1);
    }
    queue_data_t ret = queue[queue_front];
    queue_front++;
    return ret;
}

void queue_push(queue_data_t data) {
    if (queue_back >= MAX_QUEUE_ITEMS) {
        perror("QUEUE OVERFLOWED");
        exit(1);
    }
    queue[queue_back] = data;
    queue_back++;
}

void print_chain(disk_hash_t *hash, uint32_t final) {
    uint32_t current = final;
    puts("FOUND\n");
    while (data[current].prev != (uint32_t)-1) {
        printf("%s\n", disk_hash_get_string(hash, current));
        current = data[current].prev;
    }
    printf("%s\n", disk_hash_get_string(hash, current));
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: race datafile.dat start end");
    }
    disk_hash_t *hash = disk_hash_load(argv[1]);
    size = disk_hash_get_size(hash);
    uint32_t start = disk_hash_get_id(hash, BAD_CAST(argv[2]));
    uint32_t end = disk_hash_get_id(hash, BAD_CAST(argv[3]));
    data = calloc(size, sizeof(node_data_t));
    queue = malloc(MAX_QUEUE_ITEMS * sizeof(queue_data_t));
    for (uint32_t i=0; i<size; i++) {
        data[i].distance = (uint32_t)-1;
        data[i].prev = (uint32_t)-1;
    }
    queue_front = 0;
    queue_back = 0;
    data[start].distance = 0;
    queue_data_t current;
    current.current = start;
    queue_push(current);
    while (queue_size() != 0) {
        queue_data_t current = queue_pop();
        if (current.current == end) {
            print_chain(hash, current.current);
            break;
        }
        size_t links_len;
        uint32_t *links = disk_hash_get_links(hash, current.current, &links_len);
        for (size_t i = 0; i < links_len; i++) {
            if (data[links[i]].distance != (uint32_t)-1) {
                continue;
            }
            queue_data_t next;
            next.current = links[i];
            data[links[i]].distance = data[current.current].distance + 1;
            data[links[i]].prev = current.current;
            queue_push(next);
        }
    }
    return 0;
}
