#ifndef _PNG_H_
#define _PNG_H_

#include <stdint.h>
#include <string.h>

#define PNG_SIGNATURE_SIZE 8
#define PNG_DATA_OFFSET (sizeof(uint32_t) + 4 * sizeof(char))
#define PNG_CHUNK_MIN_LEN (2 * sizeof(uint32_t) + 4 * sizeof(char))
#define PNG_MAX_CHUNK_SIZE (1 << 31)

// for ease of reading png data from buffer
struct png_chunk_header {
    uint32_t length; // 4 bytes: length of the data section
    char type[4];    // 4 bytes: name of the chunk type
    void *data;      // 4 bytes: pointer to buffer of size length
    uint32_t crc;    // 4 bytes: CRC-32 redundancy checksum in network byte order
};

/* calculate the CRC32 of the data in buf */
uint32_t crc32(const char *buf, size_t len) {
    // static members to maintain CRC table
    static uint32_t table[256];
    static int have_table = 0;
 
    // generate table if we do not already have it
    if (!have_table) {
        uint32_t rem;
        for (int i = 0; i < 256; i++) {
            rem = i;
            for (int j = 0; j < 8; j++) {
                if (rem & 1) {
                    rem >>= 1;
                    rem ^= 0xedb88320;
                }
                else rem >>= 1;
            }
            table[i] = rem;
        }
        // mark table as generated
        have_table = 1;
    }
 
    // calculate the CRC from the table values
	uint32_t crc = -1;
	const char *q = buf + len;
    uint8_t octet;
	for (const char *p = buf; p < q; p++) {
		octet = *p;
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}

/* convert little-endian 4-byte blocks to big-endian and vice-versa */
uint32_t reverse_32(const uint32_t value) {
    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) <<  8) |
           ((value & 0x00FF0000) >>  8) |
           ((value & 0xFF000000) >> 24);
}

/* check for matching 8-byte PNG signature */
int is_png(const void * const _ptr) {
    const unsigned char * const ptr = (unsigned char *)_ptr;
    if (ptr[0] == 0x89 && ptr[1] == 0x50 && ptr[2] == 0x4E
        && ptr[3] == 0x47 && ptr[4] == 0x0D && ptr[5] == 0x0A
        && ptr[6] == 0x1A && ptr[7] == 0x0A) return 1;
    return 0;
}

/* fill a struct png_chunk_header with data at ptr */
int read_chunk_header(const void * const ptr, struct png_chunk_header *header) {
    // quick error-checking
    if (!ptr || !header) return 0;

    header->length = reverse_32(*(uint32_t *)ptr);
    memcpy(&header->type, (char *)ptr + 4, 4);
    header->data = ((uint32_t *)ptr + 2);
    header->crc = reverse_32(*(uint32_t *)(((char *)ptr + PNG_DATA_OFFSET + header->length)));
    return 1;
}

/* calculate the CRC32 of the chunk and cross-reference it with the chunk's stored CRC value */
int verify_chunk_header(const struct png_chunk_header * const header) {
    uint32_t crc = crc32(((char *)header->data) - 4, sizeof(uint32_t) + header->length);
    if (header->crc == crc) return 1;
    return 0;
}

/* print the data in a png_chunk_header that has already been filled */
void print_chunk_header(const struct png_chunk_header * const header) {
    static const char * critical[] = {"IHDR", "PLTE", "IDAT", "IEND", 0};
    int is_critical = 0;
    for (int i = 0; critical[i] != 0; i++) {
        if (strncmp(header->type, critical[i], 4) == 0) {
            is_critical = 1;
            break;
        }
    }
    
    printf("header.length = %u bytes\n", header->length);
    printf("  header.type = %.4s (%s)\n", header->type, is_critical ? "CRITICAL" : "ANCILLARY");
    printf("   header.crc = %u (%s)\n\n", header->crc, verify_chunk_header(header) ? "VALID" : "INVALID");
}

#endif // _PNG_H_
