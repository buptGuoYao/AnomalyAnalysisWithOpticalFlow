#ifndef EventDetection_H_TOMHEAVEN_20140730
#define EventDetection_H_TOMHEAVEN_20140730

#include <opencv2/opencv.hpp>
#include "LKTracker.h"
#include "cuda_utils.h"
using namespace cv;

using namespace cv::gpu;



/**********************************************************************************************************/
/*   ����Ϊ�Ӻ���
/**********************************************************************************************************/
//Ѱ�����ֵ
template <typename Type>
Type FindMaxVal(Type * arr, int N)
{
	Type max = arr[0];
	for(int i=1; i< N; i++)
	{
		if(max < arr[i])
			max = arr[i];
	}
	return max;
}


/** ����ֱ��ͼ��ֵͼ��
   @param angleMat �����ǶȾ���
   @param flowmagMat �����ٶȾ���
   @param nBins ����Ͱ�ĸ���
   @param BinsImages ���������ֱ��ͼ����
   @param angleOffset ����Ƕ�ʱ���õ����
*/
//����PI�Ƕȣ�������ȡ����һ���ǽǶȣ�180�ȣ�������һ���ǻ��ȣ�PI��3.1415927...)���������ǲ��ýǶ�
const float myPi = 180;
void calcBinsImages(const Mat & angleMat, const Mat & flowmagMat, int nBins, Mat * binsImages, float angleOffSet = 0.0f)
{
	//ͼ�񳤿�
	int width = angleMat.cols;
	int height =angleMat.rows;

	//ֱ��ͼÿ�����Ŀ��
	float BinWid = (float)(2*myPi) /(float)nBins;

	//��ʼ
	int bin1, bin2;
	float mag1, mag2;
	float dist,delta;
	for (int y=0; y<height; y++)
	{
		for (int x=0; x<width; x++)
		{
			float angle = angleMat.at<float>(y,x);
			float mag = flowmagMat.at<float>(y,x);

			//����Ƕ�С��angleOffSet�����ݺ���calcSparseOpticalFlow�нǶȵļ��㷽ʽ����˵���õ������ǲ���ע�ĵ㣬ֱ����������
			if (angle < angleOffSet || mag < 1e-8)
				continue;

			//����Ƕ���ֱ��ͼ���ߵı߽���
			if (angle <= BinWid/2+angleOffSet || angle >= 2*myPi+angleOffSet-BinWid/2)
			{
				bin1 = nBins-1;
				bin2 = 0;
				if (angle < BinWid/2+angleOffSet)
					dist = angle-angleOffSet + BinWid/2;
				else
					dist = angle-angleOffSet - (bin1*BinWid + BinWid/2);
			}
			else
			{
				bin1 = int(floor((angle-angleOffSet-BinWid/2) / BinWid));
				bin2 = bin1+1;
				dist = angle - angleOffSet - (bin1*BinWid + BinWid/2);
			}

			delta = dist/BinWid;
			mag2 = delta * mag;
			mag1 = mag - mag2;

			binsImages[bin1].at<float>(y,x) = mag1;
			binsImages[bin2].at<float>(y,x) = mag2;
		}
	}
}

/**
   �������ͼ�� ����OpenCV���� integral ����64λ������������������д��
   @param src �������
   @param dst ����ͼ��
*/
void myIntegral(const Mat& src, Mat& dst) {
	float rowSum = 0.0f;
	dst = dst.zeros(dst.size(), dst.type());
	for(int i = 0; i < src.rows; ++i, rowSum = 0.0f) {
		for(int j = 0; j < src.cols; ++j) {
			rowSum += src.at<float>(i, j);
			if (i == 0)
				dst.at<float>(i, j) = rowSum;
			else
			    dst.at<float>(i, j) = dst.at<float>(i - 1, j) + rowSum;
		}
	}
}

/**
   �������ͼ�� ������Ϊuchar�����Ϊfloat��
   @param src �������
   @param dst ����ͼ��
*/
void myIntegralUchar(const Mat& src, Mat& dst) {
	float rowSum = 0.0f;
	dst = dst.zeros(dst.size(), dst.type());
	for(int i = 0; i < src.rows; ++i, rowSum = 0.0f) {
		for(int j = 0; j < src.cols; ++j) {
			rowSum += (src.at<uchar>(i, j) > 128);
			if (i == 0)
				dst.at<float>(i, j) = rowSum;
			else
				dst.at<float>(i, j) = dst.at<float>(i - 1, j) + rowSum;
		}
	}
}

/** 
  �Թ���ֱ��ͼ�����ֵ��һ��
  @param binImage ����ֱ��ͼ
  @param bins ֱ��ͼ��Ͱ��
*/
void normalize(Mat& binImage, int bins) {
	float maxValue = -1e4f;
	const float eps = 1.0f;
	for (int i = 0; i < bins; ++i) {
		if (binImage.at<float>(i) > maxValue)
			maxValue = binImage.at<float>(i);
	}
	if (maxValue > eps) {
		for (int i = 0; i < bins; ++i)
			binImage.at<float>(i) /= maxValue;
	}
}

