#include <opencv2/opencv.hpp>
#include <iostream>
#include <wiringPi.h>
#include <chrono>
#include <ctime>
using namespace std;
using namespace cv;

Mat frame, matrix, framePers, frameGray, frameThresh, frameEdge, frameFinal, ROILane, ROILaneEnd, frameFinalDuplicate, frameFinalDuplicate1;
int leftLanePos, rightLanePos, laneCenter, frameCenter, result, laneEnd;
stringstream ss;

VideoCapture cam(0); // Open the default camera (index 0)

vector<int> histrogramLane;
vector<int> histrogramLaneEnd;
Point2f Source[] = {Point2f(25, 160), Point2f(290, 160), Point2f(0, 210), Point2f(315, 210)};
Point2f Destination[] = {Point2f(60, 1), Point2f(260, 1), Point2f(60, 238), Point2f(260, 238)};

void setup(VideoCapture& cam) {
    cam.set(CAP_PROP_FRAME_WIDTH, 360); // Adjust the width
    cam.set(CAP_PROP_FRAME_HEIGHT, 240); // Adjust the height
    cam.set(CAP_PROP_BRIGHTNESS, 50); // Set brightness (0-100)
    cam.set(CAP_PROP_CONTRAST, 50); // Set contrast (0-100)   
    cam.set(CAP_PROP_SATURATION, 50); // Set saturation (0-100)
    cam.set(CAP_PROP_GAIN, 50); // Set gain (0-100)
    cam.set(CAP_PROP_FPS, 0); // Set FPS (Frames Per Second) & Adjust the FPS as needed
}

void perspective() {
	line(frame, Source[0], Source[1], Scalar(0,0,255), 2);
	line(frame, Source[1], Source[3], Scalar(0,0,255), 2);
	line(frame, Source[3], Source[2], Scalar(0,0,255), 2);
	line(frame, Source[2], Source[0], Scalar(0,0,255), 2);
	
	matrix = getPerspectiveTransform(Source, Destination);
	warpPerspective(frame, framePers, matrix, Size(360, 240));
}

void threshold() {
	cvtColor(framePers, frameGray, COLOR_RGB2GRAY);
	inRange(frameGray, 240, 255, frameThresh);
	Canny(frameGray, frameEdge, 600, 700, 3, false);
	add(frameThresh, frameEdge, frameFinal);
	cvtColor(frameFinal, frameFinal, COLOR_GRAY2RGB);
	cvtColor(frameFinal, frameFinalDuplicate, COLOR_RGB2BGR);	//used in histrogram function only
	cvtColor(frameFinal, frameFinalDuplicate1, COLOR_RGB2BGR);	//used in histrogram function only
}

void histrogram() {
	histrogramLane.resize(360);
	histrogramLane.clear();
	
	for(int i=0; i<frame.size().width; i++) {
		ROILane = frameFinalDuplicate(Rect(i,140,1,100));
		divide(255, ROILane, ROILane);
		histrogramLane.push_back((int)(sum(ROILane)[0]));
	}
	
	histrogramLaneEnd.resize(360);
	histrogramLaneEnd.clear();
	
	for(int i=0; i<frame.size().width; i++) {
		ROILaneEnd = frameFinalDuplicate1(Rect(i,0,1,240));
		divide(255, ROILaneEnd, ROILaneEnd);
		histrogramLaneEnd.push_back((int)(sum(ROILaneEnd)[0]));
	}
	
	laneEnd = sum(histrogramLaneEnd)[0];
	cout<<"Lane End = "<<laneEnd<<endl;
}

void laneFinder() {
	vector<int>::iterator leftPtr;
	leftPtr = max_element(histrogramLane.begin(), histrogramLane.begin() + 160);
	leftLanePos = distance(histrogramLane.begin(), leftPtr);
	
	vector<int>::iterator rightPtr;
	rightPtr = max_element(histrogramLane.begin() + 200, histrogramLane.end());
	leftLanePos = distance(histrogramLane.begin(), rightPtr);
	
	line(frameFinal, Point2f(leftLanePos, 0), Point2f(leftLanePos, 240), Scalar(0,255,0), 2);
	line(frameFinal, Point2f(rightLanePos, 0), Point2f(rightLanePos, 240), Scalar(0,255,0), 2);
}	

void LaneCenter() {
	laneCenter = (rightLanePos - leftLanePos) / 2 + leftLanePos;
	frameCenter = 100;
	
	line(frameFinal, Point2f(laneCenter, 0), Point2f(laneCenter, 240), Scalar(0,255,0), 3);
	line(frameFinal, Point2f(frameCenter, 0), Point2f(frameCenter, 240), Scalar(255,0,0), 3);
	
	result = laneCenter - frameCenter;
}

