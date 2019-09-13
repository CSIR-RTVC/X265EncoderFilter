#include "stdafx.h"
#include "X265EncoderFilter.h"
#include <cassert>
#include <dvdmedia.h>
#include <wmcodecdsp.h>
#include <X265v2/X265v2.h>
#include <CodecUtils/ICodecv2.h>
#include <ImageUtils/RealRGB24toYUV420ConverterStl.h>
#include <CodecUtils/CodecConfigurationUtil.h>
#include <CodecUtils/H265Util.h>
#include <GeneralUtils/Conversion.h>

const unsigned char g_startCode[] = { 0, 0, 0, 1};

const REFERENCE_TIME FPS_25 = UNITS / 25;

using vpp::boolToString;

const unsigned MINIMUM_BUFFER_SIZE = 5024;
X265EncoderFilter::X265EncoderFilter()
  : CCustomBaseFilter(NAME("CSIR VPP X265 Encoder"), 0, CLSID_VPP_X265Encoder),
  m_pCodec(nullptr),
  m_bAnnexB(false),
  m_pSeqParamSet(0),
  m_uiSeqParamSetLen(0),
  m_pPicParamSet(0),
  m_uiPicParamSetLen(0),
  m_uiIFramePeriod(0),
  m_uiCurrentFrame(0),
  m_uiTargetBitrate(0),
  m_rtFrameLength(FPS_25),
  m_tStart(0),
  m_tStop(m_rtFrameLength),
  m_pConverter(nullptr),
  m_pYuvConversionBuffer(nullptr),
  m_uiConversionBufferSize(0)
{
	//Call the initialise input method to load all acceptable input types for this filter
	InitialiseInputTypes();
	initParameters();
  X265v2Factory factory;
	m_pCodec = factory.GetCodecInstance();
	// Set default codec properties 
	if (m_pCodec)
	{
    // TODO: configure default parameters
  }
	else
	{
		SetLastError("Unable to create X265 Encoder from Factory.", true);
	}
}

X265EncoderFilter::~X265EncoderFilter()
{
  if (m_pYuvConversionBuffer)
  {
    delete[] m_pYuvConversionBuffer;
    m_pYuvConversionBuffer = NULL;
  }

  if (m_pConverter)
  {
    delete m_pConverter;
    m_pConverter = NULL;
  }

	if (m_pCodec)
	{
		m_pCodec->Close();
		X265v2Factory factory;
		factory.ReleaseCodecInstance(m_pCodec);
	}

  if (m_pSeqParamSet) delete[] m_pSeqParamSet; m_pSeqParamSet = NULL;
  if (m_pPicParamSet) delete[] m_pPicParamSet; m_pPicParamSet = NULL;
}

CUnknown * WINAPI X265EncoderFilter::CreateInstance( LPUNKNOWN pUnk, HRESULT *pHr )
{
	X265EncoderFilter *pFilter = new X265EncoderFilter();
	if (pFilter== NULL) 
	{
		*pHr = E_OUTOFMEMORY;
	}
	return pFilter;
}

 
void X265EncoderFilter::InitialiseInputTypes()
{
  AddInputType(&MEDIATYPE_Video, &MEDIASUBTYPE_RGB24, &FORMAT_VideoInfo);
  AddInputType(&MEDIATYPE_Video, &MEDIASUBTYPE_I420, &FORMAT_VideoInfo);
}


