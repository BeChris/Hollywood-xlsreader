/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Copyright 2004 Komarov Valery
 * Copyright 2006 Christophe Leitienne
 * Copyright 2008-2017 David Hoerl
 * Copyright 2013 Bob Colbert
 * Copyright 2013-2018 Evan Miller
 *
 * This file is part of libxls -- A multiplatform, C/C++ library for parsing
 * Excel(TM) files.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <math.h>
/*#include <sys/types.h>*/
/*#include <wchar.h>*/

#include <locale.h>
#include <limits.h>

#include <stdlib.h>
#include <errno.h>
/*#include <memory.h>*/
#include <string.h>

#include <hollywood/plugin.h>

//#include "xls.h"
#include "../include/libxls/xlstypes.h"
#include "../include/libxls/xlsstruct.h"
#include "../include/libxls/xlstool.h"
#include "../include/libxls/brdb.h"
#include "../include/libxls/endian.h"

#include "../include/purefuncs.h"

extern int xls_debug;

/* Not a complete list */
enum xls_format_e {
    XLS_FORMAT_GENERAL, // ""
    XLS_FORMAT_NUMBER1, // "0"
    XLS_FORMAT_NUMBER2,  //     "0.00",
    XLS_FORMAT_NUMBER3,  //     "#,##0",
    XLS_FORMAT_NUMBER4,  //     "#,##0.00",
    XLS_FORMAT_CURRENCY1,  //   "\"$\"#,##0_);(\"$\"#,##0)",
    XLS_FORMAT_CURRENCY2,  //   "\"$\"#,##0_);[Red](\"$\"#,##0)",
    XLS_FORMAT_CURRENCY3,  //   "\"$\"#,##0.00_);(\"$\"#,##0.00)",
    XLS_FORMAT_CURRENCY4,  //   "\"$\"#,##0.00_);[Red](\"$\"#,##0.00)",
    XLS_FORMAT_PERCENT1,  //    "0%",
    XLS_FORMAT_PERCENT2,  //    "0.00%",
    XLS_FORMAT_SCIENTIFIC1,  // "0.00E+00",
    XLS_FORMAT_SCIENTIFIC2 = 34 // "##0.0E+0"
};

static const DWORD colors[] =
    {
        0x000000,
        0xFFFFFF,
        0xFF0000,
        0x00FF00,
        0x0000FF,
        0xFFFF00,
        0xFF00FF,
        0x00FFFF,
        0x800000,
        0x008000,
        0x000080,
        0x808000,
        0x800080,
        0x008080,
        0xC0C0C0,
        0x808080,
        0x9999FF,
        0x993366,
        0xFFFFCC,
        0xCCFFFF,
        0x660066,
        0xFF8080,
        0x0066CC,
        0xCCCCFF,
        0x000080,
        0xFF00FF,
        0xFFFF00,
        0x00FFFF,
        0x800080,
        0x800000,
        0x008080,
        0x0000FF,
        0x00CCFF,
        0xCCFFFF,
        0xCCFFCC,
        0xFFFF99,
        0x99CCFF,
        0xFF99CC,
        0xCC99FF,
        0xFFCC99,
        0x3366FF,
        0x33CCCC,
        0x99CC00,
        0xFFCC00,
        0xFF9900,
        0xFF6600,
        0x666699,
        0x969696,
        0x003366,
        0x339966,
        0x003300,
        0x333300,
        0x993300,
        0x993366,
        0x333399,
        0x333333
    };


// Display string if in debug mode
void verbose(char* str)
{
}

char *utf8_decode(const char *str, DWORD len, char *encoding)
{
	int utf8_chars = 0;
	char *ret = NULL;
    DWORD i;
	
	for(i=0; i<len; ++i) {
		if(str[i] & (BYTE)0x80) {
			++utf8_chars;
		}
	}
	
	if(utf8_chars == 0 || pure_strcmp(encoding, "UTF-8")) {
		ret = pure_malloc(len+1);
		memcpy(ret, str, len);
		ret[len] = 0;
	} else {
        DWORD i;
        char *out;
		// UTF-8 encoding inline
		ret = pure_malloc(len+utf8_chars+1);
		out = ret;
		for(i=0; i<len; ++i) {
			BYTE c = str[i];
			if(c & (BYTE)0x80) {
				*out++ = (BYTE)0xC0 | (c >> 6);
				*out++ = (BYTE)0x80 | (c & 0x3F);
			} else {
				*out++ = c;
			}
		}
		*out = 0;
	}

	return ret;
}

