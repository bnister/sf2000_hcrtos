/***************************************************************************
 *   v4l2grab Version 0.3                                                  *
 *   Copyright (C) 2012 by Tobias Müller                                   *
 *   Tobias_Mueller@twam.info                                              *
 *                                                                         *
 *   based on V4L2 Specification, Appendix B: Video Capture Example        *
 *   (http://v4l2spec.bytesex.org/spec/capture-example.html)               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
 /**************************************************************************
 *   Modification History                                                  *
 *                                                                         *
 *   Matthew Witherwax      21AUG2013                                      *
 *      Added ability to change frame interval (ie. frame rate/fps)        *
 * Martin Savc              7JUL2015
 *      Added support for continuous capture using SIGINT to stop.
 ***************************************************************************/

// compile with all three access methods
#if !defined(IO_READ) && !defined(IO_MMAP) && !defined(IO_USERPTR)
#define IO_READ
#define IO_MMAP
#define IO_USERPTR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <jpeglib.h>
#include <libv4l2.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <poll.h>

#include "config.h"
#include "yuv.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#ifndef VERSION
#define VERSION "unknown"
#endif

#if defined(IO_MMAP) || defined(IO_USERPTR)
// minimum number of buffers to request in VIDIOC_REQBUFS call
#define VIDIOC_REQBUFS_COUNT 2
#endif

typedef enum {
#ifdef IO_READ
        IO_METHOD_READ,
#endif
#ifdef IO_MMAP
        IO_METHOD_MMAP,
#endif
#ifdef IO_USERPTR
        IO_METHOD_USERPTR,
#endif
} io_method;

typedef struct tagBITMAPFILEHEADER{
    unsigned short    bfType;                // the flag of bmp, value is "BM"
    unsigned int    bfSize;                // size BMP file ,unit is bytes
    unsigned int     bfReserved;            // 0
    unsigned int     bfOffBits;             // must be 54

}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
    unsigned int     biSize;                // must be 0x28
    unsigned int     biWidth;           //
    unsigned int     biHeight;          //
    unsigned short        biPlanes;          // must be 1
    unsigned short        biBitCount;            //
    unsigned int     biCompression;         //
    unsigned int     biSizeImage;       //
    unsigned int     biXPelsPerMeter;   //
    unsigned int     biYPelsPerMeter;   //
    unsigned int     biClrUsed;             //
    unsigned int     biClrImportant;        //
}BITMAPINFOHEADER; 

struct videobuffer {
        void *                  start;
        size_t                  length;
};

//#define  IMAGEWIDTH    1280
//#define  IMAGEHEIGHT   720
#define VIDEOCOUNT  3
#define BMP          "/tmp/image_rgb.bmp"
#define JPEG          "/tmp/image_jpeg.jpg"
#define YUV            "/tmp/image_yuv.yuv"

static struct videobuffer framebuf[VIDEOCOUNT];
//static   struct   v4l2_capability   cap;//device func, VIDIOC_QUERYCAP=cmd
//static struct v4l2_fmtdesc fmtdesc;//video format

//static io_method        io              = IO_METHOD_MMAP;
static unsigned char* starter;
static unsigned char* newBuf;
static int  fd_video             = -1;
//struct buffer *         buffers         = NULL;
//static unsigned int     n_buffers       = 0;

// global settings
static unsigned int width = 1280;//640;
static unsigned int height = 720;//480;

/**
SIGINT interput handler
*/
char save_bmp_file(unsigned char *rgb_date, int date_len);