HRESULT X265EncoderFilter::SetMediaType( PIN_DIRECTION direction, const CMediaType *pmt )
{
	HRESULT hr = CCustomBaseFilter::SetMediaType(direction, pmt);
	if (direction == PINDIR_INPUT)
	{
    if (m_pConverter) delete m_pConverter;
    if (m_pYuvConversionBuffer)
    {
      delete[] m_pYuvConversionBuffer;
      m_pYuvConversionBuffer = NULL;
    }

    if (pmt->subtype == MEDIASUBTYPE_RGB24)
    {
      // MERGE from VPP
      // TODO: RTVC/artist code based has changed in mean-time.
      // It seems like it defaults to 128 which is the desired value in any case
      // TESTME/FIXME
      m_pConverter = new RealRGB24toYUV420ConverterStl<uint8_t>(m_nInWidth, m_nInHeight, 128);
      m_pConverter->SetFlip(true);
      m_pConverter->SetChrominanceOffset(128);

      m_uiConversionBufferSize = static_cast<int>(m_nInWidth * m_nInHeight * 1.5);
      m_pYuvConversionBuffer = new unsigned char[m_uiConversionBufferSize];
    }

    // try close just in case
    m_pCodec->Close();

    // m_pCodec->SetParameter(D_IN_COLOUR, D_IN_COLOUR_YUV420P8);
    m_pCodec->SetParameter(FILTER_PARAM_WIDTH, std::to_string(m_nInWidth).c_str());
    m_pCodec->SetParameter(FILTER_PARAM_HEIGHT, std::to_string(m_nInHeight).c_str());
    m_pCodec->SetParameter(FILTER_PARAM_FPS, "30");
    m_pCodec->SetParameter(FILTER_PARAM_TARGET_BITRATE_KBPS, std::to_string(m_uiTargetBitrate).c_str());
    m_pCodec->SetParameter("annexb", vpp::boolToString(m_bAnnexB).c_str());
    // generate sequence and picture parameter sets
#if 0
    if (m_pSeqParamSet) delete[] m_pSeqParamSet; m_pSeqParamSet = NULL;
    if (m_pPicParamSet) delete[] m_pPicParamSet; m_pPicParamSet = NULL;

    // set the selected parameter set numbers 
    m_pCodec->SetParameter((char *)"seq param set",  "0");
    m_pCodec->SetParameter((char *)"pic param set",  "0");

    m_pCodec->SetParameter((char *)("generate param set on open"),  "0");
    m_pCodec->SetParameter((char *)("picture coding type"),         "2");	///< Seq param set = H264V2_SEQ_PARAM.
    m_pSeqParamSet = new unsigned char[100];
    if(!m_pCodec->Code(NULL, m_pSeqParamSet, 100 * 8))
    {
      if (m_pSeqParamSet) delete[] m_pSeqParamSet; m_pSeqParamSet = NULL;
      SetLastError(m_pCodec->GetErrorStr(), true);
      return hr;
    }
    else
    {
      m_uiSeqParamSetLen = m_pCodec->GetCompressedByteLength();
      m_pSeqParamSet[m_uiSeqParamSetLen] = 0;
      m_sSeqParamSet = std::string((const char*)m_pSeqParamSet, m_uiSeqParamSetLen);
      int nCheck = strlen((char*)m_pSeqParamSet);
    }

    m_pCodec->SetParameter((char *)("picture coding type"),	  "3");	///< Pic param set = H264V2_PIC_PARAM.
    m_pPicParamSet = new unsigned char[100];
    if(!m_pCodec->Code(NULL, m_pPicParamSet, 100 * 8))
    {
      if (m_pSeqParamSet) delete[] m_pSeqParamSet; m_pSeqParamSet = NULL;
      if (m_pPicParamSet) delete[] m_pPicParamSet; m_pPicParamSet = NULL;
      SetLastError(m_pCodec->GetErrorStr(), true);
      return E_FAIL;
    }
    else
    {
      m_uiPicParamSetLen = m_pCodec->GetCompressedByteLength();
      m_pPicParamSet[m_uiPicParamSetLen] = 0;
      m_sPicParamSet = std::string((const char*)m_pPicParamSet, m_uiPicParamSetLen);
      int nCheck = strlen((char*)m_pPicParamSet);
    }

    // RG: copied from codec anayser, do we need this?
    // reset codec for standard operation
    m_pCodec->SetParameter((char *)("picture coding type"),	  "0");	///< I-frame = H264V2_INTRA.
    m_pCodec->SetParameter((char *)("generate param set on open"),  "1");

		if (!m_pCodec->Open())
		{
			//Houston: we have a failure
			char* szErrorStr = m_pCodec->GetErrorStr();
			printf("%s\n", szErrorStr);
			SetLastError(szErrorStr, true);
      return E_FAIL;
		}
#else
    if (!m_pCodec->Open())
    {
      //Houston: we have a failure
      char* szErrorStr = m_pCodec->GetErrorStr();
      printf("%s\n", szErrorStr);
      SetLastError(szErrorStr, true);
      return E_FAIL;
    }
    else
    {
      char szParamValue[256];
      memset(szParamValue, 0, 256);
      int nLenName = 0, nLenValue = 0;
      // Now get the value
      int res = m_pCodec->GetParameter("annexb_vps", &nLenValue, szParamValue);
      m_sVps = std::string(szParamValue, nLenValue);
      res = m_pCodec->GetParameter("annexb_sps", &nLenValue, szParamValue);
      m_sSps = std::string(szParamValue, nLenValue);
      res = m_pCodec->GetParameter("annexb_pps", &nLenValue, szParamValue);
      m_sPps = std::string(szParamValue, nLenValue);
    }

    // TODO: now read parameter sets

#endif
	}
	return hr;
}

