#include <libxml/xmlstring.h>

#include <libxml/parser.h>
#include <libxml/xmlreader.h>

#include "diskhash.h"
#include "linkparse.h"

static const xmlChar *TITLE = BAD_CAST("title");
static const xmlChar *TEXT = BAD_CAST("text");
static const xmlChar *REVISION = BAD_CAST("revision");
static const xmlChar *NAMESPACE = BAD_CAST("ns");
static const xmlChar *PAGE = BAD_CAST("page");
static const xmlChar *REDIRECT = BAD_CAST("redirect");

xmlNodePtr child_element_by_name(xmlNodePtr ptr, const xmlChar *name) {
    xmlNodePtr child = xmlFirstElementChild(ptr);
    while (child != NULL) {
        if (xmlStrcmp(child->name, name) == 0) {
            return child;
        }
        child = xmlNextElementSibling(child);
    }
    return NULL;
}

xmlChar* find_title(xmlNodePtr pageNode) {
    xmlNodePtr titleChildPtr = child_element_by_name(pageNode, TITLE);
    return titleChildPtr->children->content;
}

xmlChar* find_text(xmlNodePtr pageNode) {
    xmlNodePtr revisionChildPtr = child_element_by_name(pageNode, REVISION);
    xmlNodePtr textChildPtr = child_element_by_name(revisionChildPtr, TEXT);
    if (textChildPtr->children) {
        return textChildPtr->children->content;
    }
    return BAD_CAST("");


bool is_redirect(xmlNodePtr pageNode) {
	xmlNodePtr redirectChildPtr = child_element_by_name(pageNode, REDIRECT);
	return redirectChildPtr != NULL;
}

int find_namespace(xmlNodePtr pageNode) {
    xmlNodePtr nsChildPtr = child_element_by_name(pageNode, NAMESPACE);
    return atoi((char*)nsChildPtr->children->content);
}

void add_article_from_element(xmlNodePtr pageNode, disk_hash_t *hash) {
    xmlChar *title = find_title(pageNode);
    disk_hash_create_id(hash, title);
}

void add_article_links_from_element(xmlNodePtr pageNode, disk_hash_t *hash) {
    xmlChar *title = find_title(pageNode);
    uint32_t id = disk_hash_get_id(hash, title);
    if (id == (uint32_t)-1) {
        perror("NO IDEA FOR ARTICLE");
        exit(1);
    }
    disk_hash_set_current(hash, id);
    disk_hash_set_redirect(hash, id, is_redirect(pageNode));
    xmlChar *text = find_text(pageNode);
    parse_links(hash, text);
}

void processNode(xmlTextReaderPtr reader, disk_hash_t *hash, void (*page_func)(xmlNodePtr, disk_hash_t*)) {
    if (xmlTextReaderNodeType(reader) == 1) {
        xmlChar *localName = xmlTextReaderLocalName(reader);
        if (xmlStrcmp(localName, PAGE) == 0) {
            xmlNodePtr ptr = xmlTextReaderExpand(reader);
            int namespace = find_namespace(ptr);
            if (namespace == 0) {
                page_func(ptr, hash);
            }
        }
        xmlFree(localName);
    }
}

void xml_file_pass(const char *filename, disk_hash_t *hash, void (*page_func)(xmlNodePtr, disk_hash_t*)) {
    xmlTextReaderPtr reader;
    int ret;

    reader = xmlNewTextReaderFilename(filename);
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        long bytes_milestone = 0;
        while (ret == 1) {
            processNode(reader, hash, page_func);
            ret = xmlTextReaderRead(reader);
            long bytes = xmlTextReaderByteConsumed(reader);
            if (bytes > bytes_milestone) {
                printf("megabytes: %ld\n", bytes/1000000);
                disk_hash_print_stats(hash);
                bytes_milestone += 100000000;
            }
        }
        xmlFreeTextReader(reader);
        if (ret != 0) {
            printf("%s : failed to parse\n", filename);
        }
    } else {
        printf("Unable to open %s\n", filename);
    }
}

void parse_xml_file(const char *filename, const char *output_file) {
    disk_hash_t *hash = disk_hash_create(50000000, 100000000, 1000000000, 400000000);
    xml_file_pass(filename, hash, &add_article_from_element);
    xml_file_pass(filename, hash, &add_article_links_from_element);
    hash = disk_hash_compress(hash);
    disk_hash_dump(hash, output_file);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("USAGE: parse file.xml output.dat\n");
        exit(0);
    }
    

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION
    
    parse_xml_file(argv[1], argv[2]);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return 0;
}
