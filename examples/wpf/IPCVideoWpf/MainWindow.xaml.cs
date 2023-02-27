using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data.Common;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Intrinsics.X86;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.DirectoryServices.ActiveDirectory;
using System.Windows.Markup;
using System.Timers;
using System.Runtime.ConstrainedExecution;
using static System.Net.Mime.MediaTypeNames;
using System.Security.Principal;

namespace IPCVideoWpf
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        internal class IpcVideoDll
        {
            [DllImport("ipcvideo.dll", CallingConvention = CallingConvention.Cdecl)] extern public static int ipcv_open(StringBuilder sIpcName);
            [DllImport("ipcvideo.dll", CallingConvention = CallingConvention.Cdecl)] extern public static int ipcv_close();
            [DllImport("ipcvideo.dll", CallingConvention = CallingConvention.Cdecl)] extern public static int ipcv_readShareVideo(
                            int ch, IntPtr pDestImgPtr,int bufSize, 
                            out int pImgWidth, out int pImgHeight, out int pImgDepth,
                            out int pPixelFormat, out int pFrameCount, out double pLatency);
            [DllImport("ipcvideo.dll", CallingConvention = CallingConvention.Cdecl)] extern public static int ipcv_writeShareVideo(
                int ch, IntPtr pDestImgPtr, int bufSize, int imgWidth, int imgHeight, int imgDepth, int pixelFormat,out int frameCount);

            [DllImport("ipcvideo.dll", CallingConvention = CallingConvention.Cdecl)] extern public static int ipcv_getImageInfo(
                            int ch, out int pImgWidth, out int pImgHeight, out int pImgDepth, out int pPixelFmt);

            public const int IPCV_PixelFormat_BGR24 = 0; // Default
            public const int IPCV_PixelFormat_Gray  = 1;
        }

        BackgroundWorker workerIpcvideoRx= new BackgroundWorker();
        DispatcherTimer timerIpcVideoTx = new DispatcherTimer();


        int  ipc_channel_rx =0;
        int  ipc_channel_tx=1;
        bool run_worker_ipcvideorx = false;
        bool reserved_StartIPCService=false;

        static WriteableBitmap bmpRx ;
        static int bmpRx_pixelFormat = 0;
        static WriteableBitmap bmpTx ;        

        private bool IsAdministrator()
        {
            WindowsIdentity identity = WindowsIdentity.GetCurrent();
            if (null != identity)
            {
                WindowsPrincipal principal = new WindowsPrincipal(identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
            return false;
        }


        public MainWindow()
        {
            InitializeComponent();
        }

        bool stopIPCService()
        {
            if( workerIpcvideoRx.IsBusy ){
                workerIpcvideoRx.CancelAsync();

            }
            timerIpcVideoTx.Stop();
            return (IpcVideoDll.ipcv_close()>0);
        }

        void startIPCService()
        {
            stopIPCService();

            if( workerIpcvideoRx.IsBusy )
            {
                reserved_StartIPCService = true;
            }
            else
            {
                reserved_StartIPCService = false;
                if ( comboxIpcName.SelectedIndex >= 0 )
                {
                    String sIpcName = comboxIpcName.Items[comboxIpcName.SelectedIndex].ToString();
                    StringBuilder sIPCName = new StringBuilder(comboxIpcName.Items[comboxIpcName.SelectedIndex].ToString());
                    int resultCode = IpcVideoDll.ipcv_open(sIPCName);
                    if (resultCode == 1)
                    {
                        labelIPCVideoOpenStatus.Content = "IPC video share service has started";
                        workerIpcvideoRx.RunWorkerAsync();
                        timerIpcVideoTx.Interval = TimeSpan.FromSeconds(Convert.ToDouble(comboxTxInterval.Text));
                        timerIpcVideoTx.Tick += new EventHandler(timer_IpcVideoTick);
                        timerIpcVideoTx.Start();
                    }
                    else
                    {
                        if( sIpcName.Contains("Global") && !IsAdministrator() )
                        {
                            labelIPCVideoOpenStatus.Content = "IPC Video Sharing Service Failed to Start.(Execute with administrator privileges)";
                        }
                        else {
                            labelIPCVideoOpenStatus.Content = "IPC Video Sharing Service Failed to Start";
                        }                       
                    }
                }
                else
                {
                    labelIPCVideoOpenStatus.Content = "IPC Service not Selected";
                }
            }

        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            comboxIpcName.Items.Add("IPCVIDEO");
            comboxIpcName.Items.Add("Global\\IPCVIDEO");

            comboxTxChannel.Items.Add("0");
            comboxTxChannel.Items.Add("1");

            comboxRxChannel.Items.Add("0");
            comboxRxChannel.Items.Add("1");

            comboxTxInterval.Items.Add("0.01");
            comboxTxInterval.Items.Add("0.03");
            comboxTxInterval.Items.Add("0.05");
            comboxTxInterval.Items.Add("0.10");
            comboxTxInterval.Items.Add("1.00");

            bmpTx = new WriteableBitmap(1024, 768, 96, 96, PixelFormats.Bgr24, null);

            workerIpcvideoRx.DoWork += new DoWorkEventHandler(worker_DoWork_ipcvideo_rx);
            workerIpcvideoRx.WorkerSupportsCancellation = true;

            workerIpcvideoRx.RunWorkerCompleted += new RunWorkerCompletedEventHandler(worker_RunWorkerCompleted);

            comboxTxChannel.SelectedIndex = ipc_channel_tx = 0;
            comboxRxChannel.SelectedIndex = ipc_channel_rx = 1;
            comboxTxInterval.SelectedIndex = 0;
            comboxIpcName.SelectedIndex = 0;  // comboxIpcName 이 선택되면 IPC Service Start
        }

        void worker_DoWork_ipcvideo_rx(object sender, DoWorkEventArgs e)
        {
            run_worker_ipcvideorx = true;

            int imgWidth  = 0;
            int imgHeight = 0;
            int imgDepth = 0;
            int pixelFmt = 0;
            int frameCount = 0;
            double latency = 0;
            int false_return = 0;
            byte[] buffer = new byte[1027*768*3];

            while (run_worker_ipcvideorx) 
            {
                if (workerIpcvideoRx.CancellationPending == true)
                {
                    e.Cancel = true;
                    return; // abort work, if it's cancelled
                }

                IpcVideoDll.ipcv_getImageInfo(ipc_channel_rx, out imgWidth, out imgHeight, out imgDepth, out pixelFmt);
                int bufSize = imgWidth * imgHeight * imgDepth;

                if ( bufSize <= 0  ) // Requires implementation if not default(rgb) format
                {
                    this.Dispatcher.BeginInvoke(DispatcherPriority.Background, new Action(() =>
                    {
                        labelRxMsg.Content = "Image is not ready";                     
                    }));
                    Thread.Sleep(1);
                    continue;
                }

                if ( buffer.Length < bufSize )
                    buffer = new byte[bufSize];

                int result = 0;
                unsafe
                {
                    fixed (byte* p = buffer)
                    {
                        IntPtr pBackBuffer = (IntPtr)p;
                        result =  IpcVideoDll.ipcv_readShareVideo(ipc_channel_rx, pBackBuffer, bufSize, out imgWidth, out imgHeight, out imgDepth, out pixelFmt, out frameCount, out latency);
                    }
                };

                if(result > 0)
                {
                    this.Dispatcher.BeginInvoke(DispatcherPriority.Background, new Action(() =>
                    {
                        Int32Rect updateRrea = new Int32Rect(0,0,imgWidth, imgHeight);

                        bool reAlloc = false;
                        if( bmpRx == null )
                        {
                            reAlloc = true;
                        }
                        else if(bmpRx.PixelWidth != imgWidth || bmpRx.PixelHeight != imgHeight || bmpRx_pixelFormat != pixelFmt)
                        {
                            reAlloc = true;
                        }

                        if (reAlloc)
                        {
                            PixelFormat rxPixelFormat;

                            if (pixelFmt ==IpcVideoDll.IPCV_PixelFormat_BGR24)
                            {
                                rxPixelFormat = PixelFormats.Bgr24;
                            }
                            else if (pixelFmt == IpcVideoDll.IPCV_PixelFormat_Gray)
                            {
                                rxPixelFormat = PixelFormats.Gray8;
                            }else
                            {
                                labelRxMsg.Content = String.Format("Unknown Pixel Format");
                                return;
                            }
                            
                            bmpRx = new WriteableBitmap(imgWidth, imgHeight, 96, 96, rxPixelFormat, null);
                            bmpRx_pixelFormat = pixelFmt;
                            labelRxMsg.Content = String.Format("FrameCount({0}) , Latency({1:0.#0}ms)", frameCount, latency * 1000);
                        }

                        try
                        {
                            bmpRx.Lock();
                            unsafe
                            {
                                int bmpBufferSize = (int) bmpRx.BackBufferStride*bmpRx.PixelHeight;
                                if (bufSize > bmpBufferSize)
                                    bufSize = bmpBufferSize;
                                Marshal.Copy(buffer,0, bmpRx.BackBuffer, bufSize);
                            }
                            bmpRx.AddDirtyRect(updateRrea);
                        }
                        finally
                        {
                            bmpRx.Unlock();
                        }
                        VideoRx.Source = bmpRx;
                        //labelRxMsg.Content = String.Format("FrameCount({0}) , Latency({1:0.#0}ms), false return({2})", frameCount, latency*1000, false_return);
                        labelRxMsg.Content = String.Format("FrameCount({0}) , Latency({1:0.#0}ms)", frameCount, latency * 1000);
                    }));
                }
                else
                {
                    false_return++;
                }
            };
        }


        private void timer_IpcVideoTick(object sender, EventArgs e)
        {
            if(bmpTx == null)
            {
                bmpTx = new WriteableBitmap(1024, 768, 96, 96, PixelFormats.Bgr24, null);
            }            

            Random random = new Random();
            int stride = bmpTx.PixelWidth * bmpTx.Format.BitsPerPixel / 8;

            byte blue = (byte)random.Next(256);
            byte green = (byte)random.Next(256);
            byte red = (byte)random.Next(256);

            int blockWidth = random.Next(256);
            int blockHeight = random.Next(256);

            byte[] buffer = new byte[blockWidth * blockHeight * 3];

            for (int i = 0; i < buffer.Length; i += 3)
            {
                buffer[i] = blue;
                buffer[i + 1] = green;
                buffer[i + 2] = red;
            }

            Int32Rect rect = new Int32Rect(random.Next(bmpTx.PixelWidth - blockWidth), random.Next(bmpTx.PixelHeight - blockHeight), blockWidth, blockHeight);
            bmpTx.WritePixels(rect, buffer, blockWidth * 3, 0);

            try
            {
                bmpTx.Lock();
                unsafe
                {
                    IntPtr pBackBuffer = bmpTx.BackBuffer;
                    int imgDepth = 3;
                    int bufSize = bmpTx.PixelWidth * bmpTx.PixelHeight * imgDepth;
                    int frameCount;
                    IpcVideoDll.ipcv_writeShareVideo(ipc_channel_tx, pBackBuffer, bufSize, bmpTx.PixelWidth, bmpTx.PixelHeight, imgDepth, 0, out frameCount);
                }
            }
            finally
            {
                bmpTx.Unlock();
            }
            VideoTx.Source = bmpTx;

        }

        void worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            labelRxMsg.Content = String.Format("completed ipcvideo_rx thread");
            if( reserved_StartIPCService )
            {
                startIPCService();
            }
        }

        private void comboxRxChannel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ipc_channel_rx = comboxRxChannel.SelectedIndex;
        }

        private void comboxTxChannel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ipc_channel_tx = comboxTxChannel.SelectedIndex;
        }

        private void comboxTxInterval_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if( comboxTxInterval.Text.Length>0)
            {
                timerIpcVideoTx.Interval = TimeSpan.FromSeconds(Convert.ToDouble(comboxTxInterval.Items[comboxTxInterval.SelectedIndex]));
            }            
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            stopIPCService();
        }

        private void comboxIpcName_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            startIPCService();
        }
    }
}
