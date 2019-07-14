#include <stdio.h>
#include <stdlib.h>
#include "png.h"

int main (int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input PNG filepath> <output payload filepath>\n", argv[0]);
        exit(1);
    }
    char *png_name = argv[1];
    char *output_name = argv[2];

    // open the png for reading
    FILE *file_ptr = fopen(png_name, "rb");
    if (!file_ptr) {
        fprintf(stderr, "Error: Unable to open file '%s' for reading.\n", png_name);
        exit(1);
    }

    // get the png file size
    fseek(file_ptr, 0, SEEK_END);
    const unsigned long int png_file_size = ftell(file_ptr);
    rewind(file_ptr);

    // read in data from the png to a buffer
    char *png_buf = malloc(png_file_size);
    if (!png_buf) {
        fprintf(stderr, "Error: Unable to get required memory.\n");
        exit(1);
    }
    long int read_bytes = fread(png_buf, 1, png_file_size, file_ptr);
    if (read_bytes != png_file_size) {
        fprintf(stderr, "Error: Could not read entire input png. Only read %ld/%ld bytes (%.1f%%).\n", 
            read_bytes, png_file_size, (float)read_bytes / (float)png_file_size * 100);
        exit(1);
    }
    printf("Read %ld bytes from file '%s'.\n", read_bytes, png_name);
    fclose(file_ptr);
    file_ptr = NULL;

    // ensure our file is actually a PNG
    if (!is_png(png_buf)) {
        fprintf(stderr, "Error: File '%s' is not a PNG file.\n", png_name);
        exit(1);
    }

    // open the output for writing
    file_ptr = fopen(output_name, "wb");
    if (!file_ptr) {
        fprintf(stderr, "Error: Unable to open file '%s' for writing.\n", output_name);
        exit(1);
    }

    // iterate over all of the chunks in the output file
    struct png_chunk_header header;
    struct png_chunk_header next_header;
    char *ptr = png_buf + PNG_SIGNATURE_SIZE;
    char *next;
    while (ptr < png_buf + png_file_size) {
        // read in the chunk data
        if (read_chunk_header(ptr, &header) == 0) {
            fprintf(stderr, "Error: Could not read chunk header.\n");
            exit(1);
        }
        
        // quick error-checking
        if (strncmp(header.type, "IEND", 4) == 0) {
            fprintf(stderr, "Error: Unexpected error when reading file '%s': reached EOF.\n", png_name);
            exit(1);
        }

        // set next pointer to the next chunk
        next = ptr + (PNG_CHUNK_MIN_LEN + header.length);

        // read this data into the next_header
        if (read_chunk_header(next, &next_header) == 0) {
            fprintf(stderr, "Error: Could not read chunk header.\n");
            exit(1);
        }

        // if the next chunk is the last one
        if (strncmp(next_header.type, "IEND", 4) == 0) {
            // if our current chunk is a tEXt chunk
            if (strncmp(header.type, "tEXt", 4) == 0) {
                // we know that the payload lives in this chunk
                if (fwrite(ptr + PNG_DATA_OFFSET, header.length, 1, file_ptr) != 1) {
                    fprintf(stderr, "Error: Could not write data to new file for output.\n");
                    exit(1);
                }
                printf("\n***** Extraction complete. Information on extracted payload: *****\n\n");
                printf("Output filename: %s\n", output_name);
                printf("Output filesize: %u bytes (%.1f%% of PNG carrier file)\n\n", header.length, 
                    (float)header.length / (float)png_file_size * 100);
                break;
            }
        }
        // increment ptr to the next chunk
        ptr = next;
    }
    free(png_buf);
    fclose(file_ptr);

    // mark output as executable just in case... :)
    char command[strlen(output_name) + 9];
    strcpy(command, "chmod +x ");
    strcat(command, output_name);
    system(command);

    return 0;
}
