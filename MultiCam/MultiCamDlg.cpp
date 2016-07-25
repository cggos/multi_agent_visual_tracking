
// MultiCamDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MultiCam.h"
#include "MultiCamDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma region 额外增加的文件和宏定义

#include <opencv2\opencv.hpp>
#include "CvvImage.h"
#define MAX_CORNERS 500

CvCapture *capture1,*capture2;
IplImage  *m_Frame1,*m_Frame2,*Img_zh,*outImgDetect,
    *hsv, *HChannel, *SChannel,*VChannel,*screenImg;
CvvImage  m_CvvImage1,m_CvvImage2,m_CvvImage3;
CDC *pDC1,*pDC2;
HDC hDC1,hDC2,hDC3,hDCDetect;
CRect rect1,rect2,rect3,rectDetect,rectXY,rectRgnXY;
float factorWCam,factorHCam;
CWnd *pWnd;
CRgn rgn;
CvVideoWriter *writer;
int isColor;
double fps ;

#pragma endregion



// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // 对话框数据
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    // 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMultiCamDlg 对话框

CMultiCamDlg::CMultiCamDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CMultiCamDlg::IDD, pParent)
    , frameCount1(0)
    , frameCount2(0)
    , isLDownCam(false)
    , isVedioStitching(false)
    , isDetectEdge(false)
    , isDetectCorner(false)
    , pointCam(0)
    , nPort(0)
    , isTrackObject(0)
    , isLDownTrack(false)
    , isSelectObject(false)
	, widthDlg(0)
	, heightDlg(0)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMultiCamDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_FRAME_COUNT1, frameCount1);
    DDX_Text(pDX, IDC_FRAME_COUNT2, frameCount2);
    DDX_Text(pDX, IDC_EDIT_PORT, nPort);
}

BEGIN_MESSAGE_MAP(CMultiCamDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()

    ON_MESSAGE(WM_RECVDATA,&CMultiCamDlg::OnRecvData) //自定义消息WM_RECVDATA的消息映射

    ON_BN_CLICKED(IDC_OPEN_CAM1, &CMultiCamDlg::OnClickedOpenCam1)
    ON_BN_CLICKED(IDC_CLOSE_CAM1, &CMultiCamDlg::OnClickedCloseCam1)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_OPEN_CAM2, &CMultiCamDlg::OnClickedOpenCam2)
    ON_BN_CLICKED(IDC_CLOSE_CAM2, &CMultiCamDlg::OnClickedCloseCam2)
    //    ON_BN_CLICKED(IDC_GRAB_FRAME1, &CMultiCamDlg::OnBnClickedGrabFrame1)
    ON_BN_CLICKED(IDC_GRAB_FRAME2, &CMultiCamDlg::OnClickedGrabFrame2)
    ON_BN_CLICKED(IDC_VIDEO_STITCH, &CMultiCamDlg::OnBnClickedVideoStitch)
    ON_BN_CLICKED(IDC_GRAB_FRAME, &CMultiCamDlg::OnClickedGrabFrame)
    //    ON_WM_LBUTTONDOWN()
    //ON_STN_CLICKED(IDC_VEDIO_STITCHING, &CMultiCamDlg::OnClickedVedioStitching)
    ON_BN_CLICKED(IDC_EDGE_DETECT, &CMultiCamDlg::OnBnClickedEdgeDetect)
    ON_WM_LBUTTONDOWN()
    //ON_STN_CLICKED(IDC_VEDIO_STITCHING, &CMultiCamDlg::OnStnClickedVedioStitching)
    ON_BN_CLICKED(IDC_GRAB_FRAME1, &CMultiCamDlg::OnClickedGrabFrame1)
    ON_BN_CLICKED(IDC_CORNER_DETECT, &CMultiCamDlg::OnBnClickedCornerDetect)
    ON_BN_CLICKED(IDC_BEGIN_LOCATE, &CMultiCamDlg::OnBnClickedBeginLocate)
    ON_BN_CLICKED(IDC_BTN_SEND, &CMultiCamDlg::OnBnClickedBtnSend)
    ON_COMMAND(IDM_SCREEN_SHOT, &CMultiCamDlg::OnScreenShot)
    ON_COMMAND(IDM_SCREEN_VIDEO, &CMultiCamDlg::OnScreenVideo)
    ON_BN_CLICKED(IDC_BEGIN_TRACK, &CMultiCamDlg::OnBnClickedBeginTrack)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


// CMultiCamDlg 消息处理程序

BOOL CMultiCamDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO: 在此添加额外的初始化代码

    //创建并初始化套接字
    InitSocket();
    RECVPARAM *pRecvParam = new RECVPARAM;
    pRecvParam->sock = m_socket;
    pRecvParam->hwnd = m_hWnd;
    //创建接受数据的线程
    HANDLE hThread = CreateThread(NULL,0,RecvProcThread,(LPVOID)pRecvParam,0,NULL);
    //关闭该接受线程句柄，释放其引用计数
    CloseHandle(hThread);

	//设置对话框的大小和位置
	// ::SetWindowPos(this->m_hWnd,HWND_BOTTOM,50,10,800,600,SWP_NOZORDER);
	// CenterWindow(this); //居中显示对话框

	//获取对话框的高度和宽度
	CRect rectDlg;
	GetWindowRect(rectDlg);
	widthDlg = rectDlg.Width();
	heightDlg = rectDlg.Height();
	/*
	CWnd *pwnd;
	pwnd = GetDlgItem(IDC_CAM1);
	pwnd->MoveWindow(10,10,240,320);
	*/
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMultiCamDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
// 来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
// 这将由框架自动完成。

void CMultiCamDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }


    /**************************************************************
    *             在IDC_COORDINATE控件（坐标图）上画图            *
    **************************************************************/

    //获取IDC_COORDINATE控件的DC和区域
    CWnd *pWnd  = GetDlgItem(IDC_COORDINATE);
    CDC *pDC    = pWnd->GetDC();
    CRect rect;
    pWnd->GetClientRect(&rect);

    /***********  在IDC_COORDINATE控件（坐标图）上画坐标图  *************/
    //画横线、竖线
    CPen Pen(PS_SOLID,0,RGB(255,0,0));
    CPen *pPen  = pDC->SelectObject(&Pen);
    for(int row=20; row<rect.Height(); row+=20)
        for(int col=20; col<rect.Width(); col+=20)
        {
            pDC->MoveTo(0,row);
            pDC->LineTo(rect.Width()-rect.Width()%20,row);
            pDC->MoveTo(col,0);
            pDC->LineTo(col,rect.Height()-rect.Height()%20);
        }
    //画坐标轴
    CPen PenXY(PS_JOIN_BEVEL,3,RGB(255,0,0));
    pDC->SelectObject(&PenXY);
    //画Y坐标轴
    pDC->MoveTo( 0, 0);
    pDC->LineTo( 0, rect.Height()-5);
    pDC->LineTo( 3, rect.Height()-5);//画箭头
    pDC->LineTo( 0, rect.Height()  );
    pDC->LineTo(-3, rect.Height()-5);
    pDC->LineTo( 0, rect.Height()-5);
    //画X坐标轴
    pDC->MoveTo(0,0);
    pDC->LineTo(rect.Width(),  0);
    pDC->LineTo(rect.Width(),  3); //画箭头
    pDC->LineTo(rect.Width()+5,0);
    pDC->LineTo(rect.Width(), -3);
    pDC->LineTo(rect.Width(),  0);
    //画原点
    pDC->Ellipse(-3,-3,+3,+3);

    /*******  用颜色在IDC_COORDINATE控件（坐标图）上标记智能体位置  ********/
    if(isLDownCam)
    {   
        pointXY = pointCam;
        CPen PenMark(PS_JOIN_BEVEL,3,RGB(0,0,255));
        pDC->SelectObject(&PenMark);
        pDC->Ellipse(pointXY.x-5,pointXY.y-5,pointXY.x+5,pointXY.y+5);
    }

    //将原画笔选进设备描述表，并释放DC
    pDC->SelectObject(pPen);
    ReleaseDC(pDC);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMultiCamDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


/*********************   摄像头1操作  **********************/
void CMultiCamDlg::OnClickedOpenCam1()
{
    // TODO: 在此添加控件通知处理程序代码
    HWND pParam = m_hWnd; //创建打开摄像头1的线程
    HANDLE hThreadCam1 = CreateThread(NULL, 0, OpenCam1Thread, (LPVOID)pParam, 0, NULL);
    CloseHandle(hThreadCam1);//关闭线程句柄，减少线程对象的使用计数 

    // 得到 hDC2 和 rect2
    CWnd *pWnd = GetDlgItem(IDC_CAM1);
    pDC1 = pWnd->GetDC();
    hDC1 = pDC1->GetSafeHdc();
    pWnd->GetClientRect(&rect1);
}
//打开摄像头1的线程
DWORD WINAPI CMultiCamDlg::OpenCam1Thread(LPVOID lpParam)
{
    capture1 = cvCaptureFromCAM(0);
    //设置每一帧的宽度和高度
    cvSetCaptureProperty(capture1, CV_CAP_PROP_FRAME_WIDTH,  320);
    cvSetCaptureProperty(capture1, CV_CAP_PROP_FRAME_HEIGHT, 240);

    //判断是否打开了摄像头1
    if(capture1)
        ::SetTimer((HWND)lpParam, 1, 50, NULL);
    else
        AfxMessageBox(_T("摄像头1打开失败！"));
    return 0;
}
void CMultiCamDlg::OnClickedGrabFrame1()
{
    // TODO: 在此添加控件通知处理程序代码
    cvSaveImage("..\\Output\\img1.jpg",m_Frame1);
}
void CMultiCamDlg::OnClickedCloseCam1()
{
    // TODO: 在此添加控件通知处理程序代码
    KillTimer(1);
    KillTimer(3); 
    //cvReleaseImage(&m_Frame1);
    cvReleaseCapture(&capture1);

    CBrush *pBrush = CBrush::FromHandle((HBRUSH)GetStockObject(GRAY_BRUSH));
    pDC1->FillRect(&rect1,pBrush);
}

/*********************   摄像头2操作  **********************/
void CMultiCamDlg::OnClickedOpenCam2()
{
    // TODO: 在此添加控件通知处理程序代码
    HWND pParam = m_hWnd; //创建打开摄像头2的线程
    HANDLE hThreadCam2 = CreateThread(NULL, 0, OpenCam2Thread, (LPVOID)pParam, 0, NULL);
    CloseHandle(hThreadCam2);//关闭线程句柄，减少线程对象的使用计数 

    // 得到 hDC2 和 rect2
    CWnd *pWnd = FromHandle((HWND)pParam)->GetDlgItem(IDC_CAM2);
    pDC2 = pWnd->GetDC();
    hDC2 = pDC2->GetSafeHdc();
    pWnd->GetClientRect(&rect2);
}
//打开摄像头2的线程
DWORD  WINAPI CMultiCamDlg::OpenCam2Thread(LPVOID lpParam)
{
    capture2 = cvCaptureFromCAM(1);
    //设置每一帧的宽度和高度
    cvSetCaptureProperty(capture2, CV_CAP_PROP_FRAME_WIDTH,  320);
    cvSetCaptureProperty(capture2, CV_CAP_PROP_FRAME_HEIGHT, 240);

    //判断是否打开了摄像头2
    if(capture2)
        ::SetTimer((HWND)lpParam, 2, 50, NULL);
    else
        AfxMessageBox(_T("摄像头2打开失败！"));
    return 0;
}
void CMultiCamDlg::OnClickedGrabFrame2()
{
    // TODO: 在此添加控件通知处理程序代码
    cvSaveImage("..\\Output\\img2.jpg",m_Frame2);
}
void CMultiCamDlg::OnClickedCloseCam2()
{
    // TODO: 在此添加控件通知处理程序代码
    KillTimer(2);
    KillTimer(3);
    cvReleaseCapture(&capture2);
    //cvReleaseImage(&m_Frame2);

    //显示位图到窗口
    CBitmap m_Bitmap;
    m_Bitmap.LoadBitmap(IDB_GRAY);
    CDC dcCompatibleDC;
    dcCompatibleDC.CreateCompatibleDC(NULL);
    dcCompatibleDC.SelectObject(&m_Bitmap);
    BITMAP bmp;
    m_Bitmap.GetBitmap(&bmp);
    pDC2->StretchBlt(rect2.left,rect2.top,rect2.Width(),rect2.Height(),
        &dcCompatibleDC,0,0,bmp.bmWidth,bmp.bmHeight,SRCCOPY);
    m_Bitmap.DeleteObject();
    dcCompatibleDC.DeleteDC();
}

