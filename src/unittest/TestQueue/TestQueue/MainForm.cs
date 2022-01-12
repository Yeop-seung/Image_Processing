using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Windows.Forms;

namespace TestQueue
{
    public partial class MainForm : Form
    {
        CQueueBuffer m_Buffer;
        int m_PushIndex;
        Thread m_Thid;
        bool m_Run;

        public MainForm()
        {
            InitializeComponent();

            m_Buffer = new CQueueBuffer();
            m_PushIndex = 0;
            m_Thid = null;
            m_Run = false;
        }

        private void btnPush_Click(object sender, EventArgs e)
        {
            CQueueBuffer.FrameInfo frame = new CQueueBuffer.FrameInfo();

            string str = string.Format("Data Index #{0}", m_PushIndex);
            m_PushIndex++;

            frame.data = Encoding.Default.GetBytes(str);
            frame.size = frame.data.Length;

            m_Buffer.Push(ref frame);

            Console.WriteLine("<PUSH> {0}", str);
        }

        void ThreadPop()
        {
            while (m_Run)
            {
                CQueueBuffer.FrameInfo frame = m_Buffer.Pop(1000);

                if (frame.size > 0)
                {
                    string str = Encoding.Default.GetString(frame.data);
                    Console.WriteLine(" <POP> {0}", str);
                }
                else
                {
                    Console.WriteLine(" <POP> EMPTY");
                }
            }
        }

        private void btnPop_Click(object sender, EventArgs e)
        {
            if (btnPop.Text == "Start Pop")
            {
                m_Run = true;
                m_Thid = new Thread(new ThreadStart(ThreadPop));
                m_Thid.Start();
                btnPop.Text = "Stop Pop";
            }
            else
            {
                m_Run = false;
                m_Thid.Join();
                m_Thid = null;
                btnPop.Text = "Start Pop";
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if(m_Thid != null)
            {
                m_Run = false;
                m_Thid.Join();
                m_Thid = null;
            }
        }
    }
}
