/********************************************************************
    Written by James Moran
    Please see the file LICENSE.txt in the repository root directory.
*********************************************************************/
#pragma once

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
enum ErrorCodes{
    OK                         =  0,
    ATTRIBUTE_READ_FAILED      = 0x80000001,
    NO_MEM                     = 0x80000002,
    FILE_NOT_EXIST			   = 0x80000003,
    NO_TEMP_DIR            = 0x80000004,
    NO_TEMP_FILE = 0x80000005,
} ErrorCodes_t;

#define FS_SUCCEEDED(x) ((x & 0x80000000) == 0)
#define FS_FAILED(x) (!FS_SUCCEEDED(x))

typedef unsigned short      minfs_uint16_t;
typedef unsigned long       minfs_uint32_t;
typedef unsigned long long  minfs_uint64_t;
typedef void (*minfs_read_dir_callback)(const char* origpath, const char* file, void* opaque);

typedef struct MinFSDirectoryEntry {
    size_t                      entryNameLen;
    struct MinFSDirectoryEntry* next;
    char                        entryName[1];
} MinFSDirectoryEntry_t;

SMD_API minfs_uint64_t minfs_get_current_file_time();
SMD_API minfs_uint64_t minfs_get_file_mdate(const char* filepath);
SMD_API minfs_uint64_t minfs_get_file_size(const char* filepath);
SMD_API size_t minfs_canonical_path(const char* filepath, char* outpath, size_t buf_size);
SMD_API int minfs_path_exist(const char* filepath);
SMD_API int minfs_is_file(const char* filepath);
SMD_API int minfs_is_directory(const char* filepath);
SMD_API int minfs_is_sym_link(const char* filepath);
SMD_API int minfs_create_directories(const char* filepath);
SMD_API minfs_uint32_t minfs_current_working_directory_len();
SMD_API size_t minfs_current_working_directory(char* out_cwd, size_t buf_size);
SMD_API size_t minfs_path_parent(const char* filepath, char* out_cwd, size_t buf_size);
SMD_API size_t minfs_path_leaf(const char* filepath, char* out_cwd, size_t buf_size);
SMD_API size_t minfs_path_without_ext(const char* filepath, char* out_cwd, size_t buf_size);
SMD_API size_t minfs_path_join(const char* parent, const char* leaf, char* out_path, size_t buf_size);
SMD_API int minfs_read_directory(const char* filepath, void* scratch, size_t scratchlen, minfs_read_dir_callback cb, void* opaque);
SMD_API MinFSDirectoryEntry_t* minfs_read_directory_entries(const char* filepath, void* scratch, size_t scratchlen);
SMD_API int minfs_get_temp_file_name(char* dest, size_t dest_len);
SMD_API int minfs_move_file(char const* dst, char const* src);
SMD_API int minfs_delete_file(char const* file);
SMD_API int minfs_make_relative(char const* path, char const* from, char* out, size_t outlen);

#ifdef __cplusplus
} //extern "C"
#endif