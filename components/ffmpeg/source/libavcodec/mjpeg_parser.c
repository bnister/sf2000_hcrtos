/*
 * MJPEG parser
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2003 Alex Beregszaszi
 * Copyright (c) 2003-2004 Michael Niedermayer
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * MJPEG parser.
 */

#include "parser.h"
#include "mjpeg.h"
#include "get_bits.h"
#include "tiff.h"
#include "exif.h"
#include "bytestream.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct MJPEGParserContext{
    ParseContext pc;
    int size;
}MJPEGParserContext;

typedef enum MJPG_SCAN_TYPE {
	YUV444_YH1V1,
	YUV422_YH2V1,
	YUV420_YH1V2,
	YUV420_YH2V2,
	YUV411_YH4V1,
} mjpg_scan_type_e;

/**
 * Find the end of the current frame in the bitstream.
 * @return the position of the first byte of the next frame, or -1
 */
static int find_frame_end(MJPEGParserContext *m, const uint8_t *buf, int buf_size){
    ParseContext *pc= &m->pc;
    int vop_found, i;
    uint32_t state;

    vop_found= pc->frame_start_found;
    state= pc->state;

    i=0;
    if(!vop_found){
        for(i=0; i<buf_size;){
            state= (state<<8) | buf[i];
            if(state>=0xFFC00000 && state<=0xFFFEFFFF){
                if(state>=0xFFD8FFC0 && state<=0xFFD8FFFF){
                    i++;
                    vop_found=1;
                    break;
                }else if(state<0xFFD00000 || state>0xFFD9FFFF){
                    m->size= (state&0xFFFF)-1;
                }
            }
            if(m->size>0){
                int size= FFMIN(buf_size-i, m->size);
                i+=size;
                m->size-=size;
                state=0;
                continue;
            }else
                i++;
        }
    }

    if(vop_found){
        /* EOF considered as end of frame */
        if (buf_size == 0)
            return 0;
        for(; i<buf_size;){
            state= (state<<8) | buf[i];
            if(state>=0xFFC00000 && state<=0xFFFEFFFF){
                if(state>=0xFFD8FFC0 && state<=0xFFD8FFFF){
                    pc->frame_start_found=0;
                    pc->state=0;
                    return i-3;
                } else if(state<0xFFD00000 || state>0xFFD9FFFF){
                    m->size= (state&0xFFFF)-1;
                    if (m->size >= 0xF000)
                        m->size = 0;
                }
            }
            if(m->size>0){
                int size= FFMIN(buf_size-i, m->size);
                i+=size;
                m->size-=size;
                state=0;
                continue;
            }else
                i++;
        }
    }
    pc->frame_start_found= vop_found;
    pc->state= state;
    return END_NOT_FOUND;
}

static inline int mjpeg_parse_dri(GetBitContext *gb, AVCodecContext *avctx)
{
    int restart_interval = 0;
    if (get_bits_left(gb) < 32) {
        return AVERROR_INVALIDDATA;
    }

    if (get_bits(gb, 16) != 4)
        return AVERROR_INVALIDDATA;
    restart_interval = get_bits(gb, 16);
    av_log(NULL, AV_LOG_DEBUG, "restart_interval: %d\n", restart_interval);

    return 0;
}

static inline int mjpeg_parse_com(GetBitContext *gb, AVCodecContext *avctx)
{
    int len;

    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }

    len = get_bits(gb, 16);
    len -= 2;
    if (len >= 0 && 8 * len <= get_bits_left(gb)) {
        int i;
        char *cbuf = av_malloc(len - 1);
        if (!cbuf)
            return AVERROR(ENOMEM);
    
        for (i = 0; i < len - 2; i++) {
            cbuf[i] = get_bits(gb, 8);
            len--;
        }
        if (i > 0 && cbuf[i - 1] == '\n')
            cbuf[i - 1] = 0;
        else
            cbuf[i] = 0;
    
        av_log(NULL, AV_LOG_INFO, "comment: '%s'\n", cbuf);
        av_free(cbuf);
        if (len > 0)
            skip_bits(gb, 8 * len);
        return 0;
    }

    return AVERROR_INVALIDDATA;
}

