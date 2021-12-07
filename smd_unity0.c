
#include "smdconfig.h"

#ifdef SMD_UNITY_BUILD

// allow .c files to compile
#undef SMD_UNITY_BUILD

#ifdef PLATFORM_WINDOWS
// for, probably legit, reasons winsock2 must come before windows.h
#include <WinSock2.h>
#include <windows.h>
#endif


#include "getopt.c"
#include "minfs_common.c"
#ifdef PLATFORM_LINUX
#include "minfs_linux.c"
#elif PLATFORM_WINDOWS
#include "minfs_win.c"
#endif

#define SMD_SOCK_IMPL
#include "sock.h"

#define SMD_THREAD_IMPL
#include "thread.h"

#define SMD_PROCESS_IMPL
#include "smd_proc.h"

#define SMD_UNITY_BUILD 
#endif //SMD_UNITY_BUILD

// Here until i find a better place
#include <string.h>

SMD_API char* strdup(char const* str) {
  size_t l = strlen(str);
  char* r = malloc(l+1);
  strcpy(r, str);
  return r;
} 

SMD_API size_t utf8_codepoint(char const* uft8In, uint16_t* ucOut) {
  size_t ret = 0;
  if ((uft8In[ret] & UTF8_MASK3BYTES) == UTF8_MASK3BYTES) { // 1110xxxx 10xxxxxx 10xxxxxx
    *ucOut = ((uft8In[ret] & 0x0F) << 12) |
      ((uft8In[ret + 1] & UTF8_MASKBITS) << 6) |
      ((uft8In[ret + 2] & UTF8_MASKBITS));
    ret = 3;
  }
  else if ((uft8In[ret] & UTF8_MASK2BYTES) == UTF8_MASK2BYTES) { // 110xxxxx 10xxxxxx
    *ucOut = ((uft8In[ret] & 0x1F) << 6) | (uft8In[ret + 1] & UTF8_MASKBITS);
    ret = 2;
  }
  else if ((uft8In[ret] & UTF8_MASKBYTE) != UTF8_MASKBYTE) { // 0xxxxxxx
    *ucOut = uft8In[ret];
    ret = 1;
  }
  return ret;
}


/*
 * Returns number of bytes written excluding the null terminator
 */
SMD_API size_t uc2_to_utf8(uint16_t* uc_in, char* utf8_out, size_t buf_size) {
    size_t ret = 0;
    while (buf_size > 0 && *uc_in) {    // 0xxxxxxx
        if(*uc_in & 0x7F && buf_size > 0) {
            utf8_out[ ret++ ] = 0x007F & *uc_in;
            buf_size -= 1;
        } else if(*uc_in <= 0xC00 && buf_size > 1) {// 110xxxxx 10xxxxxx
            utf8_out[ ret++ ] = 0x00FF & ( UTF8_MASK2BYTES | ( *uc_in >> 6 ) );
            utf8_out[ ret++ ] = 0x00FF & ( UTF8_MASKBYTE | ( *uc_in & UTF8_MASKBITS ) );
            buf_size -= 2;
        } else if(*uc_in <= 0xE000 && buf_size > 2) { // 1110xxxx 10xxxxxx 10xxxxxx
            utf8_out[ ret++ ] = 0x00FF & ( UTF8_MASK3BYTES | ( *uc_in >> 12 ) );
            utf8_out[ ret++ ] = 0x00FF & ( UTF8_MASKBYTE | ( *uc_in >> 6 & UTF8_MASKBITS ) );
            utf8_out[ ret++ ] = 0x00FF & ( UTF8_MASKBYTE | ( *uc_in & UTF8_MASKBITS ) );
            buf_size -= 3;
        }
        ++uc_in;
    }

    if (buf_size) {
        utf8_out[ret] = 0;
    }
    return ret;
}