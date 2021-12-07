/********************************************************************
    Written by James Moran
    Please see the file LICENSE.txt in the repository root directory.
*********************************************************************/

#include "minfs.h"
#include "minfs_common.h" 
#include <stdint.h>

#ifdef PLATFORM_WINDOWS
void utf8_to_uc2(const char* src, minfs_uint16_t* dst, size_t len) {
    len -= sizeof(minfs_uint16_t);   /* save room for null char. */
    while (len >= sizeof(minfs_uint16_t))
    {
        minfs_uint32_t cp = utf8_codepoint(src, dst);
        if (cp == 0)
            break;

        dst++;// = (minfs_uint16_t)(cp & 0xFFFF);
        src++;
        len -= sizeof(minfs_uint16_t);
    } /* while */

    *dst = 0;
}

#endif

size_t minfs_path_parent(const char* filepath, char* out_cwd, size_t buf_size) {
    char* mrk;
    char* lastmrk = NULL;
    size_t outsize;
    size_t len = strlen(filepath);
    while ((mrk = strchr(filepath, '\\'))) { 
        *mrk = '/';
    }
    lastmrk = strrchr(filepath, '/');
    if (lastmrk == NULL && buf_size > len) {
        strncpy(out_cwd, filepath, buf_size);
        return len;
    } else {
        outsize = (uintptr_t)lastmrk - (uintptr_t)filepath;

        if (buf_size > outsize) {
            strncpy(out_cwd, filepath, outsize);
            out_cwd[outsize]=0;
            return outsize;
        }
    }
    // no space to write
    return NO_MEM;
}

size_t minfs_path_leaf(const char* filepath, char* out_cwd, size_t buf_size) {
    char* mrk;
    char* lastmrk = NULL;
    size_t outsize;
    size_t len = strlen(filepath);
    while ((mrk = strrchr(filepath, '\\'))) { 
        *mrk = '/';
        lastmrk = mrk;
    }
    lastmrk = strrchr(filepath, '/');
    if (lastmrk == NULL && buf_size > len) {
        strncpy(out_cwd, filepath, buf_size);
        return len;
    } else {
        outsize = len - ((uintptr_t)lastmrk - (uintptr_t)filepath);

        if (buf_size > outsize) {
            strncpy(out_cwd, lastmrk+1, outsize);
            return outsize;
        }
    }
    // no space to write
    return NO_MEM;
}

size_t minfs_path_without_ext(const char* filepath, char* out_cwd, size_t buf_size) {
    char* mrk;
    char* lastmrk = NULL;
    size_t outsize;
    size_t len = strlen(filepath);
    while ((mrk = strchr(filepath, '\\'))) { 
        *mrk = '/';
    }
    lastmrk = strrchr(filepath, '.');
    if (lastmrk == NULL && buf_size > len) {
        strncpy(out_cwd, filepath, buf_size);
        return len;
    } else {
        outsize = ((uintptr_t)lastmrk - (uintptr_t)filepath);

        if (buf_size > outsize) {
            strncpy(out_cwd, filepath, outsize);
            out_cwd[outsize]=0;
            return outsize;
        }
    }
    // no space to write
    return NO_MEM;
}

size_t minfs_path_join(const char* parent, const char* leaf, char* out_path, size_t buf_size) {
    size_t parent_len = strlen(parent);
    size_t leaf_len = strlen(leaf);
    if (parent[parent_len > 0 ? parent_len-1 : 0] == '/' && parent_len > 1) {
        --parent_len;
    }
    if (leaf[0] == '/') {
        --leaf_len;
        ++leaf;
    }

    if (leaf_len+parent_len+1 > buf_size) {
        return NO_MEM;
    }
    
    strncpy(out_path, parent, parent_len);
    if (leaf_len == 0) {
        out_path[parent_len] = 0;
        return parent_len;
    }
    if (parent_len > 1) {
        out_path[parent_len] = '/';
    } else {
        --parent_len;
    }
    strncpy(out_path + parent_len + 1, leaf, leaf_len);
    out_path[leaf_len + parent_len + 1] = 0;
    return leaf_len+parent_len+1;
}
