#include "process.h"
#include <stdio.h>
#include <string.h>

ImageData *imageData = NULL;

ImageData *process (AVFormatContext *pFormatCtx, int timeStamp) {
    // av_register_all();
    // AVFormatContext *pFormatCtx = NULL;
    // 打开文件
    // if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) < 0) {
    //     return -1;
    // }
    // 找到音视频流
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        return NULL;
    }
    // 打印moov信息
    // av_dump_format(pFormatCtx, 0, argv[1], 0);
    // 找到视频流
    int videoStream = findVideoStream(pFormatCtx);
    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // 打开相应的解码器
    AVCodecContext *pNewCodecCtx = openCodec(pCodecCtx);
    if (!pNewCodecCtx) {
        fprintf(stderr, "openCodec failed, pNewCodecCtx is NULL\n");
        return NULL;
    }
    uint8_t *frameBuffer;
    AVFrame *pFrameRGB = initAVFrame(pNewCodecCtx, &frameBuffer);
    pFrameRGB = readAVFrame(pNewCodecCtx, pFormatCtx, pFrameRGB, videoStream, timeStamp); 
    if (pFrameRGB == NULL) {
        fprintf(stderr, "%s", "read AV frame failed");
        return NULL;
    }
    // if (imageData != NULL) {
    //     if (imageData->data != NULL) free(imageData->data);
    //     free(imageData);
    // }
    imageData = (ImageData *)malloc(sizeof(ImageData));
    imageData->width = (uint32_t)pNewCodecCtx->width;
    imageData->height = (uint32_t)pNewCodecCtx->height;
    // imageData->data = pFrameRGB->data;
    imageData->data = getFrameBuffer(pFrameRGB, pNewCodecCtx);
    // 释放内存
    avcodec_close(pCodecCtx);
    av_free(frameBuffer);

    return imageData;
    // saveFrame(pFrameRGB, pNewCodecCtx, 0);
    
    // free();    
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

uint8_t *getFrameBuffer(AVFrame *pFrame, AVCodecContext *pCodecCtx) {
    // FILE *pFile;
    // char szFilename[32];
    // int  y;
    int width = pCodecCtx->width;
    int height = pCodecCtx->height;

    // Open file
    // sprintf(szFilename, "frame%d.ppm", iFrame);
    // pFile = fopen(szFilename, "wb");
    // if (pFile == NULL) {
    //     return;
    // }
    // 
    // // Write header
    // fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    // Write pixel data
    uint8_t *buffer = (uint8_t *)malloc(height * width * 3);
    for (int y = 0; y < height; y++) {
        // fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
        memcpy(buffer + y * pFrame->linesize[0], pFrame->data[0] + y * pFrame->linesize[0], width * 3);
    }
    return buffer;
    // Close file
    // fclose(pFile);
}

AVFrame *readAVFrame (AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx,
    AVFrame *pFrameRGB, int videoStream, int timeStamp) {

    struct SwsContext *sws_ctx = NULL;
    int frameFinished;
    AVPacket packet;
    AVFrame *pFrame = NULL;
    // Allocate video frame
    pFrame = av_frame_alloc();

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
    int ret = av_seek_frame(pFormatCtx, videoStream, ((double)timeStamp/(double)33492.5)*AV_TIME_BASE
                + (double)pFormatCtx->start_time, AVSEEK_FLAG_BACKWARD);

    if (ret < 0) {
        fprintf(stderr, "av_seek_frame failed");
        return NULL;
    }
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
    	    // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
        
            // Did we get a video frame?
            if(frameFinished) {
            // Convert the image from its native format to RGB
                // sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
    		    //      pFrame->linesize, 0, pCodecCtx->height,
    		    //     pFrameRGB->data, pFrameRGB->linesize);
    	
                // return pFrameRGB;
                    // Save the frame to disk
                // if (++i == timeStamp) {
                    sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                         pFrame->linesize, 0, pCodecCtx->height,
                         pFrameRGB->data, pFrameRGB->linesize);

                    return pFrameRGB;
                     // SaveFrame(pFrameRGB, pCodecCtx->width, 
                     //         pCodecCtx->height, i);
                     // break;
                // }
            }
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
    return NULL;
}

AVFrame *initAVFrame (AVCodecContext *pCodecCtx, uint8_t **frameBuffer) {
    // Allocate an AVFrame structure
    AVFrame *pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL) {
        return NULL;
    }
    int numBytes;
    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    *frameBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, *frameBuffer, AV_PIX_FMT_RGB24,
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
        fprintf(stderr, "%s", "Unsupported codec!\n");
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
