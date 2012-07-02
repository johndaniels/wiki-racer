#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/xmlstring.h>

#include "diskhash.h"

enum {
    ARTICLE_EXISTS = 1,
    ARTICLE_IS_REDIRECT = 2
};

typedef struct _article_data {
    uint32_t string_index;
    uint32_t link_index;
    uint16_t links_len;
    uint16_t flags;
} article_data_t;

struct _disk_hash {
    article_data_t *article_data;
    uint32_t article_pos;
    uint32_t max_articles;
    uint32_t *hash_to_id;
    uint32_t max_buckets;
    xmlChar *string_data;
    uint32_t string_pos;
    uint32_t max_string_data;
    uint32_t *link_data;
    uint32_t link_pos;
    uint32_t max_links;
    uint32_t current_id;
};

const uint32_t EMPTY = (uint32_t) -1;

disk_hash_t* disk_hash_create(
        uint32_t max_articles, 
        uint32_t max_buckets,
        uint32_t max_string_data, 
        uint32_t max_article_links)
{
    disk_hash_t *hash = calloc(1, sizeof(disk_hash_t));
    hash->article_data = malloc(max_articles * sizeof(article_data_t));
    hash->max_articles = max_articles;
    hash->hash_to_id = malloc(max_buckets * sizeof(disk_hash_t));
    for (uint32_t i = 0; i < max_buckets; i++) {
        hash->hash_to_id[i] = EMPTY;
    }
    hash->max_buckets = max_buckets;
    hash->string_data = malloc(max_string_data * sizeof(xmlChar));
    hash->max_string_data = max_string_data;
    hash->link_data = malloc(max_article_links * sizeof(uint32_t));
    hash->max_links = max_article_links;
    return hash;
}

void disk_hash_free(disk_hash_t* hash) {
    free(hash->article_data);
    free(hash->hash_to_id);
    free(hash->string_data);
    free(hash->link_data);
    free(hash);
}

static uint32_t get_hash(const xmlChar* data) {
    uint32_t result = 0;
    unsigned char character;
    while ((character = *(data++)) != 0) {
        result = 65599 * result + character;
    };
    return result;
}

static uint32_t alloc_chunk(disk_hash_t *hash, size_t chunk_size) {
    uint32_t ret = hash->string_pos;
    hash->string_pos += chunk_size;
    if (hash->string_pos >= hash->max_string_data) {
        perror("RAN OUT OF DATA");
        exit(1);
    }
    return ret;
}

static uint32_t disk_hash_get_bucket(disk_hash_t *hash, xmlChar *string) {
    size_t start = get_hash(string) % hash->max_buckets;
    size_t cur = start;
    do {
        uint32_t possible_id = hash->hash_to_id[cur];
        if (possible_id == EMPTY) {
            return cur;
        }
        xmlChar* possible_title = &hash->string_data[hash->article_data[possible_id].string_index];
        if (xmlStrcmp(string, possible_title) == 0) {
            return cur;
        }
        cur++;
        cur %= hash->max_buckets;
    } while (cur != start);
    return EMPTY;
}

void disk_hash_dump(disk_hash_t *hash, const char* filename) {
    FILE *file = fopen(filename, "wb");
    uint32_t string_loc = 2 * sizeof(uint32_t) + hash->article_pos * sizeof(article_data_t);
    fwrite(&string_loc, sizeof(uint32_t), 1, file);
    uint32_t link_loc = string_loc + hash->string_pos * sizeof(xmlChar);
    fwrite(&link_loc, sizeof(uint32_t), 1, file);
    fwrite(hash->article_data, sizeof(article_data_t), hash->article_pos, file);
    fwrite(hash->string_data, sizeof(xmlChar), hash->string_pos, file);
    fwrite(hash->link_data, sizeof(uint32_t), hash->link_pos, file);
    fclose(file);
}

disk_hash_t *disk_hash_load(const char* filename) {
    disk_hash_t *hash = calloc(1, sizeof(disk_hash_t));
    FILE *file = fopen(filename, "rb");
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    
    uint32_t string_loc;
    size_t bytes_read = fread(&string_loc, sizeof(uint32_t), 1, file);
    uint32_t link_loc;
    bytes_read += fread(&link_loc, sizeof(uint32_t), 1, file);
    
    size_t bytes_to_read = string_loc - 2 * sizeof(uint32_t);
    hash->article_data = malloc(bytes_to_read);
    hash->article_pos = bytes_to_read / sizeof(article_data_t);
    hash->max_articles = hash->article_pos;
    bytes_read += fread(hash->article_data, 1, bytes_to_read, file);

    bytes_to_read = link_loc - string_loc;
    hash->string_data = malloc(bytes_to_read);
    hash->string_pos = bytes_to_read;
    hash->max_string_data = bytes_to_read;
    bytes_read += fread(hash->string_data, 1, bytes_to_read, file);
    
    bytes_to_read = file_size - bytes_read;
    hash->link_data = malloc(bytes_to_read);
    hash->link_pos = bytes_read / sizeof(uint32_t);
    hash->max_links = hash->link_pos;
    bytes_read += fread(hash->link_data, 1, bytes_to_read, file);
    if (!feof(file)) {
        perror("FILE TOO LONG");
        exit(1);
    }
    fclose(file);

    hash->max_buckets = hash->article_pos * 2;
    hash->hash_to_id = malloc(hash->max_buckets * sizeof(uint32_t));
    for (uint32_t i=0; i < hash->max_buckets; i++) {
        hash->hash_to_id[i] = EMPTY;
    }
    
    for (uint32_t i=0; i < hash->article_pos; i++) {
        uint32_t bucket = disk_hash_get_bucket(hash, hash->string_data + hash->article_data[i].string_index);
        if (bucket == EMPTY) {
            perror("OUT OF HASH SPACE");
            exit(1);
        }
        
        hash->hash_to_id[bucket] = i;
    }


    return hash;
}

