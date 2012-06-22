#include "linkparse.h"
#include "diskhash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 10000
static xmlChar buf[BUF_SIZE];

static uint32_t parse_single(const xmlChar *content, size_t pos) {
    size_t original_position = pos;
    while (content[pos] != ']' && content[pos] != '\0' && content[pos] != '|' && content[pos] != ':') {
        pos++;
    }
    if (content[pos] == ':') {
        return original_position;
    }

    size_t bytes = pos - original_position;
    if (bytes == 0) {
        return original_position;
    }

    if (bytes >= BUF_SIZE) {
        perror("OMG too many bytes!");
        exit(1);
    }
    memcpy(buf, content + original_position, bytes);
    buf[bytes] = '\0';
    uint32_t target_id = disk_hash_get_id(buf);
    disk_hash_add_link(target_id);
    return pos;
}

void parse_links(const xmlChar *content) {
    size_t pos = 0;
    while (content[pos] != '\0') {
        if (content[pos] == '[' && content[pos+1] == '[') {
            pos = parse_single(content, pos + 2);
        } else {
            pos++;
        }
    }
}
