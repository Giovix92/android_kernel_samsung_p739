#include "INC_INCLUDES.h"


#include <linux/fs.h>
#include <asm/uaccess.h>

static ST_BBPINFO			g_astBBPRun[2];
extern ENSEMBLE_BAND		m_ucRfBand;
extern UPLOAD_MODE_INFO		m_ucUploadMode;
extern CLOCK_SPEED			m_ucClockSpeed;
extern INC_ACTIVE_MODE		m_ucMPI_CS_Active;
extern INC_ACTIVE_MODE		m_ucMPI_CLK_Active;
extern CTRL_MODE 			m_ucCommandMode;
extern ST_TRANSMISSION		m_ucTransMode;
extern PLL_MODE				m_ucPLL_Mode;
extern INC_DPD_MODE			m_ucDPD_Mode;
extern INC_UINT16			m_unIntCtrl;


INC_UINT8 INC_CMD_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData)
{
	if(m_ucCommandMode == INC_SPI_CTRL) return INC_SPI_REG_WRITE(ucI2CID, uiAddr, uiData);
	else if(m_ucCommandMode == INC_I2C_CTRL) return INC_I2C_WRITE(ucI2CID, uiAddr, uiData);
	else if(m_ucCommandMode == INC_EBI_CTRL) return INC_EBI_WRITE(ucI2CID, uiAddr, uiData);
	return INC_I2C_WRITE(ucI2CID, uiAddr, uiData);
}

INC_UINT16 INC_CMD_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
	if(m_ucCommandMode == INC_SPI_CTRL) return INC_SPI_REG_READ(ucI2CID, uiAddr);
	else if(m_ucCommandMode == INC_I2C_CTRL) return INC_I2C_READ(ucI2CID, uiAddr);
	else if(m_ucCommandMode == INC_EBI_CTRL) return INC_EBI_READ(ucI2CID, uiAddr);
	return INC_I2C_READ(ucI2CID, uiAddr);
}

INC_UINT8 INC_CMD_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize)
{
	if(m_ucCommandMode == INC_SPI_CTRL) return INC_SPI_READ_BURST(ucI2CID, uiAddr, pData, nSize);
	else if(m_ucCommandMode == INC_I2C_CTRL) return INC_I2C_READ_BURST(ucI2CID, uiAddr, pData, nSize);
	else if(m_ucCommandMode == INC_EBI_CTRL) return INC_EBI_READ_BURST(ucI2CID, uiAddr, pData, nSize);
	return INC_I2C_READ_BURST(ucI2CID, uiAddr, pData, nSize);
}

void INC_INIT_MPI(INC_UINT8 ucI2CID)
{
	INC_UINT16 uiMpiStatus, uiRsStatus;
	uiRsStatus = INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0, 0x0000);
	INC_DELAY(50);
	uiMpiStatus = INC_CMD_READ(ucI2CID, APB_MPI_BASE+ 0);
	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0, uiMpiStatus|0x8000);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0, uiRsStatus|0x8000);
}

void INC_MPICLOCK_SET(INC_UINT8 ucI2CID)
{
	if(m_ucUploadMode == STREAM_UPLOAD_TS){
		if(m_ucClockSpeed == INC_OUTPUT_CLOCK_1024) 	    INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x9918);
		else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_2048) 	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x440C);
		else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_4096) 	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x3308);
		else INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x3308);
		return;
	}

	if(m_ucClockSpeed == INC_OUTPUT_CLOCK_1024) 			INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x9918);
	else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_2048)	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x440C);
	else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_4096)	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x2206);
	else INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x2206);
}

void INC_UPLOAD_MODE(INC_UINT8 ucI2CID)
{
	INC_UINT16	uiStatus = 0x01;

	if(m_ucCommandMode != INC_EBI_CTRL){
		if(m_ucUploadMode == STREAM_UPLOAD_SPI) 			uiStatus = 0x05;
		if(m_ucUploadMode == STREAM_UPLOAD_SLAVE_PARALLEL) 	uiStatus = 0x04;
		if(m_ucUploadMode == STREAM_UPLOAD_TS) 				uiStatus = 0x101;
		if(m_ucMPI_CS_Active == INC_ACTIVE_HIGH) 			uiStatus |= 0x10;
		if(m_ucMPI_CLK_Active == INC_ACTIVE_HIGH) 			uiStatus |= 0x20;
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x00, uiStatus);
	}
	else
	{
		INC_CMD_WRITE(ucI2CID, APB_I2C_BASE+ 0x00, 0x0001);		// I2C Clear	('10. 10. 26)
		INC_CMD_WRITE(ucI2CID, APB_SPI_BASE+ 0x00, 0x0011);
	}

	if(m_ucUploadMode == STREAM_UPLOAD_TS){
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x02, 188);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x03, 8);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x04, 188);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x05, 0);
	}
	else {
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x02, MPI_CS_SIZE);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x04, INC_INTERRUPT_SIZE);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x05, INC_INTERRUPT_SIZE-188);
	}
}

void INC_SWAP(ST_SUBCH_INFO* pMainInfo, INC_UINT16 nNo1, INC_UINT16 nNo2)
{
	INC_CHANNEL_INFO  stChInfo;
	stChInfo = pMainInfo->astSubChInfo[nNo1];
	pMainInfo->astSubChInfo[nNo1] = pMainInfo->astSubChInfo[nNo2];
	pMainInfo->astSubChInfo[nNo2] = stChInfo;
}

