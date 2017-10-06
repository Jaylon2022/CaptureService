#pragma once
#include <windows.h>
#include "debug.h"
#ifndef INT64_C 
#define INT64_C(c) (c ## LL) 
#define UINT64_C(c) (c ## ULL) 
#endif 
#ifdef __cplusplus 
extern "C" {
#endif 
	/*Include ffmpeg header file*/
#include <libavformat/avformat.h> 
#include <libavcodec/avcodec.h> 
#include <libswscale/swscale.h> 

#include <libavutil/imgutils.h>  
#include <libavutil/opt.h>     
#include <libavutil/mathematics.h>   
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#ifdef __cplusplus 
}
#endif 

#include "TinyClient.h"
#include "protocol.h"
class  ICapture
{
public:
	virtual BOOL Init() = 0;
	virtual void Deinit() = 0;
	virtual BOOL CaptureImage() = 0;

	void SetClient(CTinyClient *client)
	{
		m_client = client;
	}

protected:
	CSRLog   m_log;/* 日志模块 */
	AVFrame *m_pRGBFrame;
	AVFrame *m_pYUVFrame;
	AVCodecContext *m_context;
	AVCodec *m_pCodecH264;
	BYTE    *m_yuv_buff;
	SwsContext *m_scxt;
	int      m_nWidth;
	int      m_nHeight;
	BYTE     *m_pH264Data;
	CTinyClient *m_client;
	FILE *fp;

