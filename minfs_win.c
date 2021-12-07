/********************************************************************
    Written by James Moran
    Please see the file LICENSE.txt in the repository root directory.
*********************************************************************/
#ifndef SMD_UNITY_BUILD

#include "minfs.h"
#include "minfs_common.h"
#include <windows.h>
#include <Shlwapi.h>
#include <malloc.h>

static HANDLE open_file_handle_read_only(const char* filepath) {
  DWORD                 access = GENERIC_READ;
  DWORD                 share = FILE_SHARE_READ;
  LPSECURITY_ATTRIBUTES secatt =
    NULL; // could be a prob if passed across threads>?
  DWORD creation = OPEN_EXISTING;
  DWORD flags = FILE_ATTRIBUTE_NORMAL;

  return CreateFile(filepath, access, share, secatt, creation, flags, NULL);
}

static int get_file_attrib(const char*                filepath,
                           WIN32_FILE_ATTRIBUTE_DATA* fileinfo) {
  wchar_t* uc2filepath;
  UTF8_TO_UC2_STACK(filepath, uc2filepath);

  if (!GetFileAttributesExW(uc2filepath, GetFileExInfoStandard, fileinfo)) {
    return -1;
  }
  return 0;
}

minfs_uint64_t minfs_get_current_file_time() {
  FILETIME   filetime;
  SYSTEMTIME systime;
  GetSystemTime(&systime);
  SystemTimeToFileTime(&systime, &filetime);
  return ((((minfs_uint64_t)filetime.dwHighDateTime) << 32) |
          filetime.dwLowDateTime);
}

minfs_uint64_t minfs_get_file_mdate(const char* filepath) {
  WIN32_FILE_ATTRIBUTE_DATA fileinfo;
  if (get_file_attrib(filepath, &fileinfo) != 0) {
    return 0;
  }
  return ((((minfs_uint64_t)fileinfo.ftLastWriteTime.dwHighDateTime) << 32) |
          fileinfo.ftLastWriteTime.dwLowDateTime);
}

minfs_uint64_t minfs_get_file_size(const char* filepath) {
  WIN32_FILE_ATTRIBUTE_DATA fileinfo;
  if (get_file_attrib(filepath, &fileinfo) != 0) {
    return 0;
  }
  return ((((minfs_uint64_t)fileinfo.nFileSizeHigh) << 32) |
          fileinfo.nFileSizeLow);
}

int minfs_path_exist(const char* filepath) {
  wchar_t* uc2filepath;
  UTF8_TO_UC2_STACK(filepath, uc2filepath);
  return GetFileAttributesW(uc2filepath) != INVALID_FILE_ATTRIBUTES;
}

int minfs_is_file(const char* filepath) {
  WIN32_FILE_ATTRIBUTE_DATA fileinfo;
  if (get_file_attrib(filepath, &fileinfo) != 0) {
    return 0;
  }
  return (fileinfo.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY)) == 0;
}

