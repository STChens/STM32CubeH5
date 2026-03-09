
/* Includes ------------------------------------------------------------------*/
#include "mbedtls/sha512.h"
#include "mbedtls/error.h"

#if defined(MBEDTLS_SHA512_C)
#if defined(MBEDTLS_SHA512_ALT)
#include <string.h>
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ST_SHA512_TIMEOUT     ((uint32_t) 3)
/* #define ST_HW_CONTEXT_SAVING */   /* Allows hash buffers interleaving */

/* Private macro -------------------------------------------------------------*/
#define SHA512_VALIDATE_RET(cond)
#define SHA512_VALIDATE(cond)  

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize(void *v, size_t n)
{
    volatile unsigned char *p = (unsigned char *)v;
    while (n--)
    {
        *p++ = 0;
    }
}

void mbedtls_sha512_init(mbedtls_sha512_context *ctx)
{
    SHA512_VALIDATE( ctx != NULL );

    mbedtls_zeroize(ctx, sizeof(mbedtls_sha512_context));

    /* Enable HASH clock */
    __HAL_RCC_HASH_CLK_ENABLE();
}

void mbedtls_sha512_free(mbedtls_sha512_context *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    mbedtls_zeroize(ctx, sizeof(mbedtls_sha512_context));
}

void mbedtls_sha512_clone(mbedtls_sha512_context *dst,
                          const mbedtls_sha512_context *src)
{
    SHA512_VALIDATE( dst != NULL );
    SHA512_VALIDATE( src != NULL );

    *dst = *src;
}

int mbedtls_sha512_starts(mbedtls_sha512_context *ctx, int is384)
{
    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( is384 == 0 || is384 == 1 );

    ctx->hhash.Instance = HASH;

    /* HASH Configuration */
    if (HAL_HASH_DeInit(&ctx->hhash) != HAL_OK)
    {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

    ctx->hhash.Init.DataType = HASH_BYTE_SWAP;

    if (is384 == 1)
    {
      ctx->hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA384;

    } else {
      ctx->hhash.Init.Algorithm = HASH_ALGOSELECTION_SHA512;

    }

    if (HAL_HASH_Init(&ctx->hhash) != HAL_OK)
    {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

    ctx->is384 = is384;

    /* first block on 17 words */
    ctx->first = ST_SHA512_EXTRA_BYTES;

    ctx->sbuf_len = 0;

#ifdef ST_HW_CONTEXT_SAVING
    /* save hw context */
    HAL_HASH_ContextSaving(&ctx->hhash, ctx->ctx_save_regs);
#endif /* ST_HW_CONTEXT_SAVING */

    return 0;
}

int mbedtls_internal_sha512_process( mbedtls_sha512_context *ctx, const unsigned char data[ST_SHA512_BLOCK_SIZE] )
{
    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( (const unsigned char *)data != NULL );

#ifdef ST_HW_CONTEXT_SAVING
    /* restore hw context */
    HAL_HASH_ContextRestoring(&ctx->hhash, ctx->ctx_save_regs);
#endif /* ST_HW_CONTEXT_SAVING */

    if (HAL_HASH_Accumulate(&ctx->hhash, (uint8_t *) data, ST_SHA512_BLOCK_SIZE, ST_SHA512_TIMEOUT) != 0)
    {
      return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

#ifdef ST_HW_CONTEXT_SAVING
    /* save hw context */
    HAL_HASH_ContextSaving(&ctx->hhash, ctx->ctx_save_regs);
#endif /* ST_HW_CONTEXT_SAVING */

    return 0;
}

int mbedtls_sha512_update(mbedtls_sha512_context *ctx, const unsigned char *input, size_t ilen)
{
    size_t currentlen = ilen;

    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( ilen == 0 || input != NULL );

#ifdef ST_HW_CONTEXT_SAVING
    /* restore hw context */
    HAL_HASH_ContextRestoring(&ctx->hhash, ctx->ctx_save_regs);
#endif /* ST_HW_CONTEXT_SAVING */

    if (currentlen < (ST_SHA512_BLOCK_SIZE + ctx->first - ctx->sbuf_len))
    {
        /* only store input data in context buffer */
        memcpy(ctx->sbuf + ctx->sbuf_len, input, currentlen);
        ctx->sbuf_len += currentlen;
    }
    else
    {
        /* fill context buffer until ST_SHA512_BLOCK_SIZE bytes, and process it */
        memcpy(ctx->sbuf + ctx->sbuf_len, input, (ST_SHA512_BLOCK_SIZE + ctx->first - ctx->sbuf_len));
        currentlen -= (ST_SHA512_BLOCK_SIZE + ctx->first - ctx->sbuf_len);

        if (HAL_HASH_Accumulate(&ctx->hhash, (uint8_t *)(ctx->sbuf), ST_SHA512_BLOCK_SIZE + ctx->first, ST_SHA512_TIMEOUT) != 0)
        {
          return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        }

        /* Process following input data with size multiple of ST_SHA256_BLOCK_SIZE bytes */
        size_t iter = currentlen / ST_SHA512_BLOCK_SIZE;
        if (iter != 0)
        {
            if (HAL_HASH_Accumulate(&ctx->hhash, (uint8_t *)(input + ST_SHA512_BLOCK_SIZE + ctx->first - ctx->sbuf_len), (iter * ST_SHA512_BLOCK_SIZE), ST_SHA512_TIMEOUT) != 0)
            {
              return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            }
        }

        /* following blocks on 16 words */
        ctx->first = 0;

        /* Store only the remaining input data up to (ST_SHA256_BLOCK_SIZE - 1) bytes */
        ctx->sbuf_len = currentlen % ST_SHA512_BLOCK_SIZE;
        if (ctx->sbuf_len != 0)
        {
            memcpy(ctx->sbuf, input + ilen - ctx->sbuf_len, ctx->sbuf_len);
        }
    }

#ifdef ST_HW_CONTEXT_SAVING
    /* save hw context */
    HAL_HASH_ContextSaving(&ctx->hhash, ctx->ctx_save_regs);
#endif /* ST_HW_CONTEXT_SAVING */
    return 0;
}

int mbedtls_sha512_finish(mbedtls_sha512_context *ctx, unsigned char output[64])
{
    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( (unsigned char *)output != NULL );

#ifdef ST_HW_CONTEXT_SAVING
    /* restore hw context */
    HAL_HASH_ContextRestoring(&ctx->hhash, ctx->ctx_save_regs);
#endif /* ST_HW_CONTEXT_SAVING */

    /* Last accumulation for pending bytes in sbuf_len, then trig processing and get digest */
    if (HAL_HASH_AccumulateLast(&ctx->hhash, ctx->sbuf, ctx->sbuf_len, output,ST_SHA512_TIMEOUT) != 0)
    {
      return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

    ctx->sbuf_len = 0;

    return 0;
}

#endif /* MBEDTLS_SHA512_ALT*/
#endif /* MBEDTLS_SHA512_C */
