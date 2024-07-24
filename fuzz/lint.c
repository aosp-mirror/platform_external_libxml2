/*
 * xml.c: a libFuzzer target to test several XML parser interfaces.
 *
 * See Copyright for the status of this software.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <libxml/catalog.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlmemory.h>

#include "fuzz.h"

#define XMLLINT_FUZZ
#include "../xmllint.c"

static const char *const switches[] = {
    "--auto",
    "--c14n",
    "--c14n11",
    "--compress",
    "--copy",
    "--debug",
    "--debugent",
    "--dropdtd",
    "--dtdattr",
    "--exc-c14n",
    "--format",
    "--htmlout",
    "--huge",
    "--insert",
    "--loaddtd",
    "--load-trace",
    "--memory",
    "--noblanks",
    "--nocdata",
    "--nocompact",
    "--nodefdtd",
    "--nodict",
    "--noenc",
    "--noent",
    "--nofixup-base-uris",
    "--nonet",
    "--noout",
    "--nowarning",
    "--nowrap",
    "--noxincludenode",
    "--nsclean",
    "--oldxml10",
    "--pedantic",
    "--postvalid",
    "--push",
    "--pushsmall",
    "--quiet",
    "--recover",
    "--sax1",
    "--testIO",
    "--timing",
    "--valid",
    "--version",
    "--walker",
    "--xinclude",
    "--xmlout"
};
static const size_t numSwitches = sizeof(switches) / sizeof(switches[0]);

struct {
    const char **argv;
    size_t argi;
} vars;

static void
pushArg(const char *str) {
    vars.argv[vars.argi++] = str;
}

int
LLVMFuzzerInitialize(int *argc ATTRIBUTE_UNUSED,
                     char ***argv ATTRIBUTE_UNUSED) {
    int fd;

    /* Redirect stdout to /dev/null */
    fd = open("/dev/null", O_WRONLY);
    if (fd == -1) {
        perror("/dev/null");
        abort();
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        abort();
    }
    close(fd);

    return 0;
}

int
LLVMFuzzerTestOneInput(const char *data, size_t size) {
    char maxmemBuf[20];
    char maxAmplBuf[20];
    char prettyBuf[20];
    const char *sval, *docBuffer, *docUrl;
    size_t ssize, docSize, i;
    unsigned uval;
    int ival;

    vars.argv = malloc((numSwitches + 5 + 6 * 2) * sizeof(vars.argv[0]));
    vars.argi = 0;
    pushArg("xmllint"),
    pushArg("--nocatalogs");

    xmlFuzzDataInit(data, size);

    for (i = 0; i < numSwitches; i++) {
        if (i % 32 == 0)
            uval = xmlFuzzReadInt(4);
        if ((uval & 1) && (switches[i] != NULL))
            pushArg(switches[i]);
        uval >>= 1;
    }

    /*
     * Use four main parsing modes with equal probability
     */
    switch (uval & 3) {
        case 0:
            /* XML parser */
            break;
        case 1:
            /* HTML parser */
            pushArg("--html");
            break;
        case 2:
            /* XML reader */
            pushArg("--stream");
            break;
        case 3:
            /* SAX parser */
            pushArg("--sax");
            break;
    }

    uval = xmlFuzzReadInt(4);
    if (uval > 0) {
        if (size <= (INT_MAX - 2000) / 20)
            uval %= size * 20 + 2000;
        else
            uval %= INT_MAX;
        snprintf(maxmemBuf, 20, "%u", uval);
        pushArg("--maxmem");
        pushArg(maxmemBuf);
    }

    ival = xmlFuzzReadInt(1);
    if (ival >= 1 && ival <= 5) {
        snprintf(maxAmplBuf, 20, "%d", ival);
        pushArg("--max-ampl");
        pushArg(maxAmplBuf);
    }

    ival = xmlFuzzReadInt(1);
    if (ival != 0) {
        snprintf(prettyBuf, 20, "%d", ival - 128);
        pushArg("--pretty");
        pushArg(prettyBuf);
    }

    sval = xmlFuzzReadString(&ssize);
    if (ssize > 0) {
        pushArg("--encode");
        pushArg(sval);
    }

    sval = xmlFuzzReadString(&ssize);
    if (ssize > 0) {
        pushArg("--pattern");
        pushArg(sval);
    }

    sval = xmlFuzzReadString(&ssize);
    if (ssize > 0) {
        pushArg("--xpath");
        pushArg(sval);
    }

    xmlFuzzReadEntities();
    docBuffer = xmlFuzzMainEntity(&docSize);
    docUrl = xmlFuzzMainUrl();
    if (docBuffer == NULL || docUrl[0] == '-')
        goto exit;
    pushArg(docUrl);

    pushArg(NULL);

    xmlSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);
#ifdef LIBXML_CATALOG_ENABLED
    xmlCatalogSetDefaults(XML_CATA_ALLOW_NONE);
#endif

    xmllintMain(vars.argi - 1, vars.argv, xmlFuzzResourceLoader);

    xmlMemSetup(free, malloc, realloc, xmlMemStrdup);

exit:
    xmlFuzzDataCleanup();
    free(vars.argv);
    return(0);
}
