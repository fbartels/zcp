/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TNEF_H
#define TNEF_H

#include <list>

#include <mapidefs.h>

// We loosely follow the MS class ITNEF with the following main exceptions:
//
// - No special RTF handling
// - No recipient handling
// - No problem reporting
//

#pragma pack(push,1)
struct AttachRendData {
    unsigned short usType;
    unsigned int ulPosition;
    unsigned short usWidth;
    unsigned short usHeight;
    unsigned int ulFlags;
};
#pragma pack(pop)

#define AttachTypeFile 0x0001
#define AttachTypeOle 0x0002

class ECTNEF {
public:
	ECTNEF(ULONG ulFlags, IMessage *lpMessage, IStream *lpStream);
	virtual ~ECTNEF();
    
	// Add properties to the TNEF stream from the message
	virtual HRESULT AddProps(ULONG ulFlags, LPSPropTagArray lpPropList);
    
	// Extract properties from the TNEF stream to the message
	virtual HRESULT ExtractProps(ULONG ulFlags, LPSPropTagArray lpPropList);
    
	// Set some extra properties (warning!, make sure that this pointer stays in memory until Finish() is called!)
	virtual HRESULT SetProps(ULONG cValues, LPSPropValue lpProps);

	// Add other components (currently only attachments supported)
	virtual HRESULT FinishComponent(ULONG ulFlags, ULONG ulComponentID, LPSPropTagArray lpPropList);
    
	// Finish up and write the data (write stream with TNEF_ENCODE, write to message with TNEF_DECODE)
	virtual HRESULT Finish();
    
private:
	HRESULT HrReadDWord(IStream *lpStream, ULONG *ulData);
	HRESULT HrReadWord(IStream *lpStream, unsigned short *ulData);
	HRESULT HrReadByte(IStream *lpStream, unsigned char *ulData);
	HRESULT HrReadData(IStream *lpStream, char *lpData, ULONG ulLen);
    
	HRESULT HrWriteDWord(IStream *lpStream, ULONG ulData);
	HRESULT HrWriteWord(IStream *lpStream, unsigned short ulData);
	HRESULT HrWriteByte(IStream *lpStream, unsigned char ulData);
	HRESULT HrWriteData(IStream *lpStream, char *lpDAta, ULONG ulLen);
    
	HRESULT HrWritePropStream(IStream *lpStream, std::list<SPropValue *> &proplist);
	HRESULT HrWriteSingleProp(IStream *lpStream, LPSPropValue lpProp);
	HRESULT HrReadPropStream(char *lpBuffer, ULONG ulSize, std::list<SPropValue *> &proplist);
	HRESULT HrReadSingleProp(char *lpBuffer, ULONG ulSize, ULONG *lpulRead, LPSPropValue *lppProp);

	HRESULT HrGetChecksum(IStream *lpStream, ULONG *lpulChecksum);
	ULONG GetChecksum(char *lpData, unsigned int ulLen);
	
	HRESULT HrWriteBlock(IStream *lpDest, IStream *lpSrc, ULONG ulBlockID, ULONG ulLevel);
	HRESULT HrWriteBlock(IStream *lpDest, char *lpData, unsigned int ulSize, ULONG ulBlockID, ULONG ulLevel);

    HRESULT HrReadStream(IStream *lpStream, void *lpBase, BYTE **lppData, ULONG *lpulSize);
	
	IStream *m_lpStream;
	IMessage *m_lpMessage;
	ULONG ulFlags;
    
	// Accumulator for properties from AddProps and SetProps
	std::list<SPropValue *> lstProps;

	struct tnefattachment {
		std::list<SPropValue *> lstProps;
		ULONG size;
		BYTE *data;
		AttachRendData rdata;
	};
	std::list<tnefattachment*> lstAttachments;

	void FreeAttachmentData(tnefattachment* lpTnefAtt);

};

// Flags for constructor
#define TNEF_ENCODE			0x00000001
#define TNEF_DECODE			0x00000002

// Flags for FinishComponent
#define TNEF_COMPONENT_MESSAGE 		0x00001000
#define TNEF_COMPONENT_ATTACHMENT 	0x00002000

// Flags for ExtractProps
#define TNEF_PROP_EXCLUDE	0x00000003
#define TNEF_PROP_INCLUDE	0x00000004

// Flags for AddProps
// #define TNEF_PROP_EXCLUDE 		0x00000001
// #define TNEF_PROP_INCLUDE 		0x00000002

#define TNEF_SIGNATURE		0x223e9f78


#endif
