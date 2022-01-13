#include <string>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <opencv2/opencv.hpp>
#include <regex>
#include <math.h>


#include <windows.h>

using namespace std;
using namespace cv;

struct foxVector {
	int distance = 0;
	int azimuth = 0;
	float x = 0;
	float y = 0;
};

static bool scanVector(cv::Mat frame, int* detectedDist, int* detectedAzim) {

	string outText = "";

	try {
		tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
		api->Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
		api->SetPageSegMode(tesseract::PSM_AUTO);
		api->SetImage(frame.data, frame.cols, frame.rows, 1, frame.step);
		outText = string(api->GetUTF8Text());
		cout << outText;
		api->End();

		string dist = outText.substr(0, outText.find("\n"));
		string azim = outText.substr(outText.find("\n"), outText.size());

		std::cout << " \n";

		dist = std::regex_replace(dist, std::regex(R"([^0-9])"), "");
		azim = std::regex_replace(azim, std::regex(R"([^0-9])"), "");

		*detectedDist = std::stoi(dist);
		*detectedAzim = std::stoi(azim);

		std::cout << dist << "\n";
		std::cout << azim << "\n";
	}
	catch (std::exception) {
		return false;
	}

	return true;
}

static bool fillVector(foxVector* vect, int dist, int azim) {
	vect->distance = dist;
	vect->azimuth = azim;
	vect->x = dist * cos(azim * (3.14f / 180.f));
	vect->y = dist * sin(azim * (3.14f / 180.f));

	return true;
}

static void clearVector(foxVector* vect) {
	vect->azimuth = 0;
	vect->distance = 0;
	vect->x = 0;
	vect->y = 0;
}



static foxVector getDifference(foxVector A, foxVector B) {
	float x1, y1, x2, y2;

	foxVector calculatedDifference;

	x1 = A.distance * cos(A.azimuth * (3.14 / 180));
	y1 = A.distance * sin(A.azimuth * (3.14 / 180));

	x2 = B.distance * cos(B.azimuth * (3.14 / 180));
	y2 = B.distance * sin(B.azimuth * (3.14 / 180));

	calculatedDifference.distance = sqrt(powf((x2 - x1), 2) + powf((y2 - y1), 2));
	calculatedDifference.azimuth = atan2(y2 - y1, x2 - x1) * (180 / 3.14);

	return calculatedDifference;

}

