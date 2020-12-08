/**
 * description:   <br>
 * @author 秦城季
 * @date 2020/12/7
 */

#include "VideoTask.h"

void VideoTask::setDataCallback(VideoCallback callback) {
    this->callback = callback;
}

VideoTask::VideoTask(int fps, int bitRate) {
    this->fps = fps;
    this->bitRate = bitRate;
    pthread_mutex_init(&mutex, 0);
}

void VideoTask::dataChange(int width, int height) {
    this->ySize = height * width;
    this->uvSize = this->ySize / 4;

    if (pic_in) {
        x264_picture_clean(pic_in);
        delete pic_in;
    }

    //初始化VLC图片编码层的参数
    pic_in = new x264_picture_t;

    x264_param_t param;
    x264_param_default(&param);
    //根据应用场景设置编码速度，以及编码质量。2：x264_preset_names，3：x264_tune_names
    x264_param_default_preset(&param, x264_preset_names[0], x264_tune_names[7]);
    //输入数据格式， yuv 4:2:0
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;

    //base_line 3.2 编码规格，影响网络带宽，图像分辨率等。 -- https://en.wikipedia.org/wiki/Advanced_Video_Coding
    param.i_level_idc = 32;
    //两张参考图片间b帧的数量
    param.i_bframe = 0;
    //参数i_rc_method表示码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    param.rc.i_rc_method = X264_RC_ABR;
    //比特率(码率, 单位Kbps)
    param.rc.i_bitrate = bitRate / 1000;
    //瞬时最大码率
    param.rc.i_vbv_max_bitrate = bitRate / 1000 * 1.2;
    //设置了i_vbv_max_bitrate必须设置此参数，码率控制区大小,单位kbps
    param.rc.i_vbv_buffer_size = bitRate / 1000;
    //帧率（每秒显示多少张画面）
    param.i_fps_num = fps;
    param.i_fps_den = 1;
    param.i_timebase_den = param.i_fps_num;
    param.i_timebase_num = param.i_fps_den;
//    param.pf_log = x264_log_default2;
    //用fps而不是时间戳来计算帧间距离
    param.b_vfr_input = 0;
    //帧距离(关键帧)  2s一个关键帧
    param.i_keyint_max = fps * 2;
    // 是否复制sps和pps放在每个关键帧的前面 该参数设置是让每个关键帧(I帧)都附带sps/pps。
    param.b_repeat_headers = 1;
    //多线程
    param.i_threads = 1;

    x264_param_apply_profile(&param, "baseline");
    //打开编码器
    videoCodec = x264_encoder_open(&param);

    x264_picture_alloc(pic_in, X264_CSP_I420, width, height);

    pthread_mutex_unlock(&mutex);
}

void VideoTask::encodeData(int8_t *data) {
    pthread_mutex_lock(&mutex);
    //传递过来的数据时nv21，需要转成i420；
    memcpy(pic_in->img.plane[0], data, ySize);
    for (int i = 0; i < uvSize; ++i) {
        //u数据
        *(pic_in->img.plane[1] + i) = *(data + ySize + i * 2 + 1);
        *(pic_in->img.plane[2] + i) = *(data + ySize + i * 2);
    }

    //编码出来的数据  （帧数据）
    x264_nal_t *pp_nal;
    //编码出来有几个数据 （多少帧）
    int pi_nal;
    x264_picture_t pic_out;
    x264_encoder_encode(videoCodec, &pp_nal, &pi_nal, pic_in, &pic_out);
    //如果是关键帧 3
    int sps_len;
    int pps_len;
    uint8_t sps[100];
    uint8_t pps[100];
    for (int i = 0; i < pi_nal; ++i) {
        if (pp_nal[i].i_type == NAL_SPS) {
            sps_len = pp_nal[i].i_payload - 4;
            memcpy(sps, pp_nal[i].p_payload + 4, sps_len);
        } else if (pp_nal[i].i_type == NAL_PPS) {
            pps_len = pp_nal[i].i_payload - 4;
            memcpy(pps, pp_nal[i].p_payload + 4, pps_len);
            //pps肯定是跟着sps的
            sendSpsPps(sps, pps, sps_len, pps_len);
        } else {
            sendFrame(pp_nal[i].i_type, pp_nal[i].p_payload, pp_nal[i].i_payload);
        }
    }

    pthread_mutex_unlock(&mutex);
}


