/*
 * FMSQ binary file writer
 */

#include "fmsq_format.h"
#include "nsf2fmsq.h"

#include <cstdio>
#include <cstring>

bool fmsq_write_file(const char* path, const fmsq_convert_result& result)
{
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open output file: %s\n", path);
        return false;
    }

    /* Write header */
    fmsq_header_t header;
    memcpy(header.magic, "FMSQ", 4);
    header.version = 1;
    header.flags = 0;
    header.frame_count = result.frame_count;
    header.data_size = (uint16_t)result.data.size();
    header.loop_offset = result.loop_offset;

    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to write header\n");
        fclose(fp);
        return false;
    }

    /* Write command data */
    if (result.data.size() > 0) {
        if (fwrite(result.data.data(), 1, result.data.size(), fp) != result.data.size()) {
            fprintf(stderr, "Error: Failed to write command data\n");
            fclose(fp);
            return false;
        }
    }

    fclose(fp);

    size_t total_size = sizeof(header) + result.data.size();
    printf("Written: %s (%zu bytes: %zu header + %zu data)\n",
           path, total_size, sizeof(header), result.data.size());

    return true;
}
