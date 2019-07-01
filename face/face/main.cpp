
#include<opencv2/opencv.hpp>
#include<iostream>

using namespace std;
using namespace cv;

CascadeClassifier cascade_face;
CascadeClassifier cascade_eye;
CascadeClassifier cascade_mouth;
CascadeClassifier cascade_nose;

bool detectFaceDetail(Mat& img);
bool detectFace();

int main(int argc,char *argv[])
{
//    detectFace();

    /*********************************** 1.加载检测器  ******************************/
    // 建立级联分类器
    // 加载训练好的 人脸检测器（.xml）
    const string path_face = "xml\\haarcascade_frontalface_alt.xml";
    if (!cascade_face.load(path_face))
    {
        cout << "cascade load failed!\n";
        return -1;
    }

    // 加载训练好的 眼睛检测器（.xml）
    const string path_eye = "xml\\haarcascade_eye.xml";
    if (!cascade_eye.load(path_eye))
    {
        cout << "cascade_eye load failed!\n";
        return -1;
    }

    // 加载训练好的 嘴巴检测器（.xml）
    const string path_mouth = "xml\\haarcascade_mcs_mouth.xml";
    if (!cascade_mouth.load(path_mouth))
    {
        cout << "cascade_mouth load failed!\n";
        return -1;
    }

    // 加载训练好的 鼻子检测器（.xml）
    const string path_nose = "xml\\haarcascade_mcs_nose.xml";
    if ( ! cascade_nose.load(path_nose))
    {
        cout << "cascade_mouth load failed!\n";
        return -1;
    }

    //单个图片检测
    string str = "face.jpg";
    Mat img = imread(str);//放到makefile文件下
    namedWindow("display");
    imshow("display", img);
    detectFaceDetail(img);
    destroyWindow("display");
    namedWindow("face_detect");
    imshow("face_detect", img);
    while(waitKey(0)!='k') ;
    destroyWindow("face_detect");

//    //摄像头检测
//    Mat frame;
//    VideoCapture capture;
//    capture.get(CAP_PROP_FRAME_HEIGHT);
//    capture.open(0); //打开摄像头
//    namedWindow("face_detect", WINDOW_AUTOSIZE);
////    resizeWindow("face_detect", 1000, 800);
//    if (!capture.isOpened())
//    {
//        printf("--(!)Error opening video capture\n");
//        return -1;
//    }
//    while (capture.read(frame)) //读取帧
//    {
//        if (frame.empty())
//        {
//            printf(" --(!) No captured frame -- Break!");
//            continue;
//        }

//        imshow("face_detect", frame);
////        waitKey(1);
//        if(detectFaceDetail(frame)){
//            imshow("getface_detect", frame);
//        }
//        if(waitKey(1)=='q'){
//            break;
//        }
//    }

//    return 0;
}

//只检测脸部的demo，直接调用
bool detectFace()
{
    /********************************** 1.打开图片 *************************************/
    string str = "face4.jpg";
    Mat img = imread(str);
    Mat imgGray;
    //转灰度图
    cvtColor(img, imgGray, COLOR_BGR2GRAY);
    //显示窗口
    namedWindow("display");
    imshow("display", img);

    /*********************************** 2.加载检测器  ******************************/
    // 建立级联分类器
    // 加载训练好的 人脸检测器（.xml）
    CascadeClassifier cascade_face;
    const string path_face = "xml\\haarcascade_frontalface_alt.xml";
    if (!cascade_face.load(path_face))
    {
        cout << "cascade load failed!\n";
        return -1;
    }
    //计时
    double t = 0;
    t = (double)getTickCount();

    /*********************************** 3.人脸检测 ******************************/
    vector<Rect> faces(0);
    cascade_face.detectMultiScale(imgGray,  //输入的原图
                             faces,    //表示检测到的人脸目标序列，输出的结果
                             1.1,    //每次图像尺寸减小的比例为1.1
                             3,      //每一个目标至少要被检测到3次才算是真的目标
                             0 ,   // 默认
                             Size(30, 30)  //最小的检测区域,根据使用场景来确定大小,比如太小的脸放弃
                             );


    cout << "detect face number is :" << faces.size() << endl;

    /********************************  4.显示人脸矩形框 ******************************/

    if (faces.size() > 0)
    {
        //对脸部进行循环
        for (size_t i = 0;i < faces.size();i++)
        {
            cout<<"face "<<i<<" area x = "<<faces[i].x<<" area y = "<<faces[i].y
               <<" area width = "<<faces[i].width<<" area height = "<<faces[i].height<<endl;
            rectangle(img, faces[i], Scalar(150, 0, 0), 3, 8, 0);

        }
    }
    else
    {
        cout << "cant not get faces " << endl;
    }

    t = (double)getTickCount() - t;  //getTickCount():  Returns the number of ticks per second.
    cout<< t * 1000 / getTickFrequency() << "ms " << endl;

    //画完人脸框的图片再显示
    imshow("display", img);

    while(waitKey(0)!='q') ;
    destroyWindow("display");
}

