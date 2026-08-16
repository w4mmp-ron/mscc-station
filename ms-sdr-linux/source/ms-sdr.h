
/*#if !defined(AFX_EXTIOCLASS_H__247C1094_0293_40d5_846A_6CC900C82E80__INCLUDED_)
#define AFX_EXTIOCLASS_H__247C1094_0293_40d5_846A_6CC900C82E80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
	typedef int (* pfnExtIOCallback)  (int cnt, int status, float IQoffs, void *IQdata);
	void __stdcall ExtIoSDRInfo( int extSDRInfo, int additionalValue, void * additionalPtr);
	void __stdcall VersionInfo(const char *progname,int ver_major,int ver_minor);
	BOOL __stdcall SetModeRxTx(int modeRxTx);
	BOOL __stdcall ActivateTx(int magicA,int magicB);
	//extern "C" __declspec(dllexport) bool __stdcall InitHW(char *name, char *model, int& type);
	BOOL __stdcall InitHW(char *name, char *model, int *type);
	BOOL __stdcall OpenHW(void);
	int __stdcall StartHW(long freq);
	void __stdcall StopHW(void);
	void __stdcall CloseHW(void);
	int __stdcall SetHWLO(long LOfreq);
	long __stdcall GetHWLO(void);
	long __stdcall GetHWSR(void);
	long __stdcall GetTune(void);
	//extern "C" __declspec(dllexport) void __stdcall GetFilters(int& loCut, int& hiCut, int& pitch);
	void __stdcall GetFilters(int *loCut, int *hiCut, int *pitch);
	char __stdcall GetMode(void);
	void __stdcall ModeChanged(char mode);
	int __stdcall GetStatus(void);
	void __stdcall ShowGUI(void);
	void __stdcall HideGUI(void);
	void __stdcall IFLimitsChanged(long low, long high);
	void __stdcall TuneChanged(long freq);
	void __stdcall FiltersChanged(int loCut, int hiCut, int pitch, BOOL mute);
	void __stdcall SetCallback(pfnExtIOCallback funcptr);
	void __stdcall RawDataReady(long samprate, int *Ldata, int *Rdata, int numsamples);

	

#endif // !defined(AFX_EXTIOCLASS_H__247C1094_0293_40d5_846A_6CC900C82E80__INCLUDED_)
*/
