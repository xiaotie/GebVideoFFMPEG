using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Geb.Video.FFMPEG.Demo
{
    using Geb.Image;

    public partial class FrmMain : Form
    {
        private VideoFileReader _reader;
        private VideoFileWriter _writer;

        public FrmMain()
        {
            InitializeComponent();
        }

        private void btnOpen_Click(object sender, EventArgs e)
        {
            dgOpenFile.ShowDialog();
        }

        private void Clear()
        {
            if (_reader != null) _reader.Close();
            if (_writer != null) _writer.Close();
            _reader = null;
            _writer = null;
        }

        private void dgOpenFile_FileOk(object sender, CancelEventArgs e)
        {
            String path = dgOpenFile.FileName;
            this.tbPath.Text = path;
            try
            {
                _reader = new VideoFileReader();
                _reader.Open(path);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                _reader = null;
            }

            tbInfo.Text = String.Empty;

            if (_reader != null)
            {
                String info = String.Format("Video info:\r\n\r\n Width-{0}\r\n Height-{1}\r\n FrameCount-{2}\r\n FrameRate-{3}\r\n Codec-{4}",
                    _reader.Width,_reader.Height,
                    _reader.FrameCount, _reader.FrameRate, _reader.CodecName);
                tbInfo.Text = info;
            }

            ShowNextFrame();
        }

        private void worker_DoWork(object sender, DoWorkEventArgs e)
        {
            // 视频播放应该在这里进行，Demo为了简化，就不放在这里了
        }

        private void btnNextFrame_Click(object sender, EventArgs e)
        {
            ShowNextFrame();
        }

        private void ShowNextFrame()
        {
            if (_reader != null)
            {
                ImageRgb24 img = _reader.ReadVideoFrame();
                if (img == null) return;

                Bitmap old = pbImg.Image as Bitmap;
                pbImg.Image = img.ToBitmap();
                if (old != null) old.Dispose();
                img.Dispose();
            }
        }

        private void FrmMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            Clear();
        }

        private void btnJump_Click(object sender, EventArgs e)
        {
            // 跳到
            if (_reader != null)
            {
                // 指定帧的编号
                Int64 idx = _reader.FrameCount * 2 / 3;
                // 跳到指定帧附近的关键帧处，true 为跳到关键帧，false 为跳到任意帧
                _reader.Seek(idx, true);
            }
            ShowNextFrame();
        }

        private void btnWriteVideo_Click(object sender, EventArgs e)
        {
            if (_writer != null) return;

            if (_reader != null && _reader.Width > 1 && _reader.Height > 1)
            {
                _writer = new VideoFileWriter();
                _writer.Open("output.avi", _reader.Width, _reader.Height, _reader.FrameRate, VideoCodec.MPEG4);

                // demo 代码，之处理 100 帧
                for (int i = 0; i < 100; i++)
                {
                    ImageRgb24 img = _reader.ReadVideoFrame();
                    if (img == null) break;
                    _writer.WriteVideoFrame(img);
                    img.Dispose();
                }
                _writer.Close();
                _writer = null;
                MessageBox.Show("视频处理完毕，生成文件 output.avi");
            }
        }
    }
}
