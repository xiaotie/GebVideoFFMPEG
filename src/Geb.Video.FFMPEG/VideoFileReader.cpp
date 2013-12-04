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
ref struct VideoContext
{
public:
	libffmpeg::AVFormatContext*		FormatContext;
	libffmpeg::AVStream*			VideoStream;
	libffmpeg::AVCodecContext*		VideoCodecContext;
	libffmpeg::AVFrame*				VideoFrame;
	struct libffmpeg::SwsContext*	ConvertContext;

	libffmpeg::AVPacket* Packet;
	int BytesRemaining;

	VideoContext( )
	{
		FormatContext     = NULL;
		VideoStream       = NULL;
		VideoCodecContext      = NULL;
		VideoFrame        = NULL;
		ConvertContext	  = NULL;
		Packet  = NULL;
		BytesRemaining = 0;
	}
};

ref struct AudioContext
{
public:
	libffmpeg::AVFormatContext*		FormatContext;
	libffmpeg::AVStream*			AudioStream;
	libffmpeg::AVCodecContext*		AudioCodecContext;
	libffmpeg::AVPacket* Packet;
	int BytesRemaining;

	AudioContext()
	{
		FormatContext     = NULL;
		AudioStream		  = NULL;
		AudioCodecContext = NULL;
		Packet  = NULL;
		BytesRemaining = 0;
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
	videoContext->Packet = new libffmpeg::AVPacket( );
	videoContext->Packet->data = NULL;

	audioContext = gcnew AudioContext();
	audioContext->Packet = new libffmpeg::AVPacket( );
	audioContext->Packet->data = NULL;

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
		videoContext->FormatContext = open_file( nativeFileName );
		if ( videoContext->FormatContext == NULL )
		{
			throw gcnew System::IO::IOException( "Cannot open the video file." );
		}

		// retrieve stream information
		if ( libffmpeg::av_find_stream_info( videoContext->FormatContext ) < 0 )
		{
			throw gcnew Exception( "Cannot find stream information." );
		}

		audioContext->FormatContext = videoContext->FormatContext;

		// search for the first video stream
		for ( unsigned int i = 0; i < videoContext->FormatContext->nb_streams; i++ )
		{
			libffmpeg::AVStream* s = videoContext->FormatContext->streams[i];
			if( s->codec->codec_type == libffmpeg::AVMEDIA_TYPE_VIDEO )
			{
				// get the pointer to the codec context for the video stream
				videoContext->VideoCodecContext = s->codec;
				videoContext->VideoStream  = s;
			}
			else if(s->codec->codec_type == libffmpeg::AVMEDIA_TYPE_AUDIO)
			{
				audioContext->AudioCodecContext = s->codec;
				audioContext->AudioStream  = s;
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
				SWS_BICUBIC, NULL, NULL, NULL );

		if ( videoContext->ConvertContext == NULL )
		{
			throw gcnew Exception( "Cannot initialize frames conversion context." );
		}

		// get some properties of the video file
		m_width  = videoContext->VideoCodecContext->width;
		m_height = videoContext->VideoCodecContext->height;
		m_frameRate = videoContext->VideoStream->r_frame_rate.num / videoContext->VideoStream->r_frame_rate.den;
		m_codecName = gcnew String( videoContext->VideoCodecContext->codec->name );
		m_framesCount = videoContext->VideoStream->nb_frames;

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
		if ( audioContext->AudioStream != NULL )
		{
			libffmpeg::av_free( audioContext->AudioStream );
		}

		if ( audioContext->AudioCodecContext != NULL )
		{
			libffmpeg::avcodec_close( audioContext->AudioCodecContext );
		}

		if ( audioContext->Packet->data != NULL )
		{
			libffmpeg::av_free_packet( audioContext->Packet );
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

		if ( videoContext->Packet->data != NULL )
		{
			libffmpeg::av_free_packet( videoContext->Packet );
		}

		if ( videoContext->FormatContext != NULL )
		{
			libffmpeg::av_close_input_file( videoContext->FormatContext );
		}

		videoContext = nullptr;
	}
}

// Read next video frame of the current video file
ImageRgb24^ VideoFileReader::ReadVideoFrame(  )
{
    CheckIfDisposed( );

	if ( videoContext == nullptr )
	{
		throw gcnew System::IO::IOException( "Cannot read video frames since video file is not open." );
	}

	int frameFinished;
	ImageRgb24^ bitmap = nullptr;

	int bytesDecoded;
	bool exit = false;

	while ( true )
	{
		// work on the current packet until we have decoded all of it
		while ( videoContext->BytesRemaining > 0 )
		{
			// decode the next chunk of data
			bytesDecoded = libffmpeg::avcodec_decode_video2( videoContext->VideoCodecContext, videoContext->VideoFrame, &frameFinished, videoContext->Packet );

			// was there an error?
			if ( bytesDecoded < 0 )
			{
				throw gcnew Exception( "Error while decoding frame." );
			}

			videoContext->BytesRemaining -= bytesDecoded;
					 
			// did we finish the current frame? Then we can return
			if ( frameFinished )
			{
				return DecodeVideoFrame( );
			}
		}

		// read the next packet, skipping all packets that aren't
		// for this stream
		do
		{
			// free old packet if any
			if ( videoContext->Packet->data != NULL )
			{
				libffmpeg::av_free_packet( videoContext->Packet );
				videoContext->Packet->data = NULL;
			}

			// read new packet
			if ( libffmpeg::av_read_frame( videoContext->FormatContext, videoContext->Packet ) < 0)
			{
				exit = true;
				break;
			}
		}
		while ( videoContext->Packet->stream_index != videoContext->VideoStream->index );

		// exit ?
		if ( exit )
			break;

		videoContext->BytesRemaining = videoContext->Packet->size;
	}

	// decode the rest of the last frame
	bytesDecoded = libffmpeg::avcodec_decode_video2(
		videoContext->VideoCodecContext, videoContext->VideoFrame, &frameFinished, videoContext->Packet );

	// free last packet
	if ( videoContext->Packet->data != NULL )
	{
		libffmpeg::av_free_packet( videoContext->Packet );
		videoContext->Packet->data = NULL;
	}

	// is there a frame
	if ( frameFinished )
	{
		bitmap = DecodeVideoFrame( );
	}

	return bitmap;
}

int VideoFileReader::Seek(long long frameIndex, Boolean seekKeyFrame)
{
	if(videoContext == nullptr || videoContext->VideoCodecContext == NULL) return -1;
	libffmpeg::AVCodecContext* pCodecCtx = videoContext->VideoCodecContext;
	if(frameIndex < 0 || frameIndex >= this->FrameCount) return -1;
	long long timeBase = (long long(pCodecCtx->time_base.num) * AV_TIME_BASE) / long long(pCodecCtx->time_base.den);
	long long seekTarget = long long(frameIndex) * timeBase;
	int val = libffmpeg::av_seek_frame(videoContext->FormatContext, -1, seekTarget, seekKeyFrame ? AVSEEK_FLAG_FRAME : AVSEEK_FLAG_ANY);
	libffmpeg::	avcodec_flush_buffers(pCodecCtx);
	return val;
}

// Decodes video frame into managed Bitmap
ImageRgb24^ VideoFileReader::DecodeVideoFrame( )
{
	ImageRgb24^ img = gcnew ImageRgb24( videoContext->VideoCodecContext->width, videoContext->VideoCodecContext->height);
	
	libffmpeg::uint8_t* ptr = reinterpret_cast<libffmpeg::uint8_t*>( static_cast<void*>( img->Start ) );

	libffmpeg::uint8_t* srcData[4] = { ptr, NULL, NULL, NULL };
	int srcLinesize[4] = { img->Width * sizeof(Rgb24), 0, 0, 0 };

	// convert video frame to the RGB bitmap
	libffmpeg::sws_scale( videoContext->ConvertContext, videoContext->VideoFrame->data, videoContext->VideoFrame->linesize, 0,
		videoContext->VideoCodecContext->height, srcData, srcLinesize );

	return img;
}

bool VideoFileReader::DecodeAudioPacket(void* pPacket)
{
	int totalOutput = 0;
	libffmpeg::AVPacket& packet = *(libffmpeg::AVPacket*)pPacket;
	// Copy the data pointer to we can muck with it
	int packetSize = packet.size;
	byte* packetData = (byte*)packet.data;
	//do
	//{
	//	byte* pBuffer = m_audioBuff;
	//	int outputBufferUsedSize = AVCODEC_MAX_AUDIO_FRAME_SIZE - totalOutput; //Must be initialized before sending in as per docs

	//	short* pcmWritePtr = (short*)(pBuffer + totalOutput);

	//	int usedInputBytes = libffmpeg::avcodec_decode_audio2(m_avCodecCtx, pcmWritePtr, outputBufferUsedSize, packetData, packetSize);
	//	
	//	if (usedInputBytes < 0) //Error in packet, ignore packet
	//		break;

	//	if (outputBufferUsedSize > 0)
	//		totalOutput += outputBufferUsedSize;

	//	packetData += usedInputBytes;
	//	packetSize -= usedInputBytes;
	//}
	//while (packetSize > 0);

	//m_bufferUsedSize = totalOutput;
	return true;
}

} } }