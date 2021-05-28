#include "format.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#define BILLION  1000000000L;
extern char* optarg;
extern int opterr, optind, optopt;
static FILE* input_file = NULL;
static FILE* output_file = NULL;
static size_t block_size = 512;
static size_t num_blocks_copied = 0; // defaults to entire file
static size_t blocks_skip_input = 0;
static size_t blocks_skip_output = 0;
static int read_stdin = 0;
static int write_stdout = 0;
static int copy_entire_file = 0;
static size_t input_file_size = 0;

void exit_process(int exit_code) {
    //printf("reached exit process\n");
    fflush(stdout);
    if (input_file) fclose(input_file);
    if (output_file) fclose(output_file);
    exit(exit_code);
}

static int print_output = 0;
void signalHandler(int signal) {
    print_output = 1;
}

int main(int argc, char **argv) {
    /*
    * -i <file>: input file (default: stdin)
    * -o <file>: output file (default stdout) - create file if doesnt exist
    * -b <size>: block size, #of bytes copied at a time (default: 512)
    * -p <count>: number of BLOCKS to skip at start of input file (default: 0)
    * -k <count>: number of BLOCKS to skip at the start of the output file (default: 0)
    * -c <size>: total number of BLOCKS copied (default: entire file)
    * any other parameters: exit(1);
    */
    signal(SIGUSR1, &signalHandler);
    char opt = 0;

    while (opt != -1) {
        opt = getopt(argc, argv, "i:o:b:c:p:k:");
        //printf("opt == %c\n", opt);
        if (opt == 'i') {
            if (access(optarg, F_OK) != -1) {
                input_file = fopen(optarg, "r");
            } else {
                print_invalid_input(optarg);
                exit_process(1);
            }
        } else if (opt == 'o') {
            output_file = fopen(optarg, "r+");
            //printf("here\n");
            if (!output_file) {
                //printf("invalid output");
                // try creating file
                output_file = fopen(optarg, "w+");
                if (!output_file) {
                    print_invalid_output(optarg);
                    exit_process(1);
                }
            }
        } else if (opt == 'b') {
            block_size = atoi(optarg);
        } else if (opt == 'c') {
            num_blocks_copied = atoi(optarg);
        } else if (opt == 'p') {
            blocks_skip_input = atoi(optarg);
        } else if (opt == 'k') {
            blocks_skip_output = atoi(optarg);
        } else {
            if (opt == -1) break;
            //printf("shouldnt end here, opt == %c\n", opt);
            exit_process(1);
        }
    }
    //printf("we are here\n");
    size_t last_block = block_size;
    long int input_offset = block_size * blocks_skip_input;
    long int output_offset = block_size * blocks_skip_output;
    if (input_file == NULL) {
        //printf("We should read from stdin\n");
        read_stdin = 1;
        //fseek(stdin, input_offset, SEEK_SET);
    } else {
        //printf("input seek\n");
        // get input file size
        fseek(input_file, 0L, SEEK_END);
        input_file_size = ftell(input_file);
        //printf("input file size = %lu\n", input_file_size);
        if (input_file_size != 0 && block_size > input_file_size) block_size = input_file_size;
        //printf("adjusting block size to be == %lu\n", block_size);
        if (input_file_size % block_size != 0) {
            last_block = input_file_size % block_size;
            //printf("last block is %zu\n", last_block);
        }
        if (num_blocks_copied != 0 && input_file_size != 0 && num_blocks_copied*block_size > input_file_size-input_offset) {
            num_blocks_copied = 0;
            //TODO: printf("Num blocks to copy has been changed to == %zu\n", num_blocks_copied);
        }
        fseek(input_file, 0L, SEEK_SET);
        fseek(input_file, input_offset, SEEK_SET);
    }

    if (num_blocks_copied == 0) {
        //printf("Should read from entire file\n");
        copy_entire_file = 1;
    }

    if (output_file == NULL) {
        //printf("should write to stdout\n");
        write_stdout = 1;
    } else {
        //printf("output fseek\n");
        fseek(output_file, output_offset, SEEK_SET);
    }

    //size_t total_bytes = 0;
    struct timespec start, end;
    size_t full_blocks_in = 0, full_blocks_out = 0, partial_blocks_in = 0, partial_blocks_out = 0, total_bytes = 0;
    // handle stdin case
    if (read_stdin == 1) {
        //printf("inside read_stdin == 1\n");
        char buffer[block_size];
        memset(buffer, 0, block_size);
        clock_gettime(CLOCK_MONOTONIC, &start);
        while(1) {
            size_t r = fread(buffer, block_size, 1, stdin);
            /*if (*input == EOF) {
                exit_process(0);
            }*/
            //printf("r = %lu\n", r);
            total_bytes += r*block_size;
            size_t w = -1;
            //printf("strlen buffer is == %zu\n", strlen(buffer));
            //printf("sizeof buffer is == %zu\n", sizeof(buffer));
            //printf("buffer in while(1) = %s\n", buffer);
            size_t size = block_size;
            int flag = 0;
            if (strlen(buffer)<block_size) {
                size = strlen(buffer);
                total_bytes += size;
                flag = 1;
                r++;
            }
            if (write_stdout == 1) {
                //printf("%s", buffer);
                w = fwrite(buffer, size, r, stdout);
                //fseek(stdout, block_size, SEEK_CUR);
            } else {
                w = fwrite(buffer, size, r, output_file);
                //fseek(output_file, block_size, SEEK_CUR);
            }

            if (r == 1 && flag == 0) {
                full_blocks_in++;
            } else if (r == 1 && flag == 1){
                partial_blocks_in++;
            }

            if (w == 1 && flag == 0) {
                full_blocks_out++;
            } else if (w == 1 && flag == 1){
                partial_blocks_out++;
            }
            if (feof(stdin) || print_output==1) {
                // print output here
                //printf("time to print output!\n");
                clock_gettime(CLOCK_MONOTONIC, &end);
                double time_elapsed = (end.tv_sec + 1e-9*end.tv_nsec) - (start.tv_sec + 1e-9*start.tv_nsec);
                //total_bytes = block_size*(full_blocks_in + partial_blocks_in);
                print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes, time_elapsed);
                print_output = 0;
                if (feof(stdin)) exit_process(0);
            }

        }
    }

    char buffer[block_size];
    //printf("blocksize == %zu\n", block_size);
    //if (buffer[0] == '\0')
    //    printf("buffer initially is == %s\n", buffer);
    size_t size = block_size;
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (copy_entire_file == 1) {
        //printf("num blocks to copy which should not be here == %lu\n", num_blocks_copied);
        while (!feof(input_file)) {
            if (print_output == 1) {
                clock_gettime(CLOCK_MONOTONIC, &end);
                double time_elapsed = (end.tv_sec + 1e-9*end.tv_nsec) - (start.tv_sec + 1e-9*start.tv_nsec);
                //total_bytes = block_size*(full_blocks_in + partial_blocks_in);
                print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes, time_elapsed);
                print_output = 0;
            }
            int flag = 0;
            if (total_bytes + size > input_file_size-input_offset) { flag = 1; size = last_block; }
            //printf("size inside loop == %lu\n", size);
            size_t r = fread(buffer, size, 1, input_file);
            //fseek(input_file, size, SEEK_CUR);
            //printf("r = %lu\n", r);
            total_bytes += r*size;
            //printf("total bytes so far == %zu\n", total_bytes);
            if (r == 1 && flag == 0) {
                full_blocks_in++;
            } else if (r == 1 && flag == 1) {
                partial_blocks_in++;
            }

            size_t w = -1;
            //printf("buffer = %s\n", buffer);
            if (write_stdout == 1) {
                //printf("buffer stdout = %s\n", buffer);
                w = fwrite(buffer, size, r, stdout);
                //fseek(stdout, block_size, SEEK_CUR);
            } else {
                //printf("buffer file = %s\n", buffer);
                //buffer[sizeof(buffer)-1] = 'a';
                w = fwrite(buffer, size, r, output_file);
                //fseek(output_file, block_size-1, SEEK_CUR);
            }
            //printf("w = %lu\n", w);
            if (w == 1 && flag == 0) {
                full_blocks_out++;
            } else if (w == 1 && flag == 1) {
                partial_blocks_out++;
            }
        }
    } else {
        size_t num_blocks_copied_already = 0;
        //printf("num blocks to copy == %lu\n", num_blocks_copied);
        while (num_blocks_copied_already != num_blocks_copied && !feof(input_file)) {
            //printf("num blocks copied already == %lu\n", num_blocks_copied_already);
            if (print_output == 1) {
                clock_gettime(CLOCK_MONOTONIC, &end);
                double time_elapsed = (end.tv_sec + 1e-9*end.tv_nsec) - (start.tv_sec + 1e-9*start.tv_nsec);
                //total_bytes = block_size*(full_blocks_in + partial_blocks_in);
                print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes, time_elapsed);
                print_output = 0;
            }
            size_t r = fread(buffer, size, 1, input_file);
            //printf("We crossed fread\n");
            total_bytes += r*size;
            //printf("total_bytes == %zu\n", total_bytes);
            if (r == 1) {
                full_blocks_in++;
            }

            size_t w = -1;
            if (write_stdout == 1) {
                //printf("stdout buffer == %s", buffer);
                w = fwrite(buffer, size, r, stdout);
                //fseek(stdout, block_size, SEEK_CUR);
            } else {
                //printf("buffer === %s\n", buffer);
                w = fwrite(buffer, size, r, output_file);
                //fseek(output_file, block_size, SEEK_CUR);
            }

            if (w == 1) {
                full_blocks_out++;
            }

            //printf("about to increment numBlocksCopiedAlready\n");
            num_blocks_copied_already++;
            //printf("incremented\n");
        }
    }
    //printf("reached end\n");
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_elapsed = (end.tv_sec + 1e-9*end.tv_nsec) - (start.tv_sec + 1e-9*start.tv_nsec);
    //total_bytes = block_size*(full_blocks_in + partial_blocks_in);
    print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes, time_elapsed);
    exit_process(0);
    return 0;
}
