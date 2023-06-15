/* 
 *
 * Filename	: gb2312.c
 * Description	: gb2312 charset mapping file.
 */

#if 1//def GB2312_SUPPORT

#include "char_map.h"
#include "gb_2312.h"
UINT16 u8u16_lookup(UINT16 key, struct u8u16 *table, INT32 count)
{
	UINT16 ret = 0xFFFF;
	INT32 mid, low=-1, high=count;
	if (table == 0)
		return ret;
	do {
		mid = (low+high)>>1;
		if (table[mid].key>key) {
			high = mid;
		} else if (table[mid].key<key) {
			low = mid;
		} else {
			ret = table[mid].value;
			break;
		}
	}while(high-low>1);
	
	return ret;
}

//#define GB2312_Debug    libc_printf
#define GB2312_Debug(...)    do{}while(0)

#define	GB2312_MAP_LENGTH		(sizeof(gb2312_map)/sizeof(struct u8u16))
#define     UNICODE_MAP_LENGTH		(sizeof(u_g_map)/sizeof(struct u8u16))
typedef char			INT8;
typedef unsigned char	UINT8;

typedef short			INT16;
typedef unsigned short	UINT16;

typedef long			INT32;
typedef unsigned long	UINT32;

typedef unsigned long long UINT64;
typedef long long INT64;

typedef signed int INT;
typedef unsigned int UINT;

/*
 * name		: convert_gb2312_to_unicode
 * description	: convert the string from gb2312 to unicode.
 * parameter	: 4
 *	IN	UINT8 *gb2312: the gb2312 string.
 *	IN	INT32 length: the gb2312 string length.
 *	OUT	UINT16 *unicode: the generated unicode.
 *	IN	INT32 maxlen: the unicode string maxiam length.
 * return value : INT32
 *	SUCCESS	:	gb2312 string is converted, and stored in unicode.
 *	ERR_FAILUE:	
 */
static int OFFSET_BIT=0;
int get_offset_bit(void)
{ 
	return OFFSET_BIT;
} 
INT32 convert_gb2312_to_unicode(const UINT8 *gb2312, INT32 length, UINT16 *unicode, INT32 maxlen)
{
	UINT16	gb;
	INT32	si, di;

	if(0== unicode || 0==gb2312)
       		return 0;

	if(0==length || 0==maxlen)
		return 0;

	for(si=0, di=0; (si<length)&&(di<maxlen);)
	{
		if (gb2312[si]<0x80)
		{
			unicode[di] = cpu_to_be16((UINT16)gb2312[si]);
			di++, si++;

			if((si%2)==1)
				OFFSET_BIT=1;
			else 
				OFFSET_BIT=0;
		}
		else if((gb2312[si]!=0xE0)&&(gb2312[si]>=0x80)&&(gb2312[si]<=0x9F))
		{
			si+=1; /* control code, ignore it now. */
		}
		else if ((gb2312[si]==0xE0)&&(si<length-1)&&
			(gb2312[si+1]>=0x80)&&(gb2312[si+1]<=0x9F))
		{
			si+=2; /* control code, ignore them now. */
		}
		else
		{
			if(si+1 < length)
			{
				gb = (UINT16)((gb2312[si]<<8) | gb2312[si+1]);
				if((unicode[di] = u8u16_lookup(gb, gb2312_map, GB2312_MAP_LENGTH)) == 0xFFFF)
					// if not found, blank
					unicode[di] = 0x3000;
				unicode[di] = cpu_to_be16(unicode[di]);
				di++, si+=2;
			}
			else
				// skip a single byte
				si++;
		}
	}
	unicode[di]=0x0000;

	return di;
}

/*
 * name		: convert_unicode_to_gb2312
 * description	: convert the string from unicode to gb2312.
 * parameter	: 4
 *	IN	    UINT16 *unicode: the generated unicode.
 *	IN	    INT32 u_len: the unicode string  length.
 *	IN	    INT32 g_maxlen: the gb2312 string maximum length.
 *	OUT	    UINT8 *gb2312: the gb2312 string.
 * return value : INT32
 *	SUCCESS	:	unicode string is converted, and stored in gb2312.
 *	ERR_FAILUE:	
 */
INT32 convert_unicode_to_gb2312(UINT16 *unicode,  INT32 u_len, UINT8 *gb2312, INT32 g_maxlen)
{
	UINT16	gb;
	INT32	gi, ui;

	if(0== unicode || 0==gb2312)
       		return 0;

	if(0==g_maxlen || 0==u_len)
		return 0;

	for(ui=0, gi=0; (gi+2<g_maxlen)&&(ui<u_len);)
	{
		unicode[ui] = U16_TO_UNICODE(unicode[ui]);		
		if( 0 == (0xFF80&unicode[ui]))
		{
			//gb2312[gi] = 0x0;
			gb2312[gi] = (UINT8)(unicode[ui]&0x00FF);
			ui++;
			gi+= 1;
		}
		else
		{	
			gb = u8u16_lookup(unicode[ui], u_g_map, UNICODE_MAP_LENGTH) ;
			if(gb == 0xFFFF)
			{
				gb2312[gi] = 0xFF;
				gb2312[gi+1] = (UINT8)(unicode[ui]&0x00FF);
				gb2312[gi+2] = (UINT8)((unicode[ui]&0xFF00)>>8);
				GB2312_Debug("%s, unicode 0x%x.  error, cannot find matched gb2312 char!!!\n", __FUNCTION__, unicode[ui]);
				ui++;
				gi += 3;	
			}
			else
			{
				if(gb<0x8080)
					gb += 0x8080;
				
				gb = cpu_to_be16(gb);
				if(gi+1 < g_maxlen)
				{
					gb2312[gi] = (UINT8)(gb&0x00FF);
					gb2312[gi+1] = (UINT8)(gb>>8);
				}
				ui++; 
				gi+= 2;
			}
		}
		
	}

	gb2312[gi] = 0x0000;
	
	return gi;
}

#endif /* GB2312_SUPPORT*/