/**
aligned(8) class AVCDecoderConfigurationRecord {
 unsigned int(8) configurationVersion = 1;
 unsigned int(8) AVCProfileIndication;
 unsigned int(8) profile_compatibility;
 unsigned int(8) AVCLevelIndication;
 bit(6) reserved = ‘111111’b;
 unsigned int(2) lengthSizeMinusOne;
 bit(3) reserved = ‘111’b;
 unsigned int(5) numOfSequenceParameterSets;
 for (i=0; i< numOfSequenceParameterSets; i++) {
 unsigned int(16) sequenceParameterSetLength ;
 bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
 }
 unsigned int(8) numOfPictureParameterSets;
 for (i=0; i< numOfPictureParameterSets; i++) {
 unsigned int(16) pictureParameterSetLength;
 bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
 }
 if( profile_idc == 100 || profile_idc == 110 ||
 profile_idc == 122 || profile_idc == 144 )
 {
 bit(6) reserved = ‘111111’b;
 unsigned int(2) chroma_format;
 bit(5) reserved = ‘11111’b;
 unsigned int(3) bit_depth_luma_minus8;
 bit(5) reserved = ‘11111’b;
 unsigned int(3) bit_depth_chroma_minus8;
 unsigned int(8) numOfSequenceParameterSetExt;
 for (i=0; i< numOfSequenceParameterSetExt; i++) {
 unsigned int(16) sequenceParameterSetExtLength;
 bit(8*sequenceParameterSetExtLength) sequenceParameterSetExtNALUnit;
 }
 }
 */
void VideoTask::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    //看表
    int bodySize = 13 + sps_len + 3 + pps_len;
    RTMPPacket *packet = new RTMPPacket;
    //
    RTMPPacket_Alloc(packet, bodySize);
    int i = 0;
    //固定头：0x17 = 0001 0111；高4位为1表示关键帧（FrameType），低4位为7表示高级视频编码(CodecID)
    packet->m_body[i++] = 0x17;
    //下面为VideoData里面的数据
    //AVCPacketType（类型）  AVC Sequence header
    packet->m_body[i++] = 0x00;
    //composition time 0x000000（合成时间）
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    //Data (视频数据)；类型不同，数据不同；参考：AVCDecoderConfigurationRecord
    //版本
    packet->m_body[i++] = 0x01; //configurationVersion
    //编码规格
    packet->m_body[i++] = sps[1]; //AVCProfileIndication
    packet->m_body[i++] = sps[2]; //profile_compatibility
    packet->m_body[i++] = sps[3]; //AVCLevelIndication
    packet->m_body[i++] = 0xFF;    //bit(6) reserved = ‘111111’b;  unsigned int(2) lengthSizeMinusOne;

    //整个sps = 1110 00001 -->1个sps
    packet->m_body[i++] = 0xE1;  // bit(3) reserved = ‘111’b;  unsigned int(5) numOfSequenceParameterSets;
    //sps长度--> unsigned int(16) sequenceParameterSetLength ;  占2个字节存储sps的长度；
    packet->m_body[i++] = (sps_len >> 8) & 0xff;  //如：0xmmnn，把高位的值存到第一个字节0xmm
    packet->m_body[i++] = sps_len & 0xff;          //如：0xmmnn，把低位的值存到第二个字节0xnn

    //sps的内容-->bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
    memcpy(&packet->m_body[i], sps, sps_len);
    i += sps_len;

    //pps
    packet->m_body[i++] = 0x01;  //unsigned int(8) numOfPictureParameterSets; pps个数
    //pps长度--> unsigned int(16) pictureParameterSetLength; 占2个字节存储pps的长度；
    packet->m_body[i++] = (pps_len >> 8) & 0xff;
    packet->m_body[i++] = (pps_len) & 0xff;
    //pps的内容-->bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
    memcpy(&packet->m_body[i], pps, pps_len);

    //对应着rtmp的Chunk Message Header中的message type id字段
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;//视频
    packet->m_nBodySize = bodySize;
    //随意分配一个管道（尽量避开rtmp.c中使用的）
    packet->m_nChannel = 10;
    //sps pps没有时间戳
    packet->m_nTimeStamp = 0;
    //不使用绝对时间
    packet->m_hasAbsTimestamp = 0;

    //对应着rtmp的Chunk Basic Header中的chunk type(fmt)字段，注意数据大小
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    //    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

    callback(packet);
}

void VideoTask::sendFrame(int type, uint8_t *payload, int i_payload) {
    if (payload[2] == 0x00) {
        i_payload -= 4;
        payload += 4;
    } else {
        i_payload -= 3;
        payload += 3;
    }
    //看表
    int bodySize = 9 + i_payload;
    RTMPPacket *packet = new RTMPPacket;
    //
    RTMPPacket_Alloc(packet, bodySize);
    //固定头：0x17 = 0001 0111；高4位为1表示关键帧（FrameType），低4位为7表示高级视频编码(CodecID)
    packet->m_body[0] = 0x27;
    if (type == NAL_SLICE_IDR) {
        packet->m_body[0] = 0x17;
        LOGD("关键帧");
    }
    //类型 AVC NALU
    packet->m_body[1] = 0x01;
    //时间戳
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    //数据长度 int 4个字节
    packet->m_body[5] = (i_payload >> 24) & 0xff;
    packet->m_body[6] = (i_payload >> 16) & 0xff;
    packet->m_body[7] = (i_payload >> 8) & 0xff;
    packet->m_body[8] = (i_payload) & 0xff;

    //图片数据
    memcpy(&packet->m_body[9], payload, i_payload);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x10;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
//    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    callback(packet);
}
