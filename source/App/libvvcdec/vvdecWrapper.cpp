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

#include "vvdecWrapper.h"

#include <algorithm>

namespace libvvdec
{

vvdecoderWrapper::vvdecoderWrapper()
{
  this->cAccessUnit.m_pucBuffer = nullptr;
  this->cAccessUnit.m_iBufSize = 0;
  this->cAccessUnit.m_uiCts = 0;
  this->cAccessUnit.m_bCtsValid = true;
  this->cAccessUnit.m_uiDts = 0;
  this->cAccessUnit.m_bDtsValid = true;
}

vvdecoderWrapper::~vvdecoderWrapper()
{
  this->closeDecoder();
}

int vvdecoderWrapper::init()
{
  vvdec::VVDecParameter cVVDecParameter;
  return this->cVVDec.init( cVVDecParameter );
}

bool vvdecoderWrapper::setAUData(const unsigned char* data8, int length)
{
  this->logMessage("Set AU data size " + std::to_string(length), LIBvvdec_LOGLEVEL_INFO);
  if (length > this->cAccessUnit.m_iBufSize)
  {
    this->logMessage("Reallocate buffer. New size " + 
                     std::to_string(length) + 
                     " previous size " + 
                     std::to_string(this->cAccessUnit.m_iBufSize), LIBvvdec_LOGLEVEL_VERBOSE);
    if (this->cAccessUnit.m_pucBuffer != nullptr)
    {
      delete[] this->cAccessUnit.m_pucBuffer;
    }
    this->cAccessUnit.m_pucBuffer = new unsigned char[length];
    if (this->cAccessUnit.m_pucBuffer == nullptr)
    {
      this->cAccessUnit.m_iBufSize = 0;
      return false;
    }
    this->cAccessUnit.m_iBufSize = length;
  }
  std::copy_n( data8 , length, this->cAccessUnit.m_pucBuffer);
  this->cAccessUnit.m_iUsedSize = length;
  return true;
}

int vvdecoderWrapper::decode()
{
  this->logMessage("Decode AU size " + std::to_string(this->cAccessUnit.m_iUsedSize), LIBvvdec_LOGLEVEL_INFO);
  if (this->flushing)
  {
    this->logMessage("Decoder is in flushing mode", LIBvvdec_LOGLEVEL_ERROR);
    return vvdec::VVDEC_ERR_UNSPECIFIED;
  }
  this->unrefCurrentFrame();
  auto ret = this->cVVDec.decode( this->cAccessUnit, &pcFrame );
  if (ret != vvdec::VVDEC_OK && ret != vvdec::VVDEC_TRY_AGAIN)
  {
    this->logMessage(this->cVVDec.getLastError(), LIBvvdec_LOGLEVEL_ERROR);
  }
  return ret;
}

int vvdecoderWrapper::flush()
{
  this->logMessage("Flush decoder", LIBvvdec_LOGLEVEL_INFO);
  this->unrefCurrentFrame();
  this->flushing = true;
  auto ret = this->cVVDec.flush( &this->pcFrame );
  if (this->pcFrame == nullptr)
  {
    this->closeDecoder();
  }
  return ret;
}

bool vvdecoderWrapper::gotFrame() const
{
  return this->pcFrame != nullptr && this->pcFrame->m_bCtsValid;
}

vvdec::Frame* vvdecoderWrapper::getFrame() const
{
  return this->pcFrame;
}

void vvdecoderWrapper::setLogging(libvvdec_logging_callback callback, void *userData, libvvdec_loglevel level)
{
  this->loggingCallback = callback;
  this->loglevel = level;
  this->loggingUserData = userData;

  this->logMessage("Logging callback set", LIBvvdec_LOGLEVEL_INFO);
}

void vvdecoderWrapper::unrefCurrentFrame()
{
  if (this->pcFrame != nullptr)
  {
    this->logMessage("Unref frame " + std::to_string(this->pcFrame->m_uiSequenceNumber), LIBvvdec_LOGLEVEL_VERBOSE);
    this->cVVDec.objectUnref( this->pcFrame );
    this->pcFrame = nullptr;
  }
}

void vvdecoderWrapper::closeDecoder()
{
  this->logMessage("Closing decoder", LIBvvdec_LOGLEVEL_INFO);
  this->unrefCurrentFrame();
  if (this->cAccessUnit.m_pucBuffer != nullptr)
  {
    delete [] this->cAccessUnit.m_pucBuffer;
    this->cAccessUnit.m_pucBuffer = nullptr;
  }
  this->isEnd = true;
}

void vvdecoderWrapper::logMessage(std::string msg, libvvdec_loglevel level)
{
  if (!this->loggingCallback)
  {
    return;
  }

  this->loggingCallback(this->loggingUserData, level, msg.c_str());
}

}