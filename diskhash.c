#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/xmlstring.h>

#include "diskhash.h"

#define NUM_ARTICLES 20000000
#define NUM_BUCKETS NUM_ARTICLES * 2
#define MAX_STRING_DATA 512000000
#define MAX_ARTICLE_LINKS 100000000

typedef struct _article_data {
    uint32_t string_index;
    uint32_t link_index;
    uint16_t links_len;
    uint16_t flags;
} article_data_t;

const uint32_t EMPTY = (uint32_t) -1;

static uint32_t hash_to_id[NUM_BUCKETS];
static article_data_t article_data[NUM_ARTICLES];
static xmlChar string_data[MAX_STRING_DATA];
static uint32_t link_data[MAX_ARTICLE_LINKS];
static uint32_t current_id = 0;
static uint32_t id_pos = 0;
static size_t string_pos = 0;
static size_t link_pos = 0;

static uint32_t get_hash(const xmlChar* data) {
    uint32_t result = 0;
    unsigned char character;
    while ((character = *(data++)) != 0) {
        result = 65599 * result + character;
    };
    return result;
}

static uint32_t alloc_chunk(size_t chunk_size) {
    uint32_t ret = string_pos;
    string_pos += chunk_size;
    if (string_pos >= MAX_STRING_DATA) {
        perror("RAN OUT OF DATA\n");
        exit(1);
    }
    return ret;
}

void disk_hash_init() {
    for (size_t i = 0; i < NUM_BUCKETS; i++) {
        hash_to_id[i] = EMPTY;
    }
}

static uint32_t disk_hash_get_bucket(xmlChar* string) {
    size_t start = get_hash(string) % NUM_BUCKETS;
    size_t cur = start;
    do {
        uint32_t possible_id = hash_to_id[cur];
        if (possible_id == EMPTY) {
            return cur;
        }
        xmlChar* possible_title = &string_data[article_data[possible_id].string_index];
        if (xmlStrcmp(string, possible_title) == 0) {
            return cur;
        }
        cur++;
        cur %= NUM_BUCKETS;
    } while (cur != start);
    perror("OUT OF SPACE IN HASH");
    exit(1);
}

void disk_hash_dump(const char* filename) {
    FILE *file = fopen(filename, "wb");
    uint32_t string_loc = 2 * sizeof(uint32_t) + id_pos * sizeof(article_data_t);
    fwrite(&string_loc, sizeof(uint32_t), 1, file);
    uint32_t link_loc = string_loc + string_pos * sizeof(xmlChar);
    fwrite(&link_loc, sizeof(uint32_t), 1, file);
    fwrite(&article_data, sizeof(article_data_t), id_pos, file);
    fwrite(&string_data, sizeof(xmlChar), string_pos, file);
    fwrite(&link_data, sizeof(uint32_t), link_pos, file);
    fclose(file);
}

void disk_hash_add_link(uint32_t link) {
    link_data[link_pos] = link;
    article_data[current_id].links_len++;
    link_pos++;
}

void disk_hash_set_current(uint32_t id) {
    current_id = id;
    article_data[current_id].links_len = 0;
    article_data[current_id].link_index = link_pos;
}

uint32_t disk_hash_get_id(xmlChar* string) {
    uint32_t bucket = disk_hash_get_bucket(string);
    if (hash_to_id[bucket] != EMPTY) {
        return hash_to_id[bucket];
    }

    size_t len = xmlStrlen(string) + 1;
    uint32_t string_index = alloc_chunk(len);
    memcpy(&(string_data[string_index]), string, len);
    article_data[id_pos].string_index = string_index;
    article_data[id_pos].link_index = EMPTY;
    article_data[id_pos].flags = 0;
    hash_to_id[bucket] = id_pos;
    
    uint32_t ret = id_pos;
    id_pos++;
    if (id_pos % 100000 == 0) {
        printf("%u %lu\n", id_pos, string_pos);
    }
    return ret;
}

