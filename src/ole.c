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

/*#include <memory.h>*/
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "../include/libxls/ole.h"
#include "../include/libxls/xlstool.h"
#include "../include/libxls/endian.h"
#include "../include/purefuncs.h"

extern int xls_debug;

//#define OLE_DEBUG

//static const DWORD MSATSECT		= 0xFFFFFFFC;	// -4
//static const DWORD FATSECT		= 0xFFFFFFFD;	// -3
static const DWORD ENDOFCHAIN	= 0xFFFFFFFE;	// -2
static const DWORD FREESECT		= 0xFFFFFFFF;	// -1

static size_t sector_pos(OLE2* ole2, DWORD sid);
static int64_t sector_read(OLE2* ole2, void *buffer, size_t buffer_len, DWORD sid);
static int64_t read_MSAT(OLE2* ole2, OLE2Header *oleh);
static void *ole_malloc(size_t len);
static void *ole_realloc(void *ptr, size_t len);

static void *ole_malloc(size_t len) {
    if (len > (1<<24) || len == 0) {
        return NULL;
    }
    return pure_malloc(len);
}

static void *ole_realloc(void *ptr, size_t len) {
    if (len > (1<<24) || len == 0) {
        pure_free(ptr);
        return NULL;
    }
    return pure_realloc(ptr, len);
}

static int ole2_validate_sector_chain(DWORD *chain, DWORD chain_count, DWORD chain_start) {
    DWORD count = 0;
    DWORD sector = chain_start;
    while (sector != ENDOFCHAIN) {
        if (sector >= chain_count)
            return 0;
        
        if (++count >= chain_count)
            return 0;

        sector = xlsIntVal(chain[sector]);
    }
    return 1;
}

static int ole2_validate_sector(DWORD sector, OLE2 *ole) {
    if (sector >= ole->SecIDCount) {
        return 0;
    }

    if (sector == xlsIntVal(ole->SecID[sector])) {
        return 0;
    }

    return 1;
}

// Read next sector of stream
static int ole2_bufread(OLE2Stream* olest)
{
	BYTE *ptr;

    if (olest == NULL || olest->ole == NULL)
        return -1;

    if ((DWORD)olest->fatpos!=ENDOFCHAIN)
    {
		if(olest->sfat) {
            if (olest->ole->SSAT == NULL || olest->buf == NULL || olest->ole->SSecID == NULL)
                return -1;

            if (olest->fatpos*olest->ole->lssector + olest->bufsize > olest->ole->SSATCount) {
                return -1;
            }

			ptr = olest->ole->SSAT + olest->fatpos*olest->ole->lssector;
			memcpy(olest->buf, ptr, olest->bufsize); 

            if (olest->fatpos >= olest->ole->SSecIDCount) {
                return -1;
            }

			olest->fatpos=xlsIntVal(olest->ole->SSecID[olest->fatpos]);
			olest->pos=0;
			olest->cfat++;
		} else {
			if ((int)olest->fatpos < 0 ||
                sector_read(olest->ole, olest->buf, olest->bufsize, olest->fatpos) == -1) {
                return -1;
            }

            if (!ole2_validate_sector(olest->fatpos, olest->ole)) {
                return -1;
            }

            olest->fatpos = xlsIntVal(olest->ole->SecID[olest->fatpos]);
			olest->pos=0;
			olest->cfat++;
		}
    }
    return 0;
}

// Read part of stream
int64_t ole2_read(void* buf, size_t size, size_t count, OLE2Stream* olest)
{
    size_t didReadCount=0;
    size_t totalReadCount;

	totalReadCount=size*count;

	// olest->size inited to -1

    if ((long)olest->size>=0 && !olest->sfat)	// directory is -1
    {
		size_t rem;
		rem = olest->size - (olest->cfat*olest->ole->lsector+olest->pos);		
        totalReadCount = rem<totalReadCount?rem:totalReadCount;
        if (rem<=0) olest->eof=1;

	}

	while ((!olest->eof) && (didReadCount < totalReadCount))
	{
		unsigned long remainingBytes;
        size_t needToReadCount;

		needToReadCount	= totalReadCount - didReadCount;
		remainingBytes	= olest->bufsize - olest->pos;

		if (needToReadCount < remainingBytes) { // does the current sector contain all the data I need?
			memcpy((BYTE*)buf + didReadCount, olest->buf + olest->pos, needToReadCount);
			olest->pos		+= needToReadCount;
			didReadCount	+= needToReadCount;
		} else {
			memcpy((BYTE*)buf + didReadCount, olest->buf + olest->pos, remainingBytes);
			olest->pos		+= remainingBytes;
			didReadCount	+= remainingBytes;
			if (ole2_bufread(olest) == -1)
                return -1;
		}
		if (((DWORD)olest->fatpos == ENDOFCHAIN) && (olest->pos >= olest->bufsize)) {
			olest->eof=1;
		}
	}
    if (didReadCount > totalReadCount)
        return -1;



    return didReadCount;
}