void capture() {
	cam.grab();
	cam.retrieve(frame);
	cvtColor(frame, frame, COLOR_BGR2RGB);
}

int main() {
    wiringPiSetup();
    pinMode(21, OUTPUT);
    pinMode(22, OUTPUT);
    pinMode(23, OUTPUT);
    pinMode(24, OUTPUT);
    	
    if (!cam.isOpened()) {
        cout << "Failed to connect to the camera" << endl;
        return -1;
    }
    
    setup(cam);

    cout << "Connecting to camera..." << endl;
    // int cameraId = static_cast<int>(cam.get(CAP_PROP_POS_MSEC));
    // cout << "Camera ID: " << cameraId << endl;

    while (true) {
	auto start = chrono::system_clock::now();
		
	capture();
	perspective();
	threshold();
	histrogram();
	laneFinder();
	LaneCenter(); 
	
	if(result == 0) {
		digitalWrite(21, 0);
		digitalWrite(22, 0);
		digitalWrite(23, 0);
		digitalWrite(24, 0);
		cout<<"Forward"<<endl;
	}
	else if(result > 0 && result < 10) {
		digitalWrite(21, 1);
		digitalWrite(22, 0);
		digitalWrite(23, 0);
		digitalWrite(24, 0);
		cout<<"Right1"<<endl;
	}
	else if(result == 10 && result < 20) {
		digitalWrite(21, 0);
		digitalWrite(22, 1);
		digitalWrite(23, 0);
		digitalWrite(24, 0);
		cout<<"Right2"<<endl;
	}
	else if(result > 20) {
		digitalWrite(21, 1);
		digitalWrite(22, 1);
		digitalWrite(23, 0);
		digitalWrite(24, 0);
		cout<<"Right3"<<endl;
	}
	else if(result < 0 && result > -10) {
		digitalWrite(21, 0);
		digitalWrite(22, 0);
		digitalWrite(23, 1);
		digitalWrite(24, 0);
		cout<<"Left1"<<endl;
	}
	else if(result < -10 && result > -20) {
		digitalWrite(21, 1);
		digitalWrite(22, 0);
		digitalWrite(23, 1);
		digitalWrite(24, 0);
		cout<<"Left2"<<endl;
	}
	else if(result < -20) {
		digitalWrite(21, 0);
		digitalWrite(22, 1);
		digitalWrite(23, 1);
		digitalWrite(24, 0);
		cout<<"Left3"<<endl;
	}
	else if(laneEnd > 3000) {
		digitalWrite(21, 1);
		digitalWrite(22, 1);
		digitalWrite(23, 1);
		digitalWrite(24, 0);
		cout<<"Lane End"<<endl;
	}
	else if(laneEnd > 3000) {
		ss.str(" ");
		ss.clear();
		ss<<"Lane End ";
		putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(255,0,0),2);
	}
	else if(result == 0) {
		ss.str(" ");
		ss.clear();
		ss<<"Result ="<<result<<" (Move Forward)";
		putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(0,0,255),2);
	}
	else if(result > 0) {
		ss.str(" ");
		ss.clear();
		ss<<"Result ="<<result<<" (Move Right)";
		putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(0,0,255),2);
	}
	else if(result < 0) {
		ss.str(" ");
		ss.clear();
		ss<<"Result ="<<result<<" (Move Left)";
		putText(frame, ss.str(), Point2f(1,50), 0,1, Scalar(0,0,255),2);
	}
		
	namedWindow("Original", WINDOW_KEEPRATIO);
        moveWindow("Original", 0, 100);
        resizeWindow("Original", 640, 480);
        imshow("Original", frame);
        
        namedWindow("Perspective", WINDOW_KEEPRATIO);
        moveWindow("Perspective", 700, 100);
        resizeWindow("Perspective", 640, 480);
        imshow("Perspective", framePers);
        
        namedWindow("GRAY", WINDOW_KEEPRATIO);
        moveWindow("GRAY", 1400, 100);
        resizeWindow("GRAY", 640, 480);
        imshow("GRAY", frameFinal);
        
		auto end = chrono::system_clock::now();
		chrono::duration<double> elapsed_seconds = end-start;
		float t = elapsed_seconds.count();
		int FPS = 1/t;
		cout << "FPS = " << FPS << endl;

        if (frame.empty()) {
            cout << "Error: Frame is empty" << endl;
            break;
        }
		
        // imshow("Camera", frame); // Display the frame
        if (waitKey(1) == 'q') // Press 'q' to exit
            break;
    }

    cam.release(); // Release the camera

    return 0;
}
