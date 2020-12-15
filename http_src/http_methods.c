/*
 * methods.c
 *
 * Functions that implement HTTP methods, including
 * GET, HEAD, PUT, POST, and DELETE.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <dirent.h>

#include "http_codes.h"
#include "http_methods.h"
#include "http_server.h"
#include "http_util.h"
#include "time_util.h"
#include "media_util.h"
#include "properties.h"
#include "string_util.h"
#include "file_util.h"

/**
 * Generate html content for directory index page.
 *
 * @param path the directory path
 * @param buf the string buffer to hold html content
 */
static void dir_content(const char *path, char *buf) {
    // Replace content base with /.
    char pathCopy[MAXBUF];
    char pathName[MAXBUF];
    sprintf(pathName,"/");
    strcpy(pathCopy, path);
    char *token = strtok(pathCopy, "/");
    while( token != NULL ) {
        token = strtok(NULL, "/");
        if (token != NULL) sprintf(pathName, "%s%s/", pathName, token);
    }
    sprintf(buf, "<html>\n"
                 "<head>\n"
                 "  <title>index of %s</title>\n"
                 "</head>\n"
                 "<body>\n"
                 "  <h1>Index of %s</h1>\n"
                 "  <table>\n"
                 "  <tr>\n"
                 "    <th valign=\"top\"></th>\n"
                 "    <th>Name</th>\n"
                 "    <th>Last modified</th>\n"
                 "    <th>Size</th>\n"
                 "    <th>Description</th>\n"
                 "  </tr>\n"
                 "  <tr>\n"
                 "    <td colspan=\"5\"><hr></td>\n"
                 "  </tr>", pathName, pathName);
    DIR *dp = opendir(path);
    struct dirent *ep;
    while (ep = readdir (dp)) {
        if (strcmp(ep->d_name, ".") == 0) {
            continue;
        }
        char name[MAXBUF], link[MAXBUF], mtime[MAXBUF], td[MAXBUF], size[MAXBUF];
        if (ep->d_type == DT_DIR) {
            sprintf(link, "%s/", ep->d_name);
            if (strcmp(ep->d_name, "..")==0) {
                if (strcmp(pathName, "/")==0) {
                    continue;
                } else {
                    sprintf(td, "&#x23ce");
                    sprintf(name, "Parent Directory");
                }
            } else {
                sprintf(td, "&#x1F4C1;");
                sprintf(name, ep->d_name);
            }
        } else {
            sprintf(name, ep->d_name);
            sprintf(link, ep->d_name);
            sprintf(td, "");
        }
        char fullPath[MAXBUF];
        sprintf(fullPath, "%s%s", path, ep->d_name);
        struct stat sb;
        stat(fullPath, &sb);
        time_t timer = sb.st_mtim.tv_sec;
        milliTimeToRFC_1123_Date_Time(timer, mtime);
        sprintf(size,"%lu",(size_t)sb.st_size);
        sprintf(buf+strlen(buf), "  <tr>\n"
                                 "    <td>%s</td>\n"
                                 "    <td><a href=\"%s\">%s</a></td>\n"
                                 "    <td align=\"right\">%s</td>\n"
                                 "    <td align=\"right\">%s</td>\n"
                                 "    <td></td>\n"
                                 "  </tr>\n",td, link, name, mtime, size);
    }
    (void) closedir (dp);
    sprintf(buf+strlen(buf), "  <tr><td colspan=\"5\"><hr></td></tr>\n"
                             "</body>\n"
                             "</html>");
}

/**
 * Handle GET or HEAD request for directory.
 *
 * @param stream the socket stream
 * @param path the directory path
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 * @param sendContent send content (GET)
 */
static void do_dir(FILE *stream, const char *path, Properties *requestHeaders, Properties *responseHeaders, bool sendContent) {
    char buf[2048];
    dir_content(path, buf);
    // Put content to tmp file and get file stat.
    FILE *tmp = tmpStringFile(buf);
    char lenBuf[MAXBUF];
    sprintf(lenBuf,"%lu", strlen(buf));
    putProperty(responseHeaders,"Content-Length", lenBuf);
    putProperty(responseHeaders, "Content-type", "text/html");
    // send response
    sendResponseStatus(stream, Http_OK, NULL);
    // Send response headers
    sendResponseHeaders(stream, responseHeaders);
    if (sendContent) {  // for GET
        copyFileStreamBytes(tmp, stream, strlen(buf));
        fclose(tmp);
    }
}

/**
 * Handle GET or HEAD request.
 *
 * @param stream the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 * @param sendContent send content (GET)
 */
static void do_get_or_head(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders, bool sendContent) {
	// get path to URI in file system
	char filePath[MAXPATHLEN];
	resolveUri(uri, filePath);
	FILE *contentStream = NULL;

	// ensure file exists
	struct stat sb;
	if (stat(filePath, &sb) != 0) {
		sendStatusResponse(stream, Http_NotFound, NULL, responseHeaders);
		return;
	}
	// directory path ends with '/'
	if (S_ISDIR(sb.st_mode) && strendswith(filePath, "/")) {
		// not allowed for this method
		do_dir(stream, filePath, requestHeaders, responseHeaders, sendContent);
		return;
	} else if (!S_ISREG(sb.st_mode)) { // error if not regular file
		sendStatusResponse(stream, Http_NotFound, NULL, responseHeaders);
		return;
	}

	// record the file length
	char buf[MAXBUF];
	size_t contentLen = (size_t)sb.st_size;
	sprintf(buf,"%lu", contentLen);
	putProperty(responseHeaders,"Content-Length", buf);

	// record the last-modified date/time
	time_t timer = sb.st_mtim.tv_sec;
	putProperty(responseHeaders,"Last-Modified",
				milliTimeToRFC_1123_Date_Time(timer, buf));

	// get mime type of file
	getMediaType(filePath, buf);
	if (strcmp(buf, "text/directory") == 0) {
		// some browsers interpret text/directory as a VCF file
		strcpy(buf,"text/html");
	}
	putProperty(responseHeaders, "Content-type", buf);

	// send response
	sendResponseStatus(stream, Http_OK, NULL);

	// Send response headers
	sendResponseHeaders(stream, responseHeaders);

	if (sendContent) {  // for GET
		contentStream = fopen(filePath, "r");
		copyFileStreamBytes(contentStream, stream, contentLen);
		fclose(contentStream);
	}
}

/**
 * Handle GET request.
 *
 * @param stream the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 * @param headOnly only perform head operation
 */
void do_get(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders) {
	do_get_or_head(stream, uri, requestHeaders, responseHeaders, true);
}

/**
 * Handle HEAD request.
 *
 * @param the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 */
void do_head(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders) {
	do_get_or_head(stream, uri, requestHeaders, responseHeaders, false);
}
