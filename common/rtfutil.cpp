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

// From http://www.wischik.com/lu/programmer/mapi_utils.html
// Parts rewritten by Zarafa

#include <zarafa/platform.h>
#include <iostream>
#include <zarafa/codepage.h>
#include <zarafa/CommonUtil.h>
#include <zarafa/Util.h>
#include <zarafa/charset/convert.h>
#include <zarafa/stringutil.h>
#include "HtmlEntity.h"

#include "rtfutil.h"

#include <string> 
#include <sstream>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

static const char szHex[] = "0123456789ABCDEF";

// Charsets used in \fcharsetXXX (from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnrtfspec/html/rtfspec_6.asp )
// charset "" is the ANSI codepage specified in \ansicpg
// charset NULL means 'no conversion', ie direct 1-to-1 translation to UNICODE 
static const struct _rtfcharset {
	int id;
	const char *charset;
} RTFCHARSET[] = {
	{0, ""}, // This is actually the codepage specified in \ansicpg
	{1, ""}, // default charset, probably also the codepage in \ansicpg
	{2, ""}, // This is SYMBOL, but in practice, we can just send the data
	// one-on-one down the line, as the actual character codes
	// don't change when converted to unicode (because the other
	// side will also be using the MS Symbol font to display)
	{3, NULL}, // 'Invalid'
	{77, "MAC"}, // Unsure if this is the correct charset
	{128,"SJIS"}, // OR cp 932 ?
	{129,"euc-kr"}, // 'Hangul' korean
	{130,"JOHAB"},
	{134,"GB2312"},
	{136,"BIG5"},
	{161,"windows-1253"},
	{162,"windows-1254"}, // 'Turkish'
	{163,"windows-1258"}, // Vietnamese
	{177,"windows-1255"}, // Hebrew
	{178,"windows-1256"}, // Arabic
	{179,"windows-1256"}, // Arabic traditional
	{180,"windows-1256"}, // Arabic user
	{181,"windows-1255"}, // Hebrew user
	{186,"windows-1257"},
	{204,"windows-1251"}, // Cyrillic for russian
	{222,NULL},		// Thai ?
	{238,"windows-1250"}, // Eastern European
	{254,"IBM437"},
	{255,NULL} 		// OEM
};

struct RTFSTATE {
	int ulFont;
	const char *szCharset;
	bool bInFontTbl;
	bool bInColorTbl;
	bool bInSkipTbl;
	std::string output;			// text in current szCharset
	bool bRTFOnly;
	int ulUnicodeSkip;			// number of characters to skip after a unicode character
	int ulSkipChars;
};

#define RTF_MAXSTATE 	256
#define RTF_MAXCMD		64
typedef map<int,int> fontmap_t;

/**
 * Converts RTF \ansicpgN <N> number to normal charset string.
 *
 * @param[in]	id	RTF codepage number
 * @param[out]	lpszCharset static charset string
 * @retval		MAPI_E_NOT_FOUND if id was unknown
 */
static HRESULT HrGetCharsetByRTFID(int id, const char **lpszCharset)
{
	for (size_t i = 0; i < ARRAY_SIZE(RTFCHARSET); ++i) {
		if(RTFCHARSET[i].id == id) {
			*lpszCharset = RTFCHARSET[i].charset;
			return hrSuccess;
		}
	}
	return MAPI_E_NOT_FOUND;
}

/** RTF ignore Commando's
 *
 * @param[in]	lpCommand	RTF command string, without leading \
 * @return	bool
 */
static bool isRTFIgnoreCommand(const char *lpCommand)
{
	if(lpCommand == NULL)
		return false;

	if (strcmp(lpCommand,"stylesheet") == 0 ||
			strcmp(lpCommand,"revtbl") == 0 || 
			strcmp(lpCommand,"xmlnstbl") == 0 ||
			strcmp(lpCommand,"rsidtbl") == 0 ||
			strcmp(lpCommand,"fldinst") == 0 ||
			strcmp(lpCommand,"shpinst") == 0 ||
			strcmp(lpCommand,"wgrffmtfilter") == 0 ||
			strcmp(lpCommand,"pnseclvl") == 0 ||
			strcmp(lpCommand,"atrfstart") == 0 ||
			strcmp(lpCommand,"atrfend") == 0 ||
			strcmp(lpCommand,"atnauthor") == 0 ||
			strcmp(lpCommand,"annotation") == 0 ||
			strcmp(lpCommand,"sp") == 0 ||
			strcmp(lpCommand,"atnid") == 0 ||
			strcmp(lpCommand,"xmlopen") == 0
			//strcmp(lpCommand,"fldrslt") == 0
	   )
		return true;
	return false;
}

/**
 * Initializes an RTFState struct to default values.
 *
 * @param[in/out]	sState	pointer to RTFState struct to init
 */
static void InitRTFState(RTFSTATE *sState)
{
	sState->bInSkipTbl = false;
	sState->bInFontTbl = false;
	sState->bInColorTbl = false;
	sState->szCharset = "us-ascii";
	sState->bRTFOnly = false;
	sState->ulFont = 0;
	sState->ulUnicodeSkip = 1;
	sState->ulSkipChars = 0;
}

static std::wstring RTFFlushStateOutput(convert_context &convertContext,
    RTFSTATE *sState, ULONG ulState)
{
	std::wstring wstrUnicode;

	if (!sState[ulState].output.empty()) {
		TryConvert(convertContext, sState[ulState].output, rawsize(sState[ulState].output), sState[ulState].szCharset, wstrUnicode);
		sState[ulState].output.clear();
	}
	return wstrUnicode;
}

