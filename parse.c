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
    return textChildPtr->children->content;
}

int find_namespace(xmlNodePtr pageNode) {
    xmlNodePtr nsChildPtr = child_element_by_name(pageNode, NAMESPACE);
    return atoi((char*)nsChildPtr->children->content);
}

void processNode(xmlTextReaderPtr reader) {
    if (xmlTextReaderNodeType(reader) == 1) {
        xmlChar *localName = xmlTextReaderLocalName(reader);
        if (xmlStrcmp(localName, PAGE) == 0) {
            xmlNodePtr ptr = xmlTextReaderExpand(reader);
            int namespace = find_namespace(ptr);
            xmlChar *title = find_title(ptr);
            if (namespace == 0) {
                uint32_t id = disk_hash_get_id(title);
                disk_hash_set_current(id);
                xmlChar *text = find_text(ptr);
                parse_links(text);

            } else {
                if (namespace != 4 && namespace != 12 && namespace != 6 && namespace != 100 && namespace != 10 && namespace != 8 && namespace != 14 && namespace != 108) {
                    printf("%d %s\n", namespace, title);
                }
            }
        }
        xmlFree(localName);
    }
}

void parse_xml_file(const char *filename, const char *output_file) {
    xmlTextReaderPtr reader;
    int ret;

    reader = xmlNewTextReaderFilename(filename);
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        long bytes_milestone = 0;
        while (ret == 1) {
            processNode(reader);
            ret = xmlTextReaderRead(reader);
            long bytes = xmlTextReaderByteConsumed(reader);
            if (bytes > bytes_milestone) {
                printf("megabytes: %ld\n", bytes/1000000);
                bytes_milestone += 100000000;
            }
        }
        xmlFreeTextReader(reader);
        if (ret != 0) {
            printf("%s : failed to parse\n", filename);
        }
        disk_hash_dump(output_file);
    } else {
        printf("Unable to open %s\n", filename);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("USAGE: parse file.xml output.dat\n");
        exit(0);
    }
    
    disk_hash_init();

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