/**�������ֱ��ͼ
   @param angleMat �����ǶȾ���
   @param flowmagMat �����ٶȾ���
   @param nBins ����Ͱ�ĸ���
   @param BinsImages ����ĸ��������ֱ��ͼ����
   @param IntegralImages ����Ĺ���ֱ��ͼ�Ļ���ͼ��
*/
void calcIntegralHist(const Mat & angleMat, const Mat & flowmagMat, int nBins, Mat * binsImages, Mat * integralImages)
{
	int i;
	for(i=0; i< nBins; i++)
	{
		binsImages[i] = Mat(angleMat.size(), CV_32FC1);
		float * p;
		for(int r =0; r < binsImages[i].rows;r++)
		{
			p = binsImages[i].ptr<float>(r);
			for(int c = 0 ; c < binsImages[i].cols; c++)
				p[c] = 0;
		}
		integralImages[i] = Mat(angleMat.rows+1,angleMat.cols+1,CV_32FC1);
	}
	integralImages[i] = Mat(angleMat.rows+1,angleMat.cols+1,CV_32FC1);
	calcBinsImages(angleMat,flowmagMat,nBins,binsImages);

	for(i=0; i< nBins; i++)
	{
	    // �� binImages ����һ��
		//normalize(binsImages[i], nBins);
		integral(binsImages[i], integralImages[i], CV_32F);
		//myIntegral(binsImages[i], integralImages[i]);
		//saveImageAsText(binsImages[0], "binsImages.txt");
		//saveImageAsText(integralImages[0], "integral.txt");
		//printf("In calcIntegralHist\n");
	}
}

//����ͷ���� Updated
void drawArrow(cv::Mat& img, cv::Point pStart, cv::Point pEnd, int len, int alpha,
	cv::Scalar& color, int thickness, int lineType)
{

	Point arrow;
	//���� �� �ǣ���򵥵�һ�����������ͼʾ���Ѿ�չʾ���ؼ����� atan2 ��������������棩   
	double angle = atan2((double)(pStart.y - pEnd.y), (double)(pStart.x - pEnd.x));
	line(img, pStart, pEnd, color, thickness, lineType);
	//������Ǳߵ���һ�˵Ķ˵�λ�ã�����Ļ��������Ҫ����ͷ��ָ��Ҳ����pStart��pEnd��λ�ã� 
	arrow.x = pEnd.x + len * cos(angle + CV_PI * alpha / 180);
	arrow.y = pEnd.y + len * sin(angle + CV_PI * alpha / 180);
	line(img, pEnd, arrow, color, thickness, lineType);
	arrow.x = pEnd.x + len * cos(angle - CV_PI * alpha / 180);
	arrow.y = pEnd.y + len * sin(angle - CV_PI * alpha / 180);
	line(img, pEnd, arrow, color, thickness, lineType);
}


/**���ܶȲ������Ե�ǰͼ��ÿ���һ���ļ��������5�����أ���ȡһ�������㣬Ϊ���޳�һЩ�޹ؽ�Ҫ�ĵ㣬����Щ������о����������ֵ��С���򽫸õ��޳���
   ���⣬ͨ������mask��������mask���ǵ�������м���
   @param grey ����ĻҶ�ͼ��
   @param points ����Ĺ������
   @param quality ���˵�����������ֵռ�������ֵ�ı���
   @param min_distance ����Ŀռ��������
   @param mask ����ļ���ʱ��mask
*/
void DenseSample1(const Mat& grey, std::vector<Point2f>& points, const double quality, const int min_distance, Mat mask)
{
	//ȷ������ͼ��grey��mask��Сһ��
	assert(grey.size()==mask.size());

	//������ĺ�����������
	int colnum = grey.cols/min_distance;
	int rownum = grey.rows/min_distance;

	//����ÿ������λ�õ���С������ֵ
	Mat eig;
	cornerMinEigenVal(grey, eig, 3, 3);
	

	//�ҵ�������С������ֵ�е����ֵ��������һ��qualityϵ������Ϊ�о���ֵ
	double maxVal = 0;
	minMaxLoc(eig, 0, &maxVal);
	const double threshold = maxVal*quality;

	points.clear();
	int offset = min_distance/2;
	//����ÿһ��ϡ�������
	int i = 0, j = 0;
//#pragma omp for private(i, j)
	for(i = 0; i < rownum; i++)
		for(j = 0; j < colnum; j++) 
		{
			//ϡ�����ԭͼ���е�����
			int x = j*min_distance+offset;
			int y = i*min_distance+offset;

			if(eig.at<float>(y, x) > threshold && mask.at<uchar>(y,x)>0)
				points.push_back(Point2f(float(x), float(y)));

		}
}