void INC_BUBBLE_SORT(ST_SUBCH_INFO* pMainInfo, INC_SORT_OPTION Opt)
{
	INC_INT16 nIndex, nLoop;
	INC_CHANNEL_INFO* pDest;
	INC_CHANNEL_INFO* pSour;

	if(pMainInfo->nSetCnt <= 1) return;

	for(nIndex = 0 ; nIndex < pMainInfo->nSetCnt-1; nIndex++) {
		pSour = &pMainInfo->astSubChInfo[nIndex];

		for(nLoop = nIndex + 1 ; nLoop < pMainInfo->nSetCnt; nLoop++) {
			pDest = &pMainInfo->astSubChInfo[nLoop];

			if(Opt == INC_SUB_CHANNEL_ID){
				if(pSour->ucSubChID > pDest->ucSubChID && pSour->ulRFFreq == pDest->ulRFFreq){
					INC_SWAP(pMainInfo, nIndex, nLoop);
				}
			}
			else if(Opt == INC_START_ADDRESS){
				if(pSour->uiStarAddr > pDest->uiStarAddr && pSour->ulRFFreq == pDest->ulRFFreq)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
			else if(Opt == INC_BIRRATE)	{
				if(pSour->uiBitRate > pDest->uiBitRate && pSour->ulRFFreq == pDest->ulRFFreq)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
			else if(Opt == INC_FREQUENCY){
				if(pSour->ulRFFreq > pDest->ulRFFreq && pSour->ulRFFreq == pDest->ulRFFreq)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
			else{
				if(pSour->uiStarAddr > pDest->uiStarAddr && pSour->ulRFFreq == pDest->ulRFFreq)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
		}
	}
}

INC_UINT8 INC_BUBBLE_SORT_by_TYPE(ST_SUBCH_INFO* pMainInfo)
{
	INC_INT16 nIndex, nLoop;
	INC_CHANNEL_INFO* pDest;
	INC_CHANNEL_INFO* pSour;

	if(pMainInfo->nSetCnt <= 1) return INC_ERROR;

	for(nIndex = 0 ; nIndex < pMainInfo->nSetCnt-1; nIndex++) 
	{
		if(pMainInfo->astSubChInfo[nIndex].uiTmID != TMID_1) // DMB
			continue;

		pSour = &pMainInfo->astSubChInfo[nIndex];

		for(nLoop = nIndex + 1 ; nLoop < pMainInfo->nSetCnt; nLoop++) 
		{
			if(pMainInfo->astSubChInfo[nLoop].uiTmID == TMID_1) // DMB
				continue;

			pDest = &pMainInfo->astSubChInfo[nLoop];
			INC_SWAP(pMainInfo, nIndex, nLoop);
			return INC_SUCCESS;
		}
	}
	return INC_ERROR;
}

ST_BBPINFO* INC_GET_STRINFO(INC_UINT8 ucI2CID)
{
	if(ucI2CID == TDMB_I2C_ID80) return &g_astBBPRun[0];
	return &g_astBBPRun[1];
}

INC_UINT16 INC_GET_FRAME_DURATION(ST_TRANSMISSION cTrnsMode)
{
	INC_UINT16 uPeriodFrame;
	switch(cTrnsMode){
	case TRANSMISSION_MODE1 : uPeriodFrame = MAX_FRAME_DURATION; break;
	case TRANSMISSION_MODE2 :
	case TRANSMISSION_MODE3 : uPeriodFrame = MAX_FRAME_DURATION/4; break;
	case TRANSMISSION_MODE4 : uPeriodFrame = MAX_FRAME_DURATION/2; break;
	default : uPeriodFrame = MAX_FRAME_DURATION; break;
	}
	return uPeriodFrame;
}

INC_UINT8 INC_GET_FIB_CNT(ST_TRANSMISSION ucMode)
{
	INC_UINT8 ucResult = 0;
	switch(ucMode){
	case TRANSMISSION_MODE1 : ucResult = 12; break;
	case TRANSMISSION_MODE2 : ucResult = 3; break;
	case TRANSMISSION_MODE3 : ucResult = 4; break;
	case TRANSMISSION_MODE4 : ucResult = 6; break;
	default: ucResult = 12; break;
	}
	return ucResult;
}

void INC_INTERRUPT_CTRL(INC_UINT8 ucI2CID)
{
	INC_CMD_WRITE(ucI2CID, APB_INT_BASE+ 0x00, m_unIntCtrl);
}

void INC_INTERRUPT_CLEAR(INC_UINT8 ucI2CID, INC_UINT16 uiClrInt)
{
	INC_CMD_WRITE(ucI2CID, APB_INT_BASE+ 0x03, uiClrInt);
}

void INC_INTERRUPT_SET(INC_UINT8 ucI2CID, INC_UINT16 uiSetInt)
{
	INC_UINT16 uiIntSet;
	uiIntSet = ~uiSetInt;
	INC_CMD_WRITE(ucI2CID, APB_INT_BASE+ 0x02, uiIntSet);
}

INC_UINT8 INC_CHIP_STATUS(INC_UINT8 ucI2CID)
{
	INC_UINT16	uiChipID;
	
    	printk(" [TDMB_T3900] INC_CHIP_STATUS \n");
	
	uiChipID = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x10) & 0xF00;
	if(uiChipID != 0xD00 && uiChipID != 0xC00){
		printk(" [TDMB_3900] Chip ID Error : 0x%X\r\n", uiChipID);
		return INC_ERROR;
	}
	printk(" [TDMB_T3900] Chip ID good : 0x%X\r\n", uiChipID);
	return INC_SUCCESS;
}

INC_UINT8 INC_PLL_SET(INC_UINT8 ucI2CID) 
{
	INC_UINT16	wData;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	wData = INC_CMD_READ(ucI2CID, APB_GEN_BASE+ 0x02) & 0xFE00;

	switch(m_ucPLL_Mode){
	case INPUT_CLOCK_24576KHZ:
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x0000);
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x06, 0x0002);
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x21, 0x3FFF);	// Reset Timing	- Register ???? ('10. 11. 08)
	break;

	case INPUT_CLOCK_27000KHZ:
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x02, wData | 0x41BE);
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x03, 0x310A);
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x7FFF);
	break;

	case INPUT_CLOCK_19200KHZ:
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x01, 0x0003);	// RFFDIV
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x02, 0x0086);	// FBDIV
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x03, 0x0066);	// FRAC[23:16]
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x04, 0x6666);	// FRAC[15:0]
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x05, 0x0075);	// POSTDIV1, POSTDIV2
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x06, 0x0000);	// Power down disable
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x07, 0x0000);	// Power down disable
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x0001);	// PLL Clock Selection	// [0] : Clock_Sel ('10. 11. 08)
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x21, 0x3FFF);	// Reset Timing	- Register ???? ('10. 11. 08)
	break;

	case INPUT_CLOCK_27120KHZ:
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x01, 0x0001);	 // RFFDIV
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x02, 0x001F);	 // FBDIV
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x03, 0x00B7);	 // FRAC[23:16]
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x04, 0x8122);	 // FRAC[15:0]
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x05, 0x0075);	 // POSTDIV1, POSTDIV2
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x06, 0x0000);	 // Power down disable
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x07, 0x0000);	 // Power down disable
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+0x00, 0x0001);	 // PLL Clock Selection
	break;

	case INPUT_CLOCK_12000KHZ:
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x02, wData | 0x4200);
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x03, 0x190A);
	INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x7FFF);
	break;
	}

	if(m_ucPLL_Mode == INPUT_CLOCK_24576KHZ) return INC_SUCCESS;
	INC_DELAY(10);

	if(!(INC_CMD_READ(ucI2CID, APB_GEN_BASE+ 0x09) & 0x0001)){
		pInfo->nBbpStatus = ERROR_PLL;
		return INC_ERROR;
	}

	return INC_SUCCESS;
}

void INC_SET_CHANNEL(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo)
{
	INC_CHANNEL_INFO* pChInfo;
	//INC_UINT16	uiStatus = 0;
	INC_UINT16  uiLoop, wData;
	INC_CTRL    wDmbMode;

#ifdef INC_MULTI_HEADER_ENABLE	
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, 0x800);
#endif

	for(uiLoop = 0; uiLoop < pMultiInfo->nSetCnt; uiLoop++)
	{
		pChInfo = &pMultiInfo->astSubChInfo[uiLoop];
		wDmbMode = (pChInfo->ucServiceType == 0x18) ? INC_DMB : INC_DAB;
		wData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x01) & (~(0x3 << uiLoop*2));

		if(wDmbMode == INC_DMB) 
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, wData | 0x02 << (uiLoop*2));
		else 
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, wData | 0x01 << (uiLoop*2));

		wData = INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x00);
		INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | (0x40 >> uiLoop));
	}

	wData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x00);
	INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x00, wData | 0x0001);

	wData = INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x00);
#ifdef INC_MULTI_CHANNEL_FIC_UPLOAD
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | 0x8080);
#else
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | 0x8000);
#endif
}

INC_UINT8 INC_INIT(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	memset(pInfo, 0 , sizeof(ST_BBPINFO));

    	printk(" [TDMB_T3900] INC_INIT\n");

	if(m_ucTransMode < TRANSMISSION_MODE1 || m_ucTransMode > TRANSMISSION_AUTOFAST)
		return INC_ERROR;

	pInfo->ucTransMode = m_ucTransMode;
 
	if(INC_PLL_SET(ucI2CID) != INC_SUCCESS)
	{
            	printk(" [TDMB_T3900] INC_PLL_SET FAIL\n");
            	return INC_ERROR;
        }    	
    	
	if(INC_CHIP_STATUS(ucI2CID) != INC_SUCCESS) 	
	{
            	printk(" [TDMB_T3900] INC_CHIP_STATUS\n");
                return INC_ERROR;
	}	    

	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x3B, 0xFFFF);
	INC_DELAY(10);

	INC_INTERRUPT_CTRL(ucI2CID);
	INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x00, 0x8000);
	INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, 0x01C1);
	INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x05, 0x0008);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE + 0x01, TS_ERR_THRESHOLD);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE + 0x09, 0x000C);

	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x00, 0xF0FF);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x88, 0x2210);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x98, 0x0000);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, 0x0CCC);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xB0, 0x8320);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xB4, 0x4C01);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xBC, 0x4088);

	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x40, 0xD05C);		// Auto Resync OFF (0xF05C -> 0xD05C) ('10. 11. 03)
	//INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x40, 0xF05C);		// Auto Resync OFF (0xF05C -> 0xD05C) ('10. 11. 03)
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x8C, 0x0183);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE + 0x0A, 0x80F0);

	INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, 0x0001);	// Deinterleaver Enable	('10. 11. 04) - ???? ????

	switch(m_ucTransMode){
	case TRANSMISSION_AUTO:
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x98, 0x8000);
	break;

	case TRANSMISSION_AUTOFAST:
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x98, 0xC000);
	break;

	case TRANSMISSION_MODE1:
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xD0, 0x7F1F);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x80, 0x4082);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x90, 0x0430);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xC8, 0x3F31);
	break;

	case TRANSMISSION_MODE2:
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xD0, 0x1F07);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x80, 0x4182); 
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x90, 0x0415); 
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xC8, 0x1F31);
	break;

	case TRANSMISSION_MODE3:
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xD0, 0x0F03);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x80, 0x4282);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x90, 0x0408);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xC8, 0x0331);
	break;

	case TRANSMISSION_MODE4:
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xD0, 0x3F0F);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x80, 0x4382);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x90, 0x0420); 
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xC8, 0x3F31);
	break;
	}

	INC_MPICLOCK_SET(ucI2CID);
	INC_UPLOAD_MODE(ucI2CID);

	switch(m_ucRfBand){
	case KOREA_BAND_ENABLE	: INC_READY(ucI2CID,	175280); break;
	case BANDIII_ENABLE 	: INC_READY(ucI2CID,	174928); break;
	case LBAND_ENABLE		: INC_READY(ucI2CID,	1452960); break;
	case CHINA_ENABLE		: INC_READY(ucI2CID,	168160); break;
	case ROAMING_ENABLE 	: INC_READY(ucI2CID,	217280); break;
	}

	INC_CMD_WRITE(ucI2CID, APB_RF_BASE + 0x0F, 0x40); 
	if(m_ucPLL_Mode == INPUT_CLOCK_19200KHZ) INC_CMD_WRITE(ucI2CID, APB_RF_BASE + 0x10, 0x6A);
	if(m_ucPLL_Mode == INPUT_CLOCK_24576KHZ) INC_CMD_WRITE(ucI2CID, APB_RF_BASE + 0x10, 0x88);
	if(m_ucPLL_Mode == INPUT_CLOCK_27000KHZ) INC_CMD_WRITE(ucI2CID, APB_RF_BASE + 0x10, 0x96);
	if(m_ucPLL_Mode == INPUT_CLOCK_27120KHZ) INC_CMD_WRITE(ucI2CID, APB_RF_BASE + 0x10, 0x96);

	INC_DELAY(100);
	printk(" [TDMB_T3900] INC_INIT START GOOD\n");
	return INC_SUCCESS;
}

