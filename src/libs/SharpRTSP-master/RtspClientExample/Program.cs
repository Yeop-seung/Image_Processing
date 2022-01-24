using Rtsp.Messages;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Security.Cryptography;
using System.Windows;
using FFmpeg.AutoGen;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;

namespace RtspClientExample
{
    class Program
    {
        //프로그램 수행을 위한 전역 변수
        static bool g_Run = false;

        public sealed unsafe class VideoDecoder
        {
            private unsafe AVCodecContext* m_codecContext;
            private unsafe AVFormatContext* m_formatContext;
            private unsafe AVFrame* m_frame;
            AVCodecID m_codecID;
            private unsafe AVCodec* m_avCodec;
            bool m_DebugFileDump = false;

            public VideoDecoder()
            {
                m_codecID = AVCodecID.AV_CODEC_ID_H264;
                unsafe
                {
                    m_avCodec = ffmpeg.avcodec_find_decoder(m_codecID);
                    m_codecContext = ffmpeg.avcodec_alloc_context3(m_avCodec);
                    ffmpeg.avcodec_open2(m_codecContext, m_avCodec, null);
                    m_frame = ffmpeg.av_frame_alloc();
                }
            }

            public int Decode(ref byte[] videoFrame)
            {
                AVPacket packet;
                int gotPicture = 0;

                unsafe
                {
                    ffmpeg.av_init_packet(&packet);

                    fixed (byte* pVideo = videoFrame)
                    {
                        packet.data = (byte*)(pVideo);
                        packet.size = videoFrame.Length;

                        int response = ffmpeg.avcodec_decode_video2(m_codecContext, m_frame, &gotPicture, &packet);

                        if (gotPicture == 1)
                        {
                            int frameSize = ffmpeg.avpicture_get_size((AVPixelFormat)m_frame->format, m_frame->width, m_frame->height);
                            Console.WriteLine("Decoded Size={0}x{1} Format={2}({3}) FrameSize={4}", 
                                        m_frame->width, m_frame->height, ffmpeg.av_get_pix_fmt_name((AVPixelFormat)m_frame->format), (int)m_frame->format, frameSize);


                            if (m_DebugFileDump==false )
                            {
                                if ((AVPixelFormat)m_frame->format == AVPixelFormat.AV_PIX_FMT_YUVJ420P)
                                {
                                    //Save YUV
                                    {
                                        int picSize = m_frame->width * m_frame->height * 3 / 2;
                                        byte[] buf = new byte[picSize];
                                        int pos = 0;

                                        Marshal.Copy((IntPtr)m_frame->data[0], buf, pos, m_frame->width * m_frame->height);
                                        pos += m_frame->width * m_frame->height;
                                        Marshal.Copy((IntPtr)m_frame->data[1], buf, pos, m_frame->width * m_frame->height / 4);
                                        pos += m_frame->width * m_frame->height / 4;
                                        Marshal.Copy((IntPtr)m_frame->data[2], buf, pos, m_frame->width * m_frame->height / 4);

                                        File.Delete("Dump.yuv");
                                        FileStream outFs = new FileStream("Dump.yuv", FileMode.CreateNew);
                                        outFs.Write(buf, 0, buf.Length);
                                        outFs.Close();

                                        Console.WriteLine("Save to Dump.yuv");
                                    }

                                    //Save BMP
                                    {
                                        VideoFrameConverter convert = new VideoFrameConverter(new System.Windows.Size(m_frame->width, m_frame->height), AVPixelFormat.AV_PIX_FMT_YUVJ420P,
                                                    new System.Windows.Size(m_frame->width, m_frame->height), AVPixelFormat.AV_PIX_FMT_RGB24);
                                        AVFrame convertedFrame = convert.Convert(*m_frame);

                                        Bitmap bitmap;

                                        bitmap = new Bitmap(convertedFrame.width, convertedFrame.height, convertedFrame.linesize[0], System.Drawing.Imaging.PixelFormat.Format24bppRgb,
                                                    (IntPtr)convertedFrame.data[0]);

                                        bitmap.Save("Dump.bmp");

                                        Console.WriteLine("Save to Dump.bmp");
                                    }
                                }

                                m_DebugFileDump = true;
                            }
                        }
                    }
                }

                return 0;
            }

        } // End of VideoDecoder        

        static void DecodingThread(object arg)
        {
            CQueueBuffer frameBuffer = (CQueueBuffer)arg;

            VideoDecoder decoder = new VideoDecoder();

            Console.WriteLine("Start Decoding Thread");
            while (g_Run)
            {
                // Queue에 들어있는 압축된 영상데이터를 꺼낸다. 
                byte[] videoFrame = frameBuffer.Pop(1000);

                // 디코딩을 한다. 
                if (videoFrame != null)
                {
                    decoder.Decode(ref videoFrame);
                }
                else
                {
                    Console.WriteLine(" <POP> EMPTY");
                }
            }
            Console.WriteLine("Stop Decoding Thread");
        }        

