#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <stdio.h>

int findVideoStream (AVFormatContext *pFormatCtx);
AVCodecContext *openCodec (AVCodecContext *pCodecCtx);
AVFrame *initAVFrame (AVCodecContext *pCodecCtx);
AVFrame *readAVFrame (AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx,
    AVFrame *pFrameRGB, int videoStream);
void saveFrame(AVFrame *pFrame, AVCodecContext *pCodecCtx, int iFrame);

int main (int argc, char *argv[]) {
    av_register_all();
    AVFormatContext *pFormatCtx = NULL;
    // 打开文件
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) < 0) {
        return -1;
    }
    // 找到音视频流
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        return -1;
    }
    // 打印moov信息
    av_dump_format(pFormatCtx, 0, argv[1], 0);
    // 找到视频流
    int videoStream = findVideoStream(pFormatCtx);
    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    printf("%d\n", pCodecCtx->codec_id == AV_CODEC_ID_H264); 

    // 打开相应的解码器
    AVCodecContext *pNewCodecCtx = openCodec(pCodecCtx);
    if (!pNewCodecCtx) {
        return -1;
    }
    AVFrame *pFrameRGB = initAVFrame(pNewCodecCtx); 
    pFrameRGB = readAVFrame(pNewCodecCtx, pFormatCtx, pFrameRGB, videoStream); 
    saveFrame(pFrameRGB, pNewCodecCtx, 0);
    
    // free();    
    return 0;
}

// void free () {
//     // Free the RGB image
//     av_free(buffer);
//     av_free(pFrameRGB);
//     
//     // Free the YUV frame
//     av_free(pFrame);
//     
//     // Close the codecs
//     avcodec_close(pCodecCtx);
//     avcodec_close(pCodecCtxOrig);
//     
//     // Close the video file
//     avformat_close_input(&pFormatCtx);
// }

/*
 * 下面的代码主要来自于：http://dranger.com/ffmpeg/tutorial01.html
 *
 */
void saveFrame(AVFrame *pFrame, AVCodecContext *pCodecCtx, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;
    int width = pCodecCtx->width;
    int height = pCodecCtx->height;

    // Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL) {
        return;
    }
    
    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    // Write pixel data
    for (y = 0; y < height; y++) {
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    }

    // Close file
    fclose(pFile);
}

AVFrame *readAVFrame (AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx,
    AVFrame *pFrameRGB, int videoStream) {

    struct SwsContext *sws_ctx = NULL;
    int frameFinished;
    AVPacket packet;
    AVFrame *pFrame = NULL;
    // Allocate video frame
    pFrame=av_frame_alloc();

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
        );
    
    int i = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
    	    // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
        
            // Did we get a video frame?
            if(frameFinished) {
            // Convert the image from its native format to RGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
    		         pFrame->linesize, 0, pCodecCtx->height,
    		        pFrameRGB->data, pFrameRGB->linesize);
    	
                return pFrameRGB;
                    // Save the frame to disk
                // if (++i <= 5) {
                //      SaveFrame(pFrameRGB, pCodecCtx->width, 
                //              pCodecCtx->height, i);
                //      break;
                // }
            }
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
}

AVFrame *initAVFrame (AVCodecContext *pCodecCtx) {
    // Allocate an AVFrame structure
    AVFrame *pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL) {
        return -1;
    }
    uint8_t *buffer = NULL;
    int numBytes;
    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24,
                pCodecCtx->width, pCodecCtx->height);
    return pFrameRGB;
}

int findVideoStream (AVFormatContext *pFormatCtx) {
    // Find the first video stream
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx -> nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    return videoStream;
}

AVCodecContext *openCodec (AVCodecContext *pCodecCtx) {
    AVCodec *pCodec = NULL;
    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return NULL; // Codec not found
    }
    // Copy context
    AVCodecContext *pNewCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_copy_context(pNewCodecCtx, pCodecCtx) != 0) {
        fprintf(stderr, "Couldn't copy codec context");
        return NULL; // Error copying codec context
    }
    // Open codec
    if (avcodec_open2(pNewCodecCtx, pCodec, NULL) < 0) {
        return NULL; // Could not open codec
    }
    return pNewCodecCtx;
}