int yuyv_2_rgb888(const void *p, int size, unsigned char *frame_buffer)
 {
    int           i,j;
    unsigned char y1,y2,u,v;
    int r1,g1,b1,r2,g2,b2;
    char * pointer;

    pointer = p;
    for(i=0;i<480;i++)
    {
        for(j=0;j<320;j++)
        {
            y1 = *( pointer + (i*320+j)*4);
            u  = *( pointer + (i*320+j)*4 + 1);
            y2 = *( pointer + (i*320+j)*4 + 2);
            v  = *( pointer + (i*320+j)*4 + 3);

            r1 = y1 + 1.042*(v-128);
            g1 = y1 - 0.34414*(u-128) - 0.71414*(v-128);
            b1 = y1 + 1.772*(u-128);
            r2 = y2 + 1.042*(v-128);

            g2 = y2 - 0.34414*(u-128) - 0.71414*(v-128);
            b2 = y2 + 1.772*(u-128);

            if(r1>255)                r1 = 255;
            else if(r1<0)                r1 = 0;
            if(b1>255)                b1 = 255;
            else if(b1<0)                b1 = 0;    
            if(g1>255)                g1 = 255;
            else if(g1<0)                g1 = 0;    
            if(r2>255)                r2 = 255;
            else if(r2<0)                r2 = 0;
            if(b2>255)                b2 = 255;
            else if(b2<0)                b2 = 0;    
            if(g2>255)                g2 = 255;
            else if(g2<0)                g2 = 0;        

            *(frame_buffer + ((480-1-i)*320+j)*6    ) = (unsigned char)b1;
            *(frame_buffer + ((480-1-i)*320+j)*6 + 1) = (unsigned char)g1;
            *(frame_buffer + ((480-1-i)*320+j)*6 + 2) = (unsigned char)r1;
            *(frame_buffer + ((480-1-i)*320+j)*6 + 3) = (unsigned char)b2;
            *(frame_buffer + ((480-1-i)*320+j)*6 + 4) = (unsigned char)g2;
            *(frame_buffer + ((480-1-i)*320+j)*6 + 5) = (unsigned char)r2;
        }
    }
    printf("change to RGB OK \n");
}

char save_bmp_file(unsigned char *rgb_date, int date_len)
{
    FILE * fp1;
    BITMAPFILEHEADER   bf;
    BITMAPINFOHEADER   bi;
    unsigned char bf_header[16]={0};

    printf("%s ####L%d\n", __func__, __LINE__);
    //Set BITMAPINFOHEADER
    bi.biSize = 40;
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = 0;
    bi.biSizeImage = bi.biWidth * bi.biHeight * 3;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    //Set BITMAPFILEHEADER
    bf.bfType = 0x4d42;
    bf.bfSize = 54 + bi.biSizeImage;     
    bf.bfReserved = 0;
    bf.bfOffBits = 54;
    bf_header[0] = bf.bfType & 0xFF;
    bf_header[1] = (bf.bfType >> 8) & 0xFF;
    bf_header[2] = bf.bfSize  & 0xFF;
    bf_header[3] = (bf.bfSize >>8) & 0xFF;
    bf_header[4] = (bf.bfSize >> 16) & 0xFF;
    bf_header[5] = (bf.bfSize >>24) & 0xFF;
    bf_header[10] = bf.bfOffBits  & 0xFF;
    bf_header[11] = (bf.bfOffBits >>8) & 0xFF;
    bf_header[12] = (bf.bfOffBits >> 16) & 0xFF;
    bf_header[13] = (bf.bfOffBits >>24) & 0xFF;
    
    fp1 = fopen(BMP, "wb");
    if(!fp1)
    {
        printf("open 'BMP' error\n");
        return(FALSE);
    }
    fwrite(bf_header, 14, 1, fp1);
    fwrite(&bi, 40, 1, fp1);
    
    fwrite(rgb_date, bi.biSizeImage, 1, fp1);
    printf("save 'BMP' OK\n");
    
    fclose(fp1);

}

int openCamera(int id)
{
    char devicename[12];
    sprintf(devicename,"/dev/video%d",id);
    fd_video = open(devicename, O_RDWR | O_NONBLOCK, 0);
    if(fd_video <0 ){
        printf("open video%d fail.\n", id);
        return -1;
    }
    
    /*fd_fb = open("/dev/fb0", O_RDWR | O_NONBLOCK, 0);
    if(fd_fb <0 ){
        printf("open screen fail.\n");
        return -1;
    }*/
    return 0;
}

void capabilityCamera(void)
{
    struct v4l2_capability cap;
    //query cap
    if (ioctl(fd_video, VIDIOC_QUERYCAP, &cap) == -1) 
    {
        printf("Error opening device: unable to query device.\n");
        return (FALSE);
    }
    else
    {
        printf("driver:\t\t%s\n",cap.driver);
        printf("card:\t\t%s\n",cap.card);
        printf("bus_info:\t%s\n",cap.bus_info);
        printf("version:\t%d\n",cap.version);
        printf("capabilities:\t%x\n",cap.capabilities);
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE) 
        {
            printf("Device: supports capture.\n");
        }

        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING) 
        {
            printf("Device: supports streaming.\n");
        }
        //printf("device_caps 0x%x\n",cap.device_caps);
    } 
}

