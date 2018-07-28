#include <emscripten.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "process.h"

int read_packet(void *opaque, uint8_t *buf, int buf_size);

int main () {
    av_register_all();
    fprintf(stdout, "ffmpeg init done\n");
    return 0;
}

typedef struct {
    uint8_t *ptr;
    size_t size;
} buffer_data;

buffer_data bufferData;

EMSCRIPTEN_KEEPALIVE //这个宏表示这个函数要作为导出的函数
ImageData *setFile(uint8_t *buff, const int buffLength, int timeStamp) {

    unsigned char *avio_ctx_buffer = NULL;
    size_t avio_ctx_buffer_size = buffLength;
    
    // AVInputFormat* in_fmt = av_find_input_format("h265");

    bufferData.ptr = buff;  /* will be grown as needed by the realloc above */
    bufferData.size = buffLength; /* no data at this point */

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    uint8_t *avioCtxBuffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);
    
    /* 读内存数据 */
    AVIOContext *avioCtx = avio_alloc_context(avioCtxBuffer, avio_ctx_buffer_size, 0, NULL, read_packet, NULL, NULL);

    pFormatCtx->pb = avioCtx;
    pFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;

    /* 打开内存缓存文件, and allocate format context */
    // pFormatCtx->probesize = 10000 * 1024;
    // pFormatCtx->max_analyze_duration = 100 * AV_TIME_BASE;
    if (avformat_open_input(&pFormatCtx, "", NULL, NULL) < 0) {
        fprintf(stderr, "Could not open input\n");
        return NULL;
    }
    av_dump_format(pFormatCtx, 0, "", 0);
    ImageData *result = process(pFormatCtx, timeStamp);
    // 清掉内存
    avformat_close_input(&pFormatCtx);
    av_free(avioCtx->buffer);
    av_free(avioCtx);
    av_free(avioCtxBuffer);
    return result;
}


int read_packet(void *opaque, uint8_t *buf, int buf_size) {

    buf_size = FFMIN(buf_size, bufferData.size);

    if (!buf_size)
        return AVERROR_EOF;
    // printf("ptr:%p size:%zu bz%zu\n", bufferData.ptr, bufferData.size, buf_size);

    /* copy internal buffer data to buf */
    memcpy(buf, bufferData.ptr, buf_size);
    bufferData.ptr += buf_size;
    bufferData.size -= buf_size;

    return buf_size;
}

