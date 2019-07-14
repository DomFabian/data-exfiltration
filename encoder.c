#include <stdio.h>
#include <stdlib.h>
#include "png.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input PNG filepath> <payload filepath>\n", argv[0]);
        exit(1);
    }
    const char *png_name = argv[1];
    const char *payload_name = argv[2];
    const char *output_name = "output.png";

    // open the png for reading
    FILE *file_ptr = fopen(png_name, "rb");
    if (!file_ptr) {
        fprintf(stderr, "Error: Unable to open file '%s' for reading.\n", png_name);
        exit(1);
    }

    // obtain the png filesize
    fseek(file_ptr, 0, SEEK_END);
    const unsigned long int png_file_size = ftell(file_ptr);
    rewind(file_ptr);

    // read in data from the png to a buffer
    void *input_buf = malloc(png_file_size);
    if (!input_buf) {
        fprintf(stderr, "Error: Unable to get required memory.\n");
        exit(1);
    }
    long int read_bytes = fread(input_buf, 1, png_file_size, file_ptr);
    if (read_bytes != png_file_size) {
        fprintf(stderr, "Error: Could not read entire input png. Only read %ld/%ld bytes (%.1f%%).\n", 
            read_bytes, png_file_size, (float)read_bytes / (float)png_file_size * 100);
        exit(1);
    }
    printf("Read %ld bytes from file '%s'.\n", read_bytes, png_name);
    fclose(file_ptr);
    file_ptr = NULL;

    // ensure our file is actually a PNG
    if (!is_png(input_buf)) {
        fprintf(stderr, "Error: File '%s' is not a PNG file.\n", png_name);
        exit(1);
    }

    // open the payload for reading
    file_ptr = fopen(payload_name, "rb");
    if (!file_ptr) {
        fprintf(stderr, "Error: Unable to open file '%s' for reading.\n", payload_name);
        exit(1);
    }

    // obtain the payload filesize
    fseek(file_ptr, 0, SEEK_END);
    const unsigned long int payload_file_size = ftell(file_ptr);
    rewind(file_ptr);
    if (payload_file_size > PNG_MAX_CHUNK_SIZE) {
        fprintf(stderr, "Error: payload is too large (%ld bytes). Maximum allowed payload is %u bytes.\n", 
            payload_file_size, PNG_MAX_CHUNK_SIZE);
        exit(1);
    }

    // read in data from the payload to a buffer
    void *payload_buf = malloc(payload_file_size);
    if (!payload_buf) {
        fprintf(stderr, "Error: Unable to get required memory.\n");
        exit(1);
    }
    read_bytes = fread(payload_buf, 1, payload_file_size, file_ptr);
    if (read_bytes != payload_file_size) {
        fprintf(stderr, "Error: Could not read entire payload. Only read %ld/%ld bytes (%.1f%%).\n", 
            read_bytes, payload_file_size, (float)read_bytes / (float)payload_file_size * 100);
        exit(1);
    }
    printf("Read %ld bytes from file '%s'.\n", read_bytes, payload_name);
    fclose(file_ptr);
    file_ptr = NULL;

    // write the output to a file
    file_ptr = fopen(output_name, "wb");
    if (!file_ptr) {
        fprintf(stderr, "Error: Unable to create new file for output.\n");
        exit(1);
    }

    // write PNG signature so that output is still usable
    if (fwrite(input_buf, PNG_SIGNATURE_SIZE, 1, file_ptr) != 1) {
        fprintf(stderr, "Error: Could not write data to new file for output.\n");
        exit(1);
    }

    // inject the payload
    struct png_chunk_header header;
    char *ptr = (char *)input_buf + PNG_SIGNATURE_SIZE;
    while (ptr < (char *)input_buf + png_file_size) {
        // read in the chunk data
        if (read_chunk_header(ptr, &header) == 0) {
            fprintf(stderr, "Error: Could not read chunk header.\n");
            exit(1);
        }

        // if we have reached the last chunk in the png
        if (strncmp(header.type, "IEND", 4) == 0) {
            // insert our payload chunk first
            // the chunk must be represented as a buffer in memory in order to perform the CRC
            char *payload_chunk = malloc(PNG_CHUNK_MIN_LEN + payload_file_size);
            if (!payload_chunk) {
                fprintf(stderr, "Error: Unable to get required memory.\n");
                exit(1);
            }
            *((uint32_t *)payload_chunk) = reverse_32(payload_file_size);
            strncpy(payload_chunk + 4, "tEXt", 4);
            memcpy(payload_chunk + PNG_DATA_OFFSET, payload_buf, payload_file_size);
            *(uint32_t *)(payload_chunk + PNG_DATA_OFFSET + payload_file_size) = 
                reverse_32(crc32(payload_chunk + 4, 4 + payload_file_size));

            // write out to the file
            if (fwrite(payload_chunk, PNG_CHUNK_MIN_LEN + payload_file_size, 1, file_ptr) != 1) {
                fprintf(stderr, "Error: Could not write data to new file for output.\n");
                exit(1);
            }

            free(payload_chunk);
        }

        // copy over the chunk as usual
        if (fwrite(ptr, PNG_CHUNK_MIN_LEN + header.length, 1, file_ptr) != 1) {
            fprintf(stderr, "Error: Could not write data to new file for output.\n");
            exit(1);
        }

        // move pointer to the next chunk
        ptr += (PNG_CHUNK_MIN_LEN + header.length);
    }
    free(input_buf);
    free(payload_buf);
    fclose(file_ptr);
    file_ptr = NULL;

    // re-open file in read mode to double check our work
    file_ptr = fopen(output_name, "rb");
    if (!file_ptr) {
        fprintf(stderr, "Error: Unable to open file '%s' for reading.\n", output_name);
        exit(1);
    }

    // obtain the output filesize
    fseek(file_ptr, 0, SEEK_END);
    const unsigned long int output_file_size = ftell(file_ptr);
    rewind(file_ptr);

    // read in data from the output file to a buffer
    void *output_buf = malloc(output_file_size);
    if (!output_buf) {
        fprintf(stderr, "Error: Unable to get required memory.\n");
        exit(1);
    }
    read_bytes = fread(output_buf, 1, output_file_size, file_ptr);
    if (read_bytes != output_file_size) {
        fprintf(stderr, "Error: Could not read entire output file. Only read %ld/%ld bytes (%.1f%%).\n", 
            read_bytes, output_file_size, (float)read_bytes / (float)output_file_size * 100);
        exit(1);
    }

    // print statistics
    printf("\n***** Injection complete. Information on program output: *****\n\n");
    printf("Output filename: %s\n", output_name);
    printf("Output filesize: %ld bytes (%.1f%% larger than original '%s')\n\n", output_file_size, 
        (float)output_file_size / (float)png_file_size * 100 - 100, png_name);


    // iterate over all of the chunks in the output file
    long int valid_chunks = 0;
    long int total_chunks = 0;
    ptr = (char *)output_buf + PNG_SIGNATURE_SIZE;
    rewind(file_ptr);
    while (ptr < (char *)output_buf + output_file_size) {
        // read in the chunk data
        if (read_chunk_header(ptr, &header) == 0) {
            fprintf(stderr, "Error: Could not read chunk header.\n");
            exit(1);
        }
        
        print_chunk_header(&header);

        // move pointer to the next chunk
        ptr += (PNG_CHUNK_MIN_LEN + header.length);

        // compute stats
        if (verify_chunk_header(&header)) valid_chunks++;
        total_chunks++;
    }
    printf("%ld of %ld chunks (%.1f%%) are valid.\n", valid_chunks, total_chunks, 
        (float)valid_chunks / (float)total_chunks * 100);
    
    fclose(file_ptr);

    return 0;
}