/****************** 视频拼接 *********************/
void CMultiCamDlg::OnBnClickedVideoStitch()
{
    // TODO: 在此添加控件通知处理程序代码

    CWnd *pWnd = GetDlgItem(IDC_VEDIO_STITCHING);
    CDC *pDC = pWnd->GetDC();
    hDC3 = pDC->GetSafeHdc();      
    pWnd->GetClientRect(rect3);

    if(capture1 && capture2)//判断两个摄像头是否全部打开
    {
        isVedioStitching = true;
        SetTimer(3, 50, NULL);
    }
    else
        MessageBox(_T("请打开摄像头！"));
}
void CMultiCamDlg::OnClickedGrabFrame()
{
    // TODO: 在此添加控件通知处理程序代码
    cvSaveImage("..\\Output\\img_zh.jpg",Img_zh);//从拼接视频上保存一帧图片
}

/*********************  边缘检测  *************************/
void CMultiCamDlg::OnBnClickedEdgeDetect()
{
    // TODO: 在此添加控件通知处理程序代码
    
    CWnd *pWnd4 = GetDlgItem(IDC_EDGE_DETECTING);
    CDC *pDC4 = pWnd4->GetDC();
    hDCDetect = pDC4->GetSafeHdc();
    pWnd4->GetClientRect(&rectDetect);

    isDetectEdge    = true;
    isDetectCorner  = false;
}
/*********************  角点检测  *************************/
void CMultiCamDlg::OnBnClickedCornerDetect()
{
    // TODO: 在此添加控件通知处理程序代码
    CWnd *pWnd4 = GetDlgItem(IDC_EDGE_DETECTING);
    CDC *pDC4 = pWnd4->GetDC();
    hDCDetect = pDC4->GetSafeHdc();
    pWnd4->GetClientRect(&rectDetect);

    isDetectEdge    = false;
    isDetectCorner  = true;
}

/******************  开始定位  *******************/
void CMultiCamDlg::OnBnClickedBeginLocate()
{
    // TODO: 在此添加控件通知处理程序代码
    isLDownCam = true;
    CRect rectCam;
    GetDlgItem(IDC_VEDIO_STITCHING)->GetWindowRect(&rectCam);
    ScreenToClient(rectCam);
	factorWCam = 320/(float)rectCam.Width();
	factorHCam = 480/(float)rectCam.Height();

    SetTimer(5, 1000, NULL);
}

/******************  屏幕录像  *******************/
void CMultiCamDlg::ScreenVideo(void)
{
    SetTimer(6,100,NULL);
}
void CMultiCamDlg::OnScreenVideo()
{
    // TODO: 在此添加命令处理程序代码
    ScreenVideo();
}

/******************  开始跟踪  *******************/
int b_flagTracking=0;

IplImage *imageTrack, *hsvTrack, *hueTrack, 
    *maskTrack, *histimgTrack,*backprojectTrack;
CvHistogram *histTrack;
CvRect selection;
CvRect track_window;
CvPoint origin;
CvConnectedComp track_comp;
CvBox2D track_box; // Meanshift跟踪算法返回的Box类

int hdims = 50; // 划分直方图bins的个数，越多越精确
float hranges_arr[] = {0,180};//像素值的范围
float* hranges = hranges_arr;//用于初始化CvHistogram类
float factorWTrack,factorHTrack;
int backproject_mode = 0;

HDC hDCTrack;
CRect rectTrack;
CvvImage imgTrack;
void CMultiCamDlg::OnBnClickedBeginTrack()
{
    // TODO: 在此添加控件通知处理程序代码
    CWnd *pWnd = GetDlgItem(IDC_TRACK);
    CDC *pDCTrack = pWnd->GetDC();
    hDCTrack = pDCTrack->GetSafeHdc();
    pWnd->GetClientRect(&rectTrack);

	factorWTrack = 320/(float)rectTrack.Width();
	factorHTrack = 480/(float)rectTrack.Height();

	b_flagTracking=1;

    imageTrack=0;
    if (!imageTrack)
    {
        imageTrack  = cvCreateImage( cvGetSize(Img_zh), 8, 3 );
        //imageTrack->origin = Img_zh->origin;
        hsvTrack    = cvCreateImage( cvGetSize(Img_zh), 8, 3 );
        hueTrack    = cvCreateImage( cvGetSize(Img_zh), 8, 1 );
        maskTrack   = cvCreateImage( cvGetSize(Img_zh), 8, 1 );//分配掩膜图像空间
        //分配反向投影图空间，大小一样，单通道
        backprojectTrack = cvCreateImage( cvGetSize(Img_zh), 8, 1 );

        //分配建立直方图空间
        histTrack = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 ); 
        //分配用于画直方图的空间
        histimgTrack = cvCreateImage( cvSize(320,240), 8, 3 );
        cvZero( histimgTrack );//将直方图的背景设为灰色
    }
    SetTimer(7,100,NULL);
}

