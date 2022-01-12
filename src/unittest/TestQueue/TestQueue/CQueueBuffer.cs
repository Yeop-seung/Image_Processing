using System;
using System.Collections.Generic;
using System.Collections;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace TestQueue
{
    class CQueueBuffer
    {
        
        public struct FrameInfo
        {
            public byte[] data;
            public int size;
        };
        
        public int Push(ref FrameInfo frame)
        {
            Lock();
            m_Queue.Add(frame);
            m_Event.Set();
            Unlock();
            
            return 0;
        }

        public FrameInfo Pop(int timeoutMs)
        {
            Lock();

            if (m_Queue.Count < 1)
            {
                m_Event.Reset();
                Unlock();
                
                bool retv = m_Event.WaitOne(timeoutMs);

                if (retv == false)
                {
                    return default(FrameInfo);
                }

                Lock();
            }

            FrameInfo frame = (FrameInfo)m_Queue[0];
            m_Queue.RemoveAt(0);

            Unlock();

            return frame;
        }
        
        ArrayList m_Queue;
        Mutex m_Mutex;
        AutoResetEvent m_Event;


        public CQueueBuffer()
        {
            m_Queue = new ArrayList();
            m_Mutex = new Mutex();
            m_Event = new AutoResetEvent(false);
        }

        void Lock()
        {
            m_Mutex.WaitOne();
        }

        void Unlock()
        {
            m_Mutex.ReleaseMutex();
        }


    }
}
