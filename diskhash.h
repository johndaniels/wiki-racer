#include <stdint.h>

#ifndef __DISK_HASH_H__
#define __DISK_HASH_H__

uint32_t disk_hash_get_id(xmlChar* string);
void disk_hash_set_current(uint32_t id);
void disk_hash_add_link(uint32_t link);
void disk_hash_init();
void disk_hash_dump(const char* filename);

#endif //__DISK_HASH_H__
