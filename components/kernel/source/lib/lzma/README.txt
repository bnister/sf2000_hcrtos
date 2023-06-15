The lib_lzma functionality was written by Igor Pavlov.
The original source cames from the LZMA SDK web page:

URL: 		http://www.7-zip.org/sdk.html
Author:         Igor Pavlov

The import is made using the import_lzmasdk.sh script that:

* untars the lzmaXYY.tar.bz2 file (from the download web page)
* copies the files LzmaDec.h, Types.h, LzmaDec.c, history.txt,
  and lzma.txt from source archive into the lib_lzma directory (pwd).

Example:

 . import_lzmasdk.sh ~/lzma465.tar.bz2

Notice: The files from lzma sdk are _not modified_ by this script!

The files LzmaTools.{c,h} are provided to export the lzmaBuffToBuffDecompress()
function that wraps the complex LzmaDecode() function from the LZMA SDK. The
do_bootm() function uses the lzmaBuffToBuffDecopress() function to expand the
compressed image.

The directory U-BOOT/include/lzma contains stubs files that permit to use the
library directly from U-BOOT code without touching the original LZMA SDK's
files.

Luigi 'Comio' Mantellini <luigi.mantellini@idf-hit.com>


Example:
a.c needs to decompress datas like:

#include <lzma/LzmaTools.h>
int decompressdatas()
{
	int param[]={                                                         
        -2147483555, -256, -1, -1674575617, 838130057,                
        -1505201059, 1602711352, -1709684522, 1374817652, 1507706951, 
        1332608279, 502792476, 840213722, 1107246859, 16474,          
	};                                                                                                       
        int outparam[1024];    
	
	char *in = (char *)param;
	char *out = (char *)outparam;   
	                  
        SizeT unlen = sizeof(outparam);    // unlen must have a default value larger than the possible space of decompressed datas.                                    
	SizeT len = sizeof(param);                                            
                                                                                                                           
	lzmaBuffToBuffDecompress(out, &unlen, in, len);   //unlen will changed by the function to the real value.                    
}                                                                     