INC_UINT8 INC_READY(INC_UINT8 ucI2CID, INC_UINT32 ulFreq)
{
	INC_UINT16	uStatus = 0;
	INC_UINT8   bModeFlag = 0;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	if(ulFreq == 189008 || ulFreq == 196736 || ulFreq == 205280 || ulFreq == 213008
		|| ulFreq == 180064 || ulFreq == 188928 || ulFreq == 195936
		|| ulFreq == 204640 || ulFreq == 213360 || ulFreq == 220352
		|| ulFreq == 222064 || ulFreq == 229072 || ulFreq == 237488
		|| ulFreq == 180144 || ulFreq == 196144 || ulFreq == 205296
		|| ulFreq == 212144 || ulFreq == 213856 
		|| ulFreq == 1466656 || ulFreq == 1475216 || ulFreq == 1482064 || ulFreq == 1490624) { 
			bModeFlag = 1;
	}
	if(m_ucRfBand == ROAMING_ENABLE) bModeFlag = 1;

	if(bModeFlag)
	{
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0xC6);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC6, uStatus | 0x0008);
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFC00;
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0x111);
	}
	else
	{
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0xC6);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC6, uStatus & 0xFFF7);
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFC00;
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0xCC);

		if(ulFreq == 1461520 || ulFreq == 1463232 || ulFreq == 1475216 || ulFreq == 1485488 ){
			uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x40);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x40, uStatus | (1<<10));
		}
		else{
			uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x40);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x40, uStatus & ~(1<<10));
		}
	}

	if(INC_RF500_START(ucI2CID, ulFreq, m_ucRfBand) != INC_SUCCESS){
		pInfo->nBbpStatus = ERROR_READY;
		return INC_ERROR;
	}

	if(m_ucDPD_Mode == INC_DPD_OFF){
		uStatus = INC_CMD_READ(ucI2CID, APB_RF_BASE+ 0x03);
		INC_CMD_WRITE(ucI2CID, APB_RF_BASE+ 0x03, uStatus | 0x80);
	}

	return INC_SUCCESS;
}

INC_UINT8 INC_SYNCDETECTOR(INC_UINT8 ucI2CID, INC_UINT32 ulFreq, INC_UINT8 ucScanMode)
{
	INC_UINT16	wOperState, wIsNullSync = 0, wSyncRefTime = 800;
	INC_UINT16	uiTimeOutCnt = 0, uiRefSleep = 30;
	//INC_UINT8	IsSyncNull = 0;
	ST_BBPINFO* pInfo;
	INC_UINT16	awData[2], uiRfNullTime = 240, wFftCnt = 0;

	pInfo = INC_GET_STRINFO(ucI2CID);
	INC_SCAN_SETTING(ucI2CID);
	if(m_ucDPD_Mode == INC_DPD_ON){
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xA8, 0x3000);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xAA, 0x0000);
	}

	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xBC, 0x4088);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0x3A, 0x1);
	INC_DELAY(200);
	while(1)
	{
		if(pInfo->ucStop){
			pInfo->nBbpStatus = ERROR_USER_STOP;
			break;
		}

		INC_DELAY(uiRefSleep);

		wOperState = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x10);
		wOperState = ((wOperState & 0x7000) >> 12);

		if(!wIsNullSync && wOperState >= 0x2) wIsNullSync = 1;

		if(!wIsNullSync && wSyncRefTime != 1500){
			awData[0] = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x14) & 0x0F00; 
			awData[1] = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x16); 

			if(!awData[0] || !awData[1]){
				pInfo->nBbpStatus = ERROR_SYNC_NO_SIGNAL;
				printk("TDMB Point 1\n");
				break;
			}
			else if(awData[0] <= 0x200 && awData[1] <= 0x4000) {
				pInfo->nBbpStatus = ERROR_SYNC_NO_SIGNAL;
				printk("TDMB Point 2\n");
				break;
			}
			else if(awData[0] >= 0x300){
				if(++wFftCnt >= (uiRfNullTime/uiRefSleep)) {
					pInfo->nBbpStatus = ERROR_SYNC_NO_SIGNAL;
					printk("TDMB Point 3\n");
					break;
				}
				else if(awData[1] <= 0x4000){
					if(++wFftCnt >= (uiRfNullTime/uiRefSleep)){
						pInfo->nBbpStatus = ERROR_SYNC_NO_SIGNAL;
						printk("TDMB Point 4\n");
						break;
					}
				}
			}
		}

		if(wOperState >= 0x5){
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xBC, 0x4008);
			return INC_SUCCESS;
		}

		uiTimeOutCnt++;
		if(uiTimeOutCnt >= (2500 / uiRefSleep)){
			pInfo->nBbpStatus = ERROR_SYNC_TIMEOUT;
			break;
		}
	}

	return INC_ERROR;
}


void FILE_SAVE_FUNCTION(char* fname, unsigned char *pData, unsigned long dwDataSize)
{
#define DUMP_FILE_NAME "/data/ts_data.ts"
	mm_segment_t oldfs;
	int ret;
	static struct file *fp_ts_data  = NULL;

	if(fp_ts_data == NULL) 
	{
		fp_ts_data = filp_open(DUMP_FILE_NAME, O_APPEND | O_CREAT, 0);
		if(fp_ts_data == NULL) 
		{
			printk("[%s] file open error!\n", __FUNCTION__);
			return;
		}
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	if(fp_ts_data != NULL &&  dwDataSize == 0)
	{
		filp_close(fp_ts_data, current->files);	
		fp_ts_data = NULL;
		return;
	}

	//printk("[%s] save ts data \n", __FUNCTION__);
	ret = fp_ts_data->f_op->write(
			fp_ts_data, 
			pData, 
			dwDataSize, 
			&fp_ts_data->f_pos);

	//fp_ts_data->f_op->flush(fp_ts_data, current->files);
	set_fs(oldfs);
}

INC_UINT8 INC_FICDECODER(INC_UINT8 ucI2CID, ST_SIMPLE_FIC bSimpleFIC)
{
	INC_UINT16		wFicLen, uPeriodFrame, uFIBCnt, uiRefSleep;
	INC_UINT16		nLoop, nFicCnt;
	INC_UINT8		abyBuff[MAX_FIC_SIZE];
	ST_BBPINFO*		pInfo;

#ifdef USER_APPLICATION_TYPE
	INC_UINT8		ucIsUserApp = INC_ERROR;
	ST_FICDB_LIST*		pList;
	INC_UINT8		ucTimeOutCnt = 0;
	pList = INC_GET_FICDB_LIST();
#endif

	pInfo = INC_GET_STRINFO(ucI2CID);

	INC_INITDB(ucI2CID);
	uFIBCnt			= INC_GET_FIB_CNT(m_ucTransMode);
	uPeriodFrame	= INC_GET_FRAME_DURATION(m_ucTransMode);
	uiRefSleep		= uPeriodFrame >> 2;
	nFicCnt			= FIC_REF_TIME_OUT / uiRefSleep;


#ifdef USER_APPLICATION_TYPE
	nFicCnt = (FIC_REF_TIME_OUT+1000) / uiRefSleep;
#endif

	for(nLoop = 0; nLoop < nFicCnt; nLoop++)
	{
		if(pInfo->ucStop == 1){
			pInfo->nBbpStatus = ERROR_USER_STOP;
			break;
		}

		INC_DELAY(uiRefSleep);

		if(!(INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x00) & 0x4000))
			continue;

		wFicLen = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x09);
		if(!wFicLen) continue;
		wFicLen++;
		
		if(wFicLen != (uFIBCnt*FIB_SIZE)) continue;
		INC_CMD_READ_BURST(ucI2CID, APB_FIC_BASE, abyBuff, wFicLen);
		//FILE_SAVE_FUNCTION("", abyBuff, wFicLen);
		
		if(INC_FICPARSING(ucI2CID, abyBuff, wFicLen, bSimpleFIC)){
			if(bSimpleFIC == SIMPLE_FIC_ENABLE){
				return INC_SUCCESS;
			}

#ifdef USER_APPLICATION_TYPE
			ucTimeOutCnt++;

			if(ucTimeOutCnt >= 10 && pList->nPacketCnt && (pList->nPacketCnt == pList->nUserAppCnt)) {
				return INC_SUCCESS;
			}
			ucIsUserApp = INC_SUCCESS;
#else

			return INC_SUCCESS;
#endif
		}
	}