HRESULT X265EncoderFilter::GetMediaType( int iPosition, CMediaType *pMediaType )
{
	if (iPosition < 0)
	{
		return E_INVALIDARG;
	}
	else if (iPosition == 0)
	{
    //if (m_nH264Type == H264_VPP)
    //{
    //  // Get the input pin's media type and return this as the output media type - we want to retain
    //  // all the information about the image
    //  HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
    //  if (FAILED(hr))
    //  {
    //    return hr;
    //  }

    //  pMediaType->subtype = MEDIASUBTYPE_VPP_H264;

    //  ASSERT(pMediaType->formattype == FORMAT_VideoInfo);
    //  VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pMediaType->pbFormat;
    //  BITMAPINFOHEADER* pBi = &(pVih->bmiHeader);

    //  // For compressed formats, this value is the implied bit 
    //  // depth of the uncompressed image, after the image has been decoded.
    //  if (pBi->biBitCount != 24)
    //    pBi->biBitCount = 24;

    //  pBi->biSizeImage = DIBSIZE(pVih->bmiHeader);
    //  pBi->biSizeImage = DIBSIZE(pVih->bmiHeader);

    //  // COMMENTING TO REMOVE
    //  // in the case of YUV I420 input to the H264 encoder, we need to change this back to RGB
    //  //pBi->biCompression = BI_RGB;
    //  pBi->biCompression = DWORD('1cva');

    //  // Store SPS and PPS in media format header
    //  int nCurrentFormatBlockSize = pMediaType->cbFormat;

    //  if (m_uiSeqParamSetLen + m_uiPicParamSetLen > 0)
    //  {
    //    // old size + one int to store size of SPS/PPS + SPS/PPS/prepended by start codes
    //    int iAdditionalLength = sizeof(int) + m_uiSeqParamSetLen + m_uiPicParamSetLen;
    //    int nNewSize = nCurrentFormatBlockSize + iAdditionalLength;
    //    pMediaType->ReallocFormatBuffer(nNewSize);

    //    BYTE* pFormat = pMediaType->Format();
    //    BYTE* pStartPos = &(pFormat[nCurrentFormatBlockSize]);
    //    // copy SPS
    //    memcpy(pStartPos, m_pSeqParamSet, m_uiSeqParamSetLen);
    //    pStartPos += m_uiSeqParamSetLen;
    //    // copy PPS
    //    memcpy(pStartPos, m_pPicParamSet, m_uiPicParamSetLen);
    //    pStartPos += m_uiPicParamSetLen;
    //    // Copy additional header size
    //    memcpy(pStartPos, &iAdditionalLength, sizeof(int));
    //  }
    //  else
    //  {
    //    // not configured: just copy in size of 0
    //    pMediaType->ReallocFormatBuffer(nCurrentFormatBlockSize + sizeof(int));
    //    BYTE* pFormat = pMediaType->Format();
    //    memset(pFormat + nCurrentFormatBlockSize, 0, sizeof(int));
    //  }
    //}
#if 0
    // testing annex b with MPEG2VIDEOINFO
    pMediaType->InitMediaType();
    pMediaType->SetType(&MEDIATYPE_Video);
    pMediaType->SetSubtype(&MEDIASUBTYPE_HVC1);

#if 0
    pMediaType->SetFormatType(&FORMAT_MPEG2Video);
    int psLen = 0;
    BYTE* pFormatBuffer = pMediaType->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO) + psLen);
    MPEG2VIDEOINFO* pMpeg2Vih = (MPEG2VIDEOINFO*)pFormatBuffer;

    ZeroMemory(pMpeg2Vih, sizeof(MPEG2VIDEOINFO) + psLen);

    pMpeg2Vih->dwFlags = 4;
    pMpeg2Vih->dwProfile = 66;
    pMpeg2Vih->dwLevel = 20;
    pMpeg2Vih->cbSequenceHeader = psLen;
    int iCurPos = 0;
    BYTE* pSequenceHeader = (BYTE*)&pMpeg2Vih->dwSequenceHeader[0];
    VIDEOINFOHEADER2* pvi2 = &pMpeg2Vih->hdr;