static uint32_t get_redirect_end(disk_hash_t *hash, uint32_t id) {
    uint32_t current = id;
    uint32_t redirect_count = 0;
    while (hash->article_data[current].flags & ARTICLE_IS_REDIRECT) {
        if (hash->article_data[current].links_len == 0) {
            return (uint32_t)-1;
        }
        current = hash->link_data[hash->article_data[current].link_index];
        if (redirect_count > 100) {
            return (uint32_t)-1;
        }
        redirect_count++;
    }
    return current;
}

disk_hash_t *disk_hash_compress(disk_hash_t *hash) {
    uint32_t *redirect_map = malloc(hash->article_pos * sizeof(uint32_t));
    uint32_t *old_to_new = malloc(hash->article_pos * sizeof(uint32_t));
    disk_hash_t *compressed = disk_hash_create(hash->article_pos, 
            hash->article_pos * 3, 
            hash->string_pos, 
            hash->link_pos);

    for (uint32_t i = 0; i < hash->article_pos; i++) {
        article_data_t data = hash->article_data[i];
        if (data.flags & ARTICLE_EXISTS && !(data.flags & ARTICLE_IS_REDIRECT)) {
            old_to_new[i] = disk_hash_create_id(compressed, hash->string_data + data.string_index);
        } else {
            old_to_new[i] = EMPTY;
        }
        redirect_map[i] = get_redirect_end(hash, i);
    }

    for (uint32_t i = 0; i < hash->article_pos; i++) {
        if (old_to_new[i] == EMPTY) {
            continue;
        }
        article_data_t old_data = hash->article_data[i];
        disk_hash_set_current(compressed, old_to_new[i]);
        for (uint32_t j = 0; j < old_data.links_len; j++) { 
            uint32_t link = redirect_map[hash->link_data[old_data.link_index + j]];
            if (link != (uint32_t)-1 && old_to_new[link] != (uint32_t)-1) {
                disk_hash_add_link(compressed, old_to_new[link]);
            }
        }
    }
    printf("Compressing articles: %u to %u\n", hash->article_pos, compressed->article_pos);
    disk_hash_free(hash);
    free(redirect_map);
    free(old_to_new);

    return compressed;
}

void disk_hash_add_link(disk_hash_t *hash, uint32_t link) {
    if (hash->link_pos >= hash->max_links) {
        perror("RAN OUT OF LINK SPACE");
        exit(1);
    }
    for (uint32_t i=0; i < hash->article_data[hash->current_id].links_len; i++) {
        uint32_t existing_link = hash->link_data[hash->article_data[hash->current_id].link_index + i];
        if (existing_link == link) {
            return;
        }
    }
    hash->link_data[hash->link_pos] = link;
    hash->article_data[hash->current_id].links_len++;
    hash->link_pos++;
}

void disk_hash_set_current(disk_hash_t *hash, uint32_t id) {
    hash->current_id = id;
    hash->article_data[hash->current_id].links_len = 0;
    hash->article_data[hash->current_id].link_index = hash->link_pos;
    hash->article_data[hash->current_id].flags |= ARTICLE_EXISTS;
}

void disk_hash_set_redirect(disk_hash_t *hash, uint32_t id, bool redirect) {
    if (redirect) {
        hash->article_data[id].flags |= ARTICLE_IS_REDIRECT;
    } else {
        hash->article_data[id].flags &= (~ARTICLE_IS_REDIRECT);
    }
}

void disk_hash_print_stats(disk_hash_t *hash) {
    printf("%u %u %u\n", hash->article_pos, hash->string_pos, hash->link_pos);
}

uint32_t disk_hash_create_id(disk_hash_t *hash, xmlChar * string) {
    uint32_t bucket = disk_hash_get_bucket(hash, string);
    if (bucket == EMPTY) {
        perror("OUT OF HASH SPACE");
        exit(1);
    }
    if (hash->hash_to_id[bucket] != EMPTY) {
        return hash->hash_to_id[bucket];
    }

    if (hash->article_pos >= hash->max_articles) {
        perror("OUT OF ARTICLE_SPACE");
        exit(1);
    }

    size_t len = xmlStrlen(string) + 1;
    uint32_t string_index = alloc_chunk(hash, len);
    memcpy(&(hash->string_data[string_index]), string, len);
    hash->article_data[hash->article_pos].string_index = string_index;
    hash->article_data[hash->article_pos].link_index = EMPTY;
    hash->article_data[hash->article_pos].flags = 0;
    hash->hash_to_id[bucket] = hash->article_pos;
    
    uint32_t ret = hash->article_pos;
    hash->article_pos++;
    return ret;
}

uint32_t disk_hash_get_id(disk_hash_t *hash, xmlChar* string) {
    uint32_t bucket = disk_hash_get_bucket(hash, string);
    if (bucket == EMPTY) {
        return EMPTY;
    }
    return hash->hash_to_id[bucket];
}

uint32_t disk_hash_get_size(disk_hash_t* hash) {
    return hash->article_pos;
}

uint32_t* disk_hash_get_links(disk_hash_t *hash, uint32_t id, size_t *size) {
    *size = hash->article_data[id].links_len;
    return hash->link_data + hash->article_data[id].link_index;
}

xmlChar* disk_hash_get_string(disk_hash_t *hash, uint32_t id) {
    return hash->string_data + hash->article_data[id].string_index;
}
