using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

// 추가 
using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Windows.Forms.Integration;
using System.Windows.Forms;

using RtspClientExample;

namespace WpfApp1
//namespace RtspClientExample
{
    /// <summary>
    /// UserWnd.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class UserWnd : System.Windows.Controls.UserControl
    {
        // 외부 DLL 선언
        //[DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern IntPtr D3DXRenderCreate(IntPtr hWnd, int nVideoWidth, int nVideoHeight, bool bIsLock);
        //[DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern void D3DXRenderClose(IntPtr pHandle, bool bIsLock);
        //        [DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern void D3DXRenderDraw(IntPtr pHandle, IntPtr hWnd, IntPtr data, int nSize, int nVideoWidth, int nVideoHeight, bool bIsLock);
        //[DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern void D3DXRenderDraw([MarshalAs(UnmanagedType.SysUInt)] IntPtr pHandle, IntPtr hWnd, ref IntPtr data, int nSize, int nVideoWidth, int nVideoHeight, bool bIsLock);
        //https://post.naver.com/viewer/postView.nhn?volumeNo=14993569&memberNo=22414895&vType=VERTICAL
        // 
        public IntPtr obj = (IntPtr)0;
        private IntPtr winHandle;
        public WindowsFormsHost host = new WindowsFormsHost();
        private PictureBox box = new PictureBox();

        //RtspClientExample.Channel ch = new RtspClientExample.Channel(0);
        RtspClientExample.Channel ch;


        public UserWnd()
        {
            InitializeComponent();
            box.BorderStyle = BorderStyle.FixedSingle;
            host.Child = box;
        }
        public void get_Test()
        {

            winHandle = box.Handle;
            unsafe
            {
                if (obj == IntPtr.Zero)
                {
                    ch = new RtspClientExample.Channel(0);
                    ch.ServiceStart(winHandle);
                    //obj = (IntPtr)D3DXRenderCreate(handle, 720, 480, false);
                }
                //D3DXRenderDraw(obj, handle, ref obj, 720 * 480 * 3 / 2, 720, 480, false);
            }   
        }
        public void Test_Stop()
        {
            ch.ServiceStop(winHandle);
        }
    }
}