static inline int mjpeg_parse_app1(GetBitContext *gb, AVCodecContext *avctx, AVCodecParserContext *s)
{
    int len, id;
    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }

    len = get_bits(gb, 16);
    len -= 2;
    if (8 * len > get_bits_left(gb))
        return AVERROR_INVALIDDATA;

    
    if (len < 4) {
        skip_bits(gb, len * 8);
        return 0;
    }

    id= get_bits_long(gb, 32);
    len -= 4;

    if (id == AV_RB32("Exif") && len >= 2) {
        const uint8_t *aligned;
        GetByteContext gbytes;
        int ret, le, ifd_offset, bytes_read;

        skip_bits(gb, 16); // skip padding
        len -= 2;

        // init byte wise reading
        aligned = align_get_bits(gb);
        bytestream2_init(&gbytes, aligned, len);

        ret = ff_tdecode_header(&gbytes, &le, &ifd_offset);
        if (ret) {
            av_log(NULL, AV_LOG_WARNING, "mjpeg: invalid TIFF header in EXIF data\n");
        } else {
            bytestream2_seek(&gbytes, ifd_offset, SEEK_SET);

            // read 0th IFD and store the metadata
            // (return values > 0 indicate the presence of subimage metadata)
            ret = ff_exif_decode_ifd(avctx, &gbytes, le, 0, &s->metadata);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "mjpeg: error decoding EXIF data\n");
            } else {
                //if (av_dict_count(s->metadata)) {
                //    AVDictionaryEntry *t = NULL;
                //    while ((t = av_dict_get(s->metadata, "", t, AV_DICT_IGNORE_SUFFIX))) {
                //        av_log(NULL, AV_LOG_DEBUG, "%s: ", t->key);
                //        av_log(NULL, AV_LOG_DEBUG, "%s\n", t->value);
                //    }
                //}
            }
        }

        bytes_read = bytestream2_tell(&gbytes);
        skip_bits(gb, bytes_read << 3);
        len -= bytes_read;
    }

    if (len > 0)
        skip_bits(gb, 8 * len);
    return 0;
}

static inline int mjpeg_parse_skip_marker(GetBitContext *gb, AVCodecContext *avctx)
{
    int len;

    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }
    len = get_bits(gb, 16);
    len -= 2;
    if (len >= 0 && 8 * len <= get_bits_left(gb)) {
        if (len > 0)
            skip_bits(gb, 8 * len);
        return 0;
    } else {
        return AVERROR_INVALIDDATA;
    }
}

