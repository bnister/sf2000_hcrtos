/*
 *提取视频中的视频数据
 * */

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>


/*
 在帧前面添加特征码(一般SPS/PPS的帧的特征码用4字节表示，为0X00000001，其他的帧特征码用3个字节表示，为0X000001。也有都用4字节表示的，我们这里采用前面的方式)
 out是要输出的AVPaket
 sps_pps是SPS和PPS数据的指针，对于非关键帧就传NULL
 sps_pps_size是SPS/PPS数据的大小，对于非关键帧传0
 in是指向当前要处理的帧的头信息的指针
 in_size是当前要处理的帧大小(nal_size)
*/
static int alloc_and_copy(AVPacket *out,const uint8_t *sps_pps, uint32_t sps_pps_size,const uint8_t *in, uint32_t in_size)
{
	uint32_t offset = out->size; // 偏移量，就是out已有数据的大小，后面再写入数据就要从偏移量处开始操作
	// 特征码的大小，SPS/PPS占4字节，其余占3字节
	uint8_t nal_header_size = sps_pps==NULL ? 3 : 4;
	int err;

	// 每次处理前都要对out进行扩容，扩容的大小就是此次要写入的内容的大小，也就是特征码大小加上sps/pps大小加上加上本帧数据大小
	if ((err = av_grow_packet(out, sps_pps_size + in_size + nal_header_size)) < 0)
		return err;

	// 1.如果有sps_pps则先将sps_pps拷贝进out（memcpy()函数用于内存拷贝，第一个参数为拷贝要存储的地方，第二个参数是要拷贝的内容，第三个参数是拷贝内容的大小）
	if (sps_pps) {
		memcpy(out->data + offset, sps_pps, sps_pps_size);
	}

	// 2.再设置特征码（sps/pps特征码4位0x00000001，其他的特征码3位0x000001）
	for (int i = 0; i < nal_header_size; i++) {
		(out->data+offset+sps_pps_size)[i] = i==nal_header_size-1 ? 1 : 0;
	}

	// 3.最后再拷贝NALU数据(当前处理的帧数据)
	memcpy(out->data + sps_pps_size + nal_header_size + offset, in, in_size);

	return 0;
}

/*
读取并拷贝sps/pps数据
codec_extradata是codecpar的扩展数据，sps/pps数据就在这个扩展数据里面
codec_extradata_size是扩展数据大小
out_extradata是输出sps/pps数据的AVPacket包
padding:就是宏AV_INPUT_BUFFER_PADDING_SIZE的值(64)，是用于解码的输入流的末尾必要的额外字节个数，需要它主要是因为一些优化的流读取器一次读取32或者64比特，可能会读取超过size大小内存的末尾。
*/
int h264_extradata_to_annexb(const uint8_t *codec_extradata, const int codec_extradata_size, AVPacket *out_extradata, int padding)
{
	uint16_t unit_size; // sps/pps数据长度
	uint64_t total_size = 0; // 所有sps/pps数据长度加上其特征码长度后的总长度

	for (int i = 0; i < codec_extradata_size; ++i) {
		printf("%02x ",*(codec_extradata+i));
	}

	/*
	    out:是一个指向一段内存的指针，这段内存用于存放所有拷贝的sps/pps数据和其特征码数据
	    unit_nb:sps/pps个数
	    sps_done：sps数据是否已经处理完毕
	    sps_seen：是否有sps数据
	    pps_seen：是否有pps数据
	    sps_offset：sps数据的偏移，为0
	    pps_offset：pps数据的偏移，因为pps数据在sps后面，所以其偏移就是所有sps数据长度+sps的特征码所占字节数
	*/
	uint8_t *out = NULL, unit_nb, sps_done = 0,
	         sps_seen = 0, pps_seen = 0, sps_offset = 0, pps_offset = 0;
	const uint8_t *extradata = codec_extradata + 4; // 扩展数据的前4位是无用的数据，直接跳过拿到真正的扩展数据
	static const uint8_t nalu_header[4] = { 0, 0, 0, 1 }; // sps/pps数据前面的4bit的特征码

	// extradata第一个字节的最后2位用于指示后面每个sps/pps数据所占字节数。(*extradata表示extradata第一个字节的数据，之后自增1指向下一个字节)
	int length_size = (*extradata++ & 0x3) + 1;

	sps_offset = pps_offset = -1;

	// extradata第二个字节最后5位用于指示sps的个数,一般情况下一个扩展只有一个sps和pps，之后指针指向下一位
	unit_nb = *extradata++ & 0x1f;
	if (!unit_nb) { // unit_nb为0表示没有sps数据，直接跳转到处理pps的地方
		goto pps;
	} else { // unit_nb不为0表有sps数据，所以sps_seen赋值1，sps_offset赋值0
		sps_offset = 0;
		sps_seen = 1;
	}

	while (unit_nb--) { // 遍历每个sps或pps(先变量sps，然后再遍历pps)
		int err;

		// 再接着2个字节表示sps/pps数据的长度
		unit_size   = (extradata[0] << 8) | extradata[1];
		total_size += unit_size + 4; // 4表示sps/pps特征码长度
		if (total_size > INT_MAX - padding) { // total_size太大会造成数据溢出，所以要做判断
			av_log(NULL, AV_LOG_ERROR,
			       "Too big extradata size, corrupted stream or invalid MP4/AVCC bitstream\n");
			av_free(out);
			return AVERROR(EINVAL);
		}

		// extradata + 2 + unit_size比整个扩展数据都长了表明数据是异常的
		if (extradata + 2 + unit_size > codec_extradata + codec_extradata_size) {
			av_log(NULL, AV_LOG_ERROR, "Packet header is not contained in global extradata, "
			       "corrupted stream or invalid MP4/AVCC bitstream\n");
			av_free(out);
			return AVERROR(EINVAL);
		}

		// av_reallocp()函数用于内存扩展，给out扩展总长加padding的长度
		if ((err = av_reallocp(&out, total_size + padding)) < 0)
			return err;

		// 先将4字节的特征码拷贝进out
		memcpy(out + total_size - unit_size - 4, nalu_header, 4);
		// 再将sps/pps数据拷贝进out,extradata + 2是因为那2字节是表示sps/pps长度的，所以要跳过
		memcpy(out + total_size - unit_size, extradata + 2, unit_size);
		// 本次sps/pps数据处理完后，指针extradata跳过本次sps/pps数据
		extradata += 2 + unit_size;
pps:
		if (!unit_nb && !sps_done++) { // 执行到这里表明sps已经处理完了，接下来处理pps数据
			// pps的个数
			unit_nb = *extradata++;
			if (unit_nb) { // 如果pps个数大于0这给pps_seen赋值1表明数据中有pps
				pps_offset = total_size;
				pps_seen = 1;
			}
		}
	}

	if (out) // 如果out有数据，那么将out + total_size后面padding(即64)个字节用0替代
		memset(out + total_size, 0, padding);

	// 如果数据中没有sps或pps则给出提示
	if (!sps_seen)
		av_log(NULL, AV_LOG_WARNING,
		       "Warning: SPS NALU missing or invalid. "
		       "The resulting stream may not play.\n");

	if (!pps_seen)
		av_log(NULL, AV_LOG_WARNING,
		       "Warning: PPS NALU missing or invalid. "
		       "The resulting stream may not play.\n");

	// 给传进来的sps/pps的AVPaket赋值
	out_extradata->data      = out;
	out_extradata->size      = total_size;

	return length_size;
}

