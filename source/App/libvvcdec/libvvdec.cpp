/* -----------------------------------------------------------------------------
The copyright in this software is being made available under the BSD
License, included below. No patent rights, trademark rights and/or 
other Intellectual Property Rights other than the copyrights concerning 
the Software are granted under this license.

For any license concerning other Intellectual Property rights than the software, 
especially patent licenses, a separate Agreement needs to be closed. 
For more information please contact:

Fraunhofer Heinrich Hertz Institute
Einsteinufer 37
10587 Berlin, Germany
www.hhi.fraunhofer.de/vvc
vvc@hhi.fraunhofer.de

Copyright (c) 2018-2020, Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V. 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of Fraunhofer nor the names of its contributors may
   be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.


------------------------------------------------------------------------------------------- */

#include "libvvdec.h"

#include "vvdec/version.h"
#include "vvdecWrapper.h"

namespace
{

unsigned getComponentIndex(libvvdec_ColorComponent c)
{
  return (c == LIBvvdec_CHROMA_U) ? 1 : (c == LIBvvdec_CHROMA_V ? 2 : 0);
}

}

extern "C" {

  vvdecAPI const char *libvvdec_get_version(void)
  {
    return VVDEC_VERSION;
  }

  vvdecAPI libvvdec_context* libvvdec_new_decoder(void)
  {
    auto decCtx = new libvvdec::vvdecoderWrapper();
    if (!decCtx)
    {
      return nullptr;
    }

    auto ret = decCtx->init();
    if (ret != 0)
    {
      // Error initializing the decoder
      delete decCtx;
      return nullptr;
    }

    return (libvvdec_context*)decCtx;
  }

  vvdecAPI libvvdec_error libvvdec_set_logging_callback(libvvdec_context* decCtx, libvvdec_logging_callback callback, void *userData, libvvdec_loglevel loglevel)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !callback)
    {
      return LIBvvdec_ERROR;
    }

    d->setLogging(callback, userData, loglevel);
    return LIBvvdec_OK;
  }

  vvdecAPI libvvdec_error libvvdec_free_decoder(libvvdec_context* decCtx)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d)
    {
      return LIBvvdec_ERROR;
    }

    delete d;
    return LIBvvdec_OK;
  }

  vvdecAPI libvvdec_error libvvdec_push_nal_unit(libvvdec_context *decCtx, const unsigned char* data8, int length, bool &checkOutputPictures)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d)
    {
      return LIBvvdec_ERROR;
    }

    int iRet = 0;
    if (data8 == nullptr)
    {
      iRet = d->flush();
      if( iRet != vvdec::VVDEC_OK && iRet != vvdec::VVDEC_EOF )
      {
        return LIBvvdec_ERROR;
      }
    }
    else
    {
      if (!d->setAUData(data8, length))
      {
        return LIBvvdec_ERROR;
      }

      iRet = d->decode();

      if (iRet != vvdec::VVDEC_OK && iRet != vvdec::VVDEC_TRY_AGAIN)
      {
        return LIBvvdec_ERROR;
      }
    }

    checkOutputPictures = iRet != vvdec::VVDEC_TRY_AGAIN && d->gotFrame();

    return LIBvvdec_OK;
  }

  vvdecAPI uint64_t libvvdec_get_picture_POC(libvvdec_context *decCtx)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !d->gotFrame())
    {
      return 0;
    }
    
    return d->getFrame()->m_uiSequenceNumber;
  }

  vvdecAPI uint32_t libvvdec_get_picture_width(libvvdec_context *decCtx, libvvdec_ColorComponent c)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !d->gotFrame())
    {
      return 0;
    }
    
    auto f = d->getFrame();
    auto idx = getComponentIndex(c);
    if (idx >= f->m_uiNumComponents)
    {
      return 0;
    }

    return f->m_cComponent[idx].m_uiWidth;
  }

  vvdecAPI uint32_t libvvdec_get_picture_height(libvvdec_context *decCtx, libvvdec_ColorComponent c)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !d->gotFrame())
    {
      return 0;
    }
    
    auto f = d->getFrame();
    auto idx = getComponentIndex(c);
    if (idx >= f->m_uiNumComponents)
    {
      return 0;
    }

    return f->m_cComponent[idx].m_uiHeight;
  }

  vvdecAPI int32_t libvvdec_get_picture_stride(libvvdec_context *decCtx, libvvdec_ColorComponent c)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !d->gotFrame())
    {
      return 0;
    }
    
    auto f = d->getFrame();
    auto idx = getComponentIndex(c);
    if (idx >= f->m_uiNumComponents)
    {
      return 0;
    }

    return f->m_cComponent[idx].m_iStride;
  }

  vvdecAPI unsigned char* libvvdec_get_picture_plane(libvvdec_context *decCtx, libvvdec_ColorComponent c)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !d->gotFrame())
    {
      return 0;
    }
    
    auto f = d->getFrame();
    auto idx = getComponentIndex(c);
    if (idx >= f->m_uiNumComponents)
    {
      return nullptr;
    }

    return f->m_cComponent[idx].m_pucBuffer;
  }

  vvdecAPI libvvdec_ChromaFormat libvvdec_get_picture_chroma_format(libvvdec_context *decCtx)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !d->gotFrame())
    {
      return LIBvvdec_CHROMA_UNKNOWN;
    }

    auto f = d->getFrame();
    switch (f->m_eColorFormat)
    {
    case vvdec::VVC_CF_YUV400_PLANAR:
      return LIBvvdec_CHROMA_400;
    case vvdec::VVC_CF_YUV420_PLANAR:
      return LIBvvdec_CHROMA_420;
    case vvdec::VVC_CF_YUV422_PLANAR:
      return LIBvvdec_CHROMA_422;
    case vvdec::VVC_CF_YUV444_PLANAR:
      return LIBvvdec_CHROMA_444;
    default:
      return LIBvvdec_CHROMA_UNKNOWN;
    }
  }

  vvdecAPI uint32_t libvvdec_get_picture_bit_depth(libvvdec_context *decCtx, libvvdec_ColorComponent c)
  {
    auto d = (libvvdec::vvdecoderWrapper*)decCtx;
    if (!d || !d->gotFrame())
    {
      return 0;
    }
    
    auto f = d->getFrame();
    auto idx = getComponentIndex(c);
    if (idx >= f->m_uiNumComponents)
    {
      return 0;
    }

    return f->m_cComponent[idx].m_uiBitDepth;
  }

} // extern "C"