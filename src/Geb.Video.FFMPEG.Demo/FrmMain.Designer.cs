namespace Geb.Video.FFMPEG.Demo
{
    partial class FrmMain
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.tbPath = new System.Windows.Forms.TextBox();
            this.btnOpen = new System.Windows.Forms.Button();
            this.pbImg = new System.Windows.Forms.PictureBox();
            this.btnNextFrame = new System.Windows.Forms.Button();
            this.btnJump = new System.Windows.Forms.Button();
            this.dgOpenFile = new System.Windows.Forms.OpenFileDialog();
            this.worker = new System.ComponentModel.BackgroundWorker();
            this.tbInfo = new System.Windows.Forms.TextBox();
            this.btnWriteVideo = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.pbImg)).BeginInit();
            this.SuspendLayout();
            // 
            // tbPath
            // 
            this.tbPath.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbPath.Location = new System.Drawing.Point(13, 13);
            this.tbPath.Name = "tbPath";
            this.tbPath.ReadOnly = true;
            this.tbPath.Size = new System.Drawing.Size(532, 21);
            this.tbPath.TabIndex = 0;
            this.tbPath.TabStop = false;
            // 
            // btnOpen
            // 
            this.btnOpen.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOpen.Location = new System.Drawing.Point(551, 11);
            this.btnOpen.Name = "btnOpen";
            this.btnOpen.Size = new System.Drawing.Size(75, 23);
            this.btnOpen.TabIndex = 1;
            this.btnOpen.Text = "打开文件";
            this.btnOpen.UseVisualStyleBackColor = true;
            this.btnOpen.Click += new System.EventHandler(this.btnOpen_Click);
            // 
            // pbImg
            // 
            this.pbImg.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.pbImg.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.pbImg.Location = new System.Drawing.Point(13, 41);
            this.pbImg.Name = "pbImg";
            this.pbImg.Size = new System.Drawing.Size(442, 424);
            this.pbImg.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.pbImg.TabIndex = 2;
            this.pbImg.TabStop = false;
            // 
            // btnNextFrame
            // 
            this.btnNextFrame.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnNextFrame.Location = new System.Drawing.Point(461, 351);
            this.btnNextFrame.Name = "btnNextFrame";
            this.btnNextFrame.Size = new System.Drawing.Size(165, 34);
            this.btnNextFrame.TabIndex = 3;
            this.btnNextFrame.Text = "下一帧";
            this.btnNextFrame.UseVisualStyleBackColor = true;
            this.btnNextFrame.Click += new System.EventHandler(this.btnNextFrame_Click);
            // 
            // btnJump
            // 
            this.btnJump.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnJump.Location = new System.Drawing.Point(461, 391);
            this.btnJump.Name = "btnJump";
            this.btnJump.Size = new System.Drawing.Size(165, 34);
            this.btnJump.TabIndex = 4;
            this.btnJump.Text = "跳到三分之二处";
            this.btnJump.UseVisualStyleBackColor = true;
            this.btnJump.Click += new System.EventHandler(this.btnJump_Click);
            // 
            // dgOpenFile
            // 
            this.dgOpenFile.DefaultExt = "*";
            this.dgOpenFile.FileOk += new System.ComponentModel.CancelEventHandler(this.dgOpenFile_FileOk);
            // 
            // worker
            // 
            this.worker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.worker_DoWork);
            // 
            // tbInfo
            // 
            this.tbInfo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbInfo.Location = new System.Drawing.Point(481, 40);
            this.tbInfo.Multiline = true;
            this.tbInfo.Name = "tbInfo";
            this.tbInfo.Size = new System.Drawing.Size(145, 265);
            this.tbInfo.TabIndex = 5;
            // 
            // btnWriteVideo
            // 
            this.btnWriteVideo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnWriteVideo.Location = new System.Drawing.Point(461, 431);
            this.btnWriteVideo.Name = "btnWriteVideo";
            this.btnWriteVideo.Size = new System.Drawing.Size(165, 34);
            this.btnWriteVideo.TabIndex = 6;
            this.btnWriteVideo.Text = "写视频(当前帧向后100帧)";
            this.btnWriteVideo.UseVisualStyleBackColor = true;
            this.btnWriteVideo.Click += new System.EventHandler(this.btnWriteVideo_Click);
            // 
            // FrmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(638, 477);
            this.Controls.Add(this.btnWriteVideo);
            this.Controls.Add(this.tbInfo);
            this.Controls.Add(this.btnJump);
            this.Controls.Add(this.btnNextFrame);
            this.Controls.Add(this.pbImg);
            this.Controls.Add(this.btnOpen);
            this.Controls.Add(this.tbPath);
            this.Name = "FrmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "VideoDemo ( by geblab: http://www.geblab.com; http:// xiaotie.cnblogs.com)";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FrmMain_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.pbImg)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox tbPath;
        private System.Windows.Forms.Button btnOpen;
        private System.Windows.Forms.PictureBox pbImg;
        private System.Windows.Forms.Button btnNextFrame;
        private System.Windows.Forms.Button btnJump;
        private System.Windows.Forms.OpenFileDialog dgOpenFile;
        private System.ComponentModel.BackgroundWorker worker;
        private System.Windows.Forms.TextBox tbInfo;
        private System.Windows.Forms.Button btnWriteVideo;
    }
}