#ifdef USER_APPLICATION_TYPE
	if(ucIsUserApp == INC_SUCCESS)
		return INC_SUCCESS;
#endif
	pInfo->nBbpStatus = ERROR_FICDECODER;

	return INC_ERROR;
}

INC_UINT8 INC_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pChInfo, INC_UINT16 IsEnsembleSame)
{
	INC_INT16 	nLoop, nSchID;
	INC_UINT16 	wData;
	ST_BBPINFO* pInfo;
	INC_CHANNEL_INFO* pTempChInfo;

	INC_UINT16 wCeil = 0, wIndex = 0, wStartAddr, wEndAddr;

	pInfo = INC_GET_STRINFO(ucI2CID);
	INC_BUBBLE_SORT(pChInfo, INC_START_ADDRESS);

	INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, 0x0000);
	wData = INC_CMD_READ(ucI2CID, APB_MPI_BASE+0);
	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+0, 0x8000|wData);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, 0x0000);

	for(nLoop = 0; nLoop < 3; nLoop++)
	{
		if(nLoop >= pChInfo->nSetCnt){
			INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x02 + (nLoop*2), 0x00);
			INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x03 + (nLoop*2), 0x00);
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x02 + nLoop, 0x00);
			continue;
		}
		pTempChInfo= &pChInfo->astSubChInfo[nLoop];

		nSchID = (INC_UINT16)pTempChInfo->ucSubChID & 0x3f;
		INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x02 + (nLoop*2), (INC_UINT16)(((INC_UINT16)nSchID << 10) + pTempChInfo->uiStarAddr));

		if(pTempChInfo->ucSlFlag == 0) {
			INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x03 + (nLoop*2), 0x8000 + (pTempChInfo->ucTableIndex & 0x3f));
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x02 + nLoop, (pTempChInfo->ucTableIndex & 0x3f));
		}
		else{
			INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE + 0x03 + (nLoop*2), 0x8000 + 0x400 + pTempChInfo->uiSchSize);
			wData = 0x8000 
				+ ((pTempChInfo->ucOption & 0x7) << 12) 
				+ ((pTempChInfo->ucProtectionLevel & 0x3) << 10) 
				+ pTempChInfo->uiDifferentRate;
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x02 + nLoop, wData);
		}

		if(m_ucDPD_Mode == INC_DPD_ON){
			switch(pInfo->ucTransMode){
				case TRANSMISSION_MODE1 : wCeil = 3072;	wIndex = 4; break; 
				case TRANSMISSION_MODE2 : wCeil = 768;	wIndex = 4;	break;
				case TRANSMISSION_MODE3 : wCeil = 384;	wIndex = 9;	break;
				case TRANSMISSION_MODE4 : wCeil = 1536;	wIndex = 4;	break;
				default : wCeil = 3072; break;
			}

			wStartAddr = ((pTempChInfo->uiStarAddr * 64) / wCeil) + wIndex - 0;
			wEndAddr = (INC_UINT16)(((pTempChInfo->uiStarAddr + pTempChInfo->uiSchSize) * 64) / wCeil) + wIndex + 1;
			wData = (wStartAddr & 0xFF) << 8 | (wEndAddr & 0xFF);

			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xAA + (nLoop * 2), wData);
			wData = INC_CMD_READ(ucI2CID, APB_PHY_BASE + 0xA8);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xA8, wData | (1<<nLoop));
		}
		else{
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xAA + (nLoop * 2), 0x0000);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xA8, 0x3000);
		}

		wData = INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x00);
		INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | (0x1 << (6-nLoop)));
	}

	INC_SET_CHANNEL(ucI2CID, pChInfo);
	INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x0, 0x0001);
	INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x1, 0x0001);

	return INC_SUCCESS;
}

INC_UINT8 INC_STOP(INC_UINT8 ucI2CID)
{
	INC_INT16 nLoop;
	INC_UINT16 uStatus;
	ST_TRANSMISSION ucTransMode;
	ST_BBPINFO* pInfo;

	pInfo = INC_GET_STRINFO(ucI2CID);
	ucTransMode = pInfo->ucTransMode;
	memset(pInfo, 0, sizeof(ST_BBPINFO));
	pInfo->ucTransMode = ucTransMode;

	INC_CMD_WRITE(ucI2CID, APB_RS_BASE + 0x00, 0x0000);

	uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x10) & 0x7000;
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE  + 0x3B, 0x4000);

   INC_DELAY(25);

	for(nLoop = 0; nLoop < 10; nLoop++){
		INC_DELAY(2);
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x3B) & 0xFFFF;
		if(uStatus == 0x8000) break;
	}

	if(nLoop >= 10){
		pInfo->nBbpStatus = ERROR_STOP;
		return INC_ERROR;
	}

	return INC_SUCCESS;
}

void INC_SCAN_SETTING(INC_UINT8 ucI2CID)
{
	INC_UINT16 uStatus;
	uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFFF;
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0xC000);

	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB6, T3900_SCAN_IF_DELAY&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB5, (T3900_SCAN_IF_DELAY>>8)&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB4, (T3900_SCAN_IF_DELAY>>16)&0xff);

	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB3, T3900_SCAN_RF_DELAY&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB2, (T3900_SCAN_RF_DELAY>>8)&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB1, (T3900_SCAN_RF_DELAY>>16)&0xff);
}

void INC_AIRPLAY_SETTING(INC_UINT8 ucI2CID)
{
	INC_UINT16 uStatus;
	uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFFF;
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0x4000);

	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB6, T3900_PLAY_IF_DELAY&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB5, (T3900_PLAY_IF_DELAY>>8)&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB4, (T3900_PLAY_IF_DELAY>>16)&0xff);

	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB3, T3900_PLAY_RF_DELAY&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB2, (T3900_PLAY_RF_DELAY>>8)&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+0xB1, (T3900_PLAY_RF_DELAY>>16)&0xff);
}