// Convert unicode string to to_enc encoding
char* unicode_decode(const char *s, size_t len, size_t *newlen, const char* to_enc)
{
	// Do wcstombs conversion
    char *converted = NULL;
    int count;
    int idx;

    if (validate(s) == FALSE) {
        return NULL;
    }

    count = (len * 2) + 1;

    converted = pure_calloc(count * 5, sizeof(char));
    memset(converted, 0, 5 * sizeof(char));

    idx = 0;
    while (idx < len) {
        int ch = getnextchar(s, &idx);
        char temp[5] = {'\0'};
        composechar(temp, ch);
        pure_strcat(converted, temp);
    }

    if (newlen) *newlen = pure_strlen(converted);
    return converted;
}

// Read and decode string
char *get_string(const char *s, size_t len, BYTE is2, BYTE is5ver, char *charset)
{
    XLS_WORD ln;
    DWORD ofs = 0;
    BYTE flag = 0;
    const char *str = s;
    char *ret = NULL;
	
    if (is2) {
		// length is two bytes
        if (ofs + 2 > len) {
            return NULL;
        }
        ln= ((BYTE*)str)[0] + (((BYTE*)str)[1] << 8);
        ofs+=2;
    } else {
		// single byte length
        if (ofs + 1 > len) {
            return NULL;
        }
        ln=*(BYTE*)str;
        ofs++;
    }

	if(!is5ver) {
		// unicode strings have a format byte before the string
        if (ofs + 1 > len) {
            return NULL;
        }
		flag=*(BYTE*)(str+ofs);
		ofs++;
	}
    if (flag&0x8) {
		// XLS_WORD rt;
        // rt=*(XLS_WORD*)(str+ofs); // unused
        ofs+=2;
    }
    if (flag&0x4) {
		// DWORD sz;
        // sz=*(DWORD*)(str+ofs); // unused
        ofs+=4;
    }
    if(flag & 0x1) {
        if (ofs + 2*ln > len) {
            return NULL;
        }
        ret = unicode_decode(str+ofs, ln*2, NULL, charset);
    } else {
        if (ofs + ln > len) {
            return NULL;
        }
		ret = utf8_decode(str+ofs, ln, charset);
    }


    return ret;
}

DWORD xls_getColor(const XLS_WORD color,XLS_WORD def)
{
    int cor=8;
    int size = 64 - cor;
    int max = size;
    XLS_WORD idx=color;
    if( idx >= cor)
        idx -= cor;
    if( idx < max )
    {
        return colors[idx];
    }
    else
        return colors[def];
}


void xls_showBookInfo(xlsWorkBook* pWB)
{
}


void xls_showBOF(BOF* bof)
{
}


void xls_showROW(struct st_row_data* row)
{
}

void xls_showColinfo(struct st_colinfo_data* col)
{
}

void xls_showCell(struct st_cell_data* cell)
{
}


void xls_showFont(struct st_font_data* font)
{
}

void xls_showFormat(struct st_format_data* frmt)
{
}

void xls_showXF(XF8* xf)
{
}

char *xls_getfcell(xlsWorkBook* pWB, struct st_cell_data* cell, BYTE *label)
{
    struct st_xf_data *xf = NULL;
	XLS_WORD	len = 0;
    DWORD   offset = 0;
    char	*ret = NULL;
    size_t  retlen = 100;

    if (cell->xf < pWB->xfs.count)
        xf=&pWB->xfs.xf[cell->xf];

    switch (cell->id)
    {
    case XLS_RECORD_LABELSST:
        offset = label[0] + (label[1] << 8);
        if(!pWB->is5ver) {
            offset += ((DWORD)label[2] << 16);
            offset += ((DWORD)label[3] << 24);
        }
        if(offset < pWB->sst.count && pWB->sst.string[offset].str) {
            ret = pure_strdup(pWB->sst.string[offset].str);
        }
        break;
    case XLS_RECORD_BLANK:
    case XLS_RECORD_MULBLANK:
        ret = pure_strdup("");
        break;
    case XLS_RECORD_LABEL:
        len = label[0] + (label[1] << 8);
        label += 2;
		if(pWB->is5ver) {
            ret = pure_malloc(len+1);
            memcpy(ret, label, len);
            ret[len] = 0;
		} else {
            if ((*(label++) & 0x01) == 0) {
                ret = utf8_decode((char *)label, len, pWB->charset);
            } else {
                ret = unicode_decode((char *)label, len*2, NULL, pWB->charset);
            }
        }
        break;
    case XLS_RECORD_RK:
    case XLS_RECORD_NUMBER:
        ret = pure_malloc(retlen);
        pure_snprintf(ret, retlen, "%lf", cell->d);
		break;
		//		if( RK || MULRK || NUMBER || FORMULA)
		//		if (cell->id==0x27e || cell->id==0x0BD || cell->id==0x203 || 6 (formula))
    default:
        if (xf) {
            ret = pure_malloc(retlen);
            switch (xf->format)
            {
                case XLS_FORMAT_GENERAL:
                case XLS_FORMAT_NUMBER1:
                case XLS_FORMAT_NUMBER3:
                    pure_snprintf(ret, retlen, "%.0lf", cell->d);
                    break;
                case XLS_FORMAT_NUMBER2:
                case XLS_FORMAT_NUMBER4:
                    pure_snprintf(ret, retlen, "%.2f", cell->d);
                    break;
                case XLS_FORMAT_PERCENT1:
                    pure_snprintf(ret, retlen, "%.0lf%%", 100 * cell->d);
                    break;
                case XLS_FORMAT_PERCENT2:
                    pure_snprintf(ret, retlen, "%.2lf%%", 100 * cell->d);
                    break;
                case XLS_FORMAT_SCIENTIFIC1:
                    pure_snprintf(ret, retlen, "%.2e", cell->d);
                    break;
                case XLS_FORMAT_SCIENTIFIC2:
                    pure_snprintf(ret, retlen, "%.1e", cell->d);
                    break;
                default:
                    pure_snprintf(ret, retlen, "%.2f", cell->d);
                    break;
            }
            break;
        }
    }

    return ret;
}