/**
 * Converts RTF text into HTML text. It will return an HTML string in
 * the given codepage.
 *
 * To convert between the RTF text and HTML codepage text, we use a
 * WCHAR string as intermediate.
 *
 * @param[in]	lpStrRTFIn		RTF input string that contains \fromtext
 * @param[out]	lpStrHTMLOut	HTML output in requested ulCodepage
 * @param[out]	ulCodepage		codepage for HTML output
 */
HRESULT HrExtractHTMLFromRTF(const std::string &rtf_unfilt,
    std::string &lpStrHTMLOut, ULONG ulCodepage)
{
	HRESULT hr;
	std::string lpStrRTFIn = string_strip_nuls(rtf_unfilt);
	const char *szInput = lpStrRTFIn.c_str();
	const char *szANSICharset = "us-ascii";
	const char *szHTMLCharset;
	std::string strConvertCharset;
	std::wstring strOutput;
	int ulState = 0;
	RTFSTATE sState[RTF_MAXSTATE];	
	fontmap_t mapFontToCharset;
	convert_context convertContext;

	// Find \\htmltag, if there is none we can't extract HTML
	if (strstr(szInput, "{\\*\\htmltag") == NULL)
		return MAPI_E_NOT_FOUND;

	// select output charset
	hr = HrGetCharsetByCP(ulCodepage, &szHTMLCharset);
	if (hr != hrSuccess) {
		szHTMLCharset = "us-ascii";
		hr = hrSuccess;
	}
	strConvertCharset = szHTMLCharset + string("//HTMLENTITIES");

	InitRTFState(&sState[0]);

	while(*szInput) {
		if(strncmp(szInput,"\\*",2) == 0) {
			szInput+=2;
		} else if(*szInput == '\\') {
			// Command
			char szCommand[RTF_MAXCMD];
			char *szCmdOutput;
			int lArg = -1;
			bool bNeg = false;

			++szInput;
			if(isalpha(*szInput)) {
				szCmdOutput = szCommand;

				while (isalpha(*szInput) && szCmdOutput < szCommand + RTF_MAXCMD - 1)
					*szCmdOutput++ = *szInput++;
				*szCmdOutput = 0;

				if(*szInput == '-') {
					bNeg = true;
					++szInput;
				}

				if(isdigit(*szInput)) {
					lArg = 0;
					while (isdigit(*szInput)) {
						lArg = lArg * 10 + *szInput - '0';
						++szInput;
					}
					if(bNeg) lArg = -lArg;
				}
				if(*szInput == ' ')
					++szInput;

				// szCommand is the command, lArg is the argument.
				if(strcmp(szCommand,"fonttbl") == 0) {
					sState[ulState].bInFontTbl = true;
				} else if(strcmp(szCommand,"colortbl") == 0) {
					sState[ulState].bInColorTbl = true;
				} else if(strcmp(szCommand,"pntext") == 0) { // pntext is the plaintext alternative, ignore it.
					sState[ulState].bRTFOnly = true;
				} else if(strcmp(szCommand,"ansicpg") == 0) {
					if(HrGetCharsetByCP(lArg, &szANSICharset) != hrSuccess)
						szANSICharset = "us-ascii";
					sState[ulState].szCharset = szANSICharset;
				} else if(strcmp(szCommand,"fcharset") == 0) {
					if(sState[ulState].bInFontTbl) {
						mapFontToCharset.insert(pair<int, int>(sState[ulState].ulFont, lArg));
					}
				} else if(strcmp(szCommand,"htmltag") == 0) {
				} else if(strcmp(szCommand,"mhtmltag") == 0) {
				} else if(strcmp(szCommand,"pard") == 0 || strcmp(szCommand,"par") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {		
						sState[ulState].output.append(1,'\r');
						sState[ulState].output.append(1,'\n');
					}
				} else if(strcmp(szCommand,"tab") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {		
						sState[ulState].output.append(1,' ');
						sState[ulState].output.append(1,' ');
						sState[ulState].output.append(1,' ');
					}
				} else if (strcmp(szCommand,"uc") == 0) {
					sState[ulState].ulUnicodeSkip = lArg;
				} else if(strcmp(szCommand,"f") == 0) {
					sState[ulState].ulFont = lArg;

					if(!sState[ulState].bInFontTbl) {
						fontmap_t::iterator i = mapFontToCharset.find(lArg);
						if (i == mapFontToCharset.end())
							continue;

						// Output any data before this point
						strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

						// Set new charset			
						HrGetCharsetByRTFID(i->second, &sState[ulState].szCharset);
						if(sState[ulState].szCharset == NULL) {
							sState[ulState].szCharset = "us-ascii";
						} else if(sState[ulState].szCharset[0] == 0) {
							sState[ulState].szCharset = szANSICharset;
						}

					} 
					// ignore error
				}
				else if (strcmp(szCommand,"u") == 0) {
					// unicode character, in signed short WCHAR
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					if (!sState[ulState].bRTFOnly)
						strOutput.append(1, (unsigned short)lArg); // add as literal character
					sState[ulState].ulSkipChars += sState[ulState].ulUnicodeSkip;
				}
				else if(strcmp(szCommand,"htmlrtf") == 0) {
					if(lArg != 0) {
						// \\htmlrtf
						sState[ulState].bRTFOnly = true;
					} else {
						// \\htmlrtf0
						sState[ulState].bRTFOnly = false;
					}
				}else if(isRTFIgnoreCommand(szCommand)) {
					sState[ulState].bInSkipTbl = true;
				}

			}
			// Non-alnum after '\'
			else if(*szInput == '\\') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'\\');
			}
			else if(*szInput == '{') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'{');
			}
			else if(*szInput == '}') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'}');
			} 
			else if(*szInput == '\'') {
				unsigned int ulChar;

				while(*szInput == '\'')
				{
					ulChar = 0;
					++szInput;

					if(*szInput) {
						ulChar = (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
						ulChar = ulChar << 4;
						++szInput;
					}

					if(*szInput) {
						ulChar += (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
						++szInput;
					}

					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl && !sState[ulState].ulSkipChars) {
						sState[ulState].output.append(1,ulChar);
					} else if (sState[ulState].ulSkipChars)
						--sState[ulState].ulSkipChars;

					if(*szInput == '\\' && *(szInput+1) == '\'')
						++szInput;
					else
						break;
				}				

			} else {
				++szInput; // skip single character after '\'
			}
		} // Non-command
		else if(*szInput == '{') {
			// Dump output data
			strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

			++ulState;
			if (ulState >= RTF_MAXSTATE)
				return MAPI_E_NOT_ENOUGH_MEMORY;
			sState[ulState] = sState[ulState-1];
			sState[ulState].output.clear();
			++szInput;
		} else if(*szInput == '}') {
			// Dump output data
			strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

			if(ulState > 0)
				--ulState;
			++szInput;
		} else if(*szInput == '\r' || *szInput == '\n') {
			++szInput;
		} else {
			if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl && !sState[ulState].ulSkipChars) {
				sState[ulState].output.append(1,*szInput);
			} else if (sState[ulState].ulSkipChars)
				--sState[ulState].ulSkipChars;
			++szInput;
		}
	}

	strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

	try {
		lpStrHTMLOut = convertContext.convert_to<string>(strConvertCharset.c_str(), strOutput, rawsize(strOutput), CHARSET_WCHAR);
	} catch (const convert_exception &ce) {
		hr = details::HrFromException(ce);
	}
	return hr;	
}