INC_UINT8 INC_CHANNEL_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pChInfo)
{
	INC_UINT16 wEnsemble;
	INC_UINT32 ulRFFreq;
	ST_BBPINFO* pInfo;
	
	INC_INTERRUPT_SET(ucI2CID,INC_MPI_INTERRUPT_ENABLE);
	INC_INTERRUPT_CLEAR(ucI2CID, INC_MPI_INTERRUPT_ENABLE);
	pInfo = INC_GET_STRINFO(ucI2CID);
	
	pInfo->ucStop				= 0;
	pInfo->nBbpStatus 			= ERROR_NON;
	ulRFFreq					= pChInfo->astSubChInfo[0].ulRFFreq;
	
	wEnsemble = pInfo->ulFreq == ulRFFreq;

	//INC_MSG_PRINTF(1, "================= T3900 INC_CHANNEL_START =================\r\n");
	//INC_MSG_PRINTF(1, "== :: Frequency              %6d[Khz]                    ==\r\n", pChInfo->astSubChInfo[0].ulRFFreq);
	//INC_MSG_PRINTF(1, "== :: stChInfo.ucSubChID     0x%.4X                      ==\r\n",  pChInfo->astSubChInfo[0].ucSubChID);
	//INC_MSG_PRINTF(1, "== :: stChInfo.ucServiceType 0x%.4X                      ==\r\n", pChInfo->astSubChInfo[0].ucServiceType);
	//INC_MSG_PRINTF(1, "===========================================================\r\n");
	printk("================= T3900 INC_CHANNEL_START =================\r\n");
	printk("== :: Frequency              %6d[Khz]                    ==\r\n", pChInfo->astSubChInfo[0].ulRFFreq);
	printk( "== :: stChInfo.ucSubChID     0x%.4X                      ==\r\n",  pChInfo->astSubChInfo[0].ucSubChID);
	printk("== :: stChInfo.ucServiceType 0x%.4X                      ==\r\n", pChInfo->astSubChInfo[0].ucServiceType);
	printk("===========================================================\r\n");
	
	if(!wEnsemble){
		if(INC_STOP(ucI2CID) != INC_SUCCESS)return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_STOP GOOD \r\n");
		printk(" T3900 INC_STOP GOOD \r\n");
		if(INC_READY(ucI2CID, ulRFFreq) != INC_SUCCESS) return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_READY GOOD \r\n");
		printk(" T3900 INC_READY GOOD \r\n");
		if(INC_SYNCDETECTOR(ucI2CID,ulRFFreq, 0) != INC_SUCCESS)return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_SYNCDETECTOR GOOD \r\n");
		printk(" T3900 INC_SYNCDETECTOR GOOD \r\n");
		if(INC_FICDECODER(ucI2CID, SIMPLE_FIC_ENABLE) != INC_SUCCESS)return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_FICDECODER GOOD \r\n");
		printk(" T3900 INC_FICDECODER GOOD \r\n");
		if(INC_FIC_UPDATE(ucI2CID, pChInfo, SIMPLE_FIC_ENABLE) != INC_SUCCESS) return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_FIC_UPDATE GOOD \r\n");
		printk(" T3900 INC_FIC_UPDATE GOOD \r\n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS) return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_START GOOD \r\n");
		printk(" T3900 INC_START GOOD \r\n");
	}
	else{
		if(INC_SYNCDETECTOR(ucI2CID, ulRFFreq, 0) != INC_SUCCESS){
			pInfo->ulFreq = 0;
			return INC_ERROR;
		}
//		INC_MSG_PRINTF(1, " T3900 INC_SYNCDETECTOR GOOD \r\n");
		printk(" T3900 INC_SYNCDETECTOR GOOD \r\n");
		if(INC_FIC_UPDATE(ucI2CID, pChInfo, SIMPLE_FIC_ENABLE) != INC_SUCCESS) return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_FIC_UPDATE GOOD \r\n");
		printk(" T3900 INC_FIC_UPDATE GOOD \r\n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS) return INC_ERROR;
//		INC_MSG_PRINTF(1, " T3900 INC_START GOOD \r\n");
		printk(" T3900 INC_START GOOD \r\n");
	}

	INC_AIRPLAY_SETTING(ucI2CID);
	
	pInfo->ucProtectionLevel = pChInfo->astSubChInfo[0].ucProtectionLevel;
	pInfo->ulFreq = ulRFFreq;
	return INC_SUCCESS;
}

INC_UINT8 INC_CHANNEL_START_TEST(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pChInfo)
{
	INC_UINT16 wEnsemble;
	INC_UINT32 ulRFFreq;
	ST_BBPINFO* pInfo;
	
	INC_INTERRUPT_SET(ucI2CID,INC_MPI_INTERRUPT_ENABLE);
	INC_INTERRUPT_CLEAR(ucI2CID, INC_MPI_INTERRUPT_ENABLE);
	pInfo = INC_GET_STRINFO(ucI2CID);
	
	pInfo->ucStop				= 0;
	pInfo->nBbpStatus 			= ERROR_NON;
	ulRFFreq					= pChInfo->astSubChInfo[0].ulRFFreq;
	
	wEnsemble = pInfo->ulFreq == ulRFFreq;

	INC_MSG_PRINTF(1, "================= T3900 INC_CHANNEL_START_TEST =================\r\n");
	INC_MSG_PRINTF(1, "== :: Frequency              %6d[Khz]                    ==\r\n", pChInfo->astSubChInfo[0].ulRFFreq);
	INC_MSG_PRINTF(1, "== :: stChInfo.ucSubChID     0x%.4X                      ==\r\n",  pChInfo->astSubChInfo[0].ucSubChID);
	INC_MSG_PRINTF(1, "== :: stChInfo.ucServiceType 0x%.4X                      ==\r\n", pChInfo->astSubChInfo[0].ucServiceType);
	INC_MSG_PRINTF(1, "===========================================================\r\n");
	
	if(!wEnsemble){
		if(INC_STOP(ucI2CID) != INC_SUCCESS)return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_STOP GOOD \r\n");
		if(INC_READY(ucI2CID, ulRFFreq) != INC_SUCCESS) return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_READY GOOD \r\n");
		if(INC_SYNCDETECTOR(ucI2CID,ulRFFreq, 0) != INC_SUCCESS)return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_SYNCDETECTOR GOOD \r\n");
		if(INC_FICDECODER(ucI2CID, SIMPLE_FIC_DISABLE) != INC_SUCCESS)return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_FICDECODER GOOD \r\n");
		if(INC_FIC_UPDATE(ucI2CID, pChInfo, SIMPLE_FIC_ENABLE) != INC_SUCCESS) return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_FIC_UPDATE GOOD \r\n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS) return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_START GOOD \r\n");
	}
	else{

		if(INC_SYNCDETECTOR(ucI2CID, ulRFFreq, 0) != INC_SUCCESS){
			pInfo->ulFreq = 0;
			return INC_ERROR;
		}
		INC_MSG_PRINTF(1, " T3900 INC_SYNCDETECTOR GOOD \r\n");
		if(INC_FIC_UPDATE(ucI2CID, pChInfo, SIMPLE_FIC_ENABLE) != INC_SUCCESS) return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_FIC_UPDATE GOOD \r\n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS) return INC_ERROR;
		INC_MSG_PRINTF(1, " T3900 INC_START GOOD \r\n");
	}

	INC_AIRPLAY_SETTING(ucI2CID);
	
	pInfo->ucProtectionLevel = pChInfo->astSubChInfo[0].ucProtectionLevel;
	pInfo->ulFreq = ulRFFreq;
	return INC_SUCCESS;
}

INC_UINT8 INC_ENSEMBLE_SCAN(INC_UINT8 ucI2CID, INC_UINT32 ulFreq)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	pInfo->nBbpStatus = ERROR_NON;
	pInfo->ucStop = 0;

	printk("[TDMB_T3900] INC_ENSEMBLE_SCAN (%d)\n", ulFreq);

	if(INC_STOP(ucI2CID) != INC_SUCCESS) return INC_ERROR;
	printk("[TDMB_T3900] INC_STOP GOOD\n");
	
	if(INC_READY(ucI2CID, ulFreq) != INC_SUCCESS)return INC_ERROR;
	printk("[TDMB_T3900] INC_READY GOOD\n");
	
	if(INC_SYNCDETECTOR(ucI2CID, ulFreq, 1) != INC_SUCCESS)	return INC_ERROR;
	printk("[TDMB_T3900] INC_SYNCDETECTOR GOOD\n");
	
	if(INC_FICDECODER(ucI2CID, SIMPLE_FIC_DISABLE) != INC_SUCCESS)	return INC_ERROR;
	printk("[TDMB_T3900] INC_FICDECODER GOOD\n");
	
	return INC_SUCCESS;
}

INC_UINT8 INC_FIC_RECONFIGURATION_HW_CHECK(INC_UINT8 ucI2CID)
{
	INC_UINT16 wStatus, uiFicLen, wReconf;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	wStatus = (INC_UINT16)INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x00);
	if(!(wStatus & 0x4000)) return INC_ERROR;
	uiFicLen = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x09) + 1;

	if(uiFicLen != MAX_FIC_SIZE) return INC_ERROR;
	wReconf = (INC_UINT16)INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x0A);

	if(wReconf & 0xC0){
		pInfo->ulReConfigTime = (INC_UINT16)(INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x0B) & 0xff) * 24;
		return INC_SUCCESS;
	}
	return INC_ERROR;
}

INC_UINT8 INC_RE_SYNC(INC_UINT8 ucI2CID)
{
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x3B, 0x4000);
	INC_DELAY(24);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x3A, 0x1);
    
	printk(" [T3900] INC_RE_SYNC \n");

	return INC_SUCCESS;
}

