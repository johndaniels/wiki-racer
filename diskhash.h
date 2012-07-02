#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libxml/xmlstring.h>

#ifndef __DISK_HASH_H__
#define __DISK_HASH_H__

typedef struct _disk_hash disk_hash_t; 

disk_hash_t* disk_hash_create(
        uint32_t max_articles, 
        uint32_t max_buckets,
        uint32_t max_string_data, 
        uint32_t max_article_links);
disk_hash_t *disk_hash_load(const char* filename);

disk_hash_t *disk_hash_compress(disk_hash_t *hash);

void disk_hash_print_stats(disk_hash_t* hash);
void disk_hash_free(disk_hash_t* hash);
uint32_t disk_hash_get_id(disk_hash_t *hash, xmlChar* string);
uint32_t disk_hash_create_id(disk_hash_t *hash, xmlChar* string);
uint32_t disk_hash_get_size(disk_hash_t *hash);
uint32_t* disk_hash_get_links(disk_hash_t *hash, uint32_t id, size_t *size); 
uint32_t disk_hash_get_size(disk_hash_t *hash);
void disk_hash_set_current(disk_hash_t *hash, uint32_t id);
void disk_hash_set_redirect(disk_hash_t *hash, uint32_t id, bool redirect);
void disk_hash_add_link(disk_hash_t *hash, uint32_t link);
void disk_hash_init(disk_hash_t *hash);
void disk_hash_dump(disk_hash_t *hash, const char* filename);
xmlChar* disk_hash_get_string(disk_hash_t *hash, uint32_t id);

#endif //__DISK_HASH_H__