/**
 * Extracts the Plain text that was encapsulated in an RTF text, and
 * writes out HTML. It will return an HTML string in the given
 * codepage.
 *
 * To convert between the RTF text and HTML codepage text, we use a
 * WCHAR string as intermediate.
 *
 * @param[in]	lpStrRTFIn		RTF input string that contains \fromtext
 * @param[out]	lpStrHTMLOut	HTML output in requested ulCodepage
 * @param[out]	ulCodepage		codepage for HTML output
 */
HRESULT HrExtractHTMLFromTextRTF(const std::string &rtf_unfilt,
    std::string &lpStrHTMLOut, ULONG ulCodepage)
{
	HRESULT hr;
	std::string lpStrRTFIn = string_strip_nuls(rtf_unfilt);
	std::wstring wstrUnicodeTmp;
	const char *szInput = lpStrRTFIn.c_str();
	const char *szANSICharset = "us-ascii";
	const char *szHTMLCharset;
	std::string strConvertCharset;
	std::wstring strOutput;
	int ulState = 0;
	bool bPar = false;
	bool bNewLine = false;
	int nLineChar=0;
	RTFSTATE sState[RTF_MAXSTATE];	
	fontmap_t mapFontToCharset;
	convert_context convertContext;
	string tmp;

	// select output charset
	hr = HrGetCharsetByCP(ulCodepage, &szHTMLCharset);
	if (hr != hrSuccess) {
		szHTMLCharset = "us-ascii";
		hr = hrSuccess;
	}
	strConvertCharset = szHTMLCharset + string("//HTMLENTITIES");

	tmp =	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\r\n" \
		 "<HTML>\r\n" \
		 "<HEAD>\r\n" \
		 "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=";
	tmp += szHTMLCharset;
	tmp += 	"\">\r\n"												\
		 "<META NAME=\"Generator\" CONTENT=\"Zarafa text/HTML builder 1.0\">\r\n" \
		 "<TITLE></TITLE>\r\n" \
		 "</HEAD>\r\n" \
		 "<BODY>\r\n" \
		 "<!-- Converted from text/plain format -->\r\n" \
		 "\r\n"; //FIXME create title on the fly ?

	wstrUnicodeTmp.resize(0,0);
	TryConvert(convertContext, tmp, rawsize(tmp), "us-ascii", wstrUnicodeTmp);
	strOutput.append(wstrUnicodeTmp);

	InitRTFState(&sState[0]);

	while(*szInput) {
		if(strncmp(szInput,"\\*",2) == 0) {
			szInput+=2;
		} else if(*szInput == '\\') {
			// Command
			char szCommand[RTF_MAXCMD];
			char *szCmdOutput;
			int lArg = -1;
			bool bNeg = false;

			++szInput;
			if(isalpha(*szInput)) {
				szCmdOutput = szCommand;

				while (isalpha(*szInput) && szCmdOutput < szCommand + RTF_MAXCMD - 1)
					*szCmdOutput++ = *szInput++;
				*szCmdOutput = 0;

				if(*szInput == '-') {
					bNeg = true;
					++szInput;
				}

				if(isdigit(*szInput)) {
					lArg = 0;
					while (isdigit(*szInput)) {
						lArg = lArg * 10 + *szInput - '0';
						++szInput;
					}
					if(bNeg) lArg = -lArg;
				}
				if(*szInput == ' ')
					++szInput;

				// szCommand is the command, lArg is the argument.
				if(strcmp(szCommand,"fonttbl") == 0) {
					sState[ulState].bInFontTbl = true;
				} else if(strcmp(szCommand,"colortbl") == 0) {
					sState[ulState].bInColorTbl = true;
				} else if(strcmp(szCommand,"pntext") == 0) { // pntext is the plaintext alternative, ignore it.
					sState[ulState].bRTFOnly = true;
				} else if(strcmp(szCommand,"ansicpg") == 0) {
					if(HrGetCharsetByCP(lArg, &szANSICharset) != hrSuccess)
						szANSICharset = "us-ascii";
					sState[ulState].szCharset = szANSICharset;
				} else if(strcmp(szCommand,"fcharset") == 0) {
					if(sState[ulState].bInFontTbl) {
						mapFontToCharset.insert(pair<int, int>(sState[ulState].ulFont, lArg));
					}
				} else if(strcmp(szCommand,"htmltag") == 0) {
				} else if(strcmp(szCommand,"mhtmltag") == 0) {
				} else if(strcmp(szCommand,"pard") == 0 || strcmp(szCommand,"par") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {

						if(bNewLine == true && nLineChar >0 && bPar == true){
							sState[ulState].output.append("</P>\r\n\r\n");
							bPar = false;
							nLineChar = 0;
						}else if(bNewLine == false && nLineChar >0)
							sState[ulState].output.append("</FONT>\r\n");
						else
							sState[ulState].output.append("\r\n");
					}
				} else if(strcmp(szCommand,"tab") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {		
						sState[ulState].output.append(1,' ');
						sState[ulState].output.append(1,' ');
						sState[ulState].output.append(1,' ');
					}
				} else if (strcmp(szCommand,"uc") == 0) {
					sState[ulState].ulUnicodeSkip = lArg;
				} else if(strcmp(szCommand,"f") == 0) {
					sState[ulState].ulFont = lArg;

					if(!sState[ulState].bInFontTbl) {
						fontmap_t::iterator i = mapFontToCharset.find(lArg);
						if (i == mapFontToCharset.end())
							continue;

						// Output any data before this point
						strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

						// Set new charset			
						HrGetCharsetByRTFID(i->second, &sState[ulState].szCharset);
						if(sState[ulState].szCharset == NULL) {
							sState[ulState].szCharset = "us-ascii";
						} else if(sState[ulState].szCharset[0] == 0) {
							sState[ulState].szCharset = szANSICharset;
						}

					} 
					// ignore error
				}
				else if (strcmp(szCommand,"u") == 0) {
					// unicode character, in signed short WCHAR
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					if (!sState[ulState].bRTFOnly)
						strOutput.append(1, (unsigned short)lArg); // add as literal character
					sState[ulState].ulSkipChars += sState[ulState].ulUnicodeSkip;
				}
				else if(strcmp(szCommand,"htmlrtf") == 0) {
					if(lArg != 0) {
						// \\htmlrtf
						sState[ulState].bRTFOnly = true;
					} else {
						// \\htmlrtf0
						sState[ulState].bRTFOnly = false;
					}
				} else if(isRTFIgnoreCommand(szCommand)) {
					sState[ulState].bInSkipTbl = true;
				}
			}
			// Non-alnum after '\'
			else if(*szInput == '\\') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'\\');
			}
			else if(*szInput == '{') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'{');
			}
			else if(*szInput == '}') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'}');
			} 
			else if(*szInput == '\'') {
				unsigned int ulChar;

				// Dump output data until now, if we're switching charsets
				if(szANSICharset == NULL || strcmp(sState[ulState].szCharset, szANSICharset) != 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
				}

				while(*szInput == '\'')
				{
					ulChar = 0;
					++szInput;

					if(*szInput) {
						ulChar = (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
						ulChar = ulChar << 4;
						++szInput;
					}

					if(*szInput) {
						ulChar += (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
						++szInput;
					}

					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].ulSkipChars) {
						sState[ulState].output.append(1,ulChar);
					} else if (sState[ulState].ulSkipChars)
						--sState[ulState].ulSkipChars;

					if(*szInput == '\\' && *(szInput+1) == '\'')
						++szInput;
					else
						break;
				}

				// Dump escaped data in charset 0 (ansicpg), if we had to switch charsets
				if(szANSICharset == NULL || strcmp(sState[ulState].szCharset, szANSICharset) != 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
				}

			} else {
				++szInput; // skip single character after '\'
			}
		} // Non-command
		else if(*szInput == '{') {
			// Dump output data

			if (!sState[ulState].output.empty()) {
				strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
			}

			++ulState;
			if (ulState >= RTF_MAXSTATE)
				return MAPI_E_NOT_ENOUGH_MEMORY;
			sState[ulState] = sState[ulState-1];

			++szInput;
		} else if(*szInput == '}') {
			// Dump output data
			strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

			if(ulState > 0)
				--ulState;
			++szInput;
		} else if(*szInput == '\r' || *szInput == '\n') {
			bNewLine = true;
			++szInput;
		} else {
			if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl && !sState[ulState].ulSkipChars) {
				if(bPar == false){
					sState[ulState].output.append("<P>");
					bPar = true;
				}else if(bNewLine == true && bPar == true)
					sState[ulState].output.append("\r\n<BR>");

				if(bNewLine == true){
					sState[ulState].output.append("<FONT SIZE=2>");
					bNewLine = false;
				}

				// Change space to &nbsp; . The last space is a real space like "&nbsp;&nbsp; " or " "
				if(*szInput == ' ') {
					++szInput;

					while(*szInput == ' ') {
						sState[ulState].output.append("&nbsp;");
						++szInput;
					}

					sState[ulState].output.append(1, ' ');
				} else {
					std::wstring entity;

					if (! CHtmlEntity::CharToHtmlEntity((WCHAR)*szInput, entity))
						sState[ulState].output.append(1, *szInput);
					else
						sState[ulState].output.append(entity.begin(), entity.end());

					++szInput;
				}

				++nLineChar;
			} else {
				if (sState[ulState].ulSkipChars)
					--sState[ulState].ulSkipChars;
				++szInput;
			}
		}
	}

	strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

	strOutput += L"\r\n" \
		     L"</BODY>\r\n" \
		     L"</HTML>\r\n";

	try {
		lpStrHTMLOut = convertContext.convert_to<string>(strConvertCharset.c_str(), strOutput, rawsize(strOutput), CHARSET_WCHAR);
	} catch (const convert_exception &ce) {
		hr = details::HrFromException(ce);
	}
	return hr;	
}

