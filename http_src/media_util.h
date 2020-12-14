/*
 * mime_util.h
 *
 * Functions for processing MIME types.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#ifndef MEDIA_UTIL_H_
#define MEDIA_UTIL_H_
#include <unistd.h>
#include "properties.h"
/**
 * Return a media type for a given filename.
 *
 * @param filename the name of the file
 * @param mediaType output buffer for media type
 * @return pointer to media type string
 */
char *getMediaType(const char *filename, char *mediaType);

/**
 * Load the file extension to media type mapping from config file
 *
 * @param configFileName the name of the configuration file
 * @return the number of properties loaded
 */
int readMediaTypes(char* configFileName);

#endif /* MEDIA_UTIL_H_ */