int minfs_is_directory(const char* filepath) {
  WIN32_FILE_ATTRIBUTE_DATA fileinfo;
  if (get_file_attrib(filepath, &fileinfo) != 0) {
    return 0;
  }
  return (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

int minfs_is_sym_link(const char* filepath) {
  WIN32_FILE_ATTRIBUTE_DATA fileinfo;
  if (get_file_attrib(filepath, &fileinfo) != 0) {
    return 0;
  }
  return (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

int minfs_create_directories(const char* filepath) {
  wchar_t* mrk;
  wchar_t* uc2filepath;
  UTF8_TO_UC2_STACK(filepath, uc2filepath);
  while (mrk = wcschr(uc2filepath, L'\\')) {
    *mrk = '/';
  }
  mrk = uc2filepath;
  while (mrk = wcschr(mrk, '/')) {
    *mrk = 0;
    if (GetFileAttributesW(uc2filepath) == INVALID_FILE_ATTRIBUTES) {
      CreateDirectoryW(uc2filepath, NULL);
    }
    *mrk = '/';
    ++mrk;
  }
  if (GetFileAttributesW(uc2filepath) == INVALID_FILE_ATTRIBUTES) {
    CreateDirectoryW(uc2filepath, NULL);
  }
  return OK;
}

size_t
minfs_canonical_path(const char* filepath, char* outpath, size_t buf_size) {
  BOOL     ret;
  size_t   rlen = 0;
  wchar_t *mrk, *prevmrk;
  wchar_t* uc2filepath;
  wchar_t* canonical;
  // UTF8_TO_UC2_STACK(filepath, uc2filepath);

  minfs_uint32_t cdlen = GetCurrentDirectoryW(0, NULL);
  uc2filepath = alloca((cdlen + strlen(filepath) + 1) * sizeof(wchar_t));
  if (filepath[1] != ':') {
    GetCurrentDirectoryW(cdlen, uc2filepath);
    uc2filepath[cdlen - 1] = '\\';
  } else {
    cdlen = 0;
  }

  utf8_to_uc2(filepath,
              uc2filepath + cdlen,
              (strlen(filepath) + 1) * sizeof(minfs_uint16_t));

  while (mrk = wcschr(uc2filepath, '/')) {
    *mrk = '\\';
  }

  canonical = alloca((cdlen + strlen(filepath) + 1) * sizeof(wchar_t) * 2);
  ret = PathCanonicalizeW(canonical, uc2filepath);
  while (mrk = wcschr(canonical, '\\')) {
    *mrk = '/';
  }
  // strip any double slashes
  rlen = wcslen(canonical);
  mrk = canonical;
  prevmrk = NULL;
  while (*mrk) {
    if (*mrk == '/' && prevmrk && *prevmrk == '/') {
      memmove(prevmrk, mrk, (rlen + 1) * sizeof(wchar_t));
    }
    prevmrk = mrk;
    ++mrk;
    --rlen;
  }
  // strip any trailing slashes
  if (ret == TRUE) {
    rlen = uc2_to_utf8(canonical, outpath, buf_size);
    if (canonical[rlen - 1] == '/') {
      canonical[rlen - 1] = 0;
      rlen--;
    }
  }
  return rlen;
}

minfs_uint32_t minfs_current_working_directory_len() {
  return GetCurrentDirectoryW(0, NULL);
}

size_t minfs_current_working_directory(char* out_cwd, size_t buf_size) {
  minfs_uint32_t len = GetCurrentDirectoryW(0, NULL);
  wchar_t*       tmpbuf = alloca(len * sizeof(wchar_t));

  GetCurrentDirectoryW(len, tmpbuf);
  return uc2_to_utf8(tmpbuf, out_cwd, buf_size);
}

int minfs_read_directory(const char*             filepath,
                         void*                   scratch,
                         size_t                  scratchlen,
                         minfs_read_dir_callback cb,
                         void*                   opaque) {
  WIN32_FIND_DATAW found;
  HANDLE           searchhandle;
  size_t           foundlen;
  wchar_t*         uc2filepath;
  UTF8_TO_UC2_STACK_PAD(filepath, uc2filepath, 4);
  wcscat(uc2filepath, L"/*");

  searchhandle = FindFirstFileW(uc2filepath, &found);

  if (searchhandle == INVALID_HANDLE_VALUE) {
    return 0;
  }

  do {
    if (wcscmp(found.cFileName, L".") && wcscmp(found.cFileName, L"..")) {
      foundlen = wcslen(found.cFileName);
      if (uc2_to_utf8(found.cFileName, (char*)scratch, scratchlen) !=
          foundlen) {
        return NO_MEM;
      }
      cb(filepath, (char*)scratch, opaque);
    }
  } while (FindNextFileW(searchhandle, &found));

  FindClose(searchhandle);
  return OK;
}

MinFSDirectoryEntry_t* minfs_read_directory_entries(const char* filepath,
                                                    void*       scratch,
                                                    size_t      scratchlen) {
  WIN32_FIND_DATAW       found;
  HANDLE                 searchhandle;
  size_t                 foundlen;
  wchar_t*               uc2filepath;
  MinFSDirectoryEntry_t *currententry = NULL, *first = NULL;
  UTF8_TO_UC2_STACK_PAD(filepath, uc2filepath, 3);
  wcscat(uc2filepath, L"/*");

  searchhandle = FindFirstFileW(uc2filepath, &found);

  if (searchhandle == INVALID_HANDLE_VALUE ||
      scratchlen < sizeof(MinFSDirectoryEntry_t)) {
    return NULL;
  }

  --scratchlen;

  first = scratch;
  first->entryName[0] = 0;
  first->entryNameLen = 0;
  first->next = NULL;

  do {
    if (wcscmp(found.cFileName, L".") && wcscmp(found.cFileName, L"..")) {
      currententry = scratch;
      if (scratchlen < sizeof(MinFSDirectoryEntry_t)) {
        FindClose(searchhandle);
        return NULL;
      }
      scratchlen -= sizeof(MinFSDirectoryEntry_t);
      foundlen = wcslen(found.cFileName);
      if (uc2_to_utf8(found.cFileName,
                      (char*)currententry->entryName,
                      scratchlen) != foundlen) {
        FindClose(searchhandle);
        return NULL;
      }
      if (scratchlen < foundlen + 1) return NULL;
      scratchlen -= foundlen + 1;
      scratch = ((char*)scratch) + sizeof(MinFSDirectoryEntry_t) + foundlen;
      currententry->next = scratch;
      currententry->entryNameLen = foundlen;
    }
  } while (FindNextFileW(searchhandle, &found));

  FindClose(searchhandle);
  if (currententry) currententry->next = NULL;
  return first;
}

int minfs_get_temp_file_name(char* dest, size_t dest_len) {
  wchar_t szTempFileName[MAX_PATH];
  wchar_t lpTempPathBuffer[MAX_PATH];
  DWORD dwRetVal = 0;
  UINT uRetVal = 0;

  //  Gets the temp path env string (no guarantee it's a valid path).
  dwRetVal = GetTempPathW(MAX_PATH, lpTempPathBuffer);
  if (dwRetVal > MAX_PATH || (dwRetVal == 0))
  {
    return NO_TEMP_DIR;
  }

  //  Generates a temporary file name. 
  uRetVal = GetTempFileNameW(lpTempPathBuffer, // directory for tmp files
    L"minfs_tmp_",     // temp file name prefix 
    1,                // create unique name 
    szTempFileName);  // buffer for name 
  if (uRetVal == 0)
  {
    return NO_TEMP_FILE;
  }

  if (wcslen(szTempFileName) >= dest_len) return NO_MEM;
  return uc2_to_utf8(szTempFileName, dest, dest_len) >= dest_len ? NO_MEM : OK;
}

int minfs_move_file(char const* dst, char const* src) {
  wchar_t* uc2dst, *uc2src;
  UTF8_TO_UC2_STACK(dst, uc2dst);
  UTF8_TO_UC2_STACK(src, uc2src);

  return MoveFileW(uc2src, uc2dst) ? OK : NO_MEM;
}

int minfs_delete_file(char const* file) {
  wchar_t* uc2file;
  UTF8_TO_UC2_STACK(file, uc2file);

  return DeleteFileW(uc2file) ? OK : NO_MEM;
}

int minfs_make_relative(char const* path, char const* from, char* out, size_t outlen) {
  wchar_t uc2out[MAX_PATH*2];
  wchar_t *uc2path, *uc2from;
  UTF8_TO_UC2_STACK(path, uc2path);
  UTF8_TO_UC2_STACK(from, uc2from);
  DWORD from_flags = minfs_is_file(from) ? 0 : FILE_ATTRIBUTE_DIRECTORY;
  DWORD path_flags = minfs_is_file(path) ? 0 : FILE_ATTRIBUTE_DIRECTORY;
  wchar_t* p = wcschr(uc2from, '/');
  while (p) { *p = '\\'; p = wcschr(p+1, '/');}
  p = wcschr(uc2path, '/');
  while (p) { *p = '\\'; p = wcschr(p + 1, '/'); }
  BOOL ret = PathRelativePathToW(uc2out, uc2from, from_flags, uc2path, path_flags);
  if (!ret) return NO_MEM;
  return uc2_to_utf8(uc2out, out, outlen) >= outlen ? NO_MEM : OK;
}

#endif