/**
 * Extracts the HTML text that was encapsulated in an RTF text. It
 * will return an HTML string in the given codepage.
 *
 * To convert between the RTF text and HTML codepage text, we use a
 * WCHAR string as intermediate.
 *
 * @param[in]	lpStrRTFIn		RTF input string that contains \fromhtml
 * @param[out]	lpStrHTMLOut	HTML output in requested ulCodepage
 * @param[out]	ulCodepage		codepage for HTML output
 *
 * @todo Export the right HTML tags, now only plain stuff
 */
HRESULT HrExtractHTMLFromRealRTF(const std::string &rtf_unfilt,
    std::string &lpStrHTMLOut, ULONG ulCodepage)
{
	HRESULT hr;
	std::string lpStrRTFIn = string_strip_nuls(rtf_unfilt);
	std::wstring wstrUnicodeTmp;
	const char *szInput = lpStrRTFIn.c_str();
	const char *szANSICharset = "us-ascii";
	const char *szHTMLCharset;
	std::string strConvertCharset;
	std::wstring strOutput;
	int ulState = 0;
	RTFSTATE sState[RTF_MAXSTATE];	
	convert_context convertContext;
	string tmp;
	fontmap_t mapFontToCharset;

	// select output charset
	hr = HrGetCharsetByCP(ulCodepage, &szHTMLCharset);
	if (hr != hrSuccess) {
		szHTMLCharset = "us-ascii";
		hr = hrSuccess;
	}
	strConvertCharset = szHTMLCharset + string("//HTMLENTITIES");

	tmp =	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\r\n" \
		 "<HTML>\r\n" \
		 "<HEAD>\r\n" \
		 "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=";
	tmp += szHTMLCharset;
	tmp +=	"\">\r\n"															\
		 "<META NAME=\"Generator\" CONTENT=\"Zarafa rtf/HTML builder 1.0\">\r\n" \
		 "<TITLE></TITLE>\r\n" \
		 "</HEAD>\r\n" \
		 "<BODY>\r\n" \
		 "<!-- Converted from text/rtf format -->\r\n" \
		 "\r\n"; //FIXME create title on the fly ?

	TryConvert(convertContext, tmp, rawsize(tmp), "us-ascii", wstrUnicodeTmp);
	strOutput.append(wstrUnicodeTmp);

	InitRTFState(&sState[0]);

	while(*szInput) {
		if(strncmp(szInput,"\\*",2) == 0) {
			szInput+=2;
		} else if(*szInput == '\\') {
			// Command
			char szCommand[RTF_MAXCMD];
			char *szCmdOutput;
			int lArg = -1;
			bool bNeg = false;

			++szInput;
			if(isalpha(*szInput)) {
				szCmdOutput = szCommand;

				while (isalpha(*szInput) && szCmdOutput < szCommand + RTF_MAXCMD - 1)
					*szCmdOutput++ = *szInput++;
				*szCmdOutput = 0;

				if(*szInput == '-') {
					bNeg = true;
					++szInput;
				}

				if(isdigit(*szInput)) {
					lArg = 0;
					while (isdigit(*szInput)) {
						lArg = lArg * 10 + *szInput - '0';
						++szInput;
					}
					if(bNeg) lArg = -lArg;
				}
				if(*szInput == ' ')
					++szInput;

				// szCommand is the command, lArg is the argument.
				if(strcmp(szCommand,"fonttbl") == 0) {
					sState[ulState].bInFontTbl = true;
				} else if(strcmp(szCommand,"colortbl") == 0) {
					sState[ulState].bInColorTbl = true;
				} else if(strcmp(szCommand,"listtable") == 0) {
					sState[ulState].bInSkipTbl = true;
				} else if(strcmp(szCommand,"pntext") == 0) { // pntext is the plaintext alternative, ignore it.
					sState[ulState].bRTFOnly = true;
				} else if(strcmp(szCommand,"ansicpg") == 0) {
					if(HrGetCharsetByCP(lArg, &szANSICharset) != hrSuccess)
						szANSICharset = "us-ascii";
					sState[ulState].szCharset = szANSICharset;
				} else if(strcmp(szCommand,"fcharset") == 0) {
					if(sState[ulState].bInFontTbl) {
						mapFontToCharset.insert(pair<int, int>(sState[ulState].ulFont, lArg));
					}
				} else if(strcmp(szCommand,"htmltag") == 0) {
				} else if(strcmp(szCommand,"latentstyles") == 0) {
					sState[ulState].bRTFOnly = true;
				} else if(strcmp(szCommand,"datastore") == 0) {
					sState[ulState].bRTFOnly = true;
				} else if(strcmp(szCommand,"mhtmltag") == 0) {
				} else if(strcmp(szCommand,"pard") == 0 || strcmp(szCommand,"par") == 0 || strcmp(szCommand,"line") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {		
						sState[ulState].output.append("<br>\r\n");
					}
				} else if(strcmp(szCommand,"tab") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {		
						sState[ulState].output.append(1,' ');
						sState[ulState].output.append(1,' ');
						sState[ulState].output.append(1,' ');
					}
				} else if (strcmp(szCommand,"bin") == 0) {
					if (lArg > 0)
						szInput += lArg; // skip all binary bytes here.
				} else if (strcmp(szCommand,"uc") == 0) {
					sState[ulState].ulUnicodeSkip = lArg;
				} else if(strcmp(szCommand,"f") == 0) {
					sState[ulState].ulFont = lArg;

					if(!sState[ulState].bInFontTbl) {
						fontmap_t::iterator i = mapFontToCharset.find(lArg);
						if (i == mapFontToCharset.end())
							continue;

						// Output any data before this point
						if (!sState[ulState].output.empty()) {
							strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
						}

						// Set new charset
						HrGetCharsetByRTFID(i->second, &sState[ulState].szCharset);
						if(sState[ulState].szCharset == NULL) {
							sState[ulState].szCharset = "us-ascii";
						} else if(sState[ulState].szCharset[0] == 0) {
							sState[ulState].szCharset = szANSICharset;
						}

					} 
					// ignore error
				}
				else if (strcmp(szCommand,"u") == 0) {
					// unicode character, in signed short WCHAR
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {
						std::wstring entity;

						if (! CHtmlEntity::CharToHtmlEntity((WCHAR)*szInput, entity))
							strOutput.append(1, (unsigned short)lArg);	// add as literal character
						else
							strOutput.append(entity.begin(), entity.end());
					}
					sState[ulState].ulSkipChars += sState[ulState].ulUnicodeSkip;
				}
				else if(strcmp(szCommand,"htmlrtf") == 0) {
					if(lArg != 0) {
						// \\htmlrtf
						sState[ulState].bRTFOnly = true;
					} else {
						// \\htmlrtf0
						sState[ulState].bRTFOnly = false;
					}
				}
				/*else if(strcmp(szCommand,"b") == 0) {
				  if( lArg == -1)
				  sState[ulState].output.append("<b>", 3);
				  else
				  sState[ulState].output.append("</b>", 4);
				  }else if(strcmp(szCommand,"i") == 0) {
				  if( lArg == -1)
				  sState[ulState].output.append("<i>", 3);
				  else
				  sState[ulState].output.append("</i>", 4);
				  }else if(strcmp(szCommand,"ul") == 0) {
				  if( lArg == -1)
				  sState[ulState].output.append("<u>", 3);
				  else
				  sState[ulState].output.append("</u>", 4);
				  }else if(strcmp(szCommand,"ulnone") == 0) {
				  sState[ulState].output.append("</u>", 4);
				  }*/
				else if(strcmp(szCommand,"generator") == 0){
					while (*szInput != ';' && *szInput != '}' && *szInput)
						++szInput;

					if(*szInput == ';') 
						++szInput;
				}
				else if(strcmp(szCommand,"bkmkstart") == 0 || strcmp(szCommand,"bkmkend") == 0){
					// skip bookmark name
					while (*szInput && isalnum(*szInput))
						++szInput;

					sState[ulState].bInSkipTbl = true;

				} else if (strcmp(szCommand, "endash") == 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					// windows-1252: 0x96, unicode 0x2013
					strOutput += 0x2013;
				} else if (strcmp(szCommand, "emdash") == 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					// windows-1252: 0x97, unicode 0x2014
					strOutput += 0x2014;
				} else if (strcmp(szCommand, "lquote") == 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					// windows-1252: 0x91, unicode 0x2018
					strOutput += 0x2018;
				} else if (strcmp(szCommand, "rquote") == 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					// windows-1252: 0x92, unicode 0x2019
					strOutput += 0x2019;
				} else if (strcmp(szCommand, "ldblquote") == 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					// windows-1252: 0x93, unicode 0x201C
					strOutput += 0x201C;
				} else if (strcmp(szCommand, "rdblquote") == 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					// windows-1252: 0x94, unicode 0x201D
					strOutput += 0x201D;
				} else if (strcmp(szCommand, "bullet") == 0) {
					strOutput += RTFFlushStateOutput(convertContext, sState, ulState);
					// windows-1252: 0x95, unicode 0x2022
					strOutput += 0x2022;
				} else if(isRTFIgnoreCommand(szCommand)) {
					sState[ulState].bInSkipTbl = true;
				}

			}
			// Non-alnum after '\'
			else if(*szInput == '\\') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'\\');
			}
			else if(*szInput == '{') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'{');
			}
			else if(*szInput == '}') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'}');
			} 
			else if(*szInput == '\'') {
				unsigned int ulChar;
				std::wstring wstrUnicode;

				while(*szInput == '\'')
				{
					ulChar = 0;
					++szInput;

					if(*szInput) {
						ulChar = (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
						ulChar = ulChar << 4;
						++szInput;
					}

					if(*szInput) {
						ulChar += (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
						++szInput;	
					}

					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl && !sState[ulState].ulSkipChars) {
						sState[ulState].output.append(1,ulChar);
					} else if (sState[ulState].ulSkipChars)
						--sState[ulState].ulSkipChars;

					if(*szInput == '\\' && *(szInput+1) == '\'')
						++szInput;
					else
						break;
				}

			} else {
				++szInput; // skip single character after '\'
			}
		} // Non-command
		else if(*szInput == '{') {
			// Dump output data
			strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

			++ulState;
			if (ulState >= RTF_MAXSTATE)
				return MAPI_E_NOT_ENOUGH_MEMORY;
			sState[ulState] = sState[ulState-1];

			++szInput;
		} else if(*szInput == '}') {
			// Dump output data
			strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

			if(ulState > 0)
				--ulState;
			++szInput;
		} else if(*szInput == '\r' || *szInput == '\n') {
			++szInput;
		} else {
			if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl && !sState[ulState].ulSkipChars) {
				// basic html escaping only
				if (*szInput == '&')
					sState[ulState].output.append("&amp;");
				else if (*szInput == '<')
					sState[ulState].output.append("&lt;");
				else if (*szInput == '>')
					sState[ulState].output.append("&gt;");
				else
					sState[ulState].output.append(1,*szInput);
			} else if (sState[ulState].ulSkipChars)
				--sState[ulState].ulSkipChars;
			++szInput;
		}
	}

	strOutput += RTFFlushStateOutput(convertContext, sState, ulState);

	strOutput += L"\r\n" \
		     L"</BODY>\r\n" \
		     L"</HTML>\r\n";

	try {
		lpStrHTMLOut = convertContext.convert_to<string>(strConvertCharset.c_str(), strOutput, rawsize(strOutput), CHARSET_WCHAR);
	} catch (const convert_exception &ce) {
		hr = details::HrFromException(ce);
	}
	return hr;
}

