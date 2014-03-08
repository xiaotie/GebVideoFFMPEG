// AForge FFMPEG Library
// AForge.NET framework
// http://www.aforgenet.com/framework/
//
// Copyright ?AForge.NET, 2009-2012
// contacts@aforgenet.com
//

#include "StdAfx.h"
#include "VideoFileReader.h"

namespace libffmpeg
{
	extern "C"
	{
		// disable warnings about badly formed documentation from FFmpeg, which we don't need at all
		#pragma warning(disable:4635) 
		// disable warning about conversion int64 to int32
		#pragma warning(disable:4244) 

		#include "libavformat\avformat.h"
		#include "libavformat\avio.h"
		#include "libavcodec\avcodec.h"
		#include "libswscale\swscale.h"
	}
}

namespace Geb { namespace Video { namespace FFMPEG
{
#pragma region Some private FFmpeg related stuff hidden out of header file

// A structure to encapsulate all FFMPEG related private variable
public ref struct VideoContext
{
public:
	libffmpeg::AVStream*			VideoStream;
	libffmpeg::AVCodecContext*		VideoCodecContext;
	libffmpeg::AVFrame*				VideoFrame;
	
	struct libffmpeg::SwsContext*	ConvertContext;
	int BytesRemaining;
	long long pts;
	long long dts;
	
	double CalcTime(long long pts)
	{
		//return pts;

		libffmpeg::AVStream* vs = VideoStream;
		libffmpeg::AVCodecContext* pCodecCtx = VideoCodecContext;
		if (vs->avg_frame_rate.den > 0 && vs->avg_frame_rate.num > 0)
		{
			return pts * vs->time_base.num / (double)(vs->time_base.den);
		}
		else if (vs->r_frame_rate.den > 0 && vs->r_frame_rate.num > 0)
		{
			return pts * vs->time_base.num / (double)(vs->time_base.den);
		}
		else
		{
			return pts * pCodecCtx->time_base.num / (double)(pCodecCtx->time_base.den);
		}
	}

	VideoContext( )
	{
		VideoStream       = NULL;
		VideoCodecContext      = NULL;
		VideoFrame        = NULL;
		ConvertContext	  = NULL;
		BytesRemaining = 0;
	}
};

public ref struct AudioContext
{
public:
	libffmpeg::AVStream*			AudioStream;
	short*							AudioFrame;
	libffmpeg::AVCodecContext*		AudioCodecContext;
	int BytesRemaining;

	AudioContext()
	{
		AudioStream		  = NULL;
		AudioCodecContext = NULL;
		AudioFrame = (short*)malloc(192000*100);
		BytesRemaining = 0;
	}

	property int Channels 
	{
		int get()
		{
			return AudioCodecContext->channels;
		}
	}

	property int SampleRate 
	{
		int get()
		{
			return AudioCodecContext->sample_rate;
		}
	}

	property int SampleSize 
	{
		int get()
		{
			switch (AudioCodecContext->sample_fmt)
                {
					case 0:
                        return 8;
                    case 1:
                        return 16;
                    case 2:
                        return 24;
                    case 3:
                        return 32;
                    default:
                        throw gcnew Exception("Unknown sample size.");
                }
		}
	}

	double CalcTime(long long pts)
	{
		//libffmpeg::AVStream* vs = AudioStream;
		libffmpeg::AVStream* vs = AudioStream;
		if (vs->time_base.den > 0 && vs->time_base.num > 0)
		{
			return pts * vs->time_base.num / (double)(vs->time_base.den);
		}
		else
		{
			return pts / (double)SampleRate;
		}
	}
};

public ref struct VideoFileContext
{
public:
	static const int VideoQueueCapability = 256;
	static const int AudioQueueCapability = 512;

	libffmpeg::AVFormatContext* FormatContext;
	Queue<IntPtr>^ VideoPackets; 
	Queue<IntPtr>^ AudioPackets;
	VideoContext^ VideoCxt;
	AudioContext^ AudioCxt;
	int videoStreamIndex;
	int audioStreamIndex;
	int minFrameCacheCount;
	long long videoPts;
	long long videoDts;
	long long audioPts;
	long long audioDts;

	void RemovePackagesBefore(double time)
	{
	}

	void ClearQueue()
	{
		IntPtr p = IntPtr::Zero;

		while(this->VideoPackets->Count > 0)
		{
			p = this->VideoPackets->Dequeue();
			libffmpeg::AVPacket* packet = (libffmpeg::AVPacket*)(void*)p;
			ClearPacket(packet);
		}

		while(this->AudioPackets->Count > 0)
		{
			p = this->AudioPackets->Dequeue();
			libffmpeg::AVPacket* packet = (libffmpeg::AVPacket*)(void*)p;
			ClearPacket(packet);
		}

		videoPts = -1;
		audioPts = -1;
		videoDts = -1;
		audioDts = -1;
	}

	void ClearPacket(libffmpeg::AVPacket* packet)
	{
		if(packet == NULL) return;
		if(packet->data != NULL) libffmpeg::av_free_packet( packet );
		delete packet;
	}

	void EnsureNextVideoPacket()
	{
		while(this->VideoPackets->Count < 1)
		{
			if( ReadNextPacket() == false) break;
		}
	}

	libffmpeg::AVPacket* NextVideoPacket()
	{
		IntPtr p = IntPtr::Zero;

		while(this->VideoPackets->Count <= this->minFrameCacheCount)
		{
			if(this->ReadNextPacket() == false) break;
		}

		if(this->VideoPackets->Count > 0)
		{
			p = this->VideoPackets->Dequeue();
		}
		else
		{
			while(this->ReadNextPacket() == true)
			{
				if(this->VideoPackets->Count > 0)
				{
					p = this->VideoPackets->Dequeue();
					break;
				}
			}
		}
		
		if(p == IntPtr::Zero)
		{
			return NULL;
		}
		else
		{
			libffmpeg::AVPacket* pkt = (libffmpeg::AVPacket*)(void*)p;
			return pkt;
		}
	}

	libffmpeg::AVPacket* NextAudioPacket(bool onlyReadCacheQuque)
	{
		IntPtr p = IntPtr::Zero;
		if(this->AudioPackets->Count > 0)
		{
			p = this->AudioPackets->Dequeue();
		}
		else if(onlyReadCacheQuque == false)
		{
			while(this->ReadNextPacket() == true)
			{
				if(this->AudioPackets->Count > 0)
				{
					p = this->AudioPackets->Dequeue();
					break;
				}
			}
		}
		
		if(p == IntPtr::Zero)
		{
			return NULL;
		}
		else
		{
			libffmpeg::AVPacket* pkt = (libffmpeg::AVPacket*)(void*)p;
			return pkt;
		}
	}

	bool ReadNextPacket()
	{
		libffmpeg::AVPacket* packet = new libffmpeg::AVPacket( );
		packet->data = NULL;
		int rtn = libffmpeg::av_read_frame( FormatContext, packet );
		if(packet ->data != NULL && packet->size > 0)
		{
			if(packet->stream_index == videoStreamIndex)
			{
				if(VideoPackets->Count >= VideoQueueCapability)
				{
					IntPtr p = VideoPackets->Dequeue();
					libffmpeg::AVPacket* pkt = (libffmpeg::AVPacket*)(void*)p;
					if(pkt != NULL)
					{
						libffmpeg::av_free_packet( pkt );
						delete pkt;
					}
				}

				VideoPackets->Enqueue((IntPtr)packet);
				videoPts = packet->pts;
				videoDts = packet->dts;
				return true;
			}
			else if(packet->stream_index == audioStreamIndex)
			{
				if(AudioPackets->Count >= AudioQueueCapability)
				{
					IntPtr p = AudioPackets->Dequeue();
					libffmpeg::AVPacket* pkt = (libffmpeg::AVPacket*)(void*)p;
					if(pkt != NULL)
					{
						libffmpeg::av_free_packet( pkt );
						delete pkt;
					}
				}

				AudioPackets->Enqueue((IntPtr)packet);
				audioPts = packet->pts;
				audioDts = packet->dts;
				return true;
			}
			else
			{
				libffmpeg::av_free_packet( packet );
				delete packet;
				return true;
			}
		}
		else
		{
			delete packet;
			return false;
		}
	}

	void SeekPacket(double time)
	{
		// 扫描现有的音频packets，清除过早的packets
		if(this->AudioPackets->Count > 0)
		{
			while(this->AudioPackets->Count > 0)
			{
				IntPtr p = this->AudioPackets->Peek();
				libffmpeg::AVPacket* pkt = (libffmpeg::AVPacket*)(void*)p;
				if( AudioCxt->CalcTime(pkt->dts) < time)
				{
					this->AudioPackets->Dequeue();
					libffmpeg::av_free_packet( pkt );
					delete pkt;
				}
				else
				{
					break;
				}
			}
		}

		// 扫描线有的视频 packets，清除过早的packets
		if(this->VideoPackets->Count > 0)
		{
			while(this->VideoPackets->Count > 0)
			{
				IntPtr p = this->VideoPackets->Peek();
				libffmpeg::AVPacket* pkt = (libffmpeg::AVPacket*)(void*)p;
				if( VideoCxt->CalcTime(pkt->dts) < time)
				{
					this->VideoPackets->Dequeue();
					libffmpeg::av_free_packet( pkt );
					delete pkt;
				}
				else
				{
					return;
				}
			}
		}

		double packetTime = 0;
		libffmpeg::AVPacket* packet = new libffmpeg::AVPacket( );
		packet->data = NULL;
		int rtn = libffmpeg::av_read_frame( FormatContext, packet );
		if(packet ->data != NULL)
		{
			if(packet->stream_index == videoStreamIndex)
			{
				packetTime = VideoCxt->CalcTime(packet->dts);
				if(packetTime < time)
				{
					libffmpeg::av_free_packet( packet );
					delete packet;
				}
				else
				{
					VideoPackets->Enqueue((IntPtr)packet);
					videoPts = packet->pts;
					videoDts = packet->dts;
					return;
				}
			}
			else if(packet->stream_index == audioStreamIndex)
			{
				packetTime = AudioCxt->CalcTime(packet->dts);
				if(packetTime < time)
				{
					libffmpeg::av_free_packet( packet );
					delete packet;
				}
				else
				{
					AudioPackets->Enqueue((IntPtr)packet);
					audioPts = packet->pts;
					audioDts = packet->dts;
				}
			}
			else
			{
				libffmpeg::av_free_packet( packet );
				delete packet;
			}
		}
		else
		{
			delete packet;
		}
	}

	VideoFileContext()
	{
		FormatContext = NULL;
		VideoPackets = gcnew Queue<IntPtr>();
		AudioPackets = gcnew Queue<IntPtr>();
		minFrameCacheCount = 0;
	}
};


#pragma endregion

// Class constructor
VideoFileReader::VideoFileReader( void ) :
    videoContext( nullptr ), audioContext(nullptr), disposed( false )
{	
	libffmpeg::av_register_all( );
	m_audioBuff = new byte[AVCODEC_MAX_AUDIO_FRAME_SIZE];
}

#pragma managed(push, off)
static libffmpeg::AVFormatContext* open_file( char* fileName )
{
	libffmpeg::AVFormatContext* formatContext;

	if ( libffmpeg::av_open_input_file( &formatContext, fileName, NULL, 0, NULL ) !=0 )
	{
		return NULL;
	}
	return formatContext;
}
#pragma managed(pop)

// Opens the specified video file
void VideoFileReader::Open( String^ fileName )
{
    CheckIfDisposed( );

	// close previous file if any was open
	Close( );

	videoContext = gcnew VideoContext( );
	audioContext = gcnew AudioContext();
	m_videoTime = 0;
	m_audioTime = 0;
	cxt = gcnew VideoFileContext();
	cxt->VideoCxt = videoContext;
	cxt->AudioCxt = audioContext;
	bool success = false;

	// convert specified managed String to UTF8 unmanaged string
	IntPtr ptr = System::Runtime::InteropServices::Marshal::StringToHGlobalUni( fileName );
    wchar_t* nativeFileNameUnicode = (wchar_t*) ptr.ToPointer( );
    int utf8StringSize = WideCharToMultiByte( CP_UTF8, 0, nativeFileNameUnicode, -1, NULL, 0, NULL, NULL );
    char* nativeFileName = new char[utf8StringSize];
    WideCharToMultiByte( CP_UTF8, 0, nativeFileNameUnicode, -1, nativeFileName, utf8StringSize, NULL, NULL );

	try
	{
		// open the specified video file
		cxt->FormatContext = open_file( nativeFileName );

		if ( cxt->FormatContext == NULL )
		{
			throw gcnew System::IO::IOException( "Cannot open the video file." );
		}
		// retrieve stream information
		if ( libffmpeg::av_find_stream_info( cxt->FormatContext ) < 0 )
		{
			throw gcnew Exception( "Cannot find stream information." );
		}

		// search for the first video stream
		for ( unsigned int i = 0; i < cxt->FormatContext->nb_streams; i++ )
		{
			libffmpeg::AVStream* s = cxt->FormatContext->streams[i];
			if( s->codec->codec_type == libffmpeg::AVMEDIA_TYPE_VIDEO )
			{
				// get the pointer to the codec context for the video stream
				videoContext->VideoCodecContext = s->codec;
				videoContext->VideoStream  = s;
				cxt->videoStreamIndex = s->index;
			}
			else if(s->codec->codec_type == libffmpeg::AVMEDIA_TYPE_AUDIO)
			{
				audioContext->AudioCodecContext = s->codec;
				audioContext->AudioStream  = s;
				cxt->audioStreamIndex = s->index;
			}
		}

		if ( videoContext->VideoStream == NULL )
		{
			throw gcnew Exception( "Cannot find video stream in the specified file." );
		}
		
		// find decoder for the video stream
		libffmpeg::AVCodec* codec = libffmpeg::avcodec_find_decoder( videoContext->VideoCodecContext->codec_id );
		if ( codec == NULL )
		{
			throw gcnew Exception( "Cannot find codec to decode the video stream." );
		}

		// open the codec
		if ( libffmpeg::avcodec_open( videoContext->VideoCodecContext, codec ) < 0 )
		{
			throw gcnew Exception( "Cannot open video codec." );
		}

		if(audioContext->AudioCodecContext != NULL)
		{
			libffmpeg::AVCodec* audioCodec = libffmpeg::avcodec_find_decoder(audioContext->AudioCodecContext->codec_id);
			if( audioCodec == NULL )
			{
				throw gcnew Exception( "Cannot find codec to decode the audio stream." );
			}

			if ( libffmpeg::avcodec_open( audioContext->AudioCodecContext, audioCodec ) < 0 )
			{
				throw gcnew Exception( "Cannot open audio codec." );
			}
		}

		// allocate video frame
		videoContext->VideoFrame = libffmpeg::avcodec_alloc_frame( );

		// prepare scaling context to convert RGB image to video format
		videoContext->ConvertContext = libffmpeg::sws_getContext( videoContext->VideoCodecContext->width, videoContext->VideoCodecContext->height, videoContext->VideoCodecContext->pix_fmt,
				videoContext->VideoCodecContext->width, videoContext->VideoCodecContext->height, libffmpeg::PIX_FMT_BGR24,
				SWS_BILINEAR, NULL, NULL, NULL );

		if ( videoContext->ConvertContext == NULL )
		{
			throw gcnew Exception( "Cannot initialize frames conversion context." );
		}

		libffmpeg::AVStream* vs = videoContext->VideoStream;

		// get some properties of the video file
		m_width  = videoContext->VideoCodecContext->width;
		m_height = videoContext->VideoCodecContext->height;
		m_codecName = gcnew String( videoContext->VideoCodecContext->codec->name );

		// 这里 AForge.Net 的代码有问题，无法读取 flv,f4v 的 frameCount
		if (vs->avg_frame_rate.den > 0 && vs->avg_frame_rate.num > 0)
		{
			m_frameRate = vs->avg_frame_rate.num / vs->avg_frame_rate.den;
		}
		else if (vs->r_frame_rate.den > 0 && vs->r_frame_rate.num > 0)
		{
			m_frameRate = vs->r_frame_rate.num / vs->r_frame_rate.den;
		}
		else
		{
			m_frameRate = videoContext->VideoCodecContext->time_base.den / videoContext->VideoCodecContext->time_base.num;
		}

		double duration = (double)(cxt->FormatContext->duration / (double)AV_TIME_BASE);
        if (duration < 0) duration = 0;
		m_framesCount = m_frameRate * duration;
		success = true;
	}
	finally
	{
		System::Runtime::InteropServices::Marshal::FreeHGlobal( ptr );
        delete [] nativeFileName;

		if ( !success )
		{
			Close( );
		}
	}
}

// Close current video file
void VideoFileReader::Close(  )
{
	if ( audioContext != nullptr )
	{
		if ( audioContext->AudioFrame != NULL )
		{
			delete audioContext->AudioFrame;
		}

		if ( audioContext->AudioCodecContext != NULL )
		{
			libffmpeg::avcodec_close( audioContext->AudioCodecContext );
		}

		audioContext = nullptr;
	}

	if ( videoContext != nullptr )
	{
		if ( videoContext->VideoFrame != NULL )
		{
			libffmpeg::av_free( videoContext->VideoFrame );
		}

		if ( videoContext->VideoCodecContext != NULL )
		{
			libffmpeg::avcodec_close( videoContext->VideoCodecContext );
		}

		if ( videoContext->ConvertContext != NULL )
		{
			libffmpeg::sws_freeContext( videoContext->ConvertContext );
		}

		videoContext = nullptr;
	}

	if ( cxt != nullptr )
	{
		cxt->ClearQueue();
		if ( cxt->FormatContext != NULL )
		{
			libffmpeg::av_close_input_file( cxt->FormatContext );
		}
		cxt = nullptr;
	}
}

ImageRgb24^ VideoFileReader::ReadVideoFrame()
{
	return ReadVideoFrame(this->Width,this->Height);
}

// Read next video frame of the current video file
ImageRgb24^ VideoFileReader::ReadVideoFrame(int width, int height)
{
    if(FetchVideoFrame() == false) return nullptr;
	else return DecodeVideoFrame(width,height);
}

ImageU8^ VideoFileReader::ReadVideoFrameU8()
{
	return ReadVideoFrameU8(this->Width,this->Height);
}

// Read next video frame of the current video file
ImageU8^ VideoFileReader::ReadVideoFrameU8(int width, int height)
{
    if(FetchVideoFrame() == false) return nullptr;
	else return DecodeVideoFrameU8(width,height);
}

bool VideoFileReader::FetchVideoFrame()
{
    CheckIfDisposed( );

	if ( videoContext == nullptr )
	{
		throw gcnew System::IO::IOException( "Cannot read video frames since video file is not open." );
	}

	int frameFinished;
	videoContext->BytesRemaining = 0;
	int bytesDecoded;
	bool exit = false;
	libffmpeg::AVPacket* packet = NULL;

	while ( true )
	{
		// 获取下一个视频packet
		cxt->ClearPacket(packet);
		packet = cxt->NextVideoPacket();
		if(packet == NULL) break;
		m_videoTime = videoContext->CalcTime(packet->dts);

		if(m_videoTime < 0 && packet->size == 0) break;

		// 解码 packet
		videoContext->BytesRemaining = packet->size;
		while (videoContext->BytesRemaining > 0 )
		{
			bytesDecoded = libffmpeg::avcodec_decode_video2( videoContext->VideoCodecContext, videoContext->VideoFrame, &frameFinished, packet );

			if ( bytesDecoded < 0 )
			{
				cxt->ClearPacket(packet);
				videoContext->BytesRemaining = 0;
				return false;
			}

			videoContext->BytesRemaining -= bytesDecoded;
			if ( frameFinished )
			{
				cxt->ClearPacket(packet);
				return true;
			}
		}
	}

	cxt->ClearPacket(packet);
	return false;
}

// Read next video frame of the current video file
array<Byte>^ VideoFileReader::ReadAudioFrame(  bool onlyCurrentVideoFrame  )
{
    CheckIfDisposed( );

	if ( audioContext == nullptr )
	{
		throw gcnew System::IO::IOException( "Cannot read video frames since video file is not open." );
	}

	int frameFinished;
	audioContext->BytesRemaining = 0;
	int bytesDecoded;
	bool exit = false;
	libffmpeg::AVPacket* packet = NULL;

	while ( true )
	{
		// 获取下一个视频packet
		cxt->ClearPacket(packet);
		packet = cxt->NextAudioPacket(onlyCurrentVideoFrame);
		if(packet == NULL) break;
		m_audioTime = audioContext->CalcTime(packet->dts);

		// 解码 packet
		audioContext->BytesRemaining = packet->size;
		while (audioContext->BytesRemaining > 0 )
		{
			frameFinished = 192000;
			bytesDecoded = libffmpeg::avcodec_decode_audio3( audioContext->AudioCodecContext, audioContext->AudioFrame, &frameFinished, packet );

			if ( bytesDecoded < 0 )
			{
				audioContext->BytesRemaining = 0;
				cxt->ClearPacket(packet);
				break;
			}

			audioContext->BytesRemaining -= bytesDecoded;
			if ( frameFinished > 0 )
			{
				cxt->ClearPacket(packet);
				return DecodeAudioFrame( frameFinished );
			}
		}
	}

	cxt->ClearPacket(packet);
	return nullptr;
}

double VideoFileReader::Seek(double time, Boolean seekKeyFrame)
{
	if(videoContext == nullptr || videoContext->VideoCodecContext == NULL) return -1;
	libffmpeg::AVCodecContext* pCodecCtx = videoContext->VideoCodecContext;
	libffmpeg::AVStream* vs = videoContext->VideoStream;
	long long seekTarget = time * AV_TIME_BASE;
	int flag = AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME;
	if(time <= 0) flag = AVSEEK_FLAG_FRAME;
	int val = libffmpeg::av_seek_frame(cxt->FormatContext, -1, seekTarget, flag);
	libffmpeg::	avcodec_flush_buffers(pCodecCtx);
	cxt->ClearQueue();
	cxt->EnsureNextVideoPacket();
	if(seekKeyFrame == false)
	{
		{
			// 刷新 CurrentVideoTime
			Geb::Image::ImageRgb24^ img = this->ReadVideoFrame();
			if(img != nullptr) delete img;
		}

		if(this->CurrentVideoTime < time)
		{
			while(true)
			{
				Geb::Image::ImageRgb24^ img = this->ReadVideoFrame();
				if(img == nullptr) break;
				else delete img;
				if(this->CurrentVideoTime >= time) break;
			}
			cxt->ClearQueue();
			cxt->EnsureNextVideoPacket();
		}
	}

	{
		// 刷新 CurrentVideoTime
		Geb::Image::ImageRgb24^ img = this->ReadVideoFrame();
		if(img != nullptr) delete img;
	}

	return CurrentVideoTime;
}

// Decodes video frame into managed Bitmap
ImageRgb24^ VideoFileReader::DecodeVideoFrame( int width, int height)
{
	ImageRgb24^ img = gcnew ImageRgb24(width, height);
	
	libffmpeg::uint8_t* ptr = reinterpret_cast<libffmpeg::uint8_t*>( static_cast<void*>( img->Start ) );

	libffmpeg::uint8_t* srcData[4] = { ptr, NULL, NULL, NULL };
	int srcLinesize[4] = { img->Width * sizeof(Rgb24), 0, 0, 0 };

	libffmpeg::SwsContext* pSwsCxt = libffmpeg::sws_getContext( videoContext->VideoCodecContext->width, videoContext->VideoCodecContext->height, videoContext->VideoCodecContext->pix_fmt,
				width, height, libffmpeg::PIX_FMT_BGR24,
				SWS_BICUBIC, NULL, NULL, NULL );
	libffmpeg::sws_scale( pSwsCxt, videoContext->VideoFrame->data, videoContext->VideoFrame->linesize, 0,
		videoContext->VideoCodecContext->height, srcData, srcLinesize );
	libffmpeg::sws_freeContext(pSwsCxt);
	return img;
}

// Decodes video frame into managed Bitmap
ImageU8^ VideoFileReader::DecodeVideoFrameU8( int width, int height)
{
	ImageU8^ img = gcnew ImageU8(width, height);
	
	libffmpeg::uint8_t* ptr = reinterpret_cast<libffmpeg::uint8_t*>( static_cast<void*>( img->Start ) );

	libffmpeg::uint8_t* srcData[4] = { ptr, NULL, NULL, NULL };
	int srcLinesize[4] = { img->Width, 0, 0, 0 };

	libffmpeg::SwsContext* pSwsCxt = libffmpeg::sws_getContext( videoContext->VideoCodecContext->width, videoContext->VideoCodecContext->height, videoContext->VideoCodecContext->pix_fmt,
				width, height, libffmpeg::PIX_FMT_GRAY8,
				SWS_BICUBIC, NULL, NULL, NULL );
	libffmpeg::sws_scale( pSwsCxt, videoContext->VideoFrame->data, videoContext->VideoFrame->linesize, 0,
		videoContext->VideoCodecContext->height, srcData, srcLinesize );
	libffmpeg::sws_freeContext(pSwsCxt);
	return img;
}

array<Byte>^ VideoFileReader::DecodeAudioFrame( int length )
{
	if(length <= 0) return nullptr;
	array<Byte>^ buff = gcnew array<Byte>( length );
	pin_ptr<unsigned char> ptr = &buff[0];
	memcpy(ptr,audioContext->AudioFrame, length);
	return buff;
}

} } }