#else
    pMediaType->SetFormatType(&FORMAT_VideoInfo2);
    VIDEOINFOHEADER2* pvi2 = (VIDEOINFOHEADER2*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
    ZeroMemory(pvi2, sizeof(VIDEOINFOHEADER2));
#endif
    pvi2->bmiHeader.biBitCount = 24;
    pvi2->bmiHeader.biSize = 40;
    pvi2->bmiHeader.biPlanes = 1;
    pvi2->bmiHeader.biWidth = m_nInWidth;
    pvi2->bmiHeader.biHeight = m_nInHeight;
    pvi2->bmiHeader.biSizeImage = DIBSIZE(pvi2->bmiHeader);
    pvi2->bmiHeader.biCompression = DWORD('1cvh');
    const REFERENCE_TIME FPS_25 = UNITS / 25;
    pvi2->AvgTimePerFrame = FPS_25;
    SetRect(&pvi2->rcSource, 0, 0, m_nInWidth, m_nInHeight);
    pvi2->rcTarget = pvi2->rcSource;
    pvi2->dwPictAspectRatioX = m_nInWidth;
    pvi2->dwPictAspectRatioY = m_nInHeight;
#else
    if (m_bAnnexB)
    {
      // CMediaType inputMediaType;
      HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
      if (FAILED(hr))
      {
        return hr;
      }

      // pMediaType->InitMediaType();
      pMediaType->SetType(&MEDIATYPE_Video);
      //static const GUID MEDIASUBTYPE_MYX265 =
      //{ 0x4b2d0db, 0xced1, 0x43ad, { 0xa5, 0x11, 0x2f, 0x42, 0x96, 0xa4, 0xee, 0x54 } };

      //pMediaType->SetSubtype(&MEDIASUBTYPE_MYX265);
      // pMediaType->SetSubtype(&MEDIASUBTYPE_HEVC);
      pMediaType->SetSubtype(&MEDIASUBTYPE_H265);
      //pMediaType->SetSubtype(&MEDIASUBTYPE_H265);
#if 0
      pMediaType->SetFormatType(&FORMAT_MPEG2Video);
      int psLen = 0;
      BYTE* pFormatBuffer = pMediaType->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO) + psLen);
      MPEG2VIDEOINFO* pMpeg2Vih = (MPEG2VIDEOINFO*)pFormatBuffer;

      ZeroMemory(pMpeg2Vih, sizeof(MPEG2VIDEOINFO) + psLen);

      pMpeg2Vih->dwFlags = 4;
      pMpeg2Vih->dwProfile = 66;
      pMpeg2Vih->dwLevel = 20;
      pMpeg2Vih->cbSequenceHeader = psLen;
      int iCurPos = 0;
      BYTE* pSequenceHeader = (BYTE*)&pMpeg2Vih->dwSequenceHeader[0];
      VIDEOINFOHEADER2* pvi2 = &pMpeg2Vih->hdr;

      pvi2->bmiHeader.biBitCount = 24;
      pvi2->bmiHeader.biSize = 40;
      pvi2->bmiHeader.biPlanes = 1;
      pvi2->bmiHeader.biWidth = m_nInWidth;
      pvi2->bmiHeader.biHeight = m_nInHeight;
      pvi2->bmiHeader.biSize = m_nInWidth * m_nInHeight * 3;
      pvi2->bmiHeader.biSizeImage = DIBSIZE(pvi2->bmiHeader);
      pvi2->bmiHeader.biCompression = DWORD('1cvh');
      //pvi2->AvgTimePerFrame = m_tFrame;
      //pvi2->AvgTimePerFrame = 1000000;
      const REFERENCE_TIME FPS_25 = UNITS / 25;
      pvi2->AvgTimePerFrame = FPS_25;
      //SetRect(&pvi2->rcSource, 0, 0, m_cx, m_cy);
      SetRect(&pvi2->rcSource, 0, 0, m_nInWidth, m_nInHeight);
      pvi2->rcTarget = pvi2->rcSource;

      pvi2->dwPictAspectRatioX = m_nInWidth;
      pvi2->dwPictAspectRatioY = m_nInHeight;