INC_UINT16 INC_RESYNC_CER(INC_UINT8 ucI2CID)
{
	INC_UINT16  uiVtbErr, unCER;
	INC_UINT16  uiVtbData;
	
	uiVtbErr  = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);
	uiVtbData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

	unCER = (!uiVtbData) ? 0 : (INC_UINT16)((INC_UINT32)(uiVtbErr * 10000) / (uiVtbData * 64));
	if(uiVtbErr == 0) unCER = 2000;

	printk("[T3900]INC_RESYNC_CER : %d ", unCER);

	return unCER;
}

INC_UINT8 INC_STATUS_CHECK(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	if(INC_RESYNC_CER(ucI2CID) >= 1200) pInfo->ucCERCnt++;
	else pInfo->ucCERCnt = 0;

	if(pInfo->ucCERCnt >= INC_CER_PERIOD){
		INC_RE_SYNC(ucI2CID);		// S/W Resync Function ('10. 11. 03)
        pInfo->ucCERCnt = 0;
		return INC_ERROR;
	}

	return INC_SUCCESS;
}


INC_UINT16 INC_GET_CER(INC_UINT8 ucI2CID)
{
	INC_INT16  nBER, nLoop;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	if(pInfo->ucProtectionLevel == 0) 
	   nBER = (INC_INT16) ((INC_INT32)INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x07) * 77/100);
	else  
	   nBER = (INC_INT16) INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x07);
	if(nBER == 0) nBER = 2000;

	if(nBER  <= BER_REF_VALUE) pInfo->auiANTBuff[pInfo->uiInCAntTick++ % BER_BUFFER_MAX] = 0;
	else pInfo->auiANTBuff[pInfo->uiInCAntTick++ % BER_BUFFER_MAX] = (nBER  - BER_REF_VALUE);

	for(nLoop = 0 , pInfo->uiInCERAvg = 0; nLoop < BER_BUFFER_MAX; nLoop++)
		pInfo->uiInCERAvg += pInfo->auiANTBuff[nLoop];

	pInfo->uiInCERAvg /= BER_BUFFER_MAX;
	
	// by jhjung
	pInfo->uiCER = pInfo->uiInCERAvg;
	return pInfo->uiCER;
}

INC_UINT8 INC_GET_ANT_LEVEL(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	INC_GET_SNR(ucI2CID);

	if(pInfo->ucSnr > 16)		pInfo->ucAntLevel = 6;
	else if(pInfo->ucSnr > 13)	pInfo->ucAntLevel = 5;
	else if(pInfo->ucSnr > 10)	pInfo->ucAntLevel = 4;
	else if(pInfo->ucSnr > 7)	pInfo->ucAntLevel = 3;
	else if(pInfo->ucSnr > 5)	pInfo->ucAntLevel = 2;
	else if(pInfo->ucSnr > 3)	pInfo->ucAntLevel = 1;
	else						pInfo->ucAntLevel = 0;

	return pInfo->ucAntLevel;
}

INC_UINT32 INC_GET_PREBER(INC_UINT8 ucI2CID)
{
	INC_UINT16 		uiVtbErr;
	INC_UINT32		uiVtbData;
	INC_UINT32		uiPreBER;

	uiVtbErr 	= INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);
	uiVtbData 	= INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

	uiPreBER = (!uiVtbData) ? 0 : ((INC_UINT32)uiVtbErr / (uiVtbData * 64));

	return uiPreBER;
}

INC_UINT32 INC_GET_POSTBER(INC_UINT8 ucI2CID)
{
	INC_UINT16	uiRSErrBit;
	INC_UINT16	uiRSErrTS;
	INC_UINT16	uiError, uiRef;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	uiRSErrBit 	= INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x02);
	uiRSErrTS 	= INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x03);
	uiRSErrBit += (uiRSErrTS * 8);

	if(uiRSErrTS == 0){
		uiError = ((uiRSErrBit * 50)  / 1000);
		uiRef = 0;
		if(uiError > 50) uiError = 50;
	}
	else if((uiRSErrTS > 0) && (uiRSErrTS < 10)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 50;
		if(uiError > 50) uiError = 50;
	}
	else if((uiRSErrTS >= 10) && (uiRSErrTS < 30)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 60;
		if(uiError > 40) uiError = 40;
	}
	else if((uiRSErrTS >= 30) && (uiRSErrTS < 80)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 70;
		if(uiError > 30) uiError = 30;
	}
	else if((uiRSErrTS >= 80) && (uiRSErrTS < 100)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 80;
		if(uiError > 20) uiError = 20;
	}
	else{
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 90;
		if(uiError > 10) uiError = 10;
	}

	pInfo->ucVber = 100 - (uiError + uiRef);
	pInfo->uiPostBER = (INC_UINT32)uiRSErrTS / TS_ERR_THRESHOLD;

	return pInfo->uiPostBER;
}

INC_UINT8 INC_GET_SNR(INC_UINT8 ucI2CID)
{
	INC_UINT16	uiFftVal;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	uiFftVal = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x07);

	if(uiFftVal == 0)		pInfo->ucSnr = 0;
	else if(uiFftVal < 11)	pInfo->ucSnr = 20;
	else if(uiFftVal < 15)	pInfo->ucSnr = 19;
	else if(uiFftVal < 20)	pInfo->ucSnr = 18;
	else if(uiFftVal < 30)	pInfo->ucSnr = 17;
	else if(uiFftVal < 45)	pInfo->ucSnr = 16;
	else if(uiFftVal < 60)	pInfo->ucSnr = 15;
	else if(uiFftVal < 75)	pInfo->ucSnr = 14;
	else if(uiFftVal < 90)	pInfo->ucSnr = 13;
	else if(uiFftVal < 110)	pInfo->ucSnr = 12;
	else if(uiFftVal < 130) pInfo->ucSnr = 11;
	else if(uiFftVal < 150) pInfo->ucSnr = 10;
	else if(uiFftVal < 200) pInfo->ucSnr = 9;
	else if(uiFftVal < 300) pInfo->ucSnr = 8;
	else if(uiFftVal < 400) pInfo->ucSnr = 7;
	else if(uiFftVal < 500) pInfo->ucSnr = 6;
	else if(uiFftVal < 600) pInfo->ucSnr = 5;
	else if(uiFftVal < 700) pInfo->ucSnr = 4;
	else if(uiFftVal < 800) pInfo->ucSnr = 3;
	else if(uiFftVal < 900) pInfo->ucSnr = 2;
	else if(uiFftVal < 1000) pInfo->ucSnr = 1;
	else pInfo->ucSnr = 0;

	return pInfo->ucSnr;
}

#define INC_RSSI_STEP		5
#define INC_ADC_STEP_MAX			17
INC_INT16 INC_GET_RSSI(INC_UINT8 ucI2CID)
{
	INC_INT16 nLoop, nRSSI = 0;
	INC_INT16 nResolution, nGapVal;
	INC_UINT32 uiAdcValue;
	INC_UINT16 aRFAGCTable[INC_ADC_STEP_MAX][2] = {

		{100,	1140},
		{95,	1080},
		{90,	1025},
		{85, 	965},

		{80,	900},
		{75,	835},
		{70, 	770},
		{65,	685},
		{60, 	635},

		{55,	590},
		{50,	525},
		{45,	470},
		{40,	430},
		{35, 	360},

		{30,	300},
		{25,	235},
		{20,	175},
	};

	uiAdcValue  = (INC_CMD_READ(ucI2CID, APB_RF_BASE+ 0xD4) >> 5) & 0x3FF;
	uiAdcValue  = (INC_INT16)((117302 * (INC_UINT32)uiAdcValue)/100000);


	if(!uiAdcValue) nRSSI = 0;
	else if(uiAdcValue >= aRFAGCTable[0][1]) nRSSI = (INC_INT16)aRFAGCTable[0][0];
	else if(uiAdcValue <= aRFAGCTable[INC_ADC_STEP_MAX-1][1]) nRSSI = (INC_INT16)aRFAGCTable[INC_ADC_STEP_MAX-1][0];
	else {
		for(nLoop = 1; nLoop < INC_ADC_STEP_MAX; nLoop++)	{
			if(uiAdcValue < aRFAGCTable[nLoop][1] || uiAdcValue >= aRFAGCTable[nLoop-1][1])
				continue;	

			nResolution = (aRFAGCTable[nLoop-1][1] - aRFAGCTable[nLoop][1]) / INC_RSSI_STEP;
			nGapVal = uiAdcValue - aRFAGCTable[nLoop][1];
			if(nResolution) nRSSI = (aRFAGCTable[nLoop][0]) + (nGapVal/nResolution);
			else nRSSI = (aRFAGCTable[nLoop][0]) + nGapVal;
			break;
		}
	}
	return nRSSI;
}