/*
char* xls_getCSS(xlsWorkBook* pWB)
{
    char color[255];
    char* align;
    char* valign;
    char borderleft[255];
    char borderright[255];
    char bordertop[255];
    char borderbottom[255];
    char italic[255];
    char underline[255];
    char bold[255];
    XLS_WORD size;
    char fontname[255];
    struct st_xf_data* xf;
    DWORD background;
    DWORD i;

    char *ret = pure_malloc(65535);
    char *buf = pure_malloc(4096);
	ret[0] = '\0';

    for (i=0;i<pWB->xfs.count;i++)
    {
        xf=&pWB->xfs.xf[i];
        switch ((xf->align & 0x70)>>4)
        {
        case 0:
            valign=(char*)"top";
            break;
        case 1:
            valign=(char*)"middle";
            break;
        case 2:
            valign=(char*)"bottom";
            break;
            //			case 3: valign=(char*)"right"; break;
            //			case 4: valign=(char*)"right"; break;
        default:
            valign=(char*)"middle";
            break;
        }

        switch (xf->align & 0x07)
        {
        case 1:
            align=(char*)"left";
            break;
        case 2:
            align=(char*)"center";
            break;
        case 3:
            align=(char*)"right";
            break;
        default:
            align=(char*)"left";
            break;
        }

        switch (xf->linestyle & 0x0f)
        {
        case 0:
            sprintf(borderleft,"%s", "");
            break;
        default:
            sprintf(borderleft,"border-left: 1px solid black;");
            break;
        }

        switch (xf->linestyle & 0x0f0)
        {
        case 0:
            sprintf(borderright,"%s", "");
            break;
        default:
            sprintf(borderright,"border-right: 1px solid black;");
            break;
        }

        switch (xf->linestyle & 0x0f00)
        {
        case 0:
            sprintf(bordertop,"%s", "");
            break;
        default:
            sprintf(bordertop,"border-top: 1px solid black;");
            break;
        }

        switch (xf->linestyle & 0x0f000)
        {
        case 0:
            sprintf(borderbottom,"%s", "");
            break;
        default:
            sprintf(borderbottom,"border-bottom: 1px solid Black;");
            break;
        }

        if (xf->font)
            sprintf(color,"color:#%.6X;",xls_getColor(pWB->fonts.font[xf->font-1].color,0));
        else
            sprintf(color,"%s", "");

        if (xf->font && (pWB->fonts.font[xf->font-1].flag & 2))
            sprintf(italic,"font-style: italic;");
        else
            sprintf(italic,"%s", "");

        if (xf->font && (pWB->fonts.font[xf->font-1].bold>400))
            sprintf(bold,"font-weight: bold;");
        else
            sprintf(bold,"%s", "");

        if (xf->font && (pWB->fonts.font[xf->font-1].underline))
            sprintf(underline,"text-decoration: underline;");
        else
            sprintf(underline,"%s", "");

        if (xf->font)
            size=pWB->fonts.font[xf->font-1].height/20;
        else
            size=10;

        if (xf->font)
            sprintf(fontname,"%s",pWB->fonts.font[xf->font-1].name);
        else
            sprintf(fontname,"Arial");

        background=xls_getColor((XLS_WORD)(xf->groundcolor & 0x7f),1);
        sprintf(buf,".xf%i{ font-size:%ipt;font-family: \"%s\";background:#%.6X;text-align:%s;vertical-align:%s;%s%s%s%s%s%s%s%s}\n",
                i,size,fontname,background,align,valign,borderleft,borderright,bordertop,borderbottom,color,italic,bold,underline);

		pure_strcat(ret,buf);
    }
	ret = pure_realloc(ret, pure_strlen(ret)+1);
	pure_free(buf);

    return ret;
}
*/

