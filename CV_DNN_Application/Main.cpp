
/*

Caffeの学習済みモデルを用いて画像を分類する。

参考URL

[1] OpenCV: Load Caffe framework models
    https://docs.opencv.org/3.4.0/d5/de7/tutorial_dnn_googlenet.html

	https://raw.githubusercontent.com/opencv/opencv/master/samples/data/dnn/bvlc_googlenet.prototxt
	http://dl.caffe.berkeleyvision.org/bvlc_googlenet.caffemodel
	

[2] OpenCV 3.4.0 reference
    https://docs.opencv.org/3.4.0/

[3] OpenCV: Deep Neural Network module
    https://docs.opencv.org/3.4.0/d6/d0f/group__dnn.html

[4] OpenCV: cv::dnn::Net Class Reference
    https://docs.opencv.org/3.4.0/db/d30/classcv_1_1dnn_1_1Net.html



*/

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <fstream>
#include <iostream>
#include <cstdlib>

/* read classes labels from file */
static std::vector<cv::String> readClassNames(const char * const filename)
{
	std::ifstream fp(filename);
	if (!fp.is_open())
	{
		std::cerr << "File with classes labels not found: " << filename << std::endl;
		exit(-1);
	}

	std::vector<cv::String> classNames;
	while (!fp.eof())
	{
		std::string name;
		std::getline(fp, name);
		if (name.length())
			classNames.push_back(name.substr(name.find(' ') + 1));
	}
	fp.close();

	return classNames;
}

static void doDnn(int argc, char **argv)
{
	// ------------------------------------------------------------------ コマンドライン引数を解析する

	// コマンドライン引数の定義
	static const auto params =
		"{ help           | false | Sample app for loading googlenet model }"	// 
		"{ proto          | bvlc_googlenet.prototxt | model configuration }"	// Neural Networkのモデル(Caffe形式)を格納したファイル
		"{ model          | bvlc_googlenet.caffemodel | model weights }"		// Neural Networkの学習結果(重み)(Caffe形式)を格納したファイル
		"{ image          | space_shuttle.jpg | path to image file }"			// Neural Networkで分類する画像ファイル
		"{ opencl         | false | enable OpenCL }"							// 演算にOpenCL(GPUで演算)を用いるかどうかを指定する
		;

	// コマンドライン引数を解析する
	const cv::CommandLineParser parser(argc, argv, params);
	if (parser.get<bool>("help"))
	{
		parser.printMessage();
	}

	// コマンドラインオプションを得る
	const auto modelTxt = parser.get<cv::String>("proto");	// Neural Networkのモデル(Caffe形式)をファイルで与える
	const auto modelBin = parser.get<cv::String>("model");	// Neural Networkの学習結果(重み)(Caffe形式)をファイルで与える
	const auto imageFile = parser.get<cv::String>("image");	// Neural Networkで分類する画像ファイル
	const auto opencl = parser.get<bool>("opencl");			// 演算にOpenCL(GPUで演算)を用いるかどうかを指定する


	// ------------------------------------------------------------------ 学習済みモデルをファイルから読み込む

	cv::dnn::Net net;
	try {
		// Caffe形式のモデルと学習結果をファイルから読む
		net = cv::dnn::readNetFromCaffe(modelTxt, modelBin);
	}
	catch (cv::Exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		if (net.empty())
		{
			// モデルと学習結果のダウンロードURLを表示する
			std::cerr << "Can't load network by using the following files: " << std::endl;
			std::cerr << "prototxt:   " << modelTxt << std::endl;
			std::cerr << "caffemodel: " << modelBin << std::endl;
			std::cerr << "bvlc_googlenet.caffemodel can be downloaded here:" << std::endl;
			std::cerr << "http://dl.caffe.berkeleyvision.org/bvlc_googlenet.caffemodel" << std::endl;
			exit(-1);
		}
	}

	// ------------------------------------------------------------------ OpenCLを使うならフラグを立てる
	if (opencl)
	{
		net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL);
	}

	// ------------------------------------------------------------------ 画像を読み込む

	// 入力画像をファイルから読み込む
	const auto img = imread(imageFile);
	if (img.empty())
	{
		std::cerr << "Can't read image from the file: " << imageFile << std::endl;
		exit(-1);
	}

	// 画像をNeural Networkの入力用の4次元のblob(Binary Large OBject) に変換する。
	// GoogLeNet の入力は「224x224 ピクセルで、Blue/Green/Redの順」なので、そのように画像を変形する。
	const auto inputBlob = cv::dnn::blobFromImage(
		img,                         // 入力画像
		1.0f,                        // 画素値への乗数
		cv::Size(224, 224),          // 出力する画像(blob)の縦横の大きさ
		cv::Scalar(104, 117, 123),   // 学習データの平均
		false                        // RedとBlueを入れ替えない
	);

	// ------------------------------------------------------------------ NNを使って画像を分類する
	net.setInput(inputBlob, "data");     // set the network input
	auto prob = net.forward("prob");     // compute output
	for (int i = 0; i < 10; i++)
	{
		net.setInput(inputBlob, "data"); // set the network input
		prob = net.forward("prob");      // compute output
	}

	// ------------------------------------------------------------------ 
	const auto probMat = prob.reshape(1, 1); //reshape the blob to 1x1000 matrix

	double classProb;
	cv::Point classNumber;
	cv::minMaxLoc(probMat, NULL, &classProb, NULL, &classNumber);
	const auto classId = classNumber.x;

	// ------------------------------------------------------------------ 識別結果を出力（表示）する
	const auto classNames = readClassNames("synset_words.txt");
	std::cout << "Best class: #" << classId << " '" << classNames.at(classId) << "'" << std::endl;
	std::cout << "Probability: " << classProb * 100 << "%" << std::endl;
}

int main(int argc, char **argv)
{
	doDnn(argc, argv);
	return 0;
}