INC_UINT32 YMDtoMJD(ST_DATE_T stDate)
{
	INC_UINT16 wMJD;
	INC_UINT32 lYear, lMouth, lDay, L;
	INC_UINT32 lTemp1, lTemp2; 

	lYear = (INC_UINT32)stDate.usYear - (INC_UINT32)1900;
	lMouth = stDate.ucMonth;
	lDay = stDate.ucDay;

	if(lMouth == 1 || lMouth == 2) L = 1;
	else L = 0;

	lTemp1 = (lYear - L) * 36525L / 100L;
	lTemp2 = (lMouth + 1L + L * 12L) * 306001L / 10000L;

	wMJD = (INC_UINT16)(14956 + lDay + lTemp1 + lTemp2);

	return wMJD;
}

void MJDtoYMD(INC_UINT16 wMJD, ST_DATE_T *pstDate)
{
	INC_UINT32 lYear, lMouth, lTemp;

	lYear = (wMJD * 100L - 1507820L) / 36525L;
	lMouth = ((wMJD * 10000L - 149561000L) - (lYear * 36525L / 100L) * 10000L) / 306001L;

	pstDate->ucDay = (INC_UINT8)(wMJD - 14956L - (lYear * 36525L / 100L) - (lMouth * 306001L / 10000L));

	if(lMouth == 14 || lMouth == 15) lTemp = 1;
	else lTemp = 0;

	pstDate->usYear		= (INC_UINT16)(lYear + lTemp + 1900);
	pstDate->ucMonth	= (INC_UINT8)(lMouth - 1 - lTemp * 12);
}

void INC_DB_COPY(INC_UINT32 ulFreq, INC_INT16 nCnt, INC_CHANNEL_INFO* pChInfo, ST_FICDB_SERVICE_COMPONENT* pSvcComponent)
{
	INC_INT16 nIndex;
	ST_FICDB_LIST*		pList;
	pList = INC_GET_FICDB_LIST();

	for(nIndex = 0; nIndex < nCnt; nIndex++, pSvcComponent++, pChInfo++){

		pChInfo->ulRFFreq			= ulFreq;
		pChInfo->uiEnsembleID		= pList->unEnsembleID;
		pChInfo->ucSubChID			= pSvcComponent->ucSubChid;
		pChInfo->ucServiceType		= pSvcComponent->ucDSCType;
		pChInfo->uiStarAddr			= pSvcComponent->unStartAddr;
		pChInfo->uiTmID				= pSvcComponent->ucTmID;
		pChInfo->ulServiceID		= pSvcComponent->ulSid;
		pChInfo->uiPacketAddr		= pSvcComponent->unPacketAddr;
		pChInfo->uiBitRate			= pSvcComponent->uiBitRate;
		pChInfo->ucSlFlag			= pSvcComponent->ucShortLong;
		pChInfo->ucTableIndex		= pSvcComponent->ucTableIndex;
		pChInfo->ucOption			= pSvcComponent->ucOption;
		pChInfo->ucProtectionLevel	= pSvcComponent->ucProtectionLevel;
		pChInfo->uiDifferentRate	= pSvcComponent->uiDifferentRate;
		pChInfo->uiSchSize			= pSvcComponent->uiSubChSize;
		memcpy(pChInfo->aucEnsembleLabel, pList->aucEnsembleName,	MAX_LABEL_CHAR);
		memcpy(pChInfo->aucLabel,		pSvcComponent->aucLabels, MAX_LABEL_CHAR);

#ifdef USER_APPLICATION_TYPE
		pChInfo->stUsrApp = pSvcComponent->stUApp;
#endif
	}
}

INC_UINT8 INC_DB_UPDATE(INC_UINT32 ulFreq, ST_SUBCH_INFO* pDMB, ST_SUBCH_INFO* pDAB, ST_SUBCH_INFO* pDATA, ST_SUBCH_INFO* pFIDC)
{
	INC_CHANNEL_INFO* pChInfo;
	ST_STREAM_INFO*		pStreamInfo;
	ST_FICDB_LIST*		pList;
	ST_FICDB_SERVICE_COMPONENT* pSvcComponent;

	pList = INC_GET_FICDB_LIST();

	/************************************************************************/
	/* DMB channel list update                                              */
	/************************************************************************/
	pStreamInfo		= &pList->stDMB;
	pChInfo			= &pDMB->astSubChInfo[pDMB->nSetCnt];
	pSvcComponent	= pStreamInfo->astPrimary;

	if((pStreamInfo->nPrimaryCnt + pDMB->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nPrimaryCnt, pChInfo, pSvcComponent);
		pDMB->nSetCnt += pStreamInfo->nPrimaryCnt;
	}

	pSvcComponent = pStreamInfo->astSecondary;
	if((pStreamInfo->nSecondaryCnt + pDMB->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nSecondaryCnt, pChInfo, pSvcComponent);
		pDMB->nSetCnt += pStreamInfo->nSecondaryCnt;
	}

	/************************************************************************/
	/* DAB channel list update                                              */
	/************************************************************************/
	pStreamInfo		= &pList->stDAB;
	pChInfo			= &pDAB->astSubChInfo[pDAB->nSetCnt];
	pSvcComponent	= pStreamInfo->astPrimary;
	if((pStreamInfo->nPrimaryCnt + pDAB->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nPrimaryCnt, pChInfo, pSvcComponent);
		pDAB->nSetCnt += pStreamInfo->nPrimaryCnt;
	}

	pSvcComponent = pStreamInfo->astSecondary;
	if((pStreamInfo->nSecondaryCnt + pDAB->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nSecondaryCnt, pChInfo, pSvcComponent);
		pDAB->nSetCnt += pStreamInfo->nSecondaryCnt;
	}

	/************************************************************************/
	/* DATA channel list update                                             */
	/************************************************************************/
	pStreamInfo		= &pList->stDATA;
	pChInfo			= &pDATA->astSubChInfo[pDATA->nSetCnt];
	pSvcComponent	= pStreamInfo->astPrimary;
	if((pStreamInfo->nPrimaryCnt + pDATA->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nPrimaryCnt, pChInfo, pSvcComponent);
		pDATA->nSetCnt += pStreamInfo->nPrimaryCnt;
	}

	pSvcComponent = pStreamInfo->astSecondary;
	if((pStreamInfo->nSecondaryCnt + pDATA->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nSecondaryCnt, pChInfo, pSvcComponent);
		pDATA->nSetCnt += pStreamInfo->nSecondaryCnt;
	}

	/************************************************************************/
	/* FIDC channel list update                                             */
	/************************************************************************/
	pStreamInfo		= &pList->stFIDC;
	pChInfo			= &pFIDC->astSubChInfo[pFIDC->nSetCnt];
	pSvcComponent	= pStreamInfo->astPrimary;
	if((pStreamInfo->nPrimaryCnt + pFIDC->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nPrimaryCnt, pChInfo, pSvcComponent);
		pFIDC->nSetCnt += pStreamInfo->nPrimaryCnt;
	}

	pSvcComponent = pStreamInfo->astSecondary;
	if((pStreamInfo->nSecondaryCnt + pFIDC->nSetCnt) < MAX_SUBCH_SIZE ){
		INC_DB_COPY(ulFreq, pStreamInfo->nSecondaryCnt, pChInfo, pSvcComponent);
		pFIDC->nSetCnt += pStreamInfo->nSecondaryCnt;
	}

	return INC_SUCCESS;
}

void INC_UPDATE_LIST(INC_CHANNEL_INFO* pUpDateCh, ST_FICDB_SERVICE_COMPONENT* pSvcComponent)
{
	pUpDateCh->uiStarAddr			= pSvcComponent->unStartAddr;
	pUpDateCh->uiBitRate			= pSvcComponent->uiBitRate;
	pUpDateCh->ucSlFlag				= pSvcComponent->ucShortLong;
	pUpDateCh->ucTableIndex			= pSvcComponent->ucTableIndex;
	pUpDateCh->ucOption				= pSvcComponent->ucOption;
	pUpDateCh->ucProtectionLevel	= pSvcComponent->ucProtectionLevel;
	pUpDateCh->uiDifferentRate		= pSvcComponent->uiDifferentRate;
	pUpDateCh->uiSchSize			= pSvcComponent->uiSubChSize;
}