/*
    为包数据添加起始码、SPS/PPS等信息后写入文件。
    AVPacket数据包可能包含一帧或几帧数据，对于视频来说只有1帧，对音频来说就包含几帧
    in为要处理的数据包
    file为输出文件的指针
*/
int h264_mp4toannexb(AVFormatContext *fmt_ctx, AVPacket *in, FILE *file)
{
	AVPacket *out = NULL; // 输出的包
	AVPacket spspps_pkt; // sps/pps数据的AVPaket

	int len; // fwrite()函数写入文件时的返回值
	uint8_t unit_type; // NALU头中nal_unit_type，也就是NALU类型，5表示是I帧，7表示SPS，8表示PPS
	int32_t nal_size; // 一个NALU(也就是一帧，其第一个字节是头信息)的大小，它存放在NALU的前面的4个字节中
	uint8_t nal_size_len = 4; // 存放nal_size的字节数
	uint32_t cumul_size    = 0; // 已经处理的字节数，当cumul_size==buf_size时表示整个包的数据都处理完了
	const uint8_t *buf; // 传进来的数据指针
	const uint8_t *buf_end; // 传进来的数据末尾指针
	int buf_size; // 传进来的数据大小
	int ret = 0, i;

	out = av_packet_alloc();

	buf      = in->data;
	buf_size = in->size;
	buf_end  = in->data + in->size; // 数据首地址加上数据大小就是数据尾地址

	do {
		ret= AVERROR(EINVAL);
		if (buf + nal_size_len > buf_end) { // 说明传进来的数据没有内容，是有问题的
			goto fail;
		}

		// 取出NALU前面的4个字节得到这一帧的数据大小
		for (nal_size = 0, i = 0; i<nal_size_len; i++) {
			nal_size = (nal_size << 8) | buf[i];
		}

		buf += nal_size_len; // buf后移4位指向NALU的头信息(1个字节)
		unit_type = *buf & 0x1f; // 取出NALU头信息的后面5个bit，这5bit记录NALU的类型

		// 数据有问题就退出
		if (nal_size > buf_end - buf || nal_size < 0) {
			goto fail;
		}

		// unit_type是5表示是关键帧，对于关键帧要在其前面添加SPS和PPS信息
		if (unit_type == 5 ) {

			// 添加SPS和PPS信息，找FFmpeg中SPS和PPS信息存放在codecpar->extradata中
			h264_extradata_to_annexb( fmt_ctx->streams[in->stream_index]->codecpar->extradata,
			                          fmt_ctx->streams[in->stream_index]->codecpar->extradata_size,
			                          &spspps_pkt,
			                          AV_INPUT_BUFFER_PADDING_SIZE);

			// 为数据添加特征码(起始码，用于分隔一帧一帧的数据)
			if ((ret=alloc_and_copy(out,
			                        spspps_pkt.data, spspps_pkt.size,
			                        buf, nal_size)) < 0)
				goto fail;
		} else {
			// 非关键帧只需要添加特征码
			if ((ret=alloc_and_copy(out, NULL, 0, buf, nal_size)) < 0)
				goto fail;
		}


		buf += nal_size; // 一帧处理完后将指针移到下一帧
		cumul_size += nal_size + nal_size_len;// 累计已经处理好的数据长度
	} while (cumul_size < buf_size);

	char s[4];
	s[0] = out->size & 0xFF;
	s[1] = out->size>>8 & 0xFF;
	s[2] = out->size>>16 & 0xFF;
	s[3] = out->size>>24 & 0xFF;
	printf("out->size: %08x\n", out->size);
	printf("%02x, %02x, %02x, %02x\n", s[0] & 0xFF, s[1], s[2], s[3]);
	printf("file: %p", file);
	
	//添加每一帧的size
	len = fwrite(s, 1, 4, file);
	if(len != 4) {
		av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",len,out->size);
	}

	// SPS、PPS和特征码都添加后将其写入文件
	len = fwrite( out->data, 1, out->size, file);
	if(len != out->size) {
		av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",len,out->size);
	}
	// fwrite()只是将数据写入缓存，fflush()才将数据正在写入文件
	fflush(file);

fail:
	av_packet_free(&out); // 凡是中途处理失败退出之前都要将促使的out释放，否则会出现内存泄露

	return ret;

}