/******************   定时器   **************************/
void CMultiCamDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    switch(nIDEvent)
    {
    case 1:     //显示摄像头1
        m_Frame1 = cvQueryFrame(capture1);
        m_CvvImage1.CopyOf(m_Frame1, 1);
        m_CvvImage1.DrawToHDC(hDC1, &rect1);
        frameCount1++;
        UpdateData(false);
        break;
    case 2:     //显示摄像头2
        m_Frame2 = cvQueryFrame(capture2);
        m_CvvImage2.CopyOf(m_Frame2, 1);
        m_CvvImage2.DrawToHDC(hDC2, &rect2);
        frameCount2++;
        UpdateData(false);
        break;
    case 3:     //视频拼接并显示
        IplImage* IMG;
        CvSize size;
        BYTE* RGB;
        BYTE* rgb1, *rgb2;
        IMG=new IplImage;
        size=cvSize(320,480);	    
        IMG=cvCreateImage(size,8,3);        
        RGB=new BYTE[320*480*3];
        for (int w=0;w<320;w++)
            for(int h=0;h<480;h++)
                for (int k=0;k<3;k++)
                    RGB[h*320*3+w*3+k]=0;			

        //      IplImage* img1;
        //    	img1=cvLoadImage("img1.jpg",-1);
        rgb1=new BYTE[320*240*3];
        memcpy(rgb1,m_Frame1->imageData,320*240*3);	

        //      IplImage* img2;	
        //  	img2=cvLoadImage("img2.jpg",-1);
        rgb2=new BYTE[320*240*3];
        memcpy(rgb2,m_Frame2->imageData,320*240*3);
        //图像上边部分
        for(int w=0;w<320;w++)
            for(int h=0;h<240;h++)
                for(int k=0;k<3;k++)
                {
                    RGB[h*320*3+w*3+k]=rgb1[h*320*3+w*3+k];
                }
                /*
        //图像重叠部分
        for(int w=0;w<320;w++)
            for(int h=0;h<40;h++)
                for(int k=0;k<3;k++)
                {
                    RGB[(h+240)*320*3+w*3+k]=rgb2[h*320*3+w*3+k];
                }  */
        //图像下边部分
        for(int w=0;w<320;w++)
            for(int h=0;h<240;h++)
                for(int k=0;k<3;k++)
                {
                    RGB[(h+240)*320*3+w*3+k]=rgb2[h*320*3+w*3+k];
                }
        
        cvSetData(IMG,RGB,320*3);
        //最终图像
//        Img_zh=new IplImage;
        Img_zh=cvCloneImage(IMG);
        m_CvvImage3.CopyOf(Img_zh, 1);
        m_CvvImage3.DrawToHDC(hDC3, rect3);
		/*
		delete IMG;
		delete rgb1;
		delete rgb2;
		delete Img_zh;
		*/
        if(isDetectEdge)//边缘检测
        {
            IplImage *CannyImage, *GrayImage;  //, *outImage
            CannyImage = cvCreateImage( cvSize(Img_zh->width,Img_zh->height), IPL_DEPTH_8U, 1 );
            GrayImage  = cvCreateImage( cvSize(Img_zh->width,Img_zh->height), IPL_DEPTH_8U, 1 );
            outImgDetect   = cvCreateImage( cvSize(Img_zh->width,Img_zh->height), IPL_DEPTH_8U, 3 );

            cvCvtColor( Img_zh, GrayImage, CV_BGR2GRAY);    
            cvCanny(GrayImage, CannyImage, 50, 150, 3);
            cvCvtColor( CannyImage, outImgDetect, CV_GRAY2BGR); 

            CvvImage m_CvvImage;
            m_CvvImage.CopyOf(outImgDetect, 1);
            m_CvvImage.DrawToHDC(hDCDetect, &rectDetect);

            cvReleaseImage(&CannyImage);
            cvReleaseImage(&GrayImage);
            cvReleaseImage(&outImgDetect);
        }

        if(isDetectCorner)//角点检测
        {
            IplImage *srcImage,*grayImage,*corners1,*corners2;
            srcImage    = cvCloneImage(Img_zh);//视频拼接的图像
            //创建单通道图像并为其分配内存空间
            grayImage   = cvCreateImage(cvGetSize(srcImage), IPL_DEPTH_8U,  1);
            corners1    = cvCreateImage(cvGetSize(srcImage), IPL_DEPTH_32F, 1);
            corners2    = cvCreateImage(cvGetSize(srcImage), IPL_DEPTH_32F, 1);
            CvPoint2D32f corners[MAX_CORNERS];//宏定义：最大角点数为500
            int cornerCount = 0;
            cvCvtColor(srcImage, grayImage, CV_BGR2GRAY);
            //确定图像的强角点
            cvGoodFeaturesToTrack(grayImage, 
                corners1, corners2, corners, &cornerCount, 0.05, 5, 0, 3, 0, 0.4);
            if(cornerCount>0)
            {
                CvScalar color = CV_RGB(255,0,0);//定义颜色：红色
                for(int i=0; i<cornerCount;i++)
                {
                    cvCircle(srcImage, cvPoint((int)(corners[i].x), 
                        (int)(corners[i].y)), 6, color, 2, CV_AA, 0);//画圆
                }
            }
            //将转换后的图像绘入相应控件的区域
            CvvImage m_CvvImage;
            m_CvvImage.CopyOf(srcImage, 1);
            m_CvvImage.DrawToHDC(hDCDetect, &rectDetect);
            //释放图像
            cvReleaseImage(&srcImage);
            cvReleaseImage(&grayImage);
            cvReleaseImage(&corners1);
            cvReleaseImage(&corners2);
        }
        break;
    case 5:
        pWnd = GetDlgItem(IDC_COORDINATE);
        pWnd->GetClientRect(&rectXY);
        pWnd->GetWindowRect(rectRgnXY);
        ScreenToClient(&rectRgnXY);
        //创建一个矩形区域
        rgn.CreateRectRgn(rectRgnXY.left,rectRgnXY.top,rectRgnXY.right,rectRgnXY.bottom);
        for( int w=0;w<320;w+=2)
            for(int h=0;h<480;h+=2)
            {
                m_Color.H=CV_IMAGE_ELEM(HChannel,uchar,h,w);
                m_Color.S=CV_IMAGE_ELEM(SChannel,uchar,h,w);
                m_Color.V=CV_IMAGE_ELEM(VChannel,uchar,h,w);
                if(TRUE==CheckHSV(colorMarked,m_Color))
                {	
                    pointCam.x = w/factorWCam;
                    pointCam.y = h/factorHCam;
                    //InvalidateRect(&rect,1);
                    RedrawWindow(&rectXY,&rgn); //窗口重绘
                } 
            }
        break;
    case 6:
        ScreenShot("..\\Output\\screenshot.jpg");
        screenImg = cvLoadImage("..\\Output\\screenshot.jpg");
        //初始化视频写入
        writer = 0;
        isColor = 1;
        fps     = 25;
        writer = cvCreateVideoWriter("..\\Output\\screenvideo.avi", CV_FOURCC('D','I','V','X'), 
            fps, cvSize(screenImg->width,screenImg->height),isColor);   //XVID  DIVX
        //写入视频文件
        cvWriteFrame(writer, screenImg);
        //在捕捉过程中查看获得的每一帧
//        cvReleaseVideoWriter(&writer);
        break;
    case 7:   // 目标跟踪   
        int i, bin_w, c;

		IplImage* m_FrameTrack=new IplImage;
		m_FrameTrack=cvCloneImage(Img_zh);

		if (!b_flagTracking)
			return;	

        cvCopy( m_FrameTrack, imageTrack, 0 );
        cvCvtColor( imageTrack, hsvTrack, CV_BGR2HSV ); //把图像从RGB表色系转为HSV表色系	

        if( isTrackObject )//如果当前有需要跟踪的物体   
        {
            int vmin = 10, vmax = 256, smin = 30;
            int _vmin = vmin, _vmax = vmax;

            //制作掩膜板，只处理像素值为H：0~180，S：smin~256，V：vmin~vmax之间的部分
            cvInRangeS( hsvTrack, cvScalar(0,smin,MIN(_vmin,_vmax),0),cvScalar(180,256,MAX(_vmin,_vmax),0), maskTrack ); 
            cvSplit( hsvTrack, hueTrack, 0, 0, 0 ); // 取得H分量

            //如果需要跟踪的物体还没有进行属性提取，则进行选取框类的图像属性提取
            if( isTrackObject < 0 )
            {
                float max_val = 0.f;
                cvSetImageROI( hueTrack, selection ); // 设置原选择框
                cvSetImageROI( maskTrack, selection ); // 设置Mask的选择框

                cvCalcHist( &hueTrack, histTrack, 0, maskTrack ); // 得到选择框内且满足掩膜板内的直方图

                cvGetMinMaxHistValue( histTrack, 0, &max_val, 0, 0 ); 
                cvConvertScale( histTrack->bins, histTrack->bins, max_val ? 255. / max_val : 0., 0 ); // 对直方图转为0~255
                cvResetImageROI( hueTrack ); //remove ROI
                cvResetImageROI( maskTrack );
                track_window = selection;
                isTrackObject = 1;

                cvZero( histimgTrack );
                bin_w = histimgTrack->width/hdims;
                for( i = 0; i < hdims; i++ )
                {
                    int val = cvRound(cvGetReal1D(histTrack->bins,i)*histimgTrack->height/255 );
                    CvScalar color = hsv2rgb(i*180.f/hdims);
                    //画直方图到图像空间
                    cvRectangle( histimgTrack, cvPoint(i*bin_w,histimgTrack->height),
						cvPoint((i+1)*bin_w,histimgTrack->height - val),color, -1, 8, 0 );
                }
            }

            //得到hue的反向投影图
            //cvCalcArrBackProject( (CvArr**)&hueTrack, backprojectTrack, histTrack );
			cvCalcBackProject( &hueTrack, backprojectTrack, histTrack ); // 得到hue的反向投影图

            //得到反向投影图mask内的内容
            cvAnd( backprojectTrack, maskTrack, backprojectTrack, 0 );
            //使用MeanShift算法对backproject中的内容进行搜索，返回跟踪结果
			try
			{
				cvCamShift( backprojectTrack, track_window,cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),&track_comp, &track_box );

			}
			catch (CException* e)
			{
			}
            track_window = track_comp.rect;//得到跟踪结果的矩形框

            if( backproject_mode )
                cvCvtColor( backprojectTrack, imageTrack, CV_GRAY2BGR ); //显示模式
            if( imageTrack->origin )
                track_box.angle = -track_box.angle;
            //画出跟踪结果的位置
            cvEllipseBox( imageTrack, track_box, CV_RGB(128,0,255), 3, CV_AA, 0 );

            //如果正处于物体选择，画出选择框
            if( isTrackObject && selection.width > 0 && selection.height > 0 )
            {
                //cvSetImageROI( imageTrack, selection );
                //cvXorS( imageTrack, cvScalarAll(255), imageTrack, 0 );
                //cvResetImageROI( imageTrack );
            }
            pointXY.x=(int)track_box.center.x;
            pointXY.y=240-(int)track_box.center.y;
        }
        
		imgTrack.CopyOf(imageTrack,1);
		imgTrack.DrawToHDC(hDCTrack, &rectTrack);

        break;
    }
    CDialogEx::OnTimer(nIDEvent);
}

