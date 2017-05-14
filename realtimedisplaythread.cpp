#include "realtimedisplaythread.h"
extern int DISPLAY_WIDTH;
extern int DISPLAY_HEIGHT;
extern int WEBCAM_WIDTH;
extern int WEBCAM_HEIGHT;
#define USE_ROI_FOLLOW_STRATEGY
Mat currentFrameCopy;
extern char VIDEO_FILE[];
QMutex  mutex;
RealTimeDisplayThread::RealTimeDisplayThread():roiRect(QRect(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT))
{

    isStopped = false;
    recording = false;
    recordflag = 0;
    vcap = NULL;

}
RealTimeDisplayThread::~RealTimeDisplayThread()
{
    if(vcap)delete vcap;
    vcap = NULL;

}
void RealTimeDisplayThread::run()
{

    if(!setupWebcam(1))
    {
        cout<<"Cannot open Camera.\n";
        return;
    }
    float FPS =  vcap->get(CV_CAP_PROP_FPS);
    Mat currentFrame;

    int count = 0;
    int current_W;
    int current_H;
    VideoWriter* vwriter = NULL;
    while(!isStopped && vcap->isOpened())
    {


        vcap->read(currentFrame);


        switch(this->recordflag)
        {
        case 1:
        {
            char currentDateTime[50];
            time_t now;
            time(&now);
            strftime(currentDateTime,sizeof(currentDateTime),"%Y-%b-%d %H-%M.avi",localtime(&now));

//            if(vwriter->isOpened())
//            {
//                vwriter->release();
//            }
            vwriter = new VideoWriter(currentDateTime,CV_FOURCC('X','V','I','D'), 25, currentFrame.size());

            if(vwriter->isOpened())
            {
                vwriter->write(currentFrame);
                recordflag = 2;
            }
            cout<<"ok3"<<endl;
            break;
        }

        case 2:
            vwriter->write(currentFrame);
            break;
        case -1:
            if(vwriter->isOpened())
            {
                vwriter->release();

            }
            recordflag = 0;
            break;
        default:
            break;


        }


        if(0 == count)
        {
            current_W = currentFrame.cols;
            current_H = currentFrame.rows;
            count = 1;
            cout<<current_W<<" "<<current_H<<endl;
            continue;
        }
#ifdef  USE_ROI_FOLLOW_STRATEGY

        cv::Rect r((int)(current_W*roiRect.x()/(float)DISPLAY_WIDTH),(int)(current_H*roiRect.y()/(float)DISPLAY_HEIGHT),(int)(current_W*roiRect.width()/(float)DISPLAY_WIDTH),(int)(current_H*roiRect.height()/(float)DISPLAY_HEIGHT));

        //if(r.x>current_W)r.x = current_W;
        if(r.x<0)r.x = 0;
        if(r.y<0)r.y = 0;
        if(r.x+ r.width > current_W)r.width = current_W - r.x;
        if(r.y+ r.height > current_H)r.height = current_H - r.y;



        Mat ROI(currentFrame,r);//int()(roiRect.x(),roiRect.y(),roiRect.width(),roiRect.height()));
        mutex.lock();
        ::currentFrameCopy = ROI.clone();//DO NOT use copyTo()function here!!!
        mutex.unlock();

#else
        currentFrame.copyTo(currentFrameCopy);
#endif
        cvtColor(currentFrame, currentFrame, CV_BGR2RGB);
        QImage imageQ((unsigned char*)currentFrame.data,currentFrame.cols,currentFrame.rows,currentFrame.cols*3,QImage::Format_RGB888);
        emit transmitCurrentFrame(imageQ);
        msleep(35);
    }
    if(vwriter->isOpened())
        vwriter->release();
}

bool RealTimeDisplayThread::setupWebcam(const char videoFilePath[])
{
    this->vcap = new VideoCapture();
    this->vcap->open(videoFilePath);
    return vcap->isOpened();
}

bool RealTimeDisplayThread::setupWebcam(const int deviceID)
{
    this->vcap = new VideoCapture();
    this->vcap->open(deviceID);

    this->vcap->set(CV_CAP_PROP_FRAME_WIDTH,WEBCAM_WIDTH);
    this->vcap->set(CV_CAP_PROP_FRAME_HEIGHT,WEBCAM_HEIGHT);
    this->vcap->set(CV_CAP_PROP_FPS,60);
     return vcap->isOpened();

}
void RealTimeDisplayThread::onScalePosChanged(QRect roi)
{

    mutex.lock();
    roiRect=roi;

    mutex.unlock();
}

void RealTimeDisplayThread::startrecording()
{
    this->recordflag = 1;

}
void RealTimeDisplayThread::stoprecording()
{
    this->recordflag = -1;
}