// Open stream in logical ole file
OLE2Stream* ole2_sopen(OLE2* ole,DWORD start, size_t size)
{
    OLE2Stream* olest=NULL;
    int success = 1;


    olest = pure_calloc(1, sizeof(OLE2Stream));
    olest->ole=ole;
    olest->size=size;
    olest->fatpos=start;
    olest->start=start;
    olest->cfat=-1;
    if((long)size > 0 && size < (size_t)ole->sectorcutoff) {
        olest->bufsize=ole->lssector;
        olest->sfat = 1;
    } else {
        olest->bufsize=ole->lsector;
    }
    if ((olest->buf = ole_malloc(olest->bufsize)) == NULL) {
        success = 0;
        goto cleanup;
    }

    if (olest->sfat) {
        if (!ole2_validate_sector_chain(ole->SSecID, ole->SSecIDCount, start)) {
            success = 0;
            goto cleanup;
        }
    } else {
        if (!ole2_validate_sector_chain(ole->SecID, ole->SecIDCount, start)) {
            success = 0;
            goto cleanup;
        }
    }

    if (ole2_bufread(olest) == -1) {
        success = 0;
        goto cleanup;
    }

cleanup:
    if (!success) {
        ole2_fclose(olest);
        olest = NULL;
    }

    return olest;
}

// Move in stream
int ole2_seek(OLE2Stream* olest,DWORD ofs)
{
	if(olest->sfat) {
		ldiv_t div_rez=ldiv(ofs,olest->ole->lssector);
		int i;
		olest->fatpos=olest->start;

        if (div_rez.quot!=0)
        {
            for (i=0;i<div_rez.quot;i++) {
                if (olest->fatpos >= olest->ole->SSecIDCount)
                    return -1;
                olest->fatpos=xlsIntVal(olest->ole->SSecID[olest->fatpos]);
            }
        }

        if (ole2_bufread(olest) == -1)
            return -1;

        olest->pos=div_rez.rem;
        olest->eof=0;
        olest->cfat=div_rez.quot;
	} else {
		ldiv_t div_rez=ldiv(ofs,olest->ole->lsector);
		int i;
		olest->fatpos=olest->start;

        if (div_rez.quot!=0)
        {
            for (i=0;i<div_rez.quot;i++) {
                if (!ole2_validate_sector(olest->fatpos, olest->ole))
                    return -1;
                olest->fatpos=xlsIntVal(olest->ole->SecID[olest->fatpos]);
            }
        }

        if (ole2_bufread(olest) == -1)
            return -1;

		olest->pos=div_rez.rem;
		olest->eof=0;
		olest->cfat=div_rez.quot;
	}
    return 0;
}

// Open logical file contained in physical OLE file
OLE2Stream*  ole2_fopen(OLE2* ole, const char *file)
{
    int i;


    for (i=0;i<ole->files.count;i++) {
		char *str = ole->files.file[i].name;
        if (str && strcmp(str,file)==0)	// newer versions of Excel don't write the "Root Entry" string for the first set of data
        {
            return ole2_sopen(ole,ole->files.file[i].start,ole->files.file[i].size);
        }
	}
    return NULL;
}

static int ole2_fseek(OLE2 *ole2, size_t pos) {
    if (ole2->handle)
    {
        if ((ole2->DOSBase->hw_FSeek(ole2->handle, pos, HWFSEEKMODE_BEGINNING) == -1))
        {
            return -1;
        }
        else
        {
            return 0;
        }
        
    }

    if (pos > ole2->buffer_len)
        return -1;

    ole2->buffer_pos = pos;
    return 0;
}