BOOL CMultiCamDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: 在此添加专用代码和/或调用基类
    if(pMsg->message == WM_LBUTTONDOWN && 
        GetDlgItem(IDC_VEDIO_STITCHING)->GetSafeHwnd() == pMsg->hwnd)
    {  
        if(isVedioStitching)//判断是否打开了视频拼接窗口
        {
            OnLButtonDown(MK_LBUTTON, pMsg->pt);//使执行CMultiCamDlg::OnLButtonDown函数
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg); 
}
//在CMultiCamDlg::PreTranslateMessage函数中调用的OnLButtonDown函数
void CMultiCamDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
	//MessageBox(_T("OK"));
    //获取控件相对于屏幕的位置，并将屏幕坐标转换为对话框（客户区）坐标
    CRect rectCam;
    GetDlgItem(IDC_VEDIO_STITCHING)->GetWindowRect(&rectCam);
    ScreenToClient(rectCam);
    //将鼠标点击的点的屏幕坐标转换为对话框（客户区）坐标
    //ScreenToClient((LPPOINT)(&point));

	/*
	TCHAR Test[256];//鼠标在IDC_VEDIO_STITCHING控件上点击的点的HSV坐标
	_stprintf(Test,TEXT("%d, %d \n %d %d %d %d"),point.x,point.y,
		rectCam.left,rectCam.right,rectCam.top,rectCam.bottom);
	MessageBox(Test,_T("Test1"));
	*/

    //判断是否点击到了IDC_VEDIO_STITCHING区域
    if(point.x>=rectCam.left && point.x<=rectCam.right && 
        point.y>=rectCam.top && point.y<=rectCam.bottom)
    {
/*******  获取鼠标在IDC_VEDIO_STITCHING控件（拼接视频）上点击的点的坐标  *********/

    //鼠标在IDC_VEDIO_STITCHING控件上点击的点的XY坐标
        pointCam.x = point.x-rectCam.left;
        pointCam.y = point.y-rectCam.top;
        //将点在控件区域的坐标转换为对应图像的像素坐标
        CvPoint2D32f pt;
        pt.x = factorWCam*pointCam.x;
        pt.y = factorHCam*pointCam.y;
        //创建H，S，V三个通道的图像并分配内存
        hsv     =cvCreateImage(cvGetSize(Img_zh), 8, 3);
	    HChannel=cvCreateImage(cvGetSize(Img_zh), 8, 1);
	    SChannel=cvCreateImage(cvGetSize(Img_zh), 8, 1);
	    VChannel=cvCreateImage(cvGetSize(Img_zh), 8, 1);
        //颜色空间转换
        cvCvtColor(Img_zh,hsv,CV_BGR2HSV);
        //分割多通道数组成几个单通道数组或者从数组中提取一个通道
        cvSplit(hsv,HChannel,SChannel,VChannel,NULL);  //cvCvtPixToPlane
        //提取出鼠标点击点的H S V
        colorMarked.H=CV_IMAGE_ELEM(HChannel,uchar,(int)pt.y,(int)pt.x);
	    colorMarked.S=CV_IMAGE_ELEM(SChannel,uchar,(int)pt.y,(int)pt.x);
	    colorMarked.V=CV_IMAGE_ELEM(VChannel,uchar,(int)pt.y,(int)pt.x);

        TCHAR szXYandHSV[256];//鼠标在IDC_VEDIO_STITCHING控件上点击的点的HSV坐标
        _stprintf(szXYandHSV,TEXT("XY:  (%d, %d)\nHSV: (%d, %d, %d)"),
            pointCam.x, pointCam.y, colorMarked.H, colorMarked.S, colorMarked.V);
        MessageBox(szXYandHSV,_T("XY & HSV"));

/*********鼠标点击IDC_VEDIO_STITCHING控件（拼接视频）时使IDC_COORDINATE控件重绘*********/
        isLDownCam = true;
        CWnd *pWnd = GetDlgItem(IDC_COORDINATE);
        CRect rect,rectRgn;
        pWnd->GetClientRect(&rect);
        pWnd->GetWindowRect(rectRgn);
        ScreenToClient(&rectRgn);
        CRgn rgn;//创建一个矩形区域
        rgn.CreateRectRgn(rectRgn.left,rectRgn.top,rectRgn.right,rectRgn.bottom);
        //InvalidateRect(&rect,1);
        RedrawWindow(&rect,&rgn); //窗口重绘
    }

    CRect rectTrack;
    GetDlgItem(IDC_TRACK)->GetWindowRect(&rectTrack);
    ScreenToClient(rectTrack);
    //将鼠标点击的点的屏幕坐标转换为对话框（客户区）坐标
    //ScreenToClient((LPPOINT)(&point));
	
	//TCHAR szXYandHSV[256];//鼠标在IDC_VEDIO_STITCHING控件上点击的点的HSV坐标
	//_stprintf(szXYandHSV,TEXT("XY:  (%d, %d)\nHSV: (%d, %d, %d,%d)"),
	//	point.x, point.y, rectTrack.left,rectTrack.top,rectTrack.right,rectTrack.bottom);
	//MessageBox(szXYandHSV,_T("XY & HSV"));
	

    //判断是否点击到了IDC_TRACK区域
    if((point.x>=rectTrack.left && point.x<=rectTrack.right) && 
        (point.y>=rectTrack.top && point.y<=rectTrack.bottom))
    {
        isLDownTrack = true;
        //鼠标在IDC_VEDIO_STITCHING控件上点击的点的XY坐标
        pointTrack.x = point.x-rectTrack.left;
        pointTrack.y = point.y-rectTrack.top;
        if( !imageTrack )
        {
            AfxMessageBox(_T("请开始跟踪"));
            return;
        }
		/*      
        if( imageTrack->origin )
              pointy = 240 - pointy;

        if (imageTrack->width==640)
        {
            pointx=pointx*640/320;
            pointy=pointy*480/240;
        }
		*/
        origin.x = pointTrack.x;
        origin.y = pointTrack.y;
        selection = cvRect(pointTrack.x,pointTrack.y,0,0);//坐标
        isSelectObject = 1;//表明开始进行选取
    }
    CDialogEx::OnLButtonDown(nFlags, point);
}

