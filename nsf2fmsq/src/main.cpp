/*
 * nsf2fmsq - NSF to FMSQ converter CLI
 *
 * Usage: nsf2fmsq <input.nsf> [options]
 *   -t <track>     Track number (default: 0)
 *   -d <seconds>   Duration in seconds (default: 60)
 *   -o <output>    Output file (default: <input>_track<N>.fmsq)
 *   --dump         Print FMSQ commands to stdout
 */

#include "nsf2fmsq.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static void usage(const char* prog)
{
    printf("Usage: %s <input.nsf> [options]\n", prog);
    printf("  -t <track>     Track number (default: 0)\n");
    printf("  -d <seconds>   Duration in seconds (default: 60)\n");
    printf("  -o <output>    Output file (default: <input>_track<N>.fmsq)\n");
    printf("  --dump         Print FMSQ commands to stdout\n");
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    std::string output_path;
    fmsq_convert_options opts = {};
    opts.track = 0;
    opts.duration_sec = 60;
    opts.dump = false;

    /* Parse arguments */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            opts.track = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            opts.duration_sec = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--dump") == 0) {
            opts.dump = true;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    /* Generate default output filename */
    if (output_path.empty()) {
        std::string base(input_path);
        size_t dot = base.rfind('.');
        if (dot != std::string::npos) {
            base = base.substr(0, dot);
        }
        char suffix[32];
        snprintf(suffix, sizeof(suffix), "_track%d.fmsq", opts.track);
        output_path = base + suffix;
    }

    printf("nsf2fmsq - NSF to FMSQ converter\n");
    printf("Input:    %s\n", input_path);
    printf("Output:   %s\n", output_path.c_str());
    printf("Track:    %d\n", opts.track);
    printf("Duration: %d seconds\n", opts.duration_sec);
    printf("\n");

    /* Convert */
    fmsq_convert_result result;
    if (!nsf2fmsq_convert(input_path, opts, result)) {
        fprintf(stderr, "Conversion failed\n");
        return 1;
    }

    /* Write output file */
    if (!fmsq_write_file(output_path.c_str(), result)) {
        fprintf(stderr, "Failed to write output file\n");
        return 1;
    }

    printf("\nConversion complete.\n");
    return 0;
}