        class RtspReceiver
        {
            RTSPClient m_Rtsp = null;
            bool m_isH264 = false;
            bool m_isH265 = false;
            CQueueBuffer m_frameBuffer = null;
            byte[] m_spsFrame = null; // Sequence ? (추가 작성 필요)
            byte[] m_ppsFrame = null; // IDR-Frame 마다 호출
            byte[] m_vpsFrame = null; // H.265에서 호출되는 초기화 프레임 

            public RtspReceiver(ref CQueueBuffer frameBuffer)
            {
                m_Rtsp = new RTSPClient();
                m_Rtsp.Received_SPS_PPS += OnReceive_SPS_PPS;
                m_Rtsp.Received_VPS_SPS_PPS += OnReceive_VPS_SPS_PPS;
                m_Rtsp.Received_NALs += OnReceived_NALs;
                m_frameBuffer = frameBuffer;
            }

            public void Start(string url)
            {
                m_Rtsp.Connect(url, RTSPClient.RTP_TRANSPORT.TCP, RTSPClient.MEDIA_REQUEST.VIDEO_AND_AUDIO);
            }

            public void Stop()
            {
                m_Rtsp.Stop();
            }

            public bool isFinished()
            {
                return m_Rtsp.StreamingFinished();
            }

            void OnReceive_SPS_PPS(byte[] sps, byte[] pps) //H.264 Config
            {
                m_isH264 = true;

                m_spsFrame = NalCopy(ref sps);
                m_ppsFrame = NalCopy(ref pps);

                Console.WriteLine("[H.264] Buffered SPS({0}Bytes) PPS({1}Bytes)", m_spsFrame.Length, m_ppsFrame.Length);
            }

