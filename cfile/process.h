#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *data;
} ImageData;

ImageData *process (AVFormatContext *pFormatCtx, int timeStamp);
int findVideoStream (AVFormatContext *pFormatCtx);
AVCodecContext *openCodec (AVCodecContext *pCodecCtx);
AVFrame *initAVFrame (AVCodecContext *pCodecCtx, uint8_t **frameBuffer);
AVFrame *readAVFrame (AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx,
    AVFrame *pFrameRGB, int videoStream, int timeStamp);
uint8_t *getFrameBuffer(AVFrame *pFrame, AVCodecContext *pCodecCtx);
