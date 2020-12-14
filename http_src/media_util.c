/*
 * media_util.c
 *
 * Functions for processing media types.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#include "media_util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "string_util.h"
#include "http_server.h"
#include "file_util.h"
#include "properties.h"
extern Properties* mediaTypeProperty;
/** default media type */
static const char *DEFAULT_MEDIA_TYPE = "application/octet-stream";

/**
 * Return a media type for a given filename.
 *
 * @param filename the name of the file
 * @param mediaType output buffer for mime type
 * @return pointer to media type string
 */
char *getMediaType(const char *filename, char *mediaType)
{
	// special-case directory based on trailing '/'
	if (filename[strlen(filename)-1] == '/') {
		strcpy(mediaType, "text/directory");
		return mediaType;
	}

	// get file extension
    char ext[MAXBUF];
    if (getExtension(filename, ext) == NULL) {
    	// default if no extension
    	strcpy(mediaType, DEFAULT_MEDIA_TYPE);
    	return mediaType;
    }

    // lower-case extension
    strlower(ext, ext);

    // media type sting for extension
    char mtstr[MAX_PROP_VAL];
    if (findProperty(mediaTypeProperty, 0, ext, mtstr) == SIZE_MAX) {
        strcpy(mtstr, DEFAULT_MEDIA_TYPE);
    }
    strcpy(mediaType, mtstr);
    return mediaType;
}

/**
 * Load the file extension to media type mapping from config file
 *
 * @param configFileName the name of the configuration file
 * @return the number of properties loaded
 */
int readMediaTypes(char* configFileName) {
    if (mediaTypeProperty == NULL) {
        mediaTypeProperty = newProperties();
    }
    FILE* propStream = fopen(configFileName, "r");
    if (propStream == NULL) {
        return 0;
    }
    char buf[MAXBUF];
    int nprops = 0;

    // get next line
    while (fgets(buf, MAXBUF, propStream) != NULL) {
        if (buf[0] == '#') { // ignore comment
            continue;
        }
        char *valp = strchr(buf, '\t');
        if (valp != NULL) {
            //
            *valp++ = '\0';
            valp = trim_trailing_tabs(valp);
            // trim newline characters
            trim_newline(valp);
            while(1) {
                char *valpp = strchr(valp, ' ');
                if (valpp == NULL) {
                    putProperty(mediaTypeProperty, valp, buf);
                    nprops++;
                    break;
                }
                *valpp++ = '\0';
                putProperty(mediaTypeProperty, valp, buf);
                valp = valpp;
                nprops++;
            }
        }
    }
    fclose(propStream);
    return nprops;

}
