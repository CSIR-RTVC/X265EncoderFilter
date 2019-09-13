/** @file

MODULE				: X265EncoderProperties

FILE NAME			: X265EncoderProperties.h

DESCRIPTION			: Properties for H264 Switch Encoder filter.

LICENSE: Software License Agreement (BSD License)

Copyright (c) 2014, Meraka Institute
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of the Meraka Institute nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
#pragma once
#include <DirectShowExt/FilterPropertiesBase.h>
#include <DirectShowExt/ParameterConstants.h>
#include <cassert>
#include <climits>
#include "resource.h"

#define BUFFER_SIZE 256

const int RADIO_BUTTON_IDS[] = { IDC_RADIO_VPP, IDC_RADIO_H264, IDC_RADIO_AVC1 };

class X265EncoderProperties : public FilterPropertiesBase
{
public:

  static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
  {
    X265EncoderProperties *pNewObject = new X265EncoderProperties(pUnk);
    if (pNewObject == NULL) 
    {
      *pHr = E_OUTOFMEMORY;
    }
    return pNewObject;
  }

  X265EncoderProperties::X265EncoderProperties(IUnknown *pUnk) : 
    FilterPropertiesBase(NAME("X265 Properties"), pUnk, IDD_H264PROP_DIALOG, IDD_H264PROP_CAPTION)
  {;}

  BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    // Let the parent class handle the message.
    return FilterPropertiesBase::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
  }

  HRESULT ReadSettings()
  {
    initialiseControls();
    return S_OK;
  }

  HRESULT OnApplyChanges(void)
  {
    return S_OK;
  } 

private:

  void initialiseControls()
  {
    InitCommonControls();
  }
};