void enumfmtCamera(void)
{
    int ret;
    int i;
    v4l2_std_id std_id;
    static struct v4l2_fmtdesc fmtdesc;//video format

    do{
        ret = ioctl(fd_video, VIDIOC_QUERYSTD,&std_id);
    }while (ret == -1 && errno == EAGAIN);
    switch(std_id){
        case V4L2_STD_NTSC:
            printf("STD NTSC format:\n");
            break;
        case V4L2_STD_PAL:
            printf("STD PAL format:\n");
            break;
        default:
            printf("==STD no define format=0x%x:\n", std_id);
            break;
    }
    
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("-------------VIDIOC_ENUM_FMT--------------\n");
    while((ioctl(fd_video, VIDIOC_ENUM_FMT, &fmtdesc)) != -1)
    {
        printf("index:%d   \npixelformat:%c%c%c%c  \ndescription:%s\n",fmtdesc.index, fmtdesc.pixelformat&0xff,
            (fmtdesc.pixelformat>>8)&0xff,(fmtdesc.pixelformat>>16)&0xff,
            (fmtdesc.pixelformat>>24)&0xff,fmtdesc.description);
        fmtdesc.index++;
    }
}

/* 4??????? VIDIOC_S_FMT struct v4l2_format */
int setfmtCamera(void)
{
    int ret;
    struct v4l2_format format;
    
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//V4L2_PIX_FMT_YUYV;  // ???yuyv????
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    ret = ioctl(fd_video, VIDIOC_S_FMT, &format);
    if(ret < 0){
        printf("VIDIOC_S_FMT fail. ret=0x%x\n", ret);
        return -1;
    }
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd_video, VIDIOC_G_FMT, &format);
    if(ret < 0)
    {
        printf("VIDIOC_G_FMT fail. ret=0x%x\n", ret);
        return -1;
    }
    else
    {
        printf("fmt.type:\t\t%d\n",format.type);
        printf("pix.pixelformat:\t%c%c%c%c\n",format.fmt.pix.pixelformat & 0xFF, (format.fmt.pix.pixelformat >> 8) & 0xFF,
            (format.fmt.pix.pixelformat >> 16) & 0xFF, (format.fmt.pix.pixelformat >> 24) & 0xFF);
        printf("pix.width:\t\t%d\n",format.fmt.pix.width);
        printf("pix.height:\t\t%d\n",format.fmt.pix.height);
        printf("pix.field:\t\t%d\n",format.fmt.pix.field);
    }
    return 0;
}

int setvideofps(void)
{
    int ret; 
    struct v4l2_streamparm setfps;   
    //set fps
    memset(&setfps, 0, sizeof(struct v4l2_streamparm));
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;//10;
    setfps.parm.capture.timeperframe.denominator = 10;//10;
    ret = ioctl(fd_video, VIDIOC_S_PARM, &setfps);
    if (ret == -1){
        printf("init VIDIOC_S_PARM Err!\n");
    }
    else 
        printf("Set fps OK!ret=%d\n", ret);
   
    ret = ioctl(fd_video, VIDIOC_G_PARM, &setfps);
    if (ret == -1){
        printf("init VIDIOC_G_PARM Err!\n");
    }
    else 
        printf("Get fps OK!ret=%d\n", ret);
    printf("Set uvc frame bps: %d / %d\t[OK], type=%d\n", setfps.parm.capture.timeperframe.denominator,
        setfps.parm.capture.timeperframe.numerator, setfps.type );
    return 0;
}
/* 5??????????VIDIOC_REQBUFS struct v4l2_requestbuffers,
*  ??????????????? VIDIOC_QUERYBUF struct v4l2_buffer mmap,?????????? 
VIDIOC_QBUF 
*/
int initmmap(void)
{
    struct v4l2_requestbuffers reqbuf;
    int i, ret;

    /*pfb = (char*)mmap(NULL, IMAGEWIDTH*IMAGEHEIGHT*4, PROT_WRITE|PROT_READ,MAP_SHARED, fd_fb, 0);
    if (pfb == NULL){
        printf("mmap pfb Error!!\n");
        return FALSE;
    }*/
    //request for 4 buffers     
    reqbuf.count = VIDEOCOUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(fd_video, VIDIOC_REQBUFS, &reqbuf);
    if(-1 == ret){
        printf("VIDIOC_REQBUFS fail. ret=0x%x\n", ret);
        return -1;
    }
    if (reqbuf.count < 2)
        printf("VIDIOC_REQBUFS count=%d Err!!\n", reqbuf.count);
    
    //v4l2_buffer
    printf("----------------mmap----------------\n");
    for(i =0; i < reqbuf.count; i++){
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(fd_video, VIDIOC_QUERYBUF, &buf);
        if (-1 == ret)
        {
            printf("query buffer error. ret=0x%x\n", ret);
            return(FALSE);
        }
        framebuf[i].length = buf.length;
        framebuf[i].start = mmap(NULL, buf.length, PROT_READ|PROT_WRITE, 
            MAP_SHARED, fd_video, buf.m.offset);
        if(framebuf[i].start == MAP_FAILED){
            printf("buffer map error\n");
            return -1;
        }
        printf("start:%x  length:%d\n",(unsigned int)framebuf[i].start, framebuf[i].length);
    }
    return 0;
}


