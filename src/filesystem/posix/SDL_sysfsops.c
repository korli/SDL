/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "SDL_internal.h"

#if defined(SDL_FSOPS_POSIX)

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../SDL_sysfilesystem.h"

int SDL_SYS_FSenumerate(const char *fullpath, const char *dirname, SDL_EnumerateDirectoryCallback cb, void *userdata)
{
    int retval = 1;

    DIR *dir = opendir(fullpath);
    if (!dir) {
        return SDL_SetError("Can't open directory: %s", strerror(errno));
    }

    struct dirent *ent;
    while ((retval == 1) && ((ent = readdir(dir)) != NULL))
    {
        const char *name = ent->d_name;
        if ((SDL_strcmp(name, ".") == 0) || (SDL_strcmp(name, "..") == 0)) {
            continue;
        }
        retval = cb(userdata, dirname, name);
    }

    closedir(dir);

    return retval;
}

int SDL_SYS_FSremove(const char *fullpath)
{
    int rc = remove(fullpath);
    if (rc < 0) {
        const int origerrno = errno;
        if (origerrno == ENOENT) {
            char *parent = SDL_strdup(fullpath);
            if (!parent) {
                return -1;
            }

            char *ptr = SDL_strrchr(parent, '/');
            if (ptr) {
                *ptr = '\0';  // chop off thing we were removing, see if parent is there.
            }

            struct stat statbuf;
            rc = stat(ptr ? parent : ".", &statbuf);
            SDL_free(parent);
            if (rc == 0) {
                return 0;  // it's already gone, and parent exists, consider it success.
            }
        }
        return SDL_SetError("Can't remove path: %s", strerror(origerrno));
    }
    return 0;
}

int SDL_SYS_FSrename(const char *oldfullpath, const char *newfullpath)
{
    if (rename(oldfullpath, newfullpath) < 0) {
        return SDL_SetError("Can't remove path: %s", strerror(errno));
    }
    return 0;
}

int SDL_SYS_FSmkdir(const char *fullpath)
{
    const int rc = mkdir(fullpath, 0770);
    if (rc < 0) {
        const int origerrno = errno;
        if (origerrno == EEXIST) {
            struct stat statbuf;
            if ((stat(fullpath, &statbuf) == 0) && (S_ISDIR(statbuf.st_mode))) {
                return 0;  // it already exists and it's a directory, consider it success.
            }
        }
        return SDL_SetError("Can't create directory: %s", strerror(origerrno));
    }
    return 0;
}

int SDL_SYS_FSstat(const char *fullpath, SDL_PathInfo *info)
{
    struct stat statbuf;
    const int rc = stat(fullpath, &statbuf);
    if (rc < 0) {
        return SDL_SetError("Can't stat: %s", strerror(errno));
    } else if (S_ISREG(statbuf.st_mode)) {
        info->type = SDL_PATHTYPE_FILE;
        info->size = (Uint64) statbuf.st_size;
    } else if (S_ISDIR(statbuf.st_mode)) {
        info->type = SDL_PATHTYPE_DIRECTORY;
        info->size = 0;
    } else {
        info->type = SDL_PATHTYPE_OTHER;
        info->size = (Uint64) statbuf.st_size;
    }

    // SDL file time is seconds since the Unix epoch, so we're already good here.
    // Note that this will fail on machines with 32-bit time_t in 2038, but that's not
    // an SDL bug; those machines need to be fixed or everything will fail in the same way.
    info->create_time = (Sint64) statbuf.st_ctime;
    info->modify_time = (Sint64) statbuf.st_mtime;
    info->access_time = (Sint64) statbuf.st_atime;

    return 0;
}

#endif // SDL_FSOPS_POSIX

