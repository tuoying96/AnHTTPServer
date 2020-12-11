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
    const char *mtstr;

    // hash on first char?
    switch (*ext) {
    case 'c':
        if (strcmp(ext, "css") == 0) { mtstr = "text/css"; }
        break;
    case 'g':
        if (strcmp(ext, "gif") == 0) { mtstr = "image/gif"; }
        break;
    case 'h':
        if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) { mtstr = "text/html"; }
        break;
    case 'i':
        if (strcmp(ext, "ico") == 0) { mtstr = "image/vnd.microsoft.icon"; }
        break;
    case 'j':
    	if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) { mtstr = "image/jpg"; }
    	else if (strcmp(ext, "js") == 0) { mtstr = "application/javascript"; }
    	else if (strcmp(ext, "json") == 0) { mtstr = "application/json"; }
    	break;
    case 'p':
        if (strcmp(ext, "png") == 0) { mtstr = "image/png"; }
        break;
    case 't':
    	if (strcmp(ext, "txt") == 0) { mtstr = "text/plain"; }
    	break;
    default:
    	mtstr = DEFAULT_MEDIA_TYPE;
    }

    strcpy(mediaType, mtstr);
    return mediaType;
}