/**
 * Checks if input is HTML "wrapped" in RTF.
 *
 * We look for the words "\fromhtml" somewhere in the file.  If the
 * rtf encodes text rather than html, then instead it will only find
 * "\fromtext".
 *
 * @param[in]	buf	character buffer containing RTF text
 * @param[in]	len	length of input buffer
 * @return true if buf is html wrapped in rtf
 */
bool isrtfhtml(const char *buf, unsigned int len)
{
	for (const char *c = buf; c < buf + len - 9; ++c)
		if (strncmp(c, "\\from", 5) == 0)
			return strncmp(c, "\\fromhtml", 9) == 0;
	return false;
}

/**
 * Checks if input is Text "wrapped" in RTF.
 *
 * We look for the words "\fromtext" somewhere in the file.  If the
 * rtf encodes text rather than text, then instead it will only find
 * "\fromhtml".
 *
 * @param[in]	buf	character buffer containing RTF text
 * @param[in]	len	length of input buffer
 * @return true if buf is html wrapped in rtf
 */
bool isrtftext(const char *buf, unsigned int len)
{
	for (const char *c = buf; c < buf + len - 9; ++c)
		if (strncmp(c, "\\from", 5) == 0)
			return strncmp(c, "\\fromtext", 9) == 0;
	return false;
}

