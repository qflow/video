#pragma once
#include "ffmpeg.h"
#include "size.h"

namespace qflow {
	namespace video {
		class converter
		{
		public:
			converter(AVPixelFormat sourcePixelFormat, size sourceSize,
				AVPixelFormat destPixelFormat, size destSize)
			{
				_context.reset(sws_getContext(sourceSize.width, sourceSize.height, sourcePixelFormat, //from
					destSize.width, destSize.height, destPixelFormat, //to
					SWS_BICUBIC, NULL, NULL, NULL));
				_to_frame = AVFramePointer(alloc_frame(destPixelFormat, destSize.width, destSize.height), AVFrameDeleter());
			}
			AVFramePointer convert(AVFramePointer frame)
			{
				int outHeight = sws_scale(_context.get(), frame->data, frame->linesize, 
					0, frame->height, _to_frame->data, _to_frame->linesize);
				assert(_to_frame->height == outHeight);
				return _to_frame;
			}
		private:
			SwsContextPointer _context;
			AVFramePointer _to_frame;
		};
	}
}