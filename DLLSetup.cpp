// X265EncoderFilter.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"

#pragma warning(push)     // disable for this header only
#pragma warning(disable:4312) 
// DirectShow
#include <streams.h>
#pragma warning(pop)      // restore original warning level
#include <dvdmedia.h>
#include <wmcodecdsp.h>

#include "X265EncoderFilter.h"
#include "X265EncoderProperties.h"

//////////////////////////////////////////////////////////////////////////
//###############################  Standard Filter DLL Code ###############################
static const WCHAR g_wszName[] = L"CSIR VPP X265 Encoder";   /// A name for the filter 

const AMOVIESETUP_MEDIATYPE sudMediaTypes[] = 
{
  { 
		&MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 
	},
  {
    &MEDIATYPE_Video, &MEDIASUBTYPE_I420
  },
  {
		&MEDIATYPE_Video, &MEDIASUBTYPE_H265
	},
};

const AMOVIESETUP_PIN sudPin[] = 
{
	//RG 01/04/trying different input type again
	//{
	//	L"",            // Obsolete, not used.
	//		FALSE,          // Is this pin rendered?
	//		FALSE,           // Is it an output pin?
	//		FALSE,          // Can the filter create zero instances?
	//		FALSE,          // Does the filter create multiple instances?
	//		&GUID_NULL,     // Obsolete.
	//		NULL,           // Obsolete.
	//		1,              // Number of media types.
	//		&sudMediaTypes[1]   // Pointer to media types.
	//},
	// RG Before testing with media type stream
	{
		L"",                // Obsolete, not used.
			FALSE,            // Is this pin rendered?
			FALSE,            // Is it an output pin?
			FALSE,            // Can the filter create zero instances?
			FALSE,            // Does the filter create multiple instances?
			&GUID_NULL,       // Obsolete.
			NULL,             // Obsolete.
			2,                // Number of media types.
			&sudMediaTypes[0] // Pointer to media types.
	},
	{
		L"",                // Obsolete, not used.
			FALSE,            // Is this pin rendered?
			TRUE,             // Is it an output pin?
			FALSE,            // Can the filter create zero instances?
			FALSE,            // Does the filter create multiple instances?
			&GUID_NULL,       // Obsolete.
			NULL,             // Obsolete.
			1,                // Number of media types.
			&sudMediaTypes[2] // Pointer to media types.
	}
};

// The next bunch of structures define information for the class factory.
AMOVIESETUP_FILTER FilterInfo =
{
	&CLSID_VPP_X265Encoder, 	// CLSID
	g_wszName,					      // Name
	MERIT_DO_NOT_USE,			    // Merit
	0,							          // Number of AMOVIESETUP_PIN structs
  sudPin	  			          // Pin registration information.
};


CFactoryTemplate g_Templates[] = 
{
	{
		g_wszName,                          // Name
		&CLSID_VPP_X265Encoder,             // CLSID
		X265EncoderFilter::CreateInstance,  // Method to create an instance of MyComponent
		NULL,                               // Initialization function
		&FilterInfo                         // Set-up information (for filters)
	},
  // This entry is for the property page.
  {
    L"H264 Properties",
    &CLSID_X265Properties,
    X265EncoderProperties::CreateInstance, 
    NULL, NULL
  }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);   

//////////////////////////////////////////////////////////////////////////
// Functions needed by the DLL, for registration.

STDAPI DllRegisterServer(void)
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

//DLL Entry point
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