int main(int argc, char* argv[])
{
	int flag;
	char* src = NULL; // 输入文件路径
	char* dst = NULL; // 提取后文件存储路径

	AVFormatContext *fmt_ctx = NULL;
	av_log_set_level(AV_LOG_INFO);

	/*
	 *1.要提取的文件路径和提取后存储路径通过参数获取
	 * main函数第一个参数是函数名，加上2个路径参数，所以此时函数参数数量最少是3个
	 * */
	if(argc < 3) {
		av_log(NULL,AV_LOG_ERROR,"参数数量最少为3个！\n");
		return -1;
	}

	// 从参数中获取两个路径
	src = argv[1];
	dst = argv[2];
	if(!src || !dst) {
		av_log(NULL,AV_LOG_ERROR,"传入的路径不能为空！\n");
		return -1;
	}

	// 打开输入文件
	flag = avformat_open_input(&fmt_ctx,src,NULL,NULL);
	if(flag < 0) {
		av_log(NULL,AV_LOG_ERROR,"打开文件失败！\n");
		// 退出程序之前记得关闭之前打开的文件释放内存
		avformat_close_input(&fmt_ctx);
		return -1;
	}

	// 打开要写入h264的文件，没有就会创建一个
	FILE *file = fopen(dst,"wb");
	if(file == NULL) {
		// 打开失败
		av_log(NULL,AV_LOG_ERROR,"不能打开或创建存储文件！");
		// 退出之前记得关闭之前打开的熟人文件来释放内存
		avformat_close_input(&fmt_ctx);
		return -1;
	}

	// 输出信息
	av_dump_format(fmt_ctx,0,src,0);

	/*
	 * 获取最好的一路流的资源
	 *第二个参数是流的类型
	 *第三个参数是流的索引号，不知道就写-1
	 *第四个参数是相关的对应的流的索引号，比如音频对应的视频流的索引号，可以不必关心填-1
	 *第五个参数是设置的编解码器，不设置就写NULL
	 *第六个参数是一些标准，暂时不关心填0
	 *返回值是所找到的流的索引值
	 * */
	int video_idx; // 视频索引值
	video_idx = av_find_best_stream(fmt_ctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
	if(video_idx < 0) {
		av_log(NULL,AV_LOG_ERROR,"获取流失败！");
		// 退出前关闭相关文件释放内存
		avformat_close_input(&fmt_ctx);
		fclose(file);
		return -1;
	}

	// 读取流中的包数据
	AVPacket pkt;
	av_init_packet(&pkt); // 初始化pkt
	pkt.data = NULL;
	pkt.size = 0;
	// 循环读取流中所有的包(这里注意传进去的是pkt的地址)
	while(av_read_frame(fmt_ctx,&pkt) >= 0) {
		// 判断读取的包所属的流是否是我们找到的流
		if(pkt.stream_index == video_idx) {

			// 为包数据添加起始码、SPS/PPS等信息
			h264_mp4toannexb(fmt_ctx, &pkt, file);
		}
		// 因为每次循环都要为pkt分配内存，所以一轮循环结束时要释放内存
		av_packet_unref(&pkt);
	}

	avformat_close_input(&fmt_ctx);
	if(file) {
		fclose(file);
	}

	return 0;
}