static size_t ole2_fread(OLE2 *ole2, void *buffer, size_t buffer_len, size_t size) {

    if (size > buffer_len)
        return 0;

    if (ole2->handle != NULL)
    {
        int res = ole2->DOSBase->hw_FRead(ole2->handle, buffer, size);
        if (res == size)
        {
            return 1;
        }
        else
        {
            return -1;
        }
        
    }

    if (ole2->buffer_pos + size > ole2->buffer_len)
        return 0;

    memcpy(buffer, (const char *)ole2->buffer + ole2->buffer_pos, size);
    ole2->buffer_pos += size;

    return 1;
}

// read header and check magic numbers
static int64_t ole2_read_header(OLE2 *ole) {
    int64_t bytes_read = 0, total_bytes_read = 0;
    OLE2Header *oleh = pure_malloc(sizeof(OLE2Header));
    if (ole2_fread(ole, oleh, sizeof(OLE2Header), sizeof(OLE2Header)) != 1) {
        total_bytes_read = -1;
        goto cleanup;
    }
    total_bytes_read += sizeof(OLE2Header);
    xlsConvertHeader(oleh);

	// make sure the file looks good. Note: this code only works on Little Endian machines
	if(oleh->id[0] != 0xE011CFD0 || oleh->id[1] != 0xE11AB1A1 || oleh->byteorder != 0xFFFE) {
        total_bytes_read = -1;
        goto cleanup;
	}

    //ole->lsector=(XLS_WORD)pow(2,oleh->lsector);
    //ole->lssector=(XLS_WORD)pow(2,oleh->lssector);
	ole->lsector=512;
    ole->lssector=64;

	if (oleh->lsectorB != 9 || oleh->lssectorB != 6) {	// 2**9 == 512, 2**6 == 64
        total_bytes_read = -1;
        goto cleanup;
    }
	
    ole->cfat=oleh->cfat;
    ole->dirstart=oleh->dirstart;
    ole->sectorcutoff=oleh->sectorcutoff;
    ole->sfatstart=oleh->sfatstart;
    ole->csfat=oleh->csfat;
    ole->difstart=oleh->difstart;
    ole->cdif=oleh->cdif;
    ole->files.count=0;

    // read directory entries
    if ((bytes_read = read_MSAT(ole, oleh)) == -1) {
        total_bytes_read = -1;
        goto cleanup;
    }
    total_bytes_read += bytes_read;

cleanup:
    pure_free(oleh);

    return total_bytes_read;
}

static int64_t ole2_read_body(OLE2 *ole) {
	// reuse this buffer
    PSS *pss = NULL;
    OLE2Stream *olest = NULL;
    char* name = NULL;
    int64_t bytes_read = 0, total_bytes_read = 0;

    if ((olest = ole2_sopen(ole,ole->dirstart, -1)) == NULL) {
        total_bytes_read = -1;
        goto cleanup;
    }
    pss = pure_malloc(sizeof(PSS));
    do {
        if ((bytes_read = ole2_read(pss,1,sizeof(PSS),olest)) == -1) {
            total_bytes_read = -1;
            goto cleanup;
        }
        total_bytes_read += bytes_read;
        xlsConvertPss(pss);
        if (pss->bsize > sizeof(pss->name)) {
            total_bytes_read = -1;
            goto cleanup;
        }
        name=unicode_decode(pss->name, pss->bsize, 0, "UTF-8");
        if (pss->type == PS_USER_ROOT || pss->type == PS_USER_STREAM) // (name!=NULL) // 
        {

            ole->files.file = pure_realloc(ole->files.file,(ole->files.count+1)*sizeof(struct st_olefiles_data));
            ole->files.file[ole->files.count].name=name;
            ole->files.file[ole->files.count].start=pss->sstart;
            ole->files.file[ole->files.count].size=pss->size;
            ole->files.count++;
			
			if(pss->sstart == ENDOFCHAIN) {
				if (xls_debug) verbose("END OF CHAIN\n");
			} else if(pss->type == PS_USER_STREAM) {
			} else if(pss->type == PS_USER_ROOT) {
				DWORD sector, k, blocks;
				BYTE *wptr;
                size_t bytes_left;
				
				blocks = (pss->size + (ole->lsector - 1)) / ole->lsector;	// count partial
				if ((ole->SSAT = ole_realloc(ole->SSAT, blocks*ole->lsector)) == NULL) {
                    total_bytes_read = -1;
                    goto cleanup;
                }
                ole->SSATCount = blocks*ole->lsector;

				sector = pss->sstart;
				wptr = (BYTE*)ole->SSAT;
                bytes_left = blocks*ole->lsector;
				for(k=0; k<blocks; ++k) {
                    if (sector == ENDOFCHAIN || sector_read(ole, wptr, bytes_left, sector) == -1) {
                        total_bytes_read = -1;
                        goto cleanup;
                    }
                    if (!ole2_validate_sector(sector, ole)) {
                        total_bytes_read = -1;
                        goto cleanup;
                    }
                    total_bytes_read += ole->lsector;
					wptr += ole->lsector;
                    bytes_left -= ole->lsector;
					sector = xlsIntVal(ole->SecID[sector]);
				}
			}	
		} else {
			pure_free(name);
		}
    } while (!olest->eof);

cleanup:
    if (olest)
        ole2_fclose(olest);
    if (pss)
        pure_free(pss);

    return total_bytes_read;
}