INC_UINT8 INC_FIC_UPDATE(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pChInfo, ST_SIMPLE_FIC bSimpleFIC)
{
	INC_INT16	nLoop = 0, nIndex, nGetChCnt = 0;
	INC_CHANNEL_INFO* pUpDateCh;
	ST_STREAM_INFO*		pStreamInfo;
	ST_FICDB_SERVICE_COMPONENT* pSvcComponent;

	for(nIndex = 0; nIndex < pChInfo->nSetCnt; nIndex++){
		if(bSimpleFIC == SIMPLE_FIC_ENABLE)
		{
			pUpDateCh = &pChInfo->astSubChInfo[nIndex];
			pStreamInfo = &INC_GET_FICDB_LIST()->stDMB;
			pSvcComponent = pStreamInfo->astPrimary;

			for(nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt; nLoop++, pSvcComponent++)
			{
				if(pUpDateCh->ucSubChID != pSvcComponent->ucSubChid) 	continue;
				INC_UPDATE_LIST(pUpDateCh, pSvcComponent);
				nGetChCnt++;
				if(pChInfo->nSetCnt == nGetChCnt) 
					return INC_SUCCESS;
			}
		}
	}

	for(nIndex = 0; nIndex < pChInfo->nSetCnt; nIndex++)
	{
		pUpDateCh = &pChInfo->astSubChInfo[nIndex];
		
		if(pUpDateCh->ucServiceType == 0x18)
		{
			pStreamInfo = &INC_GET_FICDB_LIST()->stDMB;
			pSvcComponent = pStreamInfo->astPrimary;
			for(nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt; nLoop++, pSvcComponent++)
			{
				if(pUpDateCh->ucSubChID != pSvcComponent->ucSubChid) 	continue;
				INC_UPDATE_LIST(pUpDateCh, pSvcComponent);
				nGetChCnt++;
				if(pChInfo->nSetCnt == nGetChCnt) return INC_SUCCESS;
			}

			pSvcComponent = pStreamInfo->astSecondary;
			for(nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt; nLoop++, pSvcComponent++)
			{
				if(pUpDateCh->ucSubChID != pSvcComponent->ucSubChid) 	continue;
				INC_UPDATE_LIST(pUpDateCh, pSvcComponent);
				nGetChCnt++;
				if(pChInfo->nSetCnt == nGetChCnt) return INC_SUCCESS;
			}
		}
		else
		{
			pStreamInfo = &INC_GET_FICDB_LIST()->stDAB;
			pSvcComponent = pStreamInfo->astPrimary;
			for(nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt; nLoop++, pSvcComponent++)
			{
				if(pUpDateCh->ucSubChID != pSvcComponent->ucSubChid) 	continue;
				INC_UPDATE_LIST(pUpDateCh, pSvcComponent);
				nGetChCnt++;
				if(pChInfo->nSetCnt == nGetChCnt) return INC_SUCCESS;
			}

			pSvcComponent = pStreamInfo->astSecondary;
			for(nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt; nLoop++, pSvcComponent++)
			{
				if(pUpDateCh->ucSubChID != pSvcComponent->ucSubChid) 	continue;
				INC_UPDATE_LIST(pUpDateCh, pSvcComponent);
				nGetChCnt++;
				if(pChInfo->nSetCnt == nGetChCnt) return INC_SUCCESS;
			}

			pStreamInfo = &INC_GET_FICDB_LIST()->stDATA;
			pSvcComponent = pStreamInfo->astPrimary;
			for(nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt; nLoop++, pSvcComponent++)
			{
				if(pUpDateCh->ucSubChID != pSvcComponent->ucSubChid) 	continue;
				INC_UPDATE_LIST(pUpDateCh, pSvcComponent);
				nGetChCnt++;
				if(pChInfo->nSetCnt == nGetChCnt) return INC_SUCCESS;
			}

			pSvcComponent = pStreamInfo->astSecondary;
			for(nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt; nLoop++, pSvcComponent++)
			{
				if(pUpDateCh->ucSubChID != pSvcComponent->ucSubChid) 	continue;
				INC_UPDATE_LIST(pUpDateCh, pSvcComponent);
				nGetChCnt++;
				if(pChInfo->nSetCnt == nGetChCnt) return INC_SUCCESS;
			}
		}
	}

	return INC_ERROR;
}


/* Samsung */
INC_UINT16 INC_GET_SAMSUNG_BER(INC_UINT8 ucI2CID)
{
	INC_UINT32	uiVtbErr, uiVtbData;
	INC_INT16	nLoop, nCER;
	ST_BBPINFO* pInfo;

	pInfo = INC_GET_STRINFO(ucI2CID);

	uiVtbErr = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);	
	uiVtbData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

	if(pInfo->ucProtectionLevel != 0) nCER = (INC_INT16)((((INC_UINT32)uiVtbErr*89)/1000)-30);
	else nCER = (!uiVtbData) ? 0 : (INC_INT32)((((INC_UINT32)uiVtbErr*3394)/(uiVtbData*32))-35);
	if(uiVtbData == 0) nCER = 2000;
	else{
		if(nCER <= 0) nCER = 0;
	}
 
	pInfo->auiBERBuff[pInfo->uiInCBERTick++ % BER_BUFFER_MAX] = nCER;
	pInfo->uiBerSum = 0;
	for(nLoop = 0 ; nLoop < BER_BUFFER_MAX; nLoop++)
		pInfo->uiBerSum += pInfo->auiBERBuff[nLoop];

	pInfo->uiBerSum /= BER_BUFFER_MAX;
	return pInfo->uiBerSum;
}


INC_UINT8 INC_GET_SAMSUNG_ANT_LEVEL(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	INC_UINT16 unCER, unLoop, unRefAntLevel = 0;
	INC_UINT16 aunAntTable[7][2] = {
        {6,    400},
        {5,    550},
        {4,    700},
        {3,    850},
        {2,    900},
        {1,    1050},
        {0,    10000},
	};
	pInfo = INC_GET_STRINFO(ucI2CID);
	INC_GET_CER(ucI2CID);
	unCER = pInfo->uiInCERAvg;
	for(unLoop = 0; unLoop < 6; unLoop++)
	{
		if(unCER <= aunAntTable[unLoop][1]) {
		   unRefAntLevel = aunAntTable[unLoop][0]; 
		   break ;
		}
	}
	
	if(pInfo->ucAntLevel == unRefAntLevel) pInfo->ucAntLevel = unRefAntLevel;
	else if(pInfo->ucAntLevel >= unRefAntLevel)
	{
		if((pInfo->ucAntLevel - unRefAntLevel) >= 3) pInfo->ucAntLevel -= 2;
		else pInfo->ucAntLevel--;
	}
	else {
		if((unRefAntLevel - pInfo->ucAntLevel) >= 3) pInfo->ucAntLevel += 2;
		else pInfo->ucAntLevel++;
	}

    printk("[T3900]ANT LEVEL %d BER AVG[%d]", pInfo->ucAntLevel, pInfo->uiInCERAvg);
	return pInfo->ucAntLevel;
}

INC_UINT16 INC_GET_SAMSUNG_BER_FOR_FACTORY_MODE(INC_UINT8 ucI2CID)
{
	INC_UINT32	uiVtbErr, uiVtbData;
	//INC_INT16	nLoop;
	INC_INT16	nCER;
	INC_INT16	nBERValue;
	ST_BBPINFO* pInfo;

	pInfo = INC_GET_STRINFO(ucI2CID);

	uiVtbErr = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);	
	uiVtbData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

	if(pInfo->ucProtectionLevel != 0){
		//		nBERValue = (INC_INT16)(((INC_DOUBLE32)uiVtbErr*0.089)-30.0);
		nBERValue = (INC_INT16)((((INC_UINT32)uiVtbErr*89)/1000)-30);
		if(uiVtbData == 0) 
			nBERValue = 2000;
		if(nBERValue <= 0) 
			nBERValue = 0;
	}
	else {
		//		nCER = (!uiVtbData) ? 0 : (INC_INT32)((((INC_DOUBLE32)uiVtbErr*0.3394*10000.0)/(uiVtbData*32))-35);
		nCER = (!uiVtbData) ? 0 : (INC_INT32)((((INC_UINT32)uiVtbErr*3394)/(uiVtbData*32))-35);
		if(uiVtbData == 0) nCER = 2000;
		else{
			if(nCER <= 0) nCER = 0;
		}
		nBERValue = nCER;
	}
	return nBERValue;
}


