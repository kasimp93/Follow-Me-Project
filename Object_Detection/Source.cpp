//opencv libraries
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//C++ libraries
#include <iostream>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <fstream>

using namespace cv;
using namespace std;


int main()
{
	Mat image,src_gray;

	//a) Reading a stream of images from a webcamera, and displaying the video
	// For more information on reading and writing video: http://docs.opencv.org/modules/highgui/doc/reading_and_writing_images_and_video.html
	// open the video camera no. 0
	VideoCapture cap(0);

	// if not successful, exit program
	if (!cap.isOpened())
	{
		cout << "Cannot open the video cam, working on image stream from directory." << endl;
		return -1;
	}
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640); cap.set(CV_CAP_PROP_FRAME_HEIGHT,350);

	//create a window called "MyVideo"
	namedWindow("MyVideo", WINDOW_AUTOSIZE);
	int counter = 0;
	char str[15];
	while (1)
	{

		// read a new frame from video
		bool bSuccess = cap.read(image);
		//imshow("MyVideo", image);
		//if not successful, break loop
		if (!bSuccess)
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		
		counter++;
		if (counter % 5 != 0) {
			continue;

		}
		cvtColor(image, src_gray, CV_BGR2GRAY);

		/// Reduce the noise so we avoid false circle detection
		GaussianBlur(src_gray, src_gray, Size(9, 9), 2, 2);

		vector<Vec3f> circ;
		imshow("MyVideo", src_gray);

		/// Apply the Hough Circle Transform to find the circles
		HoughCircles(src_gray, circ, CV_HOUGH_GRADIENT, 1, ceil(src_gray.rows / 3),20, 20, 30, 200);

		/// Draw the circles detected
		int sz = (int) circ.size();
		for (size_t i = 0; i < std::min(1,sz); i++)
		{
			Point center(cvRound(circ[i][0]), cvRound(circ[i][1]));
			int radius = cvRound(circ[i][2]);
			if (radius <= 2 || radius > 200)
				continue;
			// circle center
			circle(image, center, 3, Scalar(0, 255, 0), -1, 8, 0);
			std::cout << center << "::" << radius << endl;
			std::ofstream file;
			//writing the circle radius and center in a text file
			file.open ("value.txt");
			file << radius<< "::";
			file << center;
			file << "\n";
			
			file.close();
			// circle outline
			circle(image, center, radius, Scalar(0, 0, 255), 3, 8, 0);
		}
		imshow("MyVideo", image);


		if (waitKey(30) == 27)
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}

	}
	cap.release();
	return 0;
}