/**
 * Convert RTF, which should have \fromtext marker, to plain text format in WCHAR.
 *
 * @param[in]	lpStrRTFIn	string containing RTF with \fromtext marker
 * @param[out]	strBodyOut	the converted body
 * @return	mapi error code
 * @retval	MAPI_E_NOT_ENOUGH_MEMORY	too many states in rtf, > 256
 */
HRESULT HrExtractBODYFromTextRTF(const std::string &rtf_unfilt,
    std::wstring &strBodyOut)
{
	std::string lpStrRTFIn = string_strip_nuls(rtf_unfilt);
	const char *szInput = lpStrRTFIn.c_str();
	const char *szANSICharset = "us-ascii";
	int ulState = 0;
	RTFSTATE sState[RTF_MAXSTATE];	
	fontmap_t mapFontToCharset;
	convert_context convertContext;
	std::wstring strwAppend;

	strBodyOut.resize(0,0);

	InitRTFState(&sState[0]);

	while(*szInput) {
		if(*szInput == '\\') {
			// Command
			char szCommand[RTF_MAXCMD];
			char *szCmdOutput;
			int lArg = -1;
			bool bNeg = false;

			++szInput;
			if(isalpha(*szInput)) {
				szCmdOutput = szCommand;

				while (isalpha(*szInput) && szCmdOutput < szCommand + RTF_MAXCMD - 1)
					*szCmdOutput++ = *szInput++;
				*szCmdOutput = 0;

				if(*szInput == '-') {
					bNeg = true;
					++szInput;
				}

				if(isdigit(*szInput)) {
					lArg = 0;
					while (isdigit(*szInput)) {
						lArg = lArg * 10 + *szInput - '0';
						++szInput;
					}
					if(bNeg) lArg = -lArg;
				}
				if(*szInput == ' ')
					++szInput;

				// szCommand is the command, lArg is the argument.
				if(strcmp(szCommand,"fonttbl") == 0) {
					sState[ulState].bInFontTbl = true;
				} else if(strcmp(szCommand,"colortbl") == 0) {
					sState[ulState].bInColorTbl = true;
				} else if(strcmp(szCommand,"pntext") == 0) { // pntext is the plaintext alternative, ignore it.
					sState[ulState].bRTFOnly = true;
				} else if(strcmp(szCommand,"ansicpg") == 0) {
					if(HrGetCharsetByCP(lArg, &szANSICharset) != hrSuccess)
						szANSICharset = "us-ascii";
					sState[ulState].szCharset = szANSICharset;
				} else if(strcmp(szCommand,"fcharset") == 0) {
					if(sState[ulState].bInFontTbl) {
						mapFontToCharset.insert(pair<int, int>(sState[ulState].ulFont, lArg));
					}
				} else if(strcmp(szCommand,"htmltag") == 0) {
				} else if(strcmp(szCommand,"mhtmltag") == 0) {
				} else if(strcmp(szCommand,"par") == 0 || strcmp(szCommand,"line") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {		
						sState[ulState].output.append("\r\n");
					}
				} else if(strcmp(szCommand,"tab") == 0) {
					if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) {		
						sState[ulState].output.append("\t");
					}
				} else if (strcmp(szCommand,"uc") == 0) {
					sState[ulState].ulUnicodeSkip = lArg;
				} else if(strcmp(szCommand,"f") == 0) {
					sState[ulState].ulFont = lArg;

					if(!sState[ulState].bInFontTbl) {
						fontmap_t::iterator i = mapFontToCharset.find(lArg);
						if (i == mapFontToCharset.end())
							continue;

						// Output any data before this point
						strBodyOut += RTFFlushStateOutput(convertContext, sState, ulState);

						// Set new charset			
						HrGetCharsetByRTFID(i->second, &sState[ulState].szCharset);
						if(sState[ulState].szCharset == NULL) {
							sState[ulState].szCharset = "us-ascii";
						} else if(sState[ulState].szCharset[0] == 0) {
							sState[ulState].szCharset = szANSICharset;
						}						
					} 
					// ignore error
				}
				else if (strcmp(szCommand,"u") == 0) {
					// unicode character, in signed short WCHAR
					strBodyOut += RTFFlushStateOutput(convertContext, sState, ulState);
					if (!sState[ulState].bRTFOnly)
						strBodyOut.append(1, (unsigned short)lArg); // add as literal character
					sState[ulState].ulSkipChars += sState[ulState].ulUnicodeSkip;
				}
				else if(strcmp(szCommand,"htmlrtf") == 0) {
					if(lArg != 0) {
						// \\htmlrtf
						sState[ulState].bRTFOnly = true;
					} else {
						// \\htmlrtf0
						sState[ulState].bRTFOnly = false;
					}
				}
				else if(strcmp(szCommand,"generator") == 0){
					while (*szInput != ';' && *szInput != '}' && *szInput)
						++szInput;

					if(*szInput == ';') 
						++szInput;
				}
				else if(strcmp(szCommand,"bkmkstart") == 0 || strcmp(szCommand,"bkmkend") == 0){
					// skip bookmark name
					while (*szInput && isalnum(*szInput))
						++szInput;

					sState[ulState].bInSkipTbl = true;
				} else if(isRTFIgnoreCommand(szCommand)) {
					sState[ulState].bInSkipTbl = true;
				}				

			}
			// Non-alnum after '\'
			else if(*szInput == '\\') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'\\');
			}
			else if(*szInput == '{') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'{');
			}
			else if(*szInput == '}') {
				++szInput;
				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl) 
					sState[ulState].output.append(1,'}');
			} 
			else if(*szInput == '\'') {
				unsigned int ulChar = 0;

				++szInput;

				if(*szInput) {
					ulChar = (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
					ulChar = ulChar << 4;
					++szInput;
				}

				if(*szInput) {
					ulChar += (unsigned int) (strchr(szHex, toupper(*szInput)) == NULL ? 0 : (strchr(szHex, toupper(*szInput)) - szHex));
					++szInput;
				}

				if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl && !sState[ulState].ulSkipChars) {
					if(ulChar > 255)
						sState[ulState].output.append(1, '?');
					else
						sState[ulState].output.append(1, ulChar);
				} else if (sState[ulState].ulSkipChars)
					--sState[ulState].ulSkipChars;

			} else {
				++szInput; // skip single character after '\'
			}
		} // Non-command
		else if(*szInput == '{') {
			// Dump output data
			strBodyOut += RTFFlushStateOutput(convertContext, sState, ulState);

			++ulState;
			if (ulState >= RTF_MAXSTATE)
				return MAPI_E_NOT_ENOUGH_MEMORY;
			sState[ulState] = sState[ulState-1];

			++szInput;
		} else if(*szInput == '}') {
			// Dump output data
			strBodyOut += RTFFlushStateOutput(convertContext, sState, ulState);

			if(ulState > 0)
				--ulState;
			++szInput;
		} else if(*szInput == '\r' || *szInput == '\n') {
			++szInput;
		} else {
			if(!sState[ulState].bInFontTbl && !sState[ulState].bRTFOnly && !sState[ulState].bInColorTbl && !sState[ulState].bInSkipTbl && !sState[ulState].ulSkipChars) {
				sState[ulState].output.append(1,*szInput);
			} else if (sState[ulState].ulSkipChars)
				--sState[ulState].ulSkipChars;
			++szInput;
		}
	}

	strBodyOut += RTFFlushStateOutput(convertContext, sState, ulState);
	return hrSuccess;
}
