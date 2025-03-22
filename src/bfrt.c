#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <libkern/OSCacheControl.h>
#include "bfc.h"

#define BF_TAPE_SIZE 30000

typedef struct
{
    void* jit_region;
    size_t region_size;
    void* tape_memory;
} JITContext;

typedef struct JitWriteData
{
    void* dest;
    void* src;
    size_t size;
} JitWriteData;

static int jit_writing_callback(void* context)
{
    JitWriteData* data = context;
    if (!data || !data->dest || !data->src || data->size == 0)
        return -1;

    memcpy(data->dest, data->src, data->size);
    return 0;
}

PTHREAD_JIT_WRITE_ALLOW_CALLBACKS_NP(jit_writing_callback);

JITContext* init_jit(CodeBuffer *compiled)
{
    if (!compiled)
    {
        fprintf(stderr, "No compiled code to execute\n");
        return NULL;
    }

    JITContext *ctx = malloc(sizeof(JITContext));
    if (!ctx) 
    {
        perror("JIT context allocation failed");
        return NULL;
    }

    /* calculate total memory needed:
     * 1. code region (page-aligned)
     * 2. Brainfuck tape memory (30,000 bytes) */
    size_t code_size = compiled->size * sizeof(uint32_t);
    size_t page_size = 4096; // Standard page size
    size_t code_region_size = ((code_size + page_size - 1) / page_size) * page_size;
    size_t total_size = code_region_size + BF_TAPE_SIZE;
    
    /* alloc executable memory with MAP_JIT flag for macOS */
    void *jit_region = mmap(NULL, total_size,  
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
    
    if (jit_region == MAP_FAILED) 
    {
        perror("JIT memory allocation failed");
        free(ctx);
        return NULL;
    }
    
    ctx->jit_region = jit_region;
    ctx->region_size = total_size;
    ctx->tape_memory = (char*)jit_region + code_region_size;
    
    /* IMPORTANT: we set the memory area to execute to ZERO to prevent malicious program to run */
    memset(ctx->tape_memory, 0, BF_TAPE_SIZE);
    JitWriteData write_data = {
        .dest = ctx->jit_region,
        .src = compiled->code,
        .size = code_size
    };
    
    /* use pthread JIT writing callback to securely write to executable memory */
    int result = pthread_jit_write_with_callback_np(jit_writing_callback, &write_data);
    if (result != 0) 
    {
        perror("JIT code writing failed");
        munmap(jit_region, total_size);
        free(ctx);
        return NULL;
    }
    
    /* invalidate instruction cache to ensure coherency on ARM64 */
    sys_icache_invalidate(ctx->jit_region, code_size);
    return ctx;
}

int exec_jit(JITContext* ctx)
{
    if (!ctx || !ctx->jit_region) 
    {
        fprintf(stderr, "Invalid JIT context\n");
        return -1;
    }
    
    /* the codegen produces a function that takes pointer to tape memory */
    typedef int (*jit_func_t)(void *);
    jit_func_t jit_func = (jit_func_t)ctx->jit_region;
    
    return jit_func(ctx->tape_memory); /* execute */
}

void free_jit(JITContext *ctx) 
{
    if (ctx) 
    {
        if (ctx->jit_region != MAP_FAILED && ctx->jit_region != NULL)
            munmap(ctx->jit_region, ctx->region_size);
        free(ctx);
    }
}

int jit_exec(CodeBuffer *compiled)
{
    JITContext *jit_ctx = init_jit(compiled);
    if (!jit_ctx) 
    {
        fprintf(stderr, "Failed to initialize JIT environment\n");
        return -1;
    }
    
    printf("Executing JIT compiled code...\n");
    
    int result = exec_jit(jit_ctx);
    
    free_jit(jit_ctx);
    
    if (result != 0) 
        fprintf(stderr, "JIT execution completed with non-zero code: %d\n", result);
    else
        printf("JIT execution completed successfully\n");
    
    return result;
}
