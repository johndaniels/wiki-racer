#include <stdint.h>
#include <libxml/xmlstring.h>
#include "diskhash.h"

#ifndef __LINK_BUFFER_H__
#define __LINK_BUFFER_H__

void parse_links(disk_hash_t *hash, const xmlChar *content);

#endif //__LINK_BUFFER_H__
