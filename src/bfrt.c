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
    void* jit_region;      /* executable memory for code */
    size_t code_size;      /* size of compiled code */
    void* tape_memory;     /* memory for BF tape */
    size_t tape_size;      /* size of BF tape memory */
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

/* Tell macOS that we want to use our custom writing callback */
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

    /* calculate size needed for code, page-aligned */
    size_t code_size = compiled->size * sizeof(uint32_t);
    size_t page_size = 4096; /* Standard page size */
    size_t aligned_code_size = ((code_size + page_size - 1) / page_size) * page_size;
    
    /* allocate executable memory for code only */
    void *code_region = mmap(NULL, aligned_code_size,  
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
    
    if (code_region == MAP_FAILED) 
    {
        perror("JIT code memory allocation failed");
        free(ctx);
        return NULL;
    }
    
    /* allocate separate memory for BF tape (non-executable) */
    void *tape_memory = mmap(NULL, BF_TAPE_SIZE,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                            
    if (tape_memory == MAP_FAILED) 
    {
        perror("BF tape memory allocation failed");
        munmap(code_region, aligned_code_size);
        free(ctx);
        return NULL;
    }
    
    ctx->jit_region = code_region;
    ctx->code_size = aligned_code_size;
    ctx->tape_memory = tape_memory;
    ctx->tape_size = BF_TAPE_SIZE;
    
    /* Zero out the tape memory */
    memset(ctx->tape_memory, 0, BF_TAPE_SIZE);
    
    /* Use pthread's secure writing mechanism to write to executable memory */
    JitWriteData write_data = {
        .dest = ctx->jit_region,
        .src = compiled->code,
        .size = code_size
    };
    
    int result = pthread_jit_write_with_callback_np(jit_writing_callback, &write_data);
    if (result != 0) 
    {
        perror("JIT code writing failed");
        munmap(tape_memory, BF_TAPE_SIZE);
        munmap(code_region, aligned_code_size);
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
        /* Free code memory */
        if (ctx->jit_region != MAP_FAILED && ctx->jit_region != NULL)
            munmap(ctx->jit_region, ctx->code_size);
            
        /* Free tape memory */
        if (ctx->tape_memory != MAP_FAILED && ctx->tape_memory != NULL)
            munmap(ctx->tape_memory, ctx->tape_size);
            
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