/**  ���㵥�ܶȹ��� 
  @param img1 �����ǰ֡ͼ��
  @param img2 ����ĺ�֡ͼ��
  @param xflow �����x����λ��
  @param yflow �����y����λ��
  @param angle ����Ĺ����Ƕ�
  @param flowmag ����Ĺ�������
  @param sparseMask ����ļ����˹���������mask
  @param showMat ����Ĺ�����ʾͼ��
  @param min_distance ����ļ������ʱ�ռ�������
  @param mask ����ļ��������mask
  @param angleOffset ����Ƕ�ʱ���õ����
  @return һ��boolֵ����ʾ�����Ƿ�ɹ�
*/
bool calcSparseOpticalFlow(const Mat & img1, const Mat & img2, Mat & xflow, Mat & yflow, 
						   Mat & angle, Mat & flowmag, Mat & sparseMask, Mat & showMat,const int min_distance, const Mat & mask, const float angleOffset = 0.0f)
{
	size_t i;
	LKTracker track;

	//����ת���ɻҶ�ͼ��
	Mat gray1,gray2;
	cvtColor(img1,gray1,CV_BGR2GRAY);
	cvtColor(img2,gray2,CV_BGR2GRAY);

	//�Ե�һ��ͼ����е��ܶȲ���
	//std::vector<Point2f> points1(0);
	//DenseSample1(gray1, points1, 0.01, min_distance, mask);
	//DenseSample1(gray1, points1, 0.0000, min_distance, mask);
	//if (points1.size() == 0) 
		//return false;

	//Shi-Tomasi�ǵ�
	std::vector<Point2f> points1(0);

	int maxCorners = 1000;
	double qualityLevel = 0.001;

#ifdef  MEASURE_TIME
	clock_t startTime = clock();
#endif
	
#ifdef USE_CUDA 
	GoodFeaturesToTrackDetector_GPU* gfDetector = new GoodFeaturesToTrackDetector_GPU(maxCorners, qualityLevel, min_distance);
	GpuMat gGray1, gCorners, gMask;
	cout << "----------------------------------------------------EventDetection.h---------------------------------------" << endl;
	gGray1.upload(gray1);

	gMask.upload(mask);
	cout << "----------------------------------------------------EventDetection.h upload ���---------------------------------------" << endl;
#ifdef  MEASURE_TIME
	clock_t startTime1 = clock();
#endif
	gfDetector->operator()(gGray1, gCorners, gMask); //void operator ()(const GpuMat& image, GpuMat& corners, const GpuMat& mask = GpuMat());�����ڴ����

	gfDetector->releaseMemory();

#ifdef  MEASURE_TIME
	clock_t endTime1 = clock();
	printf("goodFeaturesToTrack pure time = %.3f\n", double(endTime1 - startTime1) / CLOCKS_PER_SEC);
#endif
	download(gCorners, points1);

#else
	goodFeaturesToTrack(gray1, points1, maxCorners, qualityLevel, min_distance, mask);
#endif

	



#ifdef  MEASURE_TIME
	clock_t endTime = clock();
	printf("goodFeaturesToTrack time = %.3f\n", double(endTime - startTime) / CLOCKS_PER_SEC);
#endif

	if (points1.size() == 0)
		return false;

	//����TLD��ǰ����У�����ٷ�������³������
	cout << "-----------------------------------------------EventDetection.h---------------------------------------����TLD��ǰ����У�����ٷ�������³������ " << endl;
	std::vector<Point2f> points2(0);				//??????????????????????
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

	if (!track.trackf2f(gray1, gray2, points1, points2)){
		printf("of_result: 0 matches!\n");
		points1.clear();
		points2.clear();
	}

	//����XFLOW��YFLOW�Լ�ϡ��mask
	xflow = Mat(gray1.size(),CV_32FC1,Scalar(0));
	yflow = Mat(gray1.size(),CV_32FC1,Scalar(0));
	angle = Mat(gray1.size(),CV_32FC1,Scalar(0));
	flowmag = Mat(gray1.size(),CV_32FC1,Scalar(0));
	sparseMask = Mat(gray1.size(),CV_8UC1,Scalar(0));

	//����
	for(i=0; i< points2.size(); i++)
	{
		if(points2[i].x > gray1.cols-1 || points2[i].x < 0 || points2[i].y > gray1.rows-1 || points2[i].y < 0)
			continue;

		sparseMask.at<uchar>(Point2i(points2[i])) = 255;

		float xdiff = points2[i].x-points1[i].x;
		float ydiff = points2[i].y-points1[i].y;
		float mag = sqrt(xdiff*xdiff+ydiff*ydiff);
		float angle_val = fastAtan2((float)(ydiff), (float)(xdiff)); 

		xflow.at<float>(Point2i(points2[i])) = xdiff;
		yflow.at<float>(Point2i(points2[i])) = ydiff;
		angle.at<float>(Point2i(points2[i])) = angle_val+ angleOffset;
		flowmag.at<float>(Point2i(points2[i])) = mag;
	}

#ifdef SHOW_RES
	//��ʾ����
	img2.copyTo(showMat);
	for(i=0; i< points2.size(); i++)
	{
		drawArrow(showMat,points2[i],points1[i],5,30,Scalar(0,0,255), 1, 8);
	}
#endif

	cout << "ϡ������������" << endl;


	return true;
}

#endif