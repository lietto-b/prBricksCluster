#include <iostream>
#include <fstream>
#include <string>

#include <opencv/cv.h>
#include <opencv2/imgproc.hpp>
#include <opencv/highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <ctime>
#include <windows.h>

#include <vector>
#include <algorithm>

using namespace std;
using namespace cv;

// получение пикселя изображения (по типу картинки и координатам)
#define CV_PIXEL(type,img,x,y) (((type*)((img)->imageData+(y)*(img)->widthStep))+(x)*(img)->nChannels)

const CvScalar BLACK = CV_RGB(0, 0, 0);				// чёрный
const CvScalar WHITE = CV_RGB(255, 255, 255);		// белый
const CvScalar RED = CV_RGB(255, 0, 0);				// красный
const CvScalar ORANGE = CV_RGB(255, 100, 0);		// оранжевый 
const CvScalar YELLOW = CV_RGB(255, 255, 0);		// жёлтый 
const CvScalar GREEN = CV_RGB(0, 255, 0);			// зелёный  
const CvScalar LIGHTBLUE = CV_RGB(60, 170, 255);	// голубой 
const CvScalar BLUE = CV_RGB(0, 0, 255);			// синий 
const CvScalar VIOLET = CV_RGB(194, 0, 255);		// фиолетовый
const CvScalar GINGER = CV_RGB(215, 125, 49);		// рыжий
const CvScalar PINK = CV_RGB(255, 192, 203);		// розовый
const CvScalar LIGHTGREEN = CV_RGB(153, 255, 153);	// салатовый
const CvScalar BROWN = CV_RGB(150, 75, 0);			// коричневый 

typedef unsigned char uchar;
typedef unsigned int uint;

typedef struct ColorCluster {
	CvScalar color;
	CvScalar new_color;
	int count;

	ColorCluster() :count(0) {
	}
} ColorCluster;

float rgb_euclidean(CvScalar p1, CvScalar p2)
{
	float val = sqrtf((p1.val[0] - p2.val[0]) * (p1.val[0] - p2.val[0]) +
		(p1.val[1] - p2.val[1]) * (p1.val[1] - p2.val[1]) +
		(p1.val[2] - p2.val[2]) * (p1.val[2] - p2.val[2]) +
		(p1.val[3] - p2.val[3]) * (p1.val[3] - p2.val[3]));

	return val;
}

// сортировка цветов по количеству
bool colors_sort(std::pair< int, uint > a, std::pair< int, uint > b)
{
	return (a.second > b.second);
}

void invmask(Mat left, Mat right);