	void CreateEncoder(int nWidth, int nHeight)
	{
		//fopen_s(&fp, "D:\\test123.h264", "wb");
		av_register_all();
		avcodec_register_all();
		
		m_pCodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
		m_context = avcodec_alloc_context3(m_pCodecH264);
		m_context->bit_rate = 3000000;// put sample parameters
		m_context->width = nWidth;
		m_context->height = nHeight;

		m_context->time_base.num = 1;
		m_context->time_base.den = 25;

		m_context->framerate.num = 25;
		m_context->framerate.den = 1;

		m_context->gop_size = 10;
		m_context->max_b_frames = 1;
		m_context->pix_fmt = AV_PIX_FMT_YUV420P;

		av_opt_set(m_context->priv_data, "preset", "superfast", 0);
		av_opt_set(m_context->priv_data, "tune", "zerolatency", 0);


		m_pRGBFrame = av_frame_alloc();  //(AVFrame *)av_malloc(sizeof(AVFrame));  //RGB帧数据
		m_pYUVFrame = av_frame_alloc();  //(AVFrame *)av_malloc(sizeof(AVFrame));  //YUV帧数据

		//int picture_size = avpicture_get_size(m_context->pix_fmt, m_context->width, m_context->height);

		if (avcodec_open2(m_context, m_pCodecH264, nullptr) <0) {
			m_log.SR_printf("could not open codec...\n");//异常处理
			return;
		}

		m_yuv_buff = (BYTE *)av_malloc((nWidth*nHeight * 3) / 2); // size for YUV 420
		m_pH264Data = (BYTE *)av_malloc(nWidth*nHeight * 4);

		//SWS_BICUBIC
		m_scxt = sws_getContext(nWidth, nHeight, AV_PIX_FMT_RGB32, nWidth, nHeight, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
		if (m_scxt == nullptr) {
			m_log.SR_printf("(m_scxt == nullptr");//异常处理
		}
		m_nWidth = nWidth;
		m_nHeight = nHeight;

		m_client->SendCreateEncoder(nWidth, nHeight);
	}

	void DestroyEncoder()
	{
		av_free(m_pRGBFrame);
		av_free(m_pYUVFrame);
		av_free(m_yuv_buff);
		av_free(m_pH264Data);
		avcodec_close(m_context);
		av_free(m_context);
		m_client->SendDestroyEncoder();
	}

	void  EncodeAndSend(BYTE *buff)
	{
		//m_log.SR_printf("EncodeAndSend");
		avpicture_fill((AVPicture*)m_pRGBFrame, (uint8_t*)buff, AV_PIX_FMT_RGB32, m_nWidth, m_nHeight);
		avpicture_fill((AVPicture*)m_pYUVFrame, m_yuv_buff, AV_PIX_FMT_YUV420P, m_nWidth, m_nHeight);

		int nRet = sws_scale(m_scxt, m_pRGBFrame->data, m_pRGBFrame->linesize, 0, m_context->height, m_pYUVFrame->data, m_pYUVFrame->linesize);

		int got_packet_ptr = 0;
		AVPacket avpkt;
		av_init_packet(&avpkt);
		avpkt.data = m_pH264Data;
		avpkt.size = m_context->width*m_context->height * 4;

		avcodec_encode_video2(m_context, &avpkt, m_pYUVFrame, &got_packet_ptr);
		m_pYUVFrame->pts++;

		if (avpkt.size > 0) {
			m_client->SendEncodeData(avpkt.data, avpkt.size);
			av_packet_unref(&avpkt);
		}
		//m_log.SR_printf("EncodeAndSend over");
	}
#if 0
	FILE *file[5];
	char *pBmp[5];
	int nWidth = 0;
	int nHeight = 0;
	int nDataLen = 0;
	int nLen;
	int i;
	

	av_register_all();
	avcodec_register_all();
	AVFrame *m_pRGBFrame = (AVFrame *)av_malloc(sizeof(AVFrame));  //RGB帧数据
	AVFrame *m_pYUVFrame = (AVFrame *)av_malloc(sizeof(AVFrame));  //YUV帧数据
	AVCodecContext *c = NULL;
	AVCodecContext *in_c = NULL;
	AVCodec *pCodecH264; //编码器
	uint8_t * yuv_buff;//
					   //查找h264编码器
	pCodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
	c = avcodec_alloc_context3(pCodecH264);
	c->bit_rate = 3000000;// put sample parameters
	c->width = nWidth;//
	c->height = nHeight;//
						// frames per second
	AVRational rate;
	rate.num = 1;
	rate.den = 25;
	c->time_base = rate;//(AVRational){1,25};
	c->gop_size = 10; // emit one intra frame every ten frames
	c->max_b_frames = 1;
	c->thread_count = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;//PIX_FMT_RGB24
									//打开编码器
	if (avcodec_open2(c, pCodecH264, NULL)<0) {
		printf("could not open codec...\n");//异常处理
	}
	int size = c->width * c->height;
	yuv_buff = (uint8_t *)av_malloc((size * 3) / 2); // size for YUV 420
	uint8_t * rgb_buff = (uint8_t*)av_malloc(nDataLen);
	//图象编码
	int outbuf_size = 512000;
	uint8_t * outbuf = (uint8_t*)av_malloc(outbuf_size);
	int u_size = 0;
	FILE *f = NULL;
	const char * filename = "D:\\myData.h264";
	f = fopen(filename, "wb");
	if (!f) {
		printf("could not open %s\n", filename);
		exit(1);
	}
	//初始化SwsContext
	SwsContext * scxt = sws_getContext(c->width, c->height, AV_PIX_FMT_RGB32, c->width, c->height, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
	AVPacket avpkt;
	for (i = 0; i < 25; ++i)//帧
	{
		int index = 0;
		memcpy(rgb_buff, pBmp[index] + 54, nDataLen);//只copy rgb数据
		avpicture_fill((AVPicture*)m_pRGBFrame, (uint8_t*)rgb_buff, AV_PIX_FMT_RGB32, nWidth, nHeight);
		//将YUV buffer 填充YUV Frame
		avpicture_fill((AVPicture*)m_pYUVFrame, (uint8_t*)yuv_buff, AV_PIX_FMT_YUV420P, nWidth, nHeight);
#if 0
		// 翻转RGB图像
		m_pRGBFrame->data[0] += m_pRGBFrame->linesize[0] * (nHeight - 1);
		m_pRGBFrame->linesize[0] *= -1;
		m_pRGBFrame->data[1] += m_pRGBFrame->linesize[1] * (nHeight / 2 - 1);
		m_pRGBFrame->linesize[1] *= -1;
		m_pRGBFrame->data[2] += m_pRGBFrame->linesize[2] * (nHeight / 2 - 1);
		m_pRGBFrame->linesize[2] *= -1;
#endif
		//将RGB转化为YUV
		sws_scale(scxt, m_pRGBFrame->data, m_pRGBFrame->linesize, 0, c->height, m_pYUVFrame->data, m_pYUVFrame->linesize);
		int got_packet_ptr = 0;
		av_init_packet(&avpkt);
		avpkt.data = outbuf;
		avpkt.size = outbuf_size;
		u_size = avcodec_encode_video2(c, &avpkt, m_pYUVFrame, &got_packet_ptr);
		m_pYUVFrame->pts++;
		if (u_size == 0) {
			fwrite(avpkt.data, 1, avpkt.size, f);
		}
	}
	fclose(f);
	av_free(m_pRGBFrame);
	av_free(m_pYUVFrame);
	av_free(rgb_buff);
	av_free(outbuf);
	avcodec_close(c);
	av_free(c);
	for (i = 0; i<5; i++) free(pBmp[i]);
	return 0;
#endif

	void SaveBitmapFile(PVOID pData, DWORD dwWidth, DWORD dwHight)
	{
		DWORD size = dwWidth * dwHight * 4;
		BITMAPFILEHEADER bfh;
		BITMAPINFOHEADER bih;
		memset(&bfh, 0, sizeof(BITMAPFILEHEADER));
		bfh.bfType = (WORD)0x4d42;  //bm
		bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bfh.bfSize = size + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		memset(&bih, 0, sizeof(BITMAPINFOHEADER));
		bih.biSize = sizeof(BITMAPINFOHEADER);
		bih.biWidth = dwWidth;
		bih.biHeight = dwHight;
		bih.biPlanes = 1;
		bih.biBitCount = 32;
		bih.biSizeImage = size;

		FILE *fp;
		static int  nIndex = 0;
		char szName[100];
		sprintf_s(szName, "D:\\surf%d.bmp", nIndex++);

		fopen_s(&fp, szName, "wb");
		if (fp == nullptr) {
			MessageBoxA(NULL, "open file failed", 0, 0);
			return;
		}
		fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, fp);
		fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, fp);
		fwrite(pData, 1, size, fp);
		fclose(fp);
	}
};