CPoint pointMove;
void CMultiCamDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
	/*
	CvPoint origin1,point1;
	origin1.x =selection.x;
	origin1.y =selection.y;
	point1.x = selection.x+selection.width;
	point1.y = selection.y+selection.height;
	cvRectangle(imageTrack,origin,point1,cvScalar(255,0,0));
	cvSaveImage("track.jpg",imageTrack);
	*/
	/*
	TCHAR szXYandHSV[256];//鼠标在IDC_VEDIO_STITCHING控件上点击的点的HSV坐标
	_stprintf(szXYandHSV,TEXT("XY:  (%d, %d,%d,%d)\nHSV: (%d, %d)\n%f"),
		origin.x, origin.y, pointMove.x,pointMove.y,selection.width, selection.height,factorWTrack);
	MessageBox(szXYandHSV,_T("XY & HSV"));
	*/
    if(isLDownTrack)//isLDownTrack
    {
        isSelectObject = 0;//选择结束，选择目标标志为假
        if( selection.width > 0 && selection.height > 0 )//如果选择区域大于零
            isTrackObject = -1;//如果选择物体有效，则打开跟踪功能
    }
    CDialogEx::OnLButtonUp(nFlags, point);
}
void CMultiCamDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    CRect rectTrack;
    GetDlgItem(IDC_TRACK)->GetWindowRect(&rectTrack);
    ScreenToClient(rectTrack);
    //将鼠标点击的点的屏幕坐标转换为对话框（客户区）坐标
    //ScreenToClient((LPPOINT)(&point));

    //判断是否点击到了IDC_VEDIO_STITCHING区域
    if((point.x>=rectTrack.left && point.x<=rectTrack.right) && 
        (point.y>=rectTrack.top && point.y<=rectTrack.bottom))
    {
		//加载自定义光标
		SetCursor(AfxGetApp()->LoadCursor(IDC_CURSOR));
        if (isSelectObject)
        {
			pointMove.x = point.x-rectTrack.left;
			pointMove.y = point.y-rectTrack.top;
			/*
            if( imageTrack->origin )
			{
                pointMove.y = 480- pointMove.y;
			}
            if (imageTrack->width==640)
            {
                pointMove.x=pointMove.x*640/320;
                pointMove.y=pointMove.y*480/240;
            }
			*/
            selection.x = (MIN(pointMove.x,origin.x))*factorWTrack;
            selection.y = MIN(pointMove.y,origin.y)*factorHTrack;
            selection.width =  (CV_IABS(pointMove.x - origin.x))*factorWTrack;
            selection.height = (CV_IABS(pointMove.y - origin.y))*factorHTrack;
			/*
			selection.x = MIN(pointMove.x,origin.x);
			selection.y = MIN(pointMove.y,origin.y);
			selection.width =  selection.x +CV_IABS(pointMove.x - origin.x);//
			selection.height = selection.y +CV_IABS(pointMove.y - origin.y);// 
			*/
            selection.x = MAX( selection.x, 0 );
            selection.y = MAX( selection.y, 0 );
            selection.width = MIN( selection.width, imageTrack->width );
            selection.height = MIN( selection.height, imageTrack->height );
            //selection.width -= selection.x;
            //selection.height -= selection.y;	   
        }
    }
    CDialogEx::OnMouseMove(nFlags, point);
}