int main(int argc, char* argv[])
{
	Mat br_origin = imread("origin.png");
	Mat br_mask = imread("mask.png");

	invmask(br_origin, br_mask);
	imwrite("orgmsk.png", br_origin);
	//imwrite("orgmsk2.png", br_mask);

	system("pause");

	IplImage* image = 0, *src = 0, *dst = 0, *dst2 = 0;
	char img_name[] = "orgmsk.png";
	char* image_filename = argc >= 2 ? argv[1] : img_name;
	image = cvLoadImage(image_filename, 1);
	printf("[i] image: %s\n", image_filename);
	if (!image) {
		printf("[!] Error: cant load test image: %s\n", image_filename);
		return -1;
	}

	cvNamedWindow("image");
	cvShowImage("image", image);
	// ресайзим картинку (для скорости обработки)
	src = cvCreateImage(cvSize(image->width / 2, image->height / 2), IPL_DEPTH_8U, 3);
	cvResize(image, src, CV_INTER_LINEAR);

	cvNamedWindow("img");
	cvShowImage("img", src);

	// картинка для хранения индексов кластеров
	IplImage* cluster_indexes = cvCreateImage(cvSize(image->width / 2, image->height / 2), IPL_DEPTH_8U, 1);
	cvZero(cluster_indexes);

#define CLUSTER_COUNT 2
	int cluster_count = CLUSTER_COUNT;
	ColorCluster clusters[CLUSTER_COUNT];

	int i = 0, j = 0, k = 0, x = 0, y = 0;

	// начальные цвета кластеров
#if 0
	clusters[0].new_color = RED;
	clusters[1].new_color = ORANGE;
	clusters[2].new_color = YELLOW;
	//	clusters[3].new_color = GREEN;
	//	clusters[4].new_color = LIGHTBLUE;
	//	clusters[5].new_color = BLUE;
	//	clusters[6].new_color = VIOLET;
#elif 0
	clusters[0].new_color = BLACK;
	clusters[1].new_color = GREEN;
	clusters[2].new_color = WHITE;
#else
	CvRNG rng = cvRNG(-1);
	for (k = 0; k < cluster_count; k++)
		clusters[k].new_color = CV_RGB(cvRandInt(&rng) % 255, cvRandInt(&rng) % 255, cvRandInt(&rng) % 255);
#endif

	// k-means
	float min_rgb_euclidean = 0, old_rgb_euclidean = 0;

	while (1) {
		for (k = 0; k < cluster_count; k++) {
			clusters[k].count = 0;
			clusters[k].color = clusters[k].new_color;
			clusters[k].new_color = cvScalarAll(0);
		}

		for (y = 0; y < src->height; y++) {
			for (x = 0; x < src->width; x++) {
				// получаем RGB-компоненты пикселя
				uchar B = CV_PIXEL(uchar, src, x, y)[0];	// B
				uchar G = CV_PIXEL(uchar, src, x, y)[1];	// G
				uchar R = CV_PIXEL(uchar, src, x, y)[2];	// R

				min_rgb_euclidean = 255 * 255 * 255;
				int cluster_index = -1;
				for (k = 0; k < cluster_count; k++) {
					float euclid = rgb_euclidean(cvScalar(B, G, R, 0), clusters[k].color);
					if (euclid < min_rgb_euclidean) {
						min_rgb_euclidean = euclid;
						cluster_index = k;
					}
				}
				// устанавливаем индекс кластера
				CV_PIXEL(uchar, cluster_indexes, x, y)[0] = cluster_index;

				clusters[cluster_index].count++;
				clusters[cluster_index].new_color.val[0] += B;
				clusters[cluster_index].new_color.val[1] += G;
				clusters[cluster_index].new_color.val[2] += R;
			}
		}

		min_rgb_euclidean = 0;
		for (k = 0; k < cluster_count; k++) {
			// new color
			clusters[k].new_color.val[0] /= clusters[k].count;
			clusters[k].new_color.val[1] /= clusters[k].count;
			clusters[k].new_color.val[2] /= clusters[k].count;
			float ecli = rgb_euclidean(clusters[k].new_color, clusters[k].color);
			if (ecli > min_rgb_euclidean)
				min_rgb_euclidean = ecli;
		}

		//printf("%f\n", min_rgb_euclidean);
		if (fabs(min_rgb_euclidean - old_rgb_euclidean) < 1)
			break;

		old_rgb_euclidean = min_rgb_euclidean;
	}
	//-----------------------------------------------------

	// теперь загоним массив в вектор и отсортируем :)
	std::vector< std::pair< int, uint > > colors;
	colors.reserve(CLUSTER_COUNT);

	int colors_count = 0;
	for (i = 0; i < CLUSTER_COUNT; i++) {
		std::pair< int, uint > color;
		color.first = i;
		color.second = clusters[i].count;
		colors.push_back(color);
		if (clusters[i].count > 0)
			colors_count++;
	}
	// сортируем
	std::sort(colors.begin(), colors.end(), colors_sort);

	// покажем цвета
	dst2 = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 3);
	cvZero(dst2);
	int h = dst2->height / CLUSTER_COUNT;
	int w = dst2->width;
	ofstream fout("color.txt", ios_base::app);
	for (i = 0; i < CLUSTER_COUNT; i++) {
		cvRectangle(dst2, cvPoint(0, i * h), cvPoint(w, i * h + h), clusters[colors[i].first].color, -1);
		printf("[i] Color: %d %d %d (%d)\n", (int)clusters[colors[i].first].color.val[2],
			(int)clusters[colors[i].first].color.val[1],
			(int)clusters[colors[i].first].color.val[0],
			clusters[colors[i].first].count);
		if (clusters[colors[i].first].count != 0) {
			fout << (int)clusters[colors[i].first].color.val[2] << " " <<
				(int)clusters[colors[i].first].color.val[1] << " " <<
				(int)clusters[colors[i].first].color.val[0] << endl;
		}
	}

	fout.close();

	cvNamedWindow("colors");
	cvShowImage("colors", dst2);
	//cvResize(dst2, image, CV_INTER_LINEAR);
	//cvSaveImage("dominate_colors_table.png", image);
	//-----------------------------------------------------

	// покажем картинку в найденных цветах
	dst = cvCloneImage(src);
	for (y = 0; y < dst->height; y++) {
		for (x = 0; x < dst->width; x++) {
			int cluster_index = CV_PIXEL(uchar, cluster_indexes, x, y)[0];

			CV_PIXEL(uchar, dst, x, y)[0] = clusters[cluster_index].color.val[0];
			CV_PIXEL(uchar, dst, x, y)[1] = clusters[cluster_index].color.val[1];
			CV_PIXEL(uchar, dst, x, y)[2] = clusters[cluster_index].color.val[2];
		}
	}

	cvNamedWindow("dst");
	cvShowImage("dst", dst);
	//cvResize(dst, image, CV_INTER_LINEAR);
	//cvSaveImage("dominate_colors.png", image);

	// ждём нажатия клавиши
	cvWaitKey(0);

	// освобождаем ресурсы
	cvReleaseImage(&image);
	cvReleaseImage(&src);

	cvReleaseImage(&cluster_indexes);

	cvReleaseImage(&dst);
	cvReleaseImage(&dst2);

	// удаляем окна
	cvDestroyAllWindows();

	system("pause");
	return 0;
}

void invmask(Mat org, Mat msk)
{
	int i, j = 0;
	int R, G, B = 0;

	for (i = 0; i < msk.cols; i++)
	{
		for (j = 0; j < msk.rows; j++)
		{
			Vec3b pxl = msk.at<Vec3b>(j, i);
			R = pxl[2];
			G = pxl[1];
			B = pxl[0];
			//cout << "R: " << R << ", G: " << G << ", B: " << B << endl;

			if ((R + G + B) == 0) //if black
			{
				org.at<Vec3b>(j, i) = pxl;
			}
		}
	}
}