
#include <generated/br2_autoconf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define TEMP_PATH_MAX  CONFIG_PATH_MAX

static unsigned int vfs_strnlen(const char *str, size_t maxlen)
{
    const char *p = NULL;

    for (p = str; ((maxlen-- != 0) && (*p != '\0')); ++p) {}

    return p - str;
}

static char *str_path(char *path)
{
    char *dest = path;
    char *src = path;

    while (*src != '\0') {
        if (*src == '/') {
            *dest++ = *src++;
            while (*src == '/') {
                src++;
            }
            continue;
        }
        *dest++ = *src++;
    }
    *dest = '\0';
    return path;
}

static void str_remove_path_end_slash(char *dest, const char *fullpath)
{
    if ((*dest == '.') && (*(dest - 1) == '/')) {
        *dest = '\0';
        dest--;
    }
    if ((dest != fullpath) && (*dest == '/')) {
        *dest = '\0';
    }
}

static char *str_normalize_path(char *fullpath)
{
    char *dest = fullpath;
    char *src = fullpath;

    /* 2: The position of the path character: / and the end character /0 */

    while (*src != '\0') {
        if (*src == '.') {
            if (*(src + 1) == '/') {
                src += 2;
                continue;
            } else if (*(src + 1) == '.') {
                if ((*(src + 2) == '/') || (*(src + 2) == '\0')) {
                    src += 2;
                } else {
                    while ((*src != '\0') && (*src != '/')) {
                        *dest++ = *src++;
                    }
                    continue;
                }
            } else {
                *dest++ = *src++;
                continue;
            }
        } else {
            *dest++ = *src++;
            continue;
        }

        if ((dest - 1) != fullpath) {
            dest--;
        }

        while ((dest > fullpath) && (*(dest - 1) != '/')) {
            dest--;
        }

        if (*src == '/') {
            src++;
        }
    }

    *dest = '\0';

    /* remove '/' in the end of path if exist */

    dest--;

    str_remove_path_end_slash(dest, fullpath);
    return dest;
}

static int vfs_normalize_path_parame_check(const char *filename, char **pathname)
{
    int namelen;
    char *name = NULL;

    if (pathname == NULL) {
        return -EINVAL;
    }

    /* check parameters */

    if (filename == NULL) {
        *pathname = NULL;
        return -EINVAL;
    }

    namelen = vfs_strnlen(filename, CONFIG_PATH_MAX);
    if (!namelen) {
        *pathname = NULL;
        return -EINVAL;
    } else if (namelen >= CONFIG_PATH_MAX) {
        *pathname = NULL;
        return -ENAMETOOLONG;
    }

    for (name = (char *)filename + namelen; ((name != filename) && (*name != '/')); name--) {
        if (strlen(name) > CONFIG_NAME_MAX) {
            *pathname = NULL;
            return -ENAMETOOLONG;
        }
    }

    return namelen;
}

static char *vfs_not_absolute_path(const char *directory, const char *filename, char **pathname, int namelen)
{
    int ret;
    char *fullpath = NULL;

    /* 2: The position of the path character: / and the end character /0 */

    if ((namelen > 1) && (filename[0] == '.') && (filename[1] == '/')) {
        filename += 2;
    }

    fullpath = (char *)malloc(strlen(directory) + namelen + 2);
    if (fullpath == NULL) {
        *pathname = NULL;
	errno = ENOMEM;
        return (char *)NULL;
    }

    /* join path and file name */

    ret = snprintf(fullpath, strlen(directory) + namelen + 1, "%s/%s", directory, filename);
    if (ret < 0) {
        *pathname = NULL;
        free(fullpath);
	errno = ENAMETOOLONG;
        return (char *)NULL;
    }

    return fullpath;
}

static char *vfs_normalize_fullpath(const char *directory, const char *filename, char **pathname, int namelen)
{
    char *fullpath = NULL;

    if (filename[0] != '/') {
        /* not a absolute path */

        fullpath = vfs_not_absolute_path(directory, filename, pathname, namelen);
        if (fullpath == NULL) {
            return (char *)NULL;
        }
    } else {
        /* it's a absolute path, use it directly */

        fullpath = strdup(filename); /* copy string */

        if (fullpath == NULL) {
            *pathname = NULL;
	    errno = ENOMEM;
            return (char *)NULL;
        }
        if (filename[1] == '/') {
            *pathname = NULL;
            free(fullpath);
	    errno = EINVAL;
            return (char *)NULL;
        }
    }

    return fullpath;
}

int vfs_normalize_path(const char *directory, const char *filename, char **pathname)
{
    char *fullpath = NULL;
    int namelen;

    namelen = vfs_normalize_path_parame_check(filename, pathname);
    if (namelen < 0) {
        return namelen;
    }

    if ((directory == NULL) && (filename[0] != '/')) {
        printf("NO_WORKING_DIR\n");
        *pathname = NULL;
        return -EINVAL;
    }

    /* 2: The position of the path character: / and the end character /0 */

    if ((filename[0] != '/') && (strlen(directory) + namelen + 2 > TEMP_PATH_MAX)) {
        return -ENAMETOOLONG;
    }

    fullpath = vfs_normalize_fullpath(directory, filename, pathname, namelen);
    if (fullpath == NULL) {
        return -errno;
    }

    (void)str_path(fullpath);
    (void)str_normalize_path(fullpath);
    if (strlen(fullpath) >= CONFIG_PATH_MAX) {
        *pathname = NULL;
        free(fullpath);
        return -ENAMETOOLONG;
    }

    *pathname = fullpath;
    return 0;
}