static inline int mjpeg_parse_sof (GetBitContext *gb, AVCodecContext *avctx, int lossless, int progressive)
{
    /* Number of components in image (1 for gray, 3 for YUV, etc.) */
    uint8_t nb_components = 0;
    /* precision (in bits) for the samples */
    uint8_t precision;            
    /* unique value identifying each component */
    uint8_t component_id[4];   
    /* quantization table ID to use for this comp */
    uint8_t quant_index[4];              
    /* Array[numComponents] giving the number of blocks (horiz) in this component */
    uint8_t h_count[4] = {0};
    int h_max = 1;
    /* Same for the vertical part of this component */
    uint8_t v_count[4] = {0};
    /* max val in v_count*/
    int v_max = 1;

    int i;
    uint8_t value = 0;
    int temp;
    uint16_t len = 0;
    int hw_support = 1;
    int rgb;
    unsigned pix_fmt_id;

    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }

    len = get_bits(gb, 16);
    len -= 2;
    if (len < 4 || 8 * len > get_bits_left(gb)) {
        return AVERROR_INVALIDDATA;
    }

    /* Get sample precision */
    precision = get_bits(gb, 8);len--;
    avctx->coded_height = avctx->height = get_bits(gb, 16);len -= 2;
    avctx->coded_width = avctx->width = get_bits(gb, 16);len -= 2;
    av_log(NULL, AV_LOG_VERBOSE, 
        "precision: %d, width %d, height %d\n",
        precision, avctx->height, avctx->width);

    /* Get number of components */
    nb_components = get_bits(gb, 8);len--;
    if (nb_components > 4 || len < 3 * nb_components) {
        return AVERROR_INVALIDDATA;
    }

    /* Get decimation and quantization table id for each component */
    for (i = 0; i < nb_components; i++) {
        /* Get component ID number */
        component_id[i] = get_bits(gb, 8) - 1;len--;

        /* Get decimation */
        value = get_bits(gb, 8);len--;
        h_count[i] = (value & 0xf0) >> 4;
        h_max = max(h_count[i], h_max);
        v_count[i] = (value & 0x0f);
        v_max = max(v_count[i], v_max);

        /* Get quantization table id */
        quant_index[i] = get_bits(gb, 8);len--;
        av_log(NULL, AV_LOG_VERBOSE, 
            "component_id[%d]: %d, quant_index[%d]: %d,"
            "h_count[%d]: %d, v_count[%d]: %d\n",
            i, component_id[i], i, quant_index[i], i, h_count[i], i, v_count[i]);
    }

    if (progressive) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support progressive\n");
        hw_support = 0;
    }

    if (nb_components == 1) {
        //GRAYSCALE/monochrome support hw huffman.
    }

    if (v_max == 1 && h_max == 1 && lossless==1 && (nb_components==3 || nb_components == 4))
        rgb = 1;
    else if (!lossless)
        rgb = 0;

    av_log(NULL, AV_LOG_DEBUG, "rgb %d\n", rgb);
    if (!rgb){
        //hw huffman only support: Y 1x1,2x1,1x2,2x2, UV 1x1
        if(h_count[1] == 2 || v_count[1] == 2 
            || h_count[2]== 2 || v_count[2] == 2) {
            av_log(NULL, AV_LOG_VERBOSE, "hw only support Y 1x1,2x1,1x2,2x2, UV 1x1,"
                "but currrent U %dx%d, current V %dx%d\n", h_count[1], v_count[1], h_count[2]== 2, v_count[2]);
            hw_support = 0;
        }
    }

    if (nb_components == 4 && component_id[0] == 'C' - 1 && component_id[1] == 'M' - 1
        && component_id[2] == 'Y' - 1 && component_id[3] == 'K' - 1) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support CMYK\n");
        hw_support = 0;
    }

    pix_fmt_id= ((unsigned)h_count[0] << 28) | (v_count[0] << 24) |
        (h_count[1] << 20) | (v_count[1] << 16) |
        (h_count[2] << 12) | (v_count[2] << 8) |
        (h_count[3] << 4) | v_count[3];

    if (0x11111100 == pix_fmt_id &&
        component_id[0] == 'R' - 1 && component_id[1] == 'G' - 1 && component_id[2] == 'B' - 1) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support rgb\n");
        hw_support = 0;
    }


    if (0x11111111 == pix_fmt_id && rgb) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support rgba\n");
        hw_support = 0;
    }

    if (hw_support) {
        avctx->pic_hw_support = 1;
        if (1 == h_count[0] && 1 == v_count[0]) {
            avctx->pix_fmt = YUV444_YH1V1;
        } else if (2 == h_count[0] && 1 == v_count[0]) {
            avctx->pix_fmt = YUV422_YH2V1;
        } else if (1 == h_count[0] && 2 == v_count[0]) {
            avctx->pix_fmt = YUV420_YH1V2;
        } else if (2 == h_count[0] && 2 == v_count[0]) {
            avctx->pix_fmt = YUV420_YH2V2;
        } else if (4 == h_count[0] && 1 == v_count[0]) {
            avctx->pix_fmt = YUV411_YH4V1;
        }
    }
    av_log(NULL, AV_LOG_VERBOSE, "hw_support %d\n", hw_support);

    if (len > 0)
        skip_bits(gb, 8 * len);
    return 0;
}

