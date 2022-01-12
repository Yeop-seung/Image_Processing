using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

// 추가 
using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Windows.Forms.Integration;
using System.Windows.Forms;
using WpfApp1.Division;

namespace WpfApp1
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        // 외부 DLL 선언
        [DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern int D3DXRenderCreate(IntPtr hWnd, int nVideoWidth, int nVideoHeight, bool bIsLock);
        [DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern void D3DXRenderClose(IntPtr pHandle, bool bIsLock);
        [DllImport("d3dxrender.dll", CallingConvention = CallingConvention.Cdecl)] static extern void D3DXRenderDraw(IntPtr pHandle, IntPtr hWnd, bool bIsLock);
        //https://post.naver.com/viewer/postView.nhn?volumeNo=14993569&memberNo=22414895&vType=VERTICAL

        private List<UserWnd> wnd = new List<UserWnd>();
        public MainWindow()
        {
            InitializeComponent();
            wnd.Clear();
            SingleDivision single = new SingleDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(single);

            wnd.Add(new UserWnd());
            single.test1.Children.Add(wnd[0].host);
        }
        private void SingleDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            SingleDivision single = new SingleDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(single);

            wnd.Add(new UserWnd());
            single.test1.Children.Add(wnd[0].host);
        }
        private void Two_Horiziontality_Division_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            TwoHoriziontalityDivision TwoHoriziontality = new TwoHoriziontalityDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(TwoHoriziontality);

            for (int i = 0; i < 2; i++)
            {
                wnd.Add(new UserWnd());
            }
            TwoHoriziontality.test1.Children.Add(wnd[0].host);
            TwoHoriziontality.test2.Children.Add(wnd[1].host);
        }
        private void Two_Perpendicular_Division_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            TwoPerpendicularDivision TwoPerpendicular = new TwoPerpendicularDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(TwoPerpendicular);

            for (int i = 0; i < 2; i++)
            {
                wnd.Add(new UserWnd());
            }
            TwoPerpendicular.test1.Children.Add(wnd[0].host);
            TwoPerpendicular.test2.Children.Add(wnd[1].host);
        }
        private void Three_Perpendicular_Division_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            ThreePerpendicularDivision ThreePerpendicular = new ThreePerpendicularDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(ThreePerpendicular);

            for (int i = 0; i < 3; i++)
            {
                wnd.Add(new UserWnd());
            }
            ThreePerpendicular.test1.Children.Add(wnd[0].host);
            ThreePerpendicular.test2.Children.Add(wnd[1].host);
            ThreePerpendicular.test3.Children.Add(wnd[2].host);
        }
        private void FourDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            FourDIvision four = new FourDIvision();
            gridtest.Children.Clear();
            gridtest.Children.Add(four);
            for (int i = 0; i < 4; i++)
            {
                wnd.Add(new UserWnd());
            }
            four.test1.Children.Add(wnd[0].host);
            four.test2.Children.Add(wnd[1].host);
            four.test3.Children.Add(wnd[2].host);
            four.test4.Children.Add(wnd[3].host);
        }
        private void SixDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            SixDivision six = new SixDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(six);

            for (int i = 0; i < 6; i++)
            {
                wnd.Add(new UserWnd());
            }
            six.test1.Children.Add(wnd[0].host);
            six.test2.Children.Add(wnd[1].host);
            six.test3.Children.Add(wnd[2].host);
            six.test4.Children.Add(wnd[3].host);
            six.test5.Children.Add(wnd[4].host);
            six.test6.Children.Add(wnd[5].host);
        }
        private void EightDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            EightDivision eight = new EightDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(eight);

            for (int i = 0; i < 8; i++)
            {
                wnd.Add(new UserWnd());
            }
            eight.test1.Children.Add(wnd[0].host);
            eight.test2.Children.Add(wnd[1].host);
            eight.test3.Children.Add(wnd[2].host);
            eight.test4.Children.Add(wnd[3].host);
            eight.test5.Children.Add(wnd[4].host);
            eight.test6.Children.Add(wnd[5].host);
            eight.test7.Children.Add(wnd[6].host);
            eight.test8.Children.Add(wnd[7].host);
        }
        private void NineDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            NineDivision nine = new NineDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(nine);
            
            for (int i = 0; i < 9; i++)
            {
                wnd.Add(new UserWnd());
            }
            nine.test1.Children.Add(wnd[0].host);
            nine.test2.Children.Add(wnd[1].host);
            nine.test3.Children.Add(wnd[2].host);
            nine.test4.Children.Add(wnd[3].host);
            nine.test5.Children.Add(wnd[4].host);
            nine.test6.Children.Add(wnd[5].host);
            nine.test7.Children.Add(wnd[6].host);
            nine.test8.Children.Add(wnd[7].host);
            nine.test9.Children.Add(wnd[8].host);
        }
        private void ThirteenDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            ThirteenDivision thirteen = new ThirteenDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(thirteen);

            for (int i = 0; i < 13; i++)
            {
                wnd.Add(new UserWnd());
            }
            thirteen.test1.Children.Add(wnd[0].host);
            thirteen.test2.Children.Add(wnd[1].host);
            thirteen.test3.Children.Add(wnd[2].host);
            thirteen.test4.Children.Add(wnd[3].host);
            thirteen.test5.Children.Add(wnd[4].host);
            thirteen.test6.Children.Add(wnd[5].host);
            thirteen.test7.Children.Add(wnd[6].host);
            thirteen.test8.Children.Add(wnd[7].host);
            thirteen.test9.Children.Add(wnd[8].host);
            thirteen.test10.Children.Add(wnd[9].host);
            thirteen.test11.Children.Add(wnd[10].host);
            thirteen.test12.Children.Add(wnd[11].host);
            thirteen.test13.Children.Add(wnd[12].host);
        }
        private void SixteenDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            SixteenDivision sixteen = new SixteenDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(sixteen);

            for (int i = 0; i < 16; i++)
            {
                wnd.Add(new UserWnd());
            }
            sixteen.test1.Children.Add(wnd[0].host);
            sixteen.test2.Children.Add(wnd[1].host);
            sixteen.test3.Children.Add(wnd[2].host);
            sixteen.test4.Children.Add(wnd[3].host);
            sixteen.test5.Children.Add(wnd[4].host);
            sixteen.test6.Children.Add(wnd[5].host);
            sixteen.test7.Children.Add(wnd[6].host);
            sixteen.test8.Children.Add(wnd[7].host);
            sixteen.test9.Children.Add(wnd[8].host);
            sixteen.test10.Children.Add(wnd[9].host);
            sixteen.test11.Children.Add(wnd[10].host);
            sixteen.test12.Children.Add(wnd[11].host);
            sixteen.test13.Children.Add(wnd[12].host);
            sixteen.test14.Children.Add(wnd[13].host);
            sixteen.test15.Children.Add(wnd[14].host);
            sixteen.test16.Children.Add(wnd[15].host);
        }
        private void TwentyfiveDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            TwentyfiveDivision twentyfive = new TwentyfiveDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(twentyfive);

            for (int i = 0; i < 25; i++)
            {
                wnd.Add(new UserWnd());
            }
            twentyfive.test1.Children.Add(wnd[0].host);
            twentyfive.test2.Children.Add(wnd[1].host);
            twentyfive.test3.Children.Add(wnd[2].host);
            twentyfive.test4.Children.Add(wnd[3].host);
            twentyfive.test5.Children.Add(wnd[4].host);
            twentyfive.test6.Children.Add(wnd[5].host);
            twentyfive.test7.Children.Add(wnd[6].host);
            twentyfive.test8.Children.Add(wnd[7].host);
            twentyfive.test9.Children.Add(wnd[8].host);
            twentyfive.test10.Children.Add(wnd[9].host);
            twentyfive.test11.Children.Add(wnd[10].host);
            twentyfive.test12.Children.Add(wnd[11].host);
            twentyfive.test13.Children.Add(wnd[12].host);
            twentyfive.test14.Children.Add(wnd[13].host);
            twentyfive.test15.Children.Add(wnd[14].host);
            twentyfive.test16.Children.Add(wnd[15].host);
            twentyfive.test17.Children.Add(wnd[16].host);
            twentyfive.test18.Children.Add(wnd[17].host);
            twentyfive.test19.Children.Add(wnd[18].host);
            twentyfive.test20.Children.Add(wnd[19].host);
            twentyfive.test21.Children.Add(wnd[20].host);
            twentyfive.test22.Children.Add(wnd[21].host);
            twentyfive.test23.Children.Add(wnd[22].host);
            twentyfive.test24.Children.Add(wnd[23].host);
            twentyfive.test25.Children.Add(wnd[24].host);
        }
        private void ThirtysixDivision_Click(object sender, RoutedEventArgs e)
        {
            wnd.Clear();
            ThirtysixDivision thirtysix = new ThirtysixDivision();
            gridtest.Children.Clear();
            gridtest.Children.Add(thirtysix);

            for (int i = 0; i < 36; i++)
            {
                wnd.Add(new UserWnd());
            }
            thirtysix.test1.Children.Add(wnd[0].host);
            thirtysix.test2.Children.Add(wnd[1].host);
            thirtysix.test3.Children.Add(wnd[2].host);
            thirtysix.test4.Children.Add(wnd[3].host);
            thirtysix.test5.Children.Add(wnd[4].host);
            thirtysix.test6.Children.Add(wnd[5].host);
            thirtysix.test7.Children.Add(wnd[6].host);
            thirtysix.test8.Children.Add(wnd[7].host);
            thirtysix.test9.Children.Add(wnd[8].host);
            thirtysix.test10.Children.Add(wnd[9].host);
            thirtysix.test11.Children.Add(wnd[10].host);
            thirtysix.test12.Children.Add(wnd[11].host);
            thirtysix.test13.Children.Add(wnd[12].host);
            thirtysix.test14.Children.Add(wnd[13].host);
            thirtysix.test15.Children.Add(wnd[14].host);
            thirtysix.test16.Children.Add(wnd[15].host);
            thirtysix.test17.Children.Add(wnd[16].host);
            thirtysix.test18.Children.Add(wnd[17].host);
            thirtysix.test19.Children.Add(wnd[18].host);
            thirtysix.test20.Children.Add(wnd[19].host);
            thirtysix.test21.Children.Add(wnd[20].host);
            thirtysix.test22.Children.Add(wnd[21].host);
            thirtysix.test23.Children.Add(wnd[22].host);
            thirtysix.test24.Children.Add(wnd[23].host);
            thirtysix.test25.Children.Add(wnd[24].host);
            thirtysix.test26.Children.Add(wnd[25].host);
            thirtysix.test27.Children.Add(wnd[26].host);
            thirtysix.test28.Children.Add(wnd[27].host);
            thirtysix.test29.Children.Add(wnd[28].host);
            thirtysix.test30.Children.Add(wnd[29].host);
            thirtysix.test31.Children.Add(wnd[30].host);
            thirtysix.test32.Children.Add(wnd[31].host);
            thirtysix.test33.Children.Add(wnd[32].host);
            thirtysix.test34.Children.Add(wnd[33].host);
            thirtysix.test35.Children.Add(wnd[34].host);
            thirtysix.test36.Children.Add(wnd[35].host);
        }

        private void Start_Click(object sender, RoutedEventArgs e)
        {
            for (int i = 0; i < wnd.Count; i++)
            {
                wnd[i].get_Test();
            }
        }

        private void RTSP_Start_Click(object sender, RoutedEventArgs e)
        {

        }

        private void RTSP_Stop_Click(object sender, RoutedEventArgs e)
        {

        }
    }
}
