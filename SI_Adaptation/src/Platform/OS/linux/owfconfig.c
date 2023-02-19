/* Copyright (c) 2009 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "owfconfig.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_CONFIG "openwf_config.xml"

OWF_CONF_DOCUMENT
OWF_Conf_GetGetDocument(const char *str) {
    xmlDoc *doc = NULL;

    if (str) {
        doc = xmlReadFile(str, NULL, 0);
    }

    if (!doc) {
        doc = xmlReadFile(DEFAULT_CONFIG, NULL, 0);
    }

    return doc;
}

OWF_CONF_GROUP
OWF_Conf_GetRoot(OWF_CONF_DOCUMENT doc, const char *elementName) {
    xmlNode *root = NULL;

    root = xmlDocGetRootElement(doc);

    if (root == NULL) {
        fprintf(stderr, "error: empty document\n");
        return NULL;
    }

    if (xmlStrcmp(root->name, (const xmlChar *)elementName)) {
        fprintf(stderr, "error: document of the wrong type, root node != %s",
                elementName);
        return NULL;
    }

    return root;
}

OWF_CONF_ELEMENT
OWF_Conf_GetElement(const OWF_CONF_GROUP cur, const char *elementName) {
    xmlNode *child = (cur) ? ((xmlNode *)cur)->xmlChildrenNode : NULL;

    while (child != NULL) {
        if ((xmlStrcmp(child->name, (const xmlChar *)elementName) == 0)) {
            return child;
        }

        child = child->next;
    }

    return NULL;
}

OWFint OWF_Conf_GetNbrElements(const OWF_CONF_GROUP cur,
                               const char *elementName) {
    OWFint nbr = 0;

    xmlNode *child = (cur) ? ((xmlNode *)cur)->xmlChildrenNode : NULL;

    while (child != NULL) {
        if (elementName) {
            if ((xmlStrcmp(child->name, (const xmlChar *)elementName) == 0)) {
                nbr++;
            }
        } else {
            nbr++;
        }

        child = child->next;
    }

    return nbr;
}

OWF_CONF_ELEMENT
OWF_Conf_GetNextElement(const OWF_CONF_ELEMENT cur, const char *elementName) {
    xmlNode *sib = (cur) ? ((xmlNode *)cur)->next : NULL;

    while (sib != NULL) {
        if ((xmlStrcmp(sib->name, (const xmlChar *)elementName) == 0)) {
            return sib;
        }

        sib = sib->next;
    }

    return NULL;
}

OWFint OWF_Conf_GetContenti(const OWF_CONF_ELEMENT cur, OWFint deflt) {
    xmlChar *str = NULL;

    if (cur) {
        str = xmlNodeGetContent(((xmlNode *)cur)->xmlChildrenNode);
    }

    if (str) {
        OWFint base;
        OWFint value;
        const xmlChar *c = xmlStrstr(str, (xmlChar *)"0x");

        base = (c) ? 16 : 10;
        value = (OWFint)strtol((char *)str, NULL, base);
        xmlFree(str);
        return value;
    } else {
        return deflt;
    }
}

OWFfloat OWF_Conf_GetContentf(const OWF_CONF_ELEMENT cur, OWFfloat deflt) {
    xmlChar *str = NULL;

    if (cur) {
        str = xmlNodeGetContent(((xmlNode *)cur)->xmlChildrenNode);
    }
    if (str) {
        OWFfloat value;

        value = (OWFfloat)atof((char *)str);
        xmlFree(str);
        return value;
    } else {
        return deflt;
    }
}

char *OWF_Conf_GetContentStr(const OWF_CONF_ELEMENT cur, char *deflt) {
    xmlChar *str = NULL;

    if (cur) str = xmlNodeGetContent(((xmlNode *)cur)->xmlChildrenNode);
    return (str) ? (char *)str : deflt;
}

void OWF_Conf_FreeContent(char *str) { xmlFree(str); }

OWFint OWF_Conf_GetElementContenti(const OWF_CONF_GROUP cur,
                                   const char *elementName, OWFint deflt) {
    return OWF_Conf_GetContenti(OWF_Conf_GetElement(cur, elementName), deflt);
}

OWFfloat OWF_Conf_GetElementContentf(const OWF_CONF_GROUP cur,
                                     const char *elementName, OWFfloat deflt) {
    return OWF_Conf_GetContentf(OWF_Conf_GetElement(cur, elementName), deflt);
}

char *OWF_Conf_GetElementContentStr(const OWF_CONF_GROUP cur,
                                    const char *elementName, char *deflt) {
    return OWF_Conf_GetContentStr(OWF_Conf_GetElement(cur, elementName), deflt);
}

void OWF_Conf_Cleanup(OWF_CONF_DOCUMENT doc) {
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

#ifdef __cplusplus
}
#endif