static int jpeg_parse_info(AVCodecParserContext *s, AVCodecContext *avctx, const uint8_t *buf, int buf_size)
{
    int ret;
    GetBitContext gb;
    uint8_t marker;
    int foundSOF = 0;

    if (!buf || buf_size <= 0) {
        return AVERROR_INVALIDDATA;
    }

    av_log(NULL, AV_LOG_TRACE, "avctx->coded_width %d\n", avctx->coded_width);
    if (avctx->coded_width != 0) {
        return 0;
    }

    ret = init_get_bits8(&gb, buf, buf_size);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "jpeg parser invalid buffer\n");
        return -1;
    }

    marker = get_bits(&gb, 8);
    while(marker == 0xff) {
        marker = get_bits(&gb, 8);

        av_log(NULL, AV_LOG_TRACE, "marker = %x\n", marker);
        
        switch (marker) {
            case SOS:
                /* start of scan (begins compressed data) */
                goto done;

            case SOI:
                break;

            case DRI: {
                if(mjpeg_parse_dri(&gb, avctx))
                    return AVERROR_INVALIDDATA;
                break;
            }
  
            case COM: {
                if(mjpeg_parse_com(&gb, avctx))
                    return AVERROR_INVALIDDATA;
                break;
            }
        
            case DHT:
            case DQT:
                /* Ignore these codes */
                if (mjpeg_parse_skip_marker (&gb, avctx))
                    return AVERROR_INVALIDDATA;
                break;
        
            case SOF0:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_BASELINE_DCT;
                foundSOF = 1;
                if (mjpeg_parse_sof (&gb, avctx, 0, 0))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF1:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_EXTENDED_SEQUENTIAL_DCT;
                foundSOF = 1;
                if (mjpeg_parse_sof (&gb, avctx, 0, 0))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF2:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_PROGRESSIVE_DCT;
                foundSOF = 1;
                if (mjpeg_parse_sof (&gb, avctx, 0, 1))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF3:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_LOSSLESS;
                foundSOF = 1;
                if (mjpeg_parse_sof (&gb, avctx, 1, 0))
                    return AVERROR_INVALIDDATA;
                break;

            case APP1:
                if (mjpeg_parse_app1 (&gb, avctx, s))
                    return AVERROR_INVALIDDATA;
                break;

            default:
                if (marker == JPG || (marker >= JPG0 && marker <= JPG13) ||
                    (marker >= APP0 && marker <= APP15)) {
                    if (mjpeg_parse_skip_marker (&gb, avctx))
                        return AVERROR_INVALIDDATA;
                } else {
                    av_log(NULL, AV_LOG_ERROR, "unknow marker = %x\n", marker);
                    if (mjpeg_parse_skip_marker (&gb, avctx))
                        return AVERROR_INVALIDDATA;
                }
        }

        if (get_bits_left(&gb) < 8) {
            return AVERROR_INVALIDDATA;
        }

        marker = get_bits(&gb, 8);
    }

done:
    return foundSOF ? 0 : AVERROR_INVALIDDATA;
}

static int jpeg_parse(AVCodecParserContext *s,
                      AVCodecContext *avctx,
                      const uint8_t **poutbuf, int *poutbuf_size,
                      const uint8_t *buf, int buf_size)
{
    MJPEGParserContext *m = s->priv_data;
    ParseContext *pc = &m->pc;
    int next = buf_size;

    if (poutbuf && poutbuf_size) {
        if(s->flags & PARSER_FLAG_COMPLETE_FRAMES){
            next= buf_size;
        }else{
            next= find_frame_end(m, buf, buf_size);

            if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
                *poutbuf = NULL;
                *poutbuf_size = 0;
                return buf_size;
            }
        }

        *poutbuf = buf;
        *poutbuf_size = buf_size;
    }

    jpeg_parse_info(s, avctx, buf, buf_size);

    return next;
}


AVCodecParser ff_mjpeg_parser = {
    .codec_ids      = { AV_CODEC_ID_MJPEG, AV_CODEC_ID_JPEGLS },
    .priv_data_size = sizeof(MJPEGParserContext),
    .parser_parse   = jpeg_parse,
    .parser_close   = ff_parse_close,
};