/* 6?????????   
 * ??????? VIDIOC_QBUF  struct v4l2_buffer
 * ????? VIDIOC_STREAMON
 */
static int startcap(void)
{
    int ret = -1, i = 0;
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;

    //queue
    for(i=0;i < VIDEOCOUNT; i++){
        memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(fd_video, VIDIOC_QBUF, &buf);
        if(0 != ret){
            printf("VIDIOC_QBUF fail. ret=0x%x\n", ret);
            return -1;
        }
    }
    //signal(SIGINT mysignal);//interrrupt

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd_video, VIDIOC_STREAMON, &type);
    if(0 != ret)
        printf("VIDIOC_STREAMON fail.ret=0x%x\n", ret);
    
    return 0;
}
/* 6??????????? ??poll??
 *  ??????????? VIDIOC_DQBUF
 */ 

static int readfram(void)
{
    struct pollfd pollfd;
    int ret,i;
    char filename[50];
    struct v4l2_buffer buf;
    
    //check had read frame or not, use poll fun
    printf("Check Get data:\n");
    for(i =0; i<20 ;i++){
        memset(&pollfd, 0, sizeof(pollfd));
        pollfd.fd = fd_video;
        pollfd.events = POLLIN;
        ret = poll(&pollfd, 1, 800);
        if(-1 == ret){
            printf("VIDIOC_QBUF fail. ret=0x%x\n", ret);
            return -1;
        }else if(0 == ret){
            printf("poll time out\n");
            continue;
        }
        printf("-poll revents=0x%x-\n", pollfd.revents);
        // static struct v4l2_buffer buf;

        if(pollfd.revents & POLLIN){
            printf("start get----\n");
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            ret = ioctl(fd_video, VIDIOC_DQBUF, &buf);
            if(0 != ret){
                printf("VIDIOC_DQBUF fail.ret=0x%x\n", ret);
                return -1;
            }
            //v4l2_buffer
            FILE *file = fopen(YUV, "wb");
            ret = fwrite((char*)framebuf[buf.index].start, 1, buf.length, file);
            fclose(file);

    		#if 1// RGB????
            starter = (unsigned char*)framebuf[buf.index].start;
            newBuf = (unsigned char*)calloc((unsigned int)(framebuf[buf.index].length*3/2), 1);
            yuyv_2_rgb888(starter, framebuf[buf.index].length*3/2, newBuf);
            save_bmp_file(newBuf, framebuf[buf.index].length*3/2);
    		#endif
            ret = ioctl(fd_video, VIDIOC_QBUF, &buf);
            if(0 != ret){
                printf("VIDIOC_QBUF fail. ret=0x%x\n", ret);
                return -1;
            }
        }
    }    
    return ret;
}

/* ????????????? */
static void closeCamera(void)
{
    int ret=-1, i;
    enum v4l2_buf_type type;
    
    printf("%s ####L%d\n", __func__, __LINE__);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd_video,VIDIOC_STREAMOFF, &type);
    if(0 != ret){
        printf("VIDIOC_QBUF fail. ret=0x%x\n", ret);
        return ;
    }
    for(i = 0; i < VIDEOCOUNT; i++){
        munmap(framebuf[i].start, framebuf[i].length);
    }
    close(fd_video);
    fd_video=NULL;
    //munmap(pfb, IMAGEWIDTH*IMAGEHEIGHT*4);
    //pfb = NULL;
    //close(fd_fb);
    //fd_fb=NULL;
}

int main(int argc, char **argv)
{		
	// open and initialize device
	//init_v4l2();
	//v4l2_grab();

	// start capturing
	//close_v4l2(1);
	openCamera(0);   

    capabilityCamera();
    enumfmtCamera();
    setfmtCamera();
    setvideofps();
    initmmap();
    startcap();
    readfram();
    closeCamera();
	return 0;
}