OLE2 *ole2_read_header_and_body(OLE2 *ole) {
    if (ole2_read_header(ole) == -1) {
        ole2_close(ole);
        return NULL;
    }

    if (ole2_read_body(ole) == -1) {
        ole2_close(ole);
        return NULL;
    }

    return ole;
}

// Open in-memory buffer
OLE2 *ole2_open_buffer(const void *buffer, size_t len) {
    OLE2 *ole = pure_calloc(1, sizeof(OLE2));

    ole->buffer = buffer;
    ole->buffer_len = len;

    return ole2_read_header_and_body(ole);
}

// Open physical file
OLE2* ole2_open_file(hwDOSBase *DOSBase, const char *file)
{
    OLE2* ole = NULL;


    ole = pure_calloc(1, sizeof(OLE2));

    ole->DOSBase = DOSBase;


    if ((ole->handle=DOSBase->hw_FOpen((STRPTR)file, HWFOPENMODE_READ_NEW)) == NULL) {
        pure_free(ole);
        return NULL;
    }

    return ole2_read_header_and_body(ole);
}

void ole2_close(OLE2* ole2)
{
    int i;
    if (ole2->handle)
        ole2->DOSBase->hw_FClose(ole2->handle);

    for(i=0; i<ole2->files.count; ++i) {
        pure_free(ole2->files.file[i].name);
    }
    pure_free(ole2->files.file);
    pure_free(ole2->SecID);
    pure_free(ole2->SSecID);
    pure_free(ole2->SSAT);
    pure_free(ole2);
}

void ole2_fclose(OLE2Stream* ole2st)
{
	pure_free(ole2st->buf);
	pure_free(ole2st);
}

// Return offset in bytes of a sector from its sid
static size_t sector_pos(OLE2* ole2, DWORD sid)
{
    return 512 + sid * ole2->lsector;
}
// Read one sector from its sid
static int64_t sector_read(OLE2* ole2, void *buffer, size_t buffer_len, DWORD sid)
{
	size_t num;
	size_t seeked;

	if ((seeked = ole2_fseek(ole2, sector_pos(ole2, sid))) != 0) {
        return -1;
    }

    if ((num = ole2_fread(ole2, buffer, buffer_len, ole2->lsector)) != 1) {
        return -1;
    }

    return ole2->lsector;
}

// read first 109 sectors of MSAT from header
static int64_t read_MSAT_header(OLE2* ole2, OLE2Header* oleh, DWORD sectorCount) {
    BYTE *sector = (BYTE*)ole2->SecID;
    int64_t bytes_read = 0, total_bytes_read = 0;
    size_t bytes_left = ole2->SecIDCount * sizeof(DWORD);
    DWORD sectorNum;

    for (sectorNum = 0; sectorNum < sectorCount && sectorNum < 109; sectorNum++)
    {
        if ((bytes_read = sector_read(ole2, sector, bytes_left, oleh->MSAT[sectorNum])) == -1) {
            return -1;
        }
        sector += ole2->lsector;
        bytes_left -= ole2->lsector;
        total_bytes_read += bytes_read;
    }
    return total_bytes_read;
}

