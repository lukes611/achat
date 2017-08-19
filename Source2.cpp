#include <OpenNI.h>
#include "locv3.h"
#include "lopenni.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include "R3.h"
#include "SIObj.h"
#include "Pixel3DSet.h"
#include "locv_algorithms.h"
#include "VMatF.h"
#include "LNC.h"
using namespace std;

using namespace ll_siobj;
using namespace ll_pix3d;

int main()
{
	init_LNC();

	int e;
	LClient client = newLClient("127.0.0.1", 8123, &e);
	if(e)
	{
		cout << "could not start client";
		exit(-1);
	}
	

	VideoCapture cap = 0;
	
	cout << cap.isOpened() << endl;
	Mat x;
	int every = 1000 / 40;
	while(true)
	{

		while(!cap.read(x)) ;

		//cout << x.size() << endl;
		imshow("xxx", x);
		int key = waitKey(every);
		if(key == 27) break;
		
		vector<int>param(2);
		param[0] = cv::IMWRITE_JPEG_QUALITY;
		param[1] = 50;

		vector<unsigned char> buffer;
		imencode(".jpg", x, buffer, param);

		//cout << "sending " << buffer.size() << endl;
		client.write(&client, (char*)buffer.data(), buffer.size());

	}
	//cout << buffer.size() << endl;
	
	//FILE * fo = fopen("C:/Users/luke/Desktop/abc.jpg", "wb");
	//fwrite(buffer.data(), sizeof(unsigned char), buffer.size(), fo);
	//fclose(fo);

	
	
	client.close(&client);

	

	//_CRT_SECURE_NO_WARNINGS
	del_LNC();
	
	return 0;
}