//判断h,s,v是否在允许的范围内
bool CMultiCamDlg::CheckHSV(HSV colorMarked, HSV m_Color)
{
    int H,S,V, h,s,v;
    H = colorMarked.H;
    S = colorMarked.S;
    V = colorMarked.V;
    h = m_Color.H;
    s = m_Color.S;
    v = m_Color.V;
    if(h>=H-10 && h<=H+10)
        if(s>=S-30 && s<=S+30)
            if(v>=V-30 && v<=V+30)
                return true;
            else
                return false;
}

/*****************  创建并初始化套接字  ********************/
bool CMultiCamDlg::InitSocket(void)
{
    //创建套接字
    m_socket = socket(AF_INET,SOCK_DGRAM,0);
    if(INVALID_SOCKET == m_socket)
    {
        MessageBox(_T("创建套接字失败！"));
        return false;
    }
    SOCKADDR_IN addrSock;
    addrSock.sin_family = AF_INET;
    addrSock.sin_port = htons(6000);
    addrSock.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    int retval;
    //绑定套接字
    retval = bind(m_socket,(SOCKADDR*)&addrSock,sizeof(SOCKADDR));
    if(SOCKET_ERROR == retval)
    {
        closesocket(m_socket);
        MessageBox(_T("绑定失败！"));
        return false;
    }
    return TRUE;
}
//接受数据的线程
DWORD WINAPI CMultiCamDlg::RecvProcThread(LPVOID lpParameter)
{
    //获取主线程传递的套接字和窗口句柄
    SOCKET sock = ((RECVPARAM*)lpParameter)->sock;
    HWND hwnd = ((RECVPARAM*)lpParameter)->hwnd;
    delete lpParameter;

    SOCKADDR_IN addrFrom;
    int len = sizeof(SOCKADDR);

    char recvBuf[200];
    char tempBuf[300];
    int retval;
    while(TRUE)
    {
        //接受数据
        retval = recvfrom(sock,recvBuf,200,0,(SOCKADDR*)&addrFrom,&len);
        if(SOCKET_ERROR == retval)
            break;
        sprintf(tempBuf,"%s",recvBuf);
        ::PostMessage(hwnd,WM_RECVDATA,0,(LPARAM)tempBuf);
    }
    return 0;
}
//接受数据消息映射函数
LRESULT CMultiCamDlg::OnRecvData(WPARAM wParam, LPARAM lParam)
{
    //取出接收到的数据
    CString str = _T(""); 
    str = (char*)lParam;
    CString strTemp;
    //获得已接受的数据
    GetDlgItemText(IDC_EDIT_ACCEPTCOMMENT,strTemp);
    str += "\r\n";
    strTemp += str;
    SetDlgItemText(IDC_EDIT_ACCEPTCOMMENT,strTemp);
    return 0;
}
//发送数据
void CMultiCamDlg::OnBnClickedBtnSend()
{
    // TODO: 在此添加控件通知处理程序代码
    
    DWORD dwIP; //获得对方IP
    ((CIPAddressCtrl*)GetDlgItem(IDC_IPADDRESS1))->GetAddress(dwIP);
    UpdateData();
    SOCKADDR_IN addrTo;
    addrTo.sin_family = AF_INET;
    addrTo.sin_port = htons(nPort);
    addrTo.sin_addr.S_un.S_addr = htonl(dwIP);

    CString strSend = _T("");
    //获得待发送数据
    GetDlgItemText(IDC_EDIT_SENDCOMMENT,strSend);

    //将LPCTSTR或CString strSend转换为const char* pstrSend
    const size_t strsize=(strSend.GetLength()+1)*2; //宽字符的长度;
    char * pstrSend= new char[strsize]; //分配空间;
    size_t sz=0;
    wcstombs_s(&sz,pstrSend,strsize,strSend,_TRUNCATE);
    int n=atoi((const char*)pstrSend); // 字符串已经由原来的CString 转换成了 const char*

    //发送数据
    sendto(m_socket,pstrSend,strSend.GetLength()+1,0,(SOCKADDR*)&addrTo,sizeof(SOCKADDR));
    //清空发送编辑框中的内容
    SetDlgItemText(IDC_EDIT_SENDCOMMENT,_T(""));
}


