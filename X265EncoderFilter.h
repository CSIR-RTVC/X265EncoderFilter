#pragma once
#include <fstream>
#include <DirectShowExt/CodecControlInterface.h>
#include <DirectShowExt/CustomBaseFilter.h>
#include <DirectShowExt/DirectShowMediaFormats.h>
#include <DirectShowExt/NotifyCodes.h>
#include <DirectShowExt/FilterParameterStringConstants.h>
#include "VersionInfo.h"

// Forward
class ICodecv2;
// Forward declarations
template <typename T>
class RGBtoYUV420ConverterStl;

// {287BE99D-3C3A-4621-B205-A25AF364D19F}
static const GUID CLSID_VPP_X265Encoder =
{ 0x287be99d, 0x3c3a, 0x4621, { 0xb2, 0x5, 0xa2, 0x5a, 0xf3, 0x64, 0xd1, 0x9f } };


// {56AE8453-9ACC-4C48-ADFB-5E5633E36A2F}
static const GUID CLSID_X265Properties =
{ 0x56ae8453, 0x9acc, 0x4c48, { 0xad, 0xfb, 0x5e, 0x56, 0x33, 0xe3, 0x6a, 0x2f } };

class X265EncoderFilter : public CCustomBaseFilter,
                          public ISpecifyPropertyPages,
                          public ICodecControlInterface
{
public:
  DECLARE_IUNKNOWN

	/// Constructor
	X265EncoderFilter();
	/// Destructor
	~X265EncoderFilter();

	/// Static object-creation method (for the class factory)
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr); 

	/**
	* Overriding this so that we can set whether this is an RGB24 or an RGB32 Filter
	*/
	HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);

	/**
	* Used for Media Type Negotiation 
	* Returns an HRESULT value. Possible values include those shown in the following table.
	* <table border="0" cols="2"><tr valign="top"><td><b>Value</b></td><td><b>Description</b></td></TR><TR><TD>S_OK</TD><TD>Success</TD></TR><TR><TD>VFW_S_NO_MORE_ITEMS</TD><TD>Index out of range</TD></TR><TR><TD>E_INVALIDARG</TD><TD>Index less than zero</TD></TR></TABLE>
	* The output pin's CTransformOutputPin::GetMediaType method calls this method. The derived class must implement this method. For more information, see CBasePin::GetMediaType.
	* To use custom media types, the media type can be manipulated in this method.
	*/
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	/// Buffer Allocation
	/**
	* The output pin's CTransformOutputPin::DecideBufferSize method calls this method. The derived class must implement this method. For more information, see CBaseOutputPin::DecideBufferSize. 
	* @param pAlloc Pointer to the IMemAllocator interface on the output pin's allocator.
	* @param pProp Pointer to an ALLOCATOR_PROPERTIES structure that contains buffer requirements from the downstream input pin.
	* @return Value: Returns S_OK or another HRESULT value.
	*/
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);

	/**
	* The CheckTransform method checks whether an input media type is compatible with an output media type.
	* <table border="0" cols="2"> <tr valign="top"> <td  width="50%"><b>Value</b></td> <td width="50%"><b>Description</b></td> </tr> <tr valign="top"> <td width="50%">S_OK</td> <td width="50%">The media types are compatible.</td> </tr> <tr valign="top"> <td width="50%">VFW_E_TYPE_NOT_ACCEPTED</td> <td width="50%">The media types are not compatible.</td> </tr> </table>
	*/
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);

  virtual void doGetVersion(std::string& sVersion)
  {
    sVersion = VersionInfo::toString();
  }
	/// Interface methods
	///Overridden from CSettingsInterface
	virtual void initParameters()
	{
    addParameter(FILTER_PARAM_VPS, &m_sVps, "", true);
    addParameter(FILTER_PARAM_SPS, &m_sSps, "", true);
    addParameter(FILTER_PARAM_PPS, &m_sPps, "", true);
    addParameter(FILTER_PARAM_TARGET_BITRATE_KBPS, &m_uiTargetBitrate, 500);
    addParameter("annexb", &m_bAnnexB, true);
  }

	/// Overridden from SettingsInterface
	STDMETHODIMP GetParameter( const char* szParamName, int nBufferSize, char* szValue, int* pLength );
	STDMETHODIMP SetParameter( const char* type, const char* value);
	STDMETHODIMP GetParameterSettings( char* szResult, int nSize );

  /**
   * @brief Overridden from ICodecControlInterface. Getter for frame bit limit
   */
  STDMETHODIMP GetFramebitLimit(int& iFrameBitLimit);
  /**
   * @brief @ICodecControlInterface. Overridden from ICodecControlInterface
   */
  STDMETHODIMP SetFramebitLimit(int iFrameBitLimit);
  /**
   * @brief @ICodecControlInterface. Method to query group id
   */
  STDMETHODIMP GetGroupId(int& iGroupId);
  /**
   * @brief @ICodecControlInterface. Method to set group id
   */
  STDMETHODIMP SetGroupId(int iGroupId);
  /**
   * @brief @ICodecControlInterface. Overridden from ICodecControlInterface
   */
  STDMETHODIMP GenerateIdr();
  /**
   * @brief @ICodecControlInterface. Overridden from ICodecControlInterface
   */
  STDMETHODIMP GetBitrateKbps(int& uiBitrateKbps);
  /**
   * @brief @ICodecControlInterface. Method to set bitrate in kbps
   * Overridden from ICodecControlInterface
   */
  STDMETHODIMP SetBitrateKbps(int uiBitrateKbps);

  STDMETHODIMP GetPages(CAUUID *pPages)
  {
    if (pPages == NULL) return E_POINTER;
    pPages->cElems = 1;
    pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) 
    {
      return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_X265Properties;
    return S_OK;
  }

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
  {
    if (riid == IID_ISpecifyPropertyPages)
    {
      return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
    }
    else if (riid == IID_ICodecControlInterface)
    {
      return GetInterface(static_cast<ICodecControlInterface*>(this), ppv);
    }
    else
    {
      // Call the parent class.
      return CCustomBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
  }

	/// Overridden from CCustomBaseFilter
	virtual void InitialiseInputTypes();

  HRESULT Transform(IMediaSample *pSource, IMediaSample *pDest);

private:
  /**
    This method copies the h.264 sequence and picture parameter sets into the passed in buffer
    and returns the total length including start codes
  */
  unsigned copySequenceAndPictureParameterSetsIntoBuffer(BYTE* pBuffer);
  unsigned getParameterSetLength() const;
 /**
	* This method converts the input buffer from RGB24 | 32 to YUV420P
	* @param pSource The source buffer
	* @param pDest The destination buffer
	*/
	virtual HRESULT ApplyTransform(BYTE* pBufferIn, long lInBufferSize, long lActualDataLength, BYTE* pBufferOut, long lOutBufferSize, long& lOutActualDataLength);

	ICodecv2* m_pCodec;
  /// Receive Lock
  CCritSec m_csCodec;
  bool m_bAnnexB;
  unsigned char* m_pSeqParamSet;
  unsigned m_uiSeqParamSetLen;
  unsigned char* m_pPicParamSet;
  unsigned m_uiPicParamSetLen;
  std::string m_sVps;
  std::string m_sSps;
  std::string m_sPps;

  // For auto i-frame generation
  unsigned m_uiIFramePeriod;
  // frame counter for i-frame generation
  unsigned m_uiCurrentFrame;
  unsigned m_uiTargetBitrate;

  REFERENCE_TIME m_rtFrameLength;
  REFERENCE_TIME m_tStart;
  REFERENCE_TIME m_tStop;

  RGBtoYUV420ConverterStl<unsigned char>* m_pConverter;
  unsigned m_uiConversionBufferSize;
  unsigned char* m_pYuvConversionBuffer;
};
