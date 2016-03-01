// This examples uses 3 fader controller of EDIROL UR-80 (very old product though:-) as follows. (Status, Control number)
// 0xbf,0x40
// 0xbf,0x41
// 0xbf,0x42

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

#include "midi_cv_ctrl.h" // just include .h file to use the library

// just for default slider/volume/button assignments of my old EDIROL UR-80, modify them according to your cutting edge controller. from here.
#define STATUS			(0xbf) // control change, ch 16
#define	FADER0			(0x40) // control number of each fader
#define	FADER1			(0x41)
#define	FADER2			(0x42)
#define	FADER3			(0x43)
#define	FADER4			(0x44)
#define	FADER5			(0x45)
#define	FADER6			(0x46)
#define	FADER7			(0x47)
#define	VOL_CUTOFF		(0x48)
#define	VOL_RESONANCE	(0x49)
#define	VOL_VIB_RATE	(0x4a)
#define	VOL_VIB_DEPTH	(0x4b)
#define	VOL_VIB_DELAY	(0x4c)
#define	VOL_ATTACK		(0x4d)
#define	VOL_DECAY		(0x4e)
#define	VOL_RELEASE		(0x4f)
#define	BUTTON_HOME		(0x09)
#define	BUTTON_REW		(0x13)
#define	BUTTON_FF		(0x14)
#define	BUTTON_STOP		(0x15)
#define	BUTTON_PLAY		(0x16)
#define	BUTTON_REC		(0x17)
#define	MASTER_FADER	(0x51)

// to here

int main(int argc, char **argv) {
	UINT devid; 
	std::vector<midiin_dev> devlist;
	int devnum;
	midi_cv_ctrl mcc;
		
	devid = 0u;
	
	if(!mcc.midiin_devlist(devlist,devnum)) {
		std::cout << "error: midiin_devlist failed" << std::endl;
		return -1;
	}

	for (int i = 0; i < devnum; ++i) {
		std::cout << "ID=" << devlist[i].devid << " Name=" << devlist[i].devname << std::endl;
	}
	
	if(!mcc.midiin_open(devid)) {
		std::string errmsg = mcc.get_errmsg();
		std::cout << "error: midiin_open failed: " << errmsg << std::endl;
		return -1;
	}
	
	std::cout << std::hex << std::showbase << "Succeeded to open a MIDI input device (ID=" << devid << ")" << std::endl;
	
	// Using just three left hand side faders in this example.
	if(!mcc.register_volfader(STATUS,FADER0)) 
		std::cout << "register_controller failed" << std::endl;
		
	if(!mcc.register_volfader(STATUS,FADER1)) 
		std::cout << "register_controller failed" << std::endl;
		
	if(!mcc.register_volfader(STATUS,FADER2)) 
		std::cout << "register_controller failed" << std::endl;
	
	// And register two buttons too
	if(!mcc.register_button(STATUS,BUTTON_PLAY)) 
		std::cout << "register_controller failed" << std::endl;
	
	if(!mcc.register_button(STATUS,BUTTON_STOP)) 
		std::cout << "register_controller failed" << std::endl;
	
	mcc.midiin_start();

	cv::VideoCapture cap(0);
	if(!cap.isOpened())
		return -1;
	
	cv::namedWindow("frame", cv::WINDOW_NORMAL);
    bool is_start = true;
    
	while(1) {
		char val[3];
		std::list<button_status> bs;
		
		mcc.get_volfader_val(STATUS,FADER0,val[0]);
		if(val[0] == -1) val[0] = 127;
		mcc.get_volfader_val(STATUS,FADER1,val[1]);
		if(val[1] == -1) val[1] = 127;
		mcc.get_volfader_val(STATUS,FADER2,val[2]);
		if(val[2] == -1) val[2] = 127;

		mcc.get_button_queue(bs);
		std::list<button_status>::iterator it = bs.begin();
		for(;it != bs.end();++it) {
			if((it->contnum == BUTTON_PLAY) && (it->val == 0x7f))  { // play button pushed.
				std::cout << "start" << std::endl;
				is_start = true;
			} else if((it->contnum == BUTTON_STOP) && (it->val == 0x7f)) { // stop button pushed.
				std::cout << "stop" << std::endl;
				is_start = false;
			}
		}
		
		if(is_start) {
			cv::Mat frame;
			cap >> frame;

			cv::Mat fimage;
			
			frame.convertTo(fimage, CV_32FC3, 1.0/255.);
			std::vector<cv::Mat> planes;
			cv::split(fimage, planes);
			
			planes[0] *= (double)((int)val[0]) / 127.;
			planes[1] *= (double)((int)val[1]) / 127.;
			planes[2] *= (double)((int)val[2]) / 127.;
			
			cv::merge(planes, fimage);
			fimage.convertTo(frame, CV_8UC3, 255.);

			std::ostringstream os;
			cv::Scalar col = cv::Scalar(200,0,200);
			int font = cv::FONT_HERSHEY_COMPLEX | cv::FONT_ITALIC;
			
			os << "B=" << ((double)val[0] / 127.);
			cv::putText(frame, os.str(), cv::Point(30,30), font, 1,col, 2, CV_AA);

			os.str("");
			os.clear(std::stringstream::goodbit);
			os << "G=" << ((double)val[1] / 127.);
			cv::putText(frame, os.str(), cv::Point(30,60), font, 1,col, 2, CV_AA);
			
			os.str("");
			os.clear(std::stringstream::goodbit);
			os << "R=" << ((double)val[2] / 127.);
			cv::putText(frame, os.str(), cv::Point(30,90), font, 1,col, 2, CV_AA);
			
			cv::imshow("frame", frame);
		}
		
		if(cv::waitKey(33) > 0) 
			break;
	}
	
	mcc.midiin_stop();
	mcc.midiin_reset();
	mcc.midiin_close();
	
	std::cout << "Closed a MIDI input device." << std::endl;
	return 0;
}
