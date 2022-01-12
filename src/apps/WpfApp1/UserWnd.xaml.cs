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
namespace WpfApp1
{
    /// <summary>
    /// UserWnd.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class UserWnd : System.Windows.Controls.UserControl
    {
        // 외부 DLL 선언
        [DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern int D3DXRenderCreate(IntPtr hWnd, int nVideoWidth, int nVideoHeight, bool bIsLock);
        [DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern void D3DXRenderClose(IntPtr pHandle, bool bIsLock);
        [DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern void D3DXRenderDraw(IntPtr pHandle, IntPtr hWnd, bool bIsLock);
        //https://post.naver.com/viewer/postView.nhn?volumeNo=14993569&memberNo=22414895&vType=VERTICAL
        // 
        private IntPtr obj = (IntPtr)0;
        private IntPtr handle;
        public WindowsFormsHost host = new WindowsFormsHost();
        private PictureBox box = new PictureBox();
        public UserWnd()
        {
            InitializeComponent();
            box.BorderStyle = BorderStyle.FixedSingle;
            host.Child = box;
        }
        public void get_Test()
        {
            handle = box.Handle;
            if(obj == IntPtr.Zero) { 
            obj = (IntPtr)D3DXRenderCreate(handle, 720, 480, false);
            }
            D3DXRenderDraw(obj, handle, false);
        }
    }
}