#else
#if 0
      pMediaType->SetFormatType(&FORMAT_VideoInfo2);
      VIDEOINFOHEADER2* pvi2 = (VIDEOINFOHEADER2*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
      ZeroMemory(pvi2, sizeof(VIDEOINFOHEADER2));
      pvi2->bmiHeader.biBitCount = 24;
      pvi2->bmiHeader.biSize = 40;
      pvi2->bmiHeader.biPlanes = 1;
      pvi2->bmiHeader.biWidth = m_nInWidth;
      pvi2->bmiHeader.biHeight = m_nInHeight;
      pvi2->bmiHeader.biSize = m_nInWidth * m_nInHeight * 3;
      pvi2->bmiHeader.biSizeImage = DIBSIZE(pvi2->bmiHeader);
      pvi2->bmiHeader.biCompression = DWORD('1cvh');
      //pvi2->AvgTimePerFrame = m_tFrame;
      //pvi2->AvgTimePerFrame = 1000000;
      const REFERENCE_TIME FPS_25 = UNITS / 25;
      pvi2->AvgTimePerFrame = FPS_25;
      //SetRect(&pvi2->rcSource, 0, 0, m_cx, m_cy);
      SetRect(&pvi2->rcSource, 0, 0, m_nInWidth, m_nInHeight);
      pvi2->rcTarget = pvi2->rcSource;

      pvi2->dwPictAspectRatioX = m_nInWidth;
      pvi2->dwPictAspectRatioY = m_nInHeight;
#else
      //VIDEOINFOHEADER* pvi2 = (VIDEOINFOHEADER*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
      // ZeroMemory(pvi2, sizeof(VIDEOINFOHEADER));
      // sample grabber only supports std video info
      pMediaType->SetFormatType(&FORMAT_VideoInfo);
      VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pMediaType->Format();
      pvi->bmiHeader.biBitCount = 24;
      pvi->bmiHeader.biSize = 40;
      pvi->bmiHeader.biPlanes = 1;
      pvi->bmiHeader.biWidth = m_nInWidth;
      pvi->bmiHeader.biHeight = m_nInHeight;
      pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);
      pMediaType->SetSampleSize(DIBSIZE(pvi->bmiHeader));
      pvi->bmiHeader.biCompression = DWORD('1cvh');
      //pvi->bmiHeader.biCompression = BI_RGB;

      // assert(inputMediaType.FormatType == FORMAT_VideoInfo);
      // copy original FPS
      //VIDEOINFOHEADER* pInputVih = (VIDEOINFOHEADER*)inputMediaType.Format();
      //pvi2->AvgTimePerFrame = pInputVih->AvgTimePerFrame;
      //FreeMediaType(inputMediaType);
        //pvi2->AvgTimePerFrame = m_tFrame;
      //pvi2->AvgTimePerFrame = 1000000;
      //const REFERENCE_TIME FPS_25 = UNITS / 25;
      //pvi2->AvgTimePerFrame = FPS_25;
      ////SetRect(&pvi2->rcSource, 0, 0, m_cx, m_cy);
      //SetRect(&pvi2->rcSource, 0, 0, m_nInWidth, m_nInHeight);
      //pvi2->rcTarget = pvi2->rcSource;

      // Store SPS and PPS in media format header
      int nCurrentFormatBlockSize = pMediaType->cbFormat;
      if (!m_sVps.empty())
      // if (m_uiSeqParamSetLen + m_uiPicParamSetLen > 0)
      {
        // old size + one int to store size of SPS/PPS + SPS/PPS/prepended by start codes
        int iAdditionalLength = sizeof(int) + m_sVps.length() + m_sSps.length() + m_sPps.length();
        int nNewSize = nCurrentFormatBlockSize + iAdditionalLength;
        pMediaType->ReallocFormatBuffer(nNewSize);
        pMediaType->cbFormat = nCurrentFormatBlockSize + iAdditionalLength;
        BYTE* pFormat = pMediaType->Format();
        BYTE* pStartPos = &(pFormat[nCurrentFormatBlockSize]);
        // copy VPS
        memcpy(pStartPos, m_sVps.c_str(), m_sVps.length());
        pStartPos += m_sVps.length();
        // copy SPS
        memcpy(pStartPos, m_sSps.c_str(), m_sSps.length());
        pStartPos += m_sSps.length();
        // copy PPS
        memcpy(pStartPos, m_sPps.c_str(), m_sPps.length());
        pStartPos += m_sPps.length();
        // Copy additional header size
        memcpy(pStartPos, &iAdditionalLength, sizeof(int));
      }
      else
      {
        // not configured: just copy in size of 0
        pMediaType->ReallocFormatBuffer(nCurrentFormatBlockSize + sizeof(int));
        BYTE* pFormat = pMediaType->Format();
        memset(pFormat + nCurrentFormatBlockSize, 0, sizeof(int));
        pMediaType->cbFormat = nCurrentFormatBlockSize + sizeof(int);
      }
