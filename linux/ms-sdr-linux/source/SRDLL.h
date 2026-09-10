//************************************************************************
//**
//** Project......: Control DLL for the Firmware USB AVR Si570 controler.
//**
//** Platform.....: WinXP
//**
//** Licence......: This software is freely available for non-commercial 
//**                use - i.e. for research and experimentation only!
//**
//** Programmer...: F.W. Krom, PE0FKO
//** 
//** Description..: Control the Si570 Freq. PLL chip over the USB port.
//**
//** History......: V1.0 01/06/2009: First release of PE0FKO.
//**
//**************************************************************************
//

#pragma once
//#include "extern.h"
//typedef char                    TCHAR;

//#if !defined(BOOL)
//#define	BOOL	int
//#endif

typedef struct {
	int			VID,PID;
	TCHAR		Manufacturer[128];
	TCHAR		Product[128];
	TCHAR		SerialNumber[128];
} srUsbInfo_t;


	// The parameter i2cAddr only needed with old SDR-Kit firmware (V1.4 & V2.0).
/*void* srOpen (int vid, int pid, const TCHAR* pManufacturer, const TCHAR* pProduct, const TCHAR* pSerialNumber, int iDevNum);
void srClose				(void);
BOOL	srIsOpen				(void);

srUsbInfo_t*	srGetUsbInfo		(void);
int			srGetString		(int ID, char* pStr, int iLen);

BOOL		srGetVersion			(int* major, int* minor);

BOOL		srGetFreqSubMul		(int index, double* subMHz, double* Mul);
BOOL		srSetFreqSubMul		(int index, double subMHz, double Mul);

BOOL		srGetFreq				(double* MHz);
BOOL		srSetFreq				(double MHz, int i2cAddr);
	
BOOL		srGetFreqReg			(unsigned char reg[6], int i2cAddr, int index);
BOOL		srSetFreqReg			(unsigned char reg[6], int i2cAddr);
	
BOOL		srGetXtalFreq			(double* MHz);
BOOL		srSetXtalFreq			(double MHz);
	
BOOL		srGetStartupFreq		(double* MHz);
BOOL		srSetStartupFreq		(double MHz);
	
BOOL		srGetSmoodTuneRange	(int* range);
BOOL		srSetSmoodTuneRange	(int range);

BOOL		srSetI2CAddr			(int i2c);
BOOL		srGetI2CAddr			(int* pI2C);

BOOL		srSetUsbId				(char Id);
BOOL		srGetUsbId				(char* pId);

BOOL		srSetPTTGetCWInp		(int ptt, int* CWkey);
BOOL		srGetCWInp				(int* CWkey);

BOOL		srSetIO				(int IOdat, int IOddr);
BOOL		srGetIO				(int* IOval);

BOOL		srGetFilterRXTable		(double* pTable, int* pTableLength, BOOL* pEnable);
BOOL		srSetFilterRXPoint		(double Freq, int Entry);
BOOL		srSetFilterRXEnable	(BOOL Enable);

BOOL		srGetFilterTXTable		(double* pTable, int* pTableLength, BOOL* pEnable);
BOOL		srSetFilterTXPoint		(double Freq, int Entry);
BOOL		srSetFilterTXEnable	(BOOL Enable);

BOOL		srGetBandRXFilters		(int* length, int* Filters);
BOOL		srSetBandRXFilter		(int band, int filter);

BOOL		srGetBandTXFilters		(int* length, int* Filters);
BOOL		srSetBandTXFilter		(int band, int filter);

BOOL		srSetRegSi570			(int reg, int value, int i2cAddr, BOOL* pI2CError);

BOOL		srReboot				(void);
BOOL		srGetCpuTemp			(double* pTemp);

	// Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete
BOOL		srSetPortDDR			(int DDR);			// Use srSetIO()
BOOL		srGetPortPIN			(int* pPort);		// Use srGetIO()
BOOL		srGetPortPRT			(int* pPort);		// Not very usefull
BOOL		srSetPort				(int Port);			// Use srSetIO(), Only when no ABPF
	// Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete Obsolete

	//++ Mobo extensions
BOOL		srMoboSetPABias		(int iIndex, int iValue, int* pValue);
BOOL		srMoboGetAdc			(int iIndex, double* pValue, double dMax);
*/



//double srFromSi570RegToFreq(unsigned char reg[6], double xtal);

//srGetSi570Grade	(int* pGrade, int* pDCOmin, int* pDCOmax, int* piRFFreqIndex, BOOL* pbFreezeM);
//srSetSi570Grade (int iGrade, int iDCOmin, int iDCOmax, int iRfFreqIndex, BOOL bFreezeM);

//void*		srUsbHandle;
//int			srUsbTimeout;