void CMultiCamDlg::OnScreenShot()
{
    // TODO: 在此添加命令处理程序代码
    ScreenShot("..\\Output\\screenshot.jpg");
}
//屏幕截图
void CMultiCamDlg::ScreenShot(char* filename)
{
    CDC *pDC ;
    //CDC::FromHandle(::GetDC(GetSafeHwnd()));
    pDC =CDC::FromHandle(::GetWindowDC(GetSafeHwnd()));//获取当前整个对话框的DC
    //GetDlgItem(IDD_MULTICAM1_DIALOG)->GetDC();
    //HWND hwnd = ::GetForegroundWindow(); // 获得当前活动窗口
    //CClientDC dc(this);
    //pDC = CDC::FromHandle(NULL); //获取当前整个屏幕DC
    //GetActiveWindow();

    CRect rect;
    GetClientRect(&rect);
    int Width = rect.Width()+3;
    int Height = rect.Height()+3;
    int BitPerPixel = pDC->GetDeviceCaps(BITSPIXEL);//获得颜色模式：多少位色彩
//    int Width = pDC->GetDeviceCaps(HORZRES);        //屏幕宽度
//    int Height = pDC->GetDeviceCaps(VERTRES);       //屏幕高度

    CDC memDC;//内存DC
    memDC.CreateCompatibleDC(pDC);

    CBitmap memBitmap, *oldmemBitmap;//建立和屏幕兼容的bitmap
    memBitmap.CreateCompatibleBitmap(pDC, Width, Height);

    oldmemBitmap = memDC.SelectObject(&memBitmap);//将memBitmap选入内存DC
    memDC.BitBlt(0, 0, Width, Height, pDC, 0, 0, SRCCOPY);//复制屏幕图像到内存DC

    //以下代码保存memDC中的位图到文件
    BITMAP bmp;
    memBitmap.GetBitmap(&bmp);//获得位图信息

    FILE *fp = fopen(filename, "w+b");

    BITMAPINFOHEADER bih = {0};//位图信息头
    bih.biBitCount = bmp.bmBitsPixel;//每个像素字节大小
    bih.biCompression = BI_RGB;
    bih.biHeight = bmp.bmHeight;//高度
    bih.biPlanes = 1;
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biSizeImage = bmp.bmWidthBytes * bmp.bmHeight;//图像数据大小
    bih.biWidth = bmp.bmWidth;//宽度

    BITMAPFILEHEADER bfh = {0};//位图文件头
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);//到位图数据的偏移量
    bfh.bfSize = bfh.bfOffBits + bmp.bmWidthBytes * bmp.bmHeight;//文件总的大小
    bfh.bfType = (WORD)0x4d42;
    fwrite(&bfh, 1, sizeof(BITMAPFILEHEADER), fp);//写入位图文件头
    fwrite(&bih, 1, sizeof(BITMAPINFOHEADER), fp);//写入位图信息头
    byte * p = new byte[bmp.bmWidthBytes * bmp.bmHeight];//申请内存保存位图数据
    GetDIBits(memDC.m_hDC, (HBITMAP) memBitmap.m_hObject, 0, Height, p, 
        (LPBITMAPINFO) &bih, DIB_RGB_COLORS);//获取位图数据
    fwrite(p, 1, bmp.bmWidthBytes * bmp.bmHeight, fp);//写入位图数据
    delete [] p;
    fclose(fp);
    memDC.SelectObject(oldmemBitmap);
}

CvScalar CMultiCamDlg::hsv2rgb(float hue)
{
    int rgb[3], p, sector;
    static const int sector_data[][3]={{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
    hue *= 0.033333333333333333333333333333333f;
    sector = cvFloor(hue);
    p = cvRound(255*(hue - sector));
    p ^= sector & 1 ? 255 : 0;

    rgb[sector_data[sector][0]] = 255;
    rgb[sector_data[sector][1]] = 0;
    rgb[sector_data[sector][2]] = p;

    return (cvScalar(rgb[2], rgb[1], rgb[0],0));//返回对应的颜色值
}