#endif
#endif
    }
    else
    {
      pMediaType->InitMediaType();
      pMediaType->SetType(&MEDIATYPE_Video);
      pMediaType->SetSubtype(&MEDIASUBTYPE_HVC1);
      pMediaType->SetFormatType(&FORMAT_MPEG2Video);

#if 0
      ASSERT(m_uiSeqParamSetLen > 0 && m_uiPicParamSetLen > 0);

      // ps have start codes: for this media type we replace each start code with 2 byte lenght prefix
      int psLen = m_uiSeqParamSetLen + m_uiPicParamSetLen - 4;
#else
      int psLen = 0;
#endif
      BYTE* pFormatBuffer = pMediaType->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO) + psLen);
      MPEG2VIDEOINFO* pMpeg2Vih = (MPEG2VIDEOINFO*)pFormatBuffer;

      ZeroMemory(pMpeg2Vih, sizeof(MPEG2VIDEOINFO) + psLen);

      pMpeg2Vih->dwFlags = 4;
      pMpeg2Vih->dwProfile = 66;
      pMpeg2Vih->dwLevel = 20;

      pMpeg2Vih->cbSequenceHeader = psLen;
      int iCurPos = 0;

      BYTE* pSequenceHeader = (BYTE*)&pMpeg2Vih->dwSequenceHeader[0];
#if 0
      // parameter set length includes start code
      pSequenceHeader[iCurPos] = ((m_uiSeqParamSetLen - 4) >> 8);
      pSequenceHeader[iCurPos + 1] = ((m_uiSeqParamSetLen - 4) & 0xFF);
      memcpy(pSequenceHeader + iCurPos + 2, m_pSeqParamSet + 4, m_uiSeqParamSetLen - 4);
      iCurPos += m_uiSeqParamSetLen - 4 + 2;
      pSequenceHeader[iCurPos] = ((m_uiPicParamSetLen - 4) >> 8);
      pSequenceHeader[iCurPos + 1] = ((m_uiPicParamSetLen - 4) & 0xFF);
      memcpy(pSequenceHeader + iCurPos + 2, m_pPicParamSet + 4, m_uiPicParamSetLen - 4);
#endif

      VIDEOINFOHEADER2* pvi2 = &pMpeg2Vih->hdr;
      pvi2->bmiHeader.biBitCount = 24;
      pvi2->bmiHeader.biSize = 40;
      pvi2->bmiHeader.biPlanes = 1;
      pvi2->bmiHeader.biWidth = m_nInWidth;
      pvi2->bmiHeader.biHeight = m_nInHeight;
      pvi2->bmiHeader.biSizeImage = DIBSIZE(pvi2->bmiHeader);
      pvi2->bmiHeader.biCompression = DWORD('1cvh');
      //pvi2->AvgTimePerFrame = m_tFrame;
      //pvi2->AvgTimePerFrame = 1000000;
      const REFERENCE_TIME FPS_25 = UNITS / 25;
      pvi2->AvgTimePerFrame = FPS_25;
      //SetRect(&pvi2->rcSource, 0, 0, m_cx, m_cy);
      SetRect(&pvi2->rcSource, 0, 0, m_nInWidth, m_nInHeight);
      pvi2->rcTarget = pvi2->rcSource;

      pvi2->dwPictAspectRatioX = m_nInWidth;
      pvi2->dwPictAspectRatioY = m_nInHeight;
    }
#endif

    return S_OK;
	}
	return VFW_S_NO_MORE_ITEMS;
}

HRESULT X265EncoderFilter::DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp )
{
	AM_MEDIA_TYPE mt;
	HRESULT hr = m_pOutput->ConnectionMediaType(&mt);
	if (FAILED(hr))
	{
		return hr;
	}

  if (m_bAnnexB)
  {
    //if (m_nH264Type == H264_H264)
    //{
    //  //Make sure that the format type is our custom format
#if 0
#if 0
    ASSERT(mt.formattype == FORMAT_VideoInfo2);
    VIDEOINFOHEADER2* pVih = (VIDEOINFOHEADER2*)mt.pbFormat;
    BITMAPINFOHEADER *pbmi = &pVih->bmiHeader;
    //TOREVISE: Should actually change mode and see what the maximum size is per frame?
    pProp->cbBuffer = DIBSIZE(*pbmi);
#else
    ASSERT(mt.formattype == FORMAT_MPEG2Video);
    MPEG2VIDEOINFO* pMpeg2Vih = (MPEG2VIDEOINFO*)mt.pbFormat;
    BITMAPINFOHEADER *pbmi = &pMpeg2Vih->hdr.bmiHeader;
    //TOREVISE: Should actually change mode and see what the maximum size is per frame?
    pProp->cbBuffer = DIBSIZE(*pbmi);
#endif
  } else
    //else if (m_nH264Type == H264_AVC1)
  {
    //Make sure that the format type is our custom format
    ASSERT(mt.formattype == FORMAT_MPEG2Video);
    MPEG2VIDEOINFO* pMpeg2Vih = (MPEG2VIDEOINFO*)mt.pbFormat;
    BITMAPINFOHEADER *pbmi = &pMpeg2Vih->hdr.bmiHeader;
    //TOREVISE: Should actually change mode and see what the maximum size is per frame?
    pProp->cbBuffer = DIBSIZE(*pbmi);
  }
  //else if (m_nH264Type == H264_VPP)
  //{
  //  ASSERT(mt.formattype == FORMAT_VideoInfo);
  //  BITMAPINFOHEADER *pbmi = HEADER(mt.pbFormat);
  //  //TOREVISE: Should actually change mode and see what the maximum size is per frame?
  //  pProp->cbBuffer = DIBSIZE(*pbmi);
  //}
#else
    ASSERT(mt.formattype == FORMAT_VideoInfo);
    BITMAPINFOHEADER *pbmi = HEADER(mt.pbFormat);
    //TOREVISE: Should actually change mode and see what the maximum size is per frame?
    pProp->cbBuffer = DIBSIZE(*pbmi);
  }
