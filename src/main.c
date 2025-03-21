#include "bfc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_source_file(const char *filename) 
{
    FILE *input = fopen(filename, "r");
    if (!input)
    {
        perror("Error opening input file");
        return NULL;
    }

    /* get file size */
    fseek(input, 0, SEEK_END);
    size_t program_size = ftell(input);
    fseek(input, 0, SEEK_SET);

    char *program = malloc(program_size + 1);
    if (!program) 
    {
        perror("Memory allocation error");
        fclose(input);
        return NULL;
    }

    /* read the program into memory */
    size_t bytes_read = fread(program, 1, program_size, input);
    fclose(input);

    program[bytes_read] = '\0';
    return program;
}

/* write the compiled program to a binary file */
void write_binary_file(const char *filename, CodeBuffer *buf) 
{
    FILE *out = fopen(filename, "wb");
    if (!out) 
    {
        perror("Error opening output file");
        exit(1);
    }

    /* write the machine binary */
    fwrite(buf->code, sizeof(uint32_t), buf->size, out);
    fclose(out);
}

void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [options] <brainfuck_file> <output_file>\n",
            program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "  -v, --verbose     Print verbose output during compilation\n");
    fprintf(stderr, "  -O0               Disable optimizations\n");
    fprintf(stderr, "  -O1               Enable basic optimizations (default)\n");
    fprintf(stderr, "  -O2               Enable intermediate optimizations\n");
    fprintf(stderr, "  -O3               Enable aggressive optimizations\n");
    fprintf(stderr, "  -j, --jit         Enable JIT runtime execution\n");
    fprintf(stderr, "  -h, --help        Display this help message\n");
}


int main(int argc, char *argv[]) 
{
    int verbose = 0;
    int opt_level = 1;
    int use_jit = 0;

    int arg_idx = 1;
    while (arg_idx < argc && argv[arg_idx][0] == '-') 
    {
        if (strcmp(argv[arg_idx], "-v") == 0 ||
            strcmp(argv[arg_idx], "--verbose") == 0) 
        {
            verbose = 1;
        } 
        else if (strcmp(argv[arg_idx], "-O0") == 0) 
        {
            opt_level = 0;
        }
        else if (strcmp(argv[arg_idx], "-O1") == 0) 
        {
            opt_level = 1;
        } 
        else if (strcmp(argv[arg_idx], "-O2") == 0)
        {
            opt_level = 2;
        }
        else if (strcmp(argv[arg_idx], "-O3") == 0)
        {
            opt_level = 3;
        }
        else if (strcmp(argv[arg_idx], "-j") == 0 ||
                 strcmp(argv[arg_idx], "--jit") == 0)
        {
            use_jit = 1;
        }
        else if (strcmp(argv[arg_idx], "-h") == 0 ||
                strcmp(argv[arg_idx], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        } 
        else 
        {
            fprintf(stderr, "Unknown option: %s\n", argv[arg_idx]);
            print_usage(argv[0]);
            return 1;
        }
        arg_idx++;
    }

    if (argc - arg_idx < 2) 
    {
        fprintf(stderr, "Error: Missing input or output file\n");
        print_usage(argv[0]);
        return 1;
    }

    const char *input_file = argv[arg_idx];
    const char *output_file = argv[arg_idx + 1];
    char *program = read_source_file(input_file);
    if (!program)
        return 1;

    char *processed_program = preproc(program);
    if (!processed_program) 
    {
        fprintf(stderr, "Failed to preprocess input\n");
        free(program);
        return 1;
    }

    if (verbose) 
    {
        printf("Preprocessed program size: %zu bytes\n", strlen(processed_program));
    }

    TokenArray *tokens = tokenize(processed_program);
    if (!tokens) {
        fprintf(stderr, "Tokenization failed\n");
        free(program);
        free(processed_program);
        return 1;
    }

    if (verbose)
        printf("Token count: %zu\n", tokens->count);

    /* conv to IR; note that we skip AST generation because brainfuck is too simple for it */
    IRProgram *ir_program = parse(tokens);
    if (!ir_program) 
    {
        fprintf(stderr, "IR conversion failed\n");
        free(program);
        free(processed_program);
        free_tkarr(tokens);
        return 1;
    }

    if (verbose) 
    {
        printf("Before optimization: %zu\n", ir_program->count);
        ir_dump(ir_program);
    }
    
    if (opt_level >= 1) 
    {
        ir_program = optimize1(ir_program);
        if (verbose) 
        {
            printf("After basic optimization: %zu\n", ir_program->count);
            ir_dump(ir_program);
        }
    }
    
    if (opt_level >= 2) 
    {
        ir_program = optimize2(ir_program);
        if (verbose) 
        {
            printf("After intermediate optimization: %zu\n", ir_program->count);
            ir_dump(ir_program);
        }
    }
    
    if (opt_level >= 3) 
    {
        ir_program = optimize3(ir_program);
        if (verbose) 
        {
            printf("Final after advanced optimization: %zu\n", ir_program->count);
            ir_dump(ir_program);
        }
    }

    CodeBuffer *compiled = codegen(ir_program);
    if (!compiled) 
    {
        fprintf(stderr, "Compilation failed\n");
        free(program);
        free(processed_program);
        free_tkarr(tokens);
        free_ir_program(ir_program);
        return 1;
    }
    
    if (use_jit) 
    {
        if (verbose)
            printf("Using JIT runtime execution\n");
        
        /* jit_execute_compiled(compiled); */
        printf("JIT runtime execution not yet implemented\n");
    } 
    else /* standard AOT output */
    {
        write_binary_file(output_file, compiled);

        printf("Compiled successfully. Output written to %s\n", output_file);
        printf("Code size: %zu instructions (%zu bytes)\n", compiled->size,
                compiled->size * sizeof(uint32_t));
        printf("IR operations: %zu\n", ir_program->count);
    }
    
    free_code_buffer(compiled);
    free(program);
    free(processed_program);
    free_tkarr(tokens);
    free_ir_program(ir_program);
    return 0;
}