//检测脸部、眼睛、鼻子、嘴巴，传入需要检测的图像
bool detectFaceDetail(Mat& img)
{
    bool getFace = false;
    Mat imgGray;
    /* 因为用的是类haar特征，所以都是基于灰度图像的，这里要转换成灰度图像 */
    cvtColor(img, imgGray, COLOR_BGR2GRAY);


    //计时
    double t = 0;
    t = (double)getTickCount();
    /*********************************** 2.人脸检测 ******************************/
    vector<Rect> faces(0);

    cascade_face.detectMultiScale(imgGray,
                             faces,
                             1.1,
                             3,
                             0 ,
                             Size(30, 30) //过大则检测不到人脸,根据使用场景来确定大小,比如太小的脸放弃
                             );

    cout << "detect face number is :" << faces.size() << endl;
    /********************************  3.显示人脸矩形框 ******************************/

    if (faces.size() > 0)
    {
        //对脸部进行循环
        for (size_t i = 0;i < faces.size();i++)
        {
            cout<<"face "<<i<<" area x = "<<faces[i].x<<" area y = "<<faces[i].y
               <<" area width = "<<faces[i].width<<" area height = "<<faces[i].height<<endl;
            rectangle(img, faces[i], Scalar(150, 0, 0), 3, 8, 0);

            bool bGetEyes = false;
            bool bGetNose = false;
            bool bGetMouth = false;
            //脸部区域
            Mat faceROIGray = imgGray(faces[i]);
            Mat faceROI = img(faces[i]);
            //眼睛识别
            vector<Rect> eyes;
            cascade_eye.detectMultiScale(faceROIGray, eyes, 1.1, 3, 0 ,Size(10, 10));
            if(eyes.size() > 0)
            {
                cout << "detect eye number is :" << eyes.size() << endl;
                int iCoutEye = 0;
                for(size_t j = 0; j<eyes.size(); j++)
                {
                    //现在限制一下眼睛的位置，可以进一步细化
                    if(eyes[j].y+eyes[j].height < faces[i].height/3*2 && eyes[j].width<faces[i].width/2){
                        rectangle(faceROI, eyes[j], Scalar(150, 150, 0), 3, 8, 0);
                        iCoutEye++;
                        cout<<"eyes "<<j<<" area x = "<<eyes[j].x<<" area y = "<<eyes[j].y
                           <<" area width = "<<eyes[j].width<<" area height = "<<eyes[j].height<<endl;
                    }
                }
                //检测到两个眼睛
                if(iCoutEye >= 2){
                    bGetEyes = true;
                }
            }

            //鼻子识别
            vector<Rect> noses;
            cascade_nose.detectMultiScale(faceROIGray, noses, 1.1, 3, 0 ,Size(30, 30));
            if(noses.size() > 0)
            {
                cout << "detect nose number is :" << noses.size() << endl;

                for(size_t j = 0; j<noses.size(); j++)
                {
                    //现在限制一下鼻子的位置，可以进一步细化，比如要求正脸正对摄像头就可以对鼻子的中心点做些限制
                    if(noses[j].y > faces[i].height/4){
                        bGetNose = true;
                        rectangle(faceROI, noses[j], Scalar(150, 200, 200), 3, 8, 0);
                        cout<<"eyes "<<j<<" area x = "<<noses[j].x<<" area y = "<<noses[j].y
                           <<" area width = "<<noses[j].width<<" area height = "<<noses[j].height<<endl;
                    }
                }
            }

            //嘴巴识别
            vector<Rect> mouths;
            cascade_mouth.detectMultiScale(faceROIGray, mouths, 1.1, 3, 0 | CASCADE_SCALE_IMAGE ,Size(30, 30));
            if(mouths.size() > 0)
            {
                cout << "detect eye number is :" << mouths.size() << endl;

                for(size_t j = 0; j<mouths.size(); j++)
                {
//                    Point mouth_center(faces[i].x + mouths[0].x + mouths[0].width / 2, faces[i].y + mouths[0].y + mouths[0].height / 2); //嘴巴的中心
                    //现在限制一下嘴巴的位置，可以进一步细化
                    if(mouths[j].y > faces[i].height/3)
                    {
                        bGetMouth = true;
                        rectangle(faceROI, mouths[j], Scalar(150, 150, 150), 3, 8, 0);
                        cout<<"eyes "<<j<<" area x = "<<mouths[j].x<<" area y = "<<mouths[j].y
                           <<" area width = "<<mouths[j].width<<" area height = "<<mouths[j].height<<endl;
                    }
                }
            }

            //到时只有检测到眼睛、鼻子、嘴巴才把人脸框画出来，其它的都不画
            if(bGetEyes && bGetMouth && bGetNose){
                getFace = true;
//                rectangle(img, faces[i], Scalar(150, 0, 0), 3, 8, 0);
            }

        }// end face while
    }
    else
    {
        cout << "cant not get faces " << endl;
    }

    t = (double)getTickCount() - t;  //getTickCount():  Returns the number of ticks per second.
    cout<< t * 1000 / getTickFrequency() << "ms " << endl;

    return getFace;
}