#endif
	if (pProp->cbAlign == 0)
	{
		pProp->cbAlign = 1;
	}
	if (pProp->cBuffers == 0)
	{
		pProp->cBuffers = 1;
	}
	// Release the format block.
	FreeMediaType(mt);

	// Set allocator properties.
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}
	// Even when it succeeds, check the actual result.
	if (pProp->cbBuffer > Actual.cbBuffer) 
	{
		return E_FAIL;
	}
	return S_OK;
}

inline unsigned X265EncoderFilter::getParameterSetLength() const
{
  return m_uiSeqParamSetLen + m_uiPicParamSetLen;
}

inline unsigned X265EncoderFilter::copySequenceAndPictureParameterSetsIntoBuffer( BYTE* pBuffer )
{
  // The SPS and PPS contain start code
  assert (m_uiSeqParamSetLen > 0 && m_uiPicParamSetLen > 0);
  memcpy(pBuffer, (void*)m_pSeqParamSet, m_uiSeqParamSetLen);
  pBuffer += m_uiSeqParamSetLen;
  memcpy(pBuffer, m_pPicParamSet, m_uiPicParamSetLen);
  pBuffer += m_uiPicParamSetLen;
  return getParameterSetLength();
}

HRESULT X265EncoderFilter::Transform( IMediaSample *pSource, IMediaSample *pDest )
{
  HRESULT hr = CCustomBaseFilter::Transform(pSource, pDest);
#if 0
  pDest->SetTime( &m_tStart, &m_tStop );
  // inc
  m_tStart = m_tStop;
  m_tStop += m_rtFrameLength;
#endif
  return hr;
}

HRESULT X265EncoderFilter::ApplyTransform(BYTE* pBufferIn, long lInBufferSize, long lActualDataLength, BYTE* pBufferOut, long lOutBufferSize, long& lOutActualDataLength)
{
  // lock filter so that it can not be reconfigured during a code operation
  CAutoLock lck(&m_csCodec);

  BYTE* pInput = pBufferIn;
  long lInputLength = lInBufferSize;

  if (m_pConverter)
  {
    // we need to convert to YUV first
    if (!m_pConverter->Convert(pBufferIn, lActualDataLength, m_pYuvConversionBuffer, m_uiConversionBufferSize))
    {
      DbgLog((LOG_TRACE, 0, TEXT("Conversion failed from RGB to I420: %s"), m_pConverter->getLastError().c_str()));
      return E_FAIL;
    }
    DbgLog((LOG_TRACE, 0, TEXT("Converted to I420 directly")));
    pInput = m_pYuvConversionBuffer;
    lInputLength = m_uiConversionBufferSize;
  }

  lOutActualDataLength = 0;
	//make sure we were able to initialise our Codec
	if (m_pCodec)
	{
		if (m_pCodec->Ready())
		{
      if (m_uiIFramePeriod)
      {
        ++m_uiCurrentFrame;
        if (m_uiCurrentFrame%m_uiIFramePeriod == 0)
        {
          m_pCodec->Restart();
        }
      }

      BYTE* pOutBufferPos = pBufferOut;

#if 0
      DbgLog((LOG_TRACE, 0, 
        TEXT("H264 Codec Byte Limit: %d"), nFrameBitLimit));
#endif
      int nResult = m_pCodec->Code(pInput, pOutBufferPos, lOutBufferSize);
      if (nResult)
      {
        //Encoding was successful
        lOutActualDataLength += m_pCodec->GetCompressedByteLength();
			}
			else
			{
				//An error has occurred
				DbgLog((LOG_TRACE, 0, TEXT("X265 Codec Error: %s"), m_pCodec->GetErrorStr()));
				std::string sError = m_pCodec->GetErrorStr();
        sError += ". Out buffer size=" + std::to_string(lOutBufferSize) + ".";
        m_pCodec->Restart();
        SetLastError(sError.c_str(), true);
        lOutActualDataLength = 0;
      }
		}
	}
  return S_OK;
}