// Add additional sectors of the MSAT
static int64_t read_MSAT_body(OLE2 *ole2, DWORD sectorOffset, DWORD sectorCount) {
    DWORD sid = ole2->difstart;
    int64_t bytes_read = 0, total_bytes_read = 0;
    DWORD sectorNum = sectorOffset;

    DWORD *sector = ole_malloc(ole2->lsector);
    while (sid != ENDOFCHAIN && sid != FREESECT) // FREESECT only here due to an actual file that requires it (old Apple Numbers bug)
    {
        int posInSector;
        // read MSAT sector
        if ((bytes_read = sector_read(ole2, sector, ole2->lsector, sid)) == -1) {
            total_bytes_read = -1;
            goto cleanup;
        }
        total_bytes_read += bytes_read;

        // read content
        for (posInSector = 0; posInSector < (ole2->lsector-4)/4; posInSector++)
        {
            DWORD s = sector[posInSector];

            if (s != ENDOFCHAIN && s != FREESECT) // see patch in Bug 31. For very large files
            {
                if (sectorNum == sectorCount) {
                    total_bytes_read = -1;
                    goto cleanup;
                }
                if ((bytes_read = sector_read(ole2, (BYTE*)(ole2->SecID)+sectorNum*ole2->lsector,
                                (ole2->SecIDCount * sizeof(DWORD) - sectorNum*ole2->lsector), s)) == -1) {
                    total_bytes_read = -1;
                    goto cleanup;
                }
                total_bytes_read += bytes_read;
                sectorNum++;
            }
        }
        sid = sector[posInSector];
    }

cleanup:
    pure_free(sector);
    return total_bytes_read;
}

// read in short table
static int64_t read_MSAT_trailer(OLE2 *ole2) {
    int64_t total_bytes_read = 0;
    DWORD sector, k;
    BYTE *wptr;
    size_t bytes_left;

    if(ole2->sfatstart == ENDOFCHAIN)
        return 0;

    if ((ole2->SSecID = ole_malloc(ole2->csfat*(size_t)ole2->lsector)) == NULL) {
        return -1;
    }
    ole2->SSecIDCount = ole2->csfat*(size_t)ole2->lsector/4;
    sector = ole2->sfatstart;
    wptr=(BYTE*)ole2->SSecID;
    bytes_left = ole2->SSecIDCount * sizeof(DWORD);
    for(k=0; k<ole2->csfat; ++k) {
        if (sector == ENDOFCHAIN || sector_read(ole2, wptr, bytes_left, sector) == -1) {
            total_bytes_read = -1;
            goto cleanup;
        }
        if (!ole2_validate_sector(sector, ole2)) {
            total_bytes_read = -1;
            goto cleanup;
        }
        wptr += ole2->lsector;
        bytes_left -= ole2->lsector;
        total_bytes_read += ole2->lsector;
        sector = xlsIntVal(ole2->SecID[sector]);
    }

cleanup:
    return total_bytes_read;
}


// Read MSAT
static int64_t read_MSAT(OLE2* ole2, OLE2Header* oleh)
{
    int64_t total_bytes_read = 0;
    int64_t bytes_read = 0;

    // reconstitution of the MSAT
    DWORD count = ole2->cfat;
    if(count == 0 || count > (1 << 24)) {
        return -1;
    }

    ole2->SecIDCount = count*ole2->lsector/4;
    if ((ole2->SecID = ole_malloc(ole2->SecIDCount * sizeof(DWORD))) == NULL) {
        total_bytes_read = -1;
        goto cleanup;
    }

    if ((bytes_read = read_MSAT_header(ole2, oleh, count)) == -1) {
        total_bytes_read = -1;
        goto cleanup;
    }
    total_bytes_read += bytes_read;

    if ((bytes_read = read_MSAT_body(ole2, total_bytes_read / ole2->lsector, count)) == -1) {
        total_bytes_read = -1;
        goto cleanup;
    }
    total_bytes_read += bytes_read;

    if ((bytes_read = read_MSAT_trailer(ole2)) == -1) {
        total_bytes_read = -1;
        goto cleanup;
    }
    total_bytes_read += bytes_read;

cleanup:
    if (total_bytes_read == -1) {
        if (ole2->SecID) {
            pure_free(ole2->SecID);
            ole2->SecID = NULL;
        }
        if (ole2->SSecID) {
            pure_free(ole2->SSecID);
            ole2->SSecID = NULL;
        }
    }

    return total_bytes_read;
}