            void OnReceive_VPS_SPS_PPS(byte[] vps, byte[] sps, byte[] pps) //H.265 Config
            {
                m_isH265 = true;
                m_vpsFrame = NalCopy(ref vps);
                m_spsFrame = NalCopy(ref sps);
                m_ppsFrame = NalCopy(ref pps);

                Console.WriteLine("[H.265] Buffered VPS({0}Bytes) SPS({1}Bytes) PPS({2}Bytes)", m_vpsFrame.Length, m_spsFrame.Length, m_ppsFrame.Length);
            }
            void OnReceived_NALs(List<byte[]> nal_units) //Nal Units
            {
                byte[] videoFrame = null;

                //foreach (byte[] nal_unit in nal_units)
                for (int i = 0; i < nal_units.Count; i++)
                {
                    byte[] nal_unit = nal_units[i];
                    if (m_isH264 && nal_unit.Length > 0)
                    {
                        int nal_ref_idc = (nal_unit[0] >> 5) & 0x03;
                        int nal_unit_type = nal_unit[0] & 0x1F;
                        //String description = "";
                        if (nal_unit_type == 1)
                        {
                            //description = "NON IDR NAL";
                            videoFrame = NalCopy(ref nal_unit);
                        }
                        else if (nal_unit_type == 5)
                        {
                            //description = "IDR NAL";
                            videoFrame = MakeIDR(ref m_spsFrame, ref m_ppsFrame, ref nal_unit);
                        }
                        else if (nal_unit_type == 6)
                        {
                            //description = "SEI NAL";
                        }
                        else if (nal_unit_type == 7)
                        {
                            //description = "SPS NAL";
                            m_spsFrame = NalCopy(ref nal_unit);
                        }
                        else if (nal_unit_type == 8)
                        {
                            //description = "PPS NAL";
                            m_ppsFrame = NalCopy(ref nal_unit);
                        }
                        else if (nal_unit_type == 9)
                        {
                            //description = "ACCESS UNIT DELIMITER NAL";
                        }
                        else
                        {
                            //description = "OTHER NAL";
                        }
                        //Console.WriteLine("NAL Ref = " + nal_ref_idc + " NAL Type = " + nal_unit_type + " " + description);
                    }

                    // Output some H265 stream information
                    if (m_isH265 && nal_unit.Length > 0)
                    {
                        int nal_unit_type = (nal_unit[0] >> 1) & 0x3F;
                        //String description = "";
                        if (nal_unit_type == 1)
                        {
                            //description = "NON IDR NAL";
                            videoFrame = NalCopy(ref nal_unit);
                        }
                        else if (nal_unit_type == 19)
                        {
                            //description = "IDR NAL";
                            videoFrame = MakeIDR(ref m_vpsFrame, ref m_spsFrame, ref m_ppsFrame, ref nal_unit);
                        }
                        else if (nal_unit_type == 32)
                        {
                            //description = "VPS NAL";
                            m_vpsFrame = NalCopy(ref nal_unit);
                        }
                        else if (nal_unit_type == 33)
                        {
                            //description = "SPS NAL";
                            m_spsFrame = NalCopy(ref nal_unit);
                        }
                        else if (nal_unit_type == 34)
                        {
                            //description = "PPS NAL";
                            m_ppsFrame = NalCopy(ref nal_unit);
                        }
                        else if (nal_unit_type == 39)
                        {
                            //description = "SEI NAL";
                        }
                        else
                        {
                            //description = "OTHER NAL";
                        }
                    }

                    if (videoFrame != null)
                    {
                        //Push videoFrame                       
                        m_frameBuffer.Push(ref videoFrame);
                    }

                    videoFrame = null;
                }
            }
            byte[] NalCopy(ref byte[] src)
            {
                byte[] nalStartPrefix = new byte[] { 0x00, 0x00, 0x00, 0x01 };

                byte[] dst = new byte[nalStartPrefix.Length + src.Length];

                Buffer.BlockCopy(nalStartPrefix, 0, dst, 0, nalStartPrefix.Length);
                Buffer.BlockCopy(src, 0, dst, nalStartPrefix.Length, src.Length);

                return dst;
            }
            byte[] MakeIDR(ref byte[] spsFrame, ref byte[] ppsFrame, ref byte[] nalFrame)
            {
                byte[] nalStartPrefix = new byte[] { 0x00, 0x00, 0x00, 0x01 };

                byte[] idr = new byte[spsFrame.Length + ppsFrame.Length + nalStartPrefix.Length + nalFrame.Length];

                Buffer.BlockCopy(spsFrame, 0, idr, 0, spsFrame.Length);
                Buffer.BlockCopy(ppsFrame, 0, idr, spsFrame.Length, ppsFrame.Length);
                Buffer.BlockCopy(nalStartPrefix, 0, idr, spsFrame.Length + ppsFrame.Length, nalStartPrefix.Length);
                Buffer.BlockCopy(nalFrame, 0, idr, spsFrame.Length + ppsFrame.Length + nalStartPrefix.Length, nalFrame.Length);

                return idr;
            }
            byte[] MakeIDR(ref byte[] vpsFrame, ref byte[] spsFrame, ref byte[] ppsFrame, ref byte[] nalFrame)
            {
                byte[] nalStartPrefix = new byte[] { 0x00, 0x00, 0x00, 0x01 };

                byte[] idr = new byte[vpsFrame.Length + spsFrame.Length + ppsFrame.Length + nalStartPrefix.Length + nalFrame.Length];

                Buffer.BlockCopy(spsFrame, 0, idr, 0, vpsFrame.Length);
                Buffer.BlockCopy(spsFrame, 0, idr, vpsFrame.Length, spsFrame.Length);
                Buffer.BlockCopy(ppsFrame, 0, idr, vpsFrame.Length + spsFrame.Length, ppsFrame.Length);
                Buffer.BlockCopy(nalStartPrefix, 0, idr, vpsFrame.Length + spsFrame.Length + ppsFrame.Length, nalStartPrefix.Length);
                Buffer.BlockCopy(nalFrame, 0, idr, vpsFrame.Length + spsFrame.Length + ppsFrame.Length + nalStartPrefix.Length, nalFrame.Length);

                return idr;
            }
        }



        static void Main(string[] args)
        {
            String url = "rtsp://admin:olzetek1234@173.208.195.45:554/ISAPI/streaming/channels/01";

            //프레임 큐 버퍼 할당
            CQueueBuffer frameBuffer = new CQueueBuffer();

            //압축영상 디코딩을 위한 쓰레드 생성 
            Thread thid = null;
            g_Run = true;
            thid = new Thread(new ParameterizedThreadStart(DecodingThread));
            thid.Start(frameBuffer);

            //RTSP 초기화
            RtspReceiver rtsp = new RtspReceiver(ref frameBuffer);

            //스트리밍 시작
            rtsp.Start(url);

            Console.WriteLine("Press ENTER to exit");

            String readline = null;
            while (readline == null && rtsp.isFinished() == false)
            {
                readline = Console.ReadLine();

                // Avoid maxing out CPU on systems that instantly return null for ReadLine
                if (readline == null) Thread.Sleep(500);
            }

            rtsp.Stop();

            Thread.Sleep(1000);

            g_Run = false;
            thid.Join();
        }
    }
}