HRESULT X265EncoderFilter::CheckTransform( const CMediaType *mtIn, const CMediaType *mtOut )
{
	// Check the major type.
	if (mtOut->majortype != MEDIATYPE_Video)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

  if (mtOut->subtype != MEDIASUBTYPE_H265 && mtOut->subtype != MEDIASUBTYPE_HVC1 && mtOut->subtype != MEDIASUBTYPE_HEVC)
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  if (mtOut->formattype != FORMAT_MPEG2Video && mtOut->formattype != FORMAT_VideoInfo)
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  // Everything is good.
	return S_OK;
}

STDMETHODIMP X265EncoderFilter::GetParameter( const char* szParamName, int nBufferSize, char* szValue, int* pLength )
{
	if (SUCCEEDED(CCustomBaseFilter::GetParameter(szParamName, nBufferSize, szValue, pLength)))
	{
		return S_OK;
	}
	else
	{
    CAutoLock lck(&m_csCodec);
		// Check if its a codec parameter
		if (m_pCodec && m_pCodec->GetParameter(szParamName, pLength, szValue))
		{
			return S_OK;
		}
		return E_FAIL;
	}
}

STDMETHODIMP X265EncoderFilter::SetParameter( const char* type, const char* value )
{
	if (SUCCEEDED(CCustomBaseFilter::SetParameter(type, value)))
	{
		return S_OK;
	}
	else
	{
    CAutoLock lck(&m_csCodec);
		// Check if it's a codec parameter
		if (m_pCodec && m_pCodec->SetParameter(type, value))
		{
			return S_OK;
		}
		return E_FAIL;
	}
}

STDMETHODIMP X265EncoderFilter::GetParameterSettings( char* szResult, int nSize )
{
	if (SUCCEEDED(CCustomBaseFilter::GetParameterSettings(szResult, nSize)))
	{
		// Now add the codec parameters to the output:
		int nLen = strlen(szResult);
		char szValue[10];
		int nParamLength = 0;
    CAutoLock lck(&m_csCodec);
		std::string sCodecParams("Codec Parameters:\r\n");
		if( m_pCodec->GetParameter("parameters", &nParamLength, szValue))
		{
			int nParamCount = atoi(szValue);
			char szParamValue[256];
			memset(szParamValue, 0, 256);

			int nLenName = 0, nLenValue = 0;
			for (int i = 0; i < nParamCount; i++)
			{
				// Get parameter name
				const char* szParamName;
				m_pCodec->GetParameterName(i, &szParamName, &nLenName);
				// Now get the value
				m_pCodec->GetParameter(szParamName, &nLenValue, szParamValue);
				sCodecParams += "Parameter: " + std::string(szParamName) + " : Value:" + std::string(szParamValue) + "\r\n";
			}
			// now check if the buffer is big enough:
			int nTotalSize = sCodecParams.length() + nLen;
			if (nTotalSize < nSize)
			{
				memcpy(szResult + nLen, sCodecParams.c_str(), sCodecParams.length());
				// Set null terminator
				szResult[nTotalSize] = 0;
				return S_OK;
			}
			else
			{
				return E_FAIL;
			}
		}
		else
		{
			return E_FAIL;
		}
	}
	else
	{
		return E_FAIL;
	}
}

STDMETHODIMP X265EncoderFilter::GetFramebitLimit(int& iFrameBitLimit)
{
  return E_NOTIMPL;
}

STDMETHODIMP X265EncoderFilter::SetFramebitLimit(int iFrameBitLimit)
{
  return E_NOTIMPL;
}

STDMETHODIMP X265EncoderFilter::GetGroupId(int& iGroupId)
{
  return E_NOTIMPL;
}

STDMETHODIMP X265EncoderFilter::SetGroupId(int iGroupId)
{
  return E_NOTIMPL;
}

STDMETHODIMP X265EncoderFilter::GenerateIdr()
{
  // lock filter so that it can not be reconfigured during a code operation
  CAutoLock lck(&m_csCodec);
  m_pCodec->Restart();
  return S_OK;
}

STDMETHODIMP X265EncoderFilter::GetBitrateKbps(int& uiBitrateKbps)
{
  return E_NOTIMPL;
}

STDMETHODIMP X265EncoderFilter::SetBitrateKbps(int uiBitrateKbps)
{
  return E_NOTIMPL;
}
