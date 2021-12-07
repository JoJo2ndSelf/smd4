#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UTF8_MASKBITS   (0x3F)
#define UTF8_MASKBYTE   (0x80)
#define UTF8_MASK2BYTES (0xC0)
#define UTF8_MASK3BYTES (0xE0)
#define UTF8_MASK4BYTES (0xF0)
#define UTF8_MASK5BYTES (0xF8)
#define UTF8_MASK6BYTES (0xFC)

SMD_API size_t utf8_codepoint(char const* uft8In, uint16_t* ucOut);
/*
 * Returns number of bytes written excluding the null terminator
 */
SMD_API size_t uc2_to_utf8(uint16_t* uc_in, char* utf8_out, size_t buf_size);

#ifdef __cplusplus
}//extern "C" {
#endif