int main()
{

	HDC desktopHDC = GetWindowDC(GetDesktopWindow());

	HDC dcTarget = CreateCompatibleDC(desktopHDC);

	int cursorAreaX = 100;
	int cursorAreaY = 50;

	HBITMAP screenCapture = CreateCompatibleBitmap(desktopHDC, cursorAreaX, cursorAreaY);

	BITMAPINFOHEADER  bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = cursorAreaX;
	bi.biHeight = -cursorAreaY;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	POINT currentCursor;
	currentCursor.x = 0;
	currentCursor.y = 0;

	std::string capturedText = "Smartillery";

	cv::Mat CursorArea;
	cv::namedWindow(capturedText, cv::WINDOW_GUI_EXPANDED);
	cv::resizeWindow(capturedText, 400, 300);

	int whiteThreshhold = 0;

	cv::createTrackbar("slider", capturedText, &whiteThreshhold, 255);

	//tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
	//api->Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
	//api->SetPageSegMode(tesseract::PSM_AUTO);


	/*
	string outText, imPath = "C:\\Users\\Yabba\\Desktop\\testing\\test5.png";
	Mat im = cv::imread(imPath, IMREAD_COLOR);
	tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();

	api->Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
	api->SetPageSegMode(tesseract::PSM_AUTO);
	api->SetImage(im.data, im.cols, im.rows, 3, im.step);
	outText = string(api->GetUTF8Text());
	cout << outText;
	api->End();
	*/

	bool lClickLast = false;
	bool lClick = false;
	bool rClick = false;

	//friendly
	foxVector toArtillery;

	//target
	foxVector toTarget;

	//fire mission
	foxVector fireMission01;

	//error
	foxVector toDeviation;

	//
	//foxVector toCorrection;

	//correction
	foxVector fireMission02;

	string dist = "";
	string azim = "";

	int detectedDist = 0;
	int detectedAzim = 0;

	foxVector detectedVector;

	foxVector* detectedVectors[3] = { &toArtillery, &toTarget, &toDeviation };
	int stage = 0;

	std::cout << "Begin\n";
	std::cout << "- - - - - - - - - - \n";

	for (;;) {

		cv::Mat frame, mask;
		frame.create(cursorAreaY, cursorAreaX, CV_8UC4);
		mask.create(cursorAreaY, cursorAreaX, CV_8UC4);

		SelectObject(dcTarget, screenCapture);
		BitBlt(dcTarget, 0, 0, cursorAreaX, cursorAreaY, desktopHDC, currentCursor.x - cursorAreaX / 2, currentCursor.y + 5, SRCCOPY | CAPTUREBLT);
		GetDIBits(dcTarget, screenCapture, 0, cursorAreaY, frame.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

		//cv::inRange(frame, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255), frame);

		//cv::inRange(frame, cv::Scalar(200,200,200), cv::Scalar(255,255,255), frame);
		cvtColor(frame, frame, COLOR_RGB2HSV);
		//cvtColor(frame, frame, COLOR_RGB2GRAY);
		//bitwise_not(frame, frame);

		cv::inRange(frame, cv::Scalar(0, 0, 150), cv::Scalar(255, 255, 255), frame);

		bitwise_not(frame, frame);

		GetCursorPos(&currentCursor);

		lClick = (GetKeyState(VK_LBUTTON) & 0x8000);
		rClick = (GetKeyState(VK_RBUTTON) & 0x8000);

		if (lClick && rClick && !lClickLast)
		{
			/*
			tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
			api->Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
			api->SetPageSegMode(tesseract::PSM_AUTO);
			api->SetImage(frame.data, frame.cols, frame.rows, 1, frame.step);
			outText = string(api->GetUTF8Text());
			cout << outText;
			api->End();

			dist = outText.substr(0, outText.find("\n"));
			azim = outText.substr(outText.find("\n") , outText.size());

			std::cout << " \n";

			dist = std::regex_replace(dist, std::regex(R"([^0-9])"), "");
			azim = std::regex_replace(azim, std::regex(R"([^0-9])"), "");

			detectedDist = std::stoi(dist);
			detectedAzim = std::stoi(azim);

			std::cout << dist << "\n";
			std::cout << azim << "\n";
			*/

			bool success = scanVector(frame, &detectedDist, &detectedAzim);
			if (success) {
				std::cout << "Successful scan\n";
				switch (stage) {

				case 0: {
					//Artillery

					fillVector(&toArtillery, detectedDist, detectedAzim);
					std::cout << "Artillery Clicked \n";
					std::cout << "- - - - - - - - - - \n";
					break;
				}

				case 1: {
					//Target Clicked

					fillVector(&toTarget, detectedDist, detectedAzim);

					std::cout << "Initial vector :\n\n";

					fireMission01 = getDifference(toArtillery, toTarget);

					std::cout << "Di =  " << fireMission01.distance << "\n";
					std::cout << "Az =  " << fireMission01.azimuth << "\n\n";

					std::cout << "- - - - - - - - - - \n";
					break;

				}

				case 2: {
					//Deviation clicked

					fillVector(&toDeviation, detectedDist, detectedAzim);

					//this is where the fuckery lies
					float correctionX = toTarget.x - (toDeviation.x - toTarget.x);
					float correctionY = toTarget.y - (toDeviation.y - toTarget.y);

					//wrong?
					//fireMission02.distance = sqrt(powf((correctionX - toArtillery.x), 2) + powf((correctionY - toArtillery.y), 2));
					//fireMission02.azimuth = atan2(correctionY - toArtillery.y, correctionX - toArtillery.x) * (180 / 3.14);

					//the above, flipped
					fireMission02.distance = sqrt(powf((toArtillery.x - correctionX), 2) + powf((toArtillery.y - correctionY), 2));
					fireMission02.azimuth = atan2(correctionY - toArtillery.y, correctionX - toArtillery.x) * (180 / 3.14);

					std::cout << "Revised Fire Mission : \n\n";
					std::cout << "Di = " << fireMission02.distance << "\n";
					std::cout << "Az = " << fireMission02.azimuth << "\n\n";

					std::cout << "- - - - - - - - - - \n";
					break;
				}

				case 3: {
					clearVector(&toArtillery);
					clearVector(&toTarget);
					clearVector(&toDeviation);
					stage = 0;
				}

				default: {
					stage = -0;
					break;
				}


				}

				stage++;
			}
			else {
				std::cout << "Scan Failed :( \n";
			}




		}

		lClickLast = lClick;

		//cv::putText(frame, "blah", cv::Point(5, 20), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 2, false);

		cv::imshow(capturedText, frame);

		if (cv::waitKey(30) >= 0) {
			break;
		};

		//std::cout << "blah\n";
	}

	return 0;

}