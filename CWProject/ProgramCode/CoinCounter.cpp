#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <iomanip>
#include <algorithm>
#include <filesystem>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

static vector<vector<double>> coinsCombinations;
const vector<double> defaultDiameters = { 15.5, 17.5, 18.5, 19.5, 20.5, 22.0, 23.0, 25.0 };
const double coinsNominals[] = { 0.01, 0.1, 0.05, 0.5, 1.0, 10.0, 2.0, 5.0 };

string getUserImagePath() {
    string imagePath;
    bool validPath = false;

    do {
        cout << "Enter the path to image: ";
        getline(cin, imagePath);

        if (fs::exists(imagePath)) {
            validPath = true;
        }
        else {
            cout << "Error: The file was not found. Please make sure that you have specified the correct path." << endl;
        }
    } while (!validPath);

    return imagePath;
}

void generateCombinations(const vector<double>& arr, int n, vector<double>& combination, int index) {
    if (combination.size() == n) {
        coinsCombinations.push_back(combination);
        return;
    }

    for (int i = index; i < arr.size(); ++i) {
        if (combination.empty() || arr[i] >= combination.back()) {
            combination.push_back(arr[i]);
            generateCombinations(arr, n, combination, i);
            combination.pop_back();
        }
    }
}

void showImg(string name, Mat img) {
    namedWindow(name, WINDOW_GUI_EXPANDED);
    resizeWindow(name, img.cols / 2, img.rows / 2);
    imshow(name, img);
}

Mat imgRead(string path) {
    Mat imgRead;

    string extension = path.substr(path.find_last_of(".") + 1);
    if (extension != "jpg" && extension != "jpeg" && extension != "png" && extension != "bmp") {
        cerr << "Error: The specified file is not an image." << endl;
        return imgRead;
    }

    imgRead = imread(path);
    if (imgRead.empty()) {
        cerr << "Failed to upload image." << endl;
    }
    return imgRead;
}


bool isCircle(const vector<Point>& contour, double epsilon)
{
    double arcLength = cv::arcLength(contour, true);
    double area = cv::contourArea(contour);
    double circularity = 4 * CV_PI * area / (arcLength * arcLength);
    return (circularity > epsilon);
}

bool compareContourAreas(vector<Point> contour1, vector<Point> contour2) {
    double area1 = contourArea(contour1);
    double area2 = contourArea(contour2);
    return (area1 < area2);
}

void printResult(const double coinsNominals[], const int coinsCounter[], int size) {
    cout << "<Coin> : <Counter>" << endl;
    for (int i = 0; i < size; ++i) {
        if (coinsNominals[i] < 1.0) {
            cout << static_cast<int>(coinsNominals[i] * 100) << " KOP : " << coinsCounter[i] << " pcs." << endl;
        }
        else {
            cout << static_cast<int>(coinsNominals[i]) << " RUB : " << coinsCounter[i] << " pcs." << endl;
        }
    }
}

int main()
{
    Mat img;
    string path;
    bool validImage = false;

    do {
        path = getUserImagePath();
        img = imgRead(path);

        if (!img.empty()) {
            validImage = true;
        }
    } while (!validImage);

    Mat imgBlur, imgGray, imgCanny, imgClosed;

    GaussianBlur(img, imgBlur, Size(5, 5), 3);

    cvtColor(imgBlur, imgGray, COLOR_BGR2GRAY);

    threshold(imgGray, imgGray, 200, 255, THRESH_BINARY_INV);

    Canny(imgGray, imgCanny, 5, 50);

    Mat kernel = getStructuringElement(MORPH_RECT, Size(7, 7));
    morphologyEx(imgCanny, imgClosed, MORPH_CLOSE, kernel, Point(-1, -1), 2);

    vector<vector<Point>> contours;
    findContours(imgClosed, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


    vector<vector<Point>> circles;
    vector<vector<Point>> other_contours;


    for (const auto& contour : contours) {
        double area = contourArea(contour);
        vector<Point> approx;
        approxPolyDP(contour, approx, 0.01 * arcLength(contour, true), true);
        bool isContourCircle = false;
        if (isCircle(approx, 0.95)) {
            isContourCircle = true;
        }
        if (isContourCircle) {
            circles.push_back(contour);
        }
        else {
            other_contours.push_back(contour);
        }
    }

    int coinsCounter[8] = { 0 };
    double result = 0;

    if (circles.empty()) {
        cout << "\n\nThe total value of the coins in the photo: " << result << " RUB" << endl << endl;
        printResult(coinsNominals, coinsCounter, 8);
        showImg("IMG", img);
        return 0;
    }

    sort(circles.begin(), circles.end(), compareContourAreas);

    vector<double> combination;

    generateCombinations(defaultDiameters, circles.size(), combination, 0);

    vector<double> circlesLength;
    for (int i = 0; i < circles.size(); ++i) {
        circlesLength.push_back(arcLength(circles[i], true));
    }

    int bestCombinationIndex = -1;
    double optimisationFunc = circles.size();
    double currentFunc = 0;
    for (int i = 0; i < coinsCombinations.size(); ++i) {
        for (int j = 0; j < coinsCombinations[i].size() - 1; ++j) {
            double combinationRatio;
            double circleRatio;
            double diff;
            combinationRatio = coinsCombinations[i][j] / coinsCombinations[i][j + 1];
            circleRatio = circlesLength[j] / circlesLength[j + 1];
            diff = abs(combinationRatio - circleRatio);
            currentFunc += diff;
        }
        if (currentFunc < optimisationFunc && currentFunc > 0) {
            optimisationFunc = currentFunc;
            bestCombinationIndex = i;
        }
        currentFunc = 0;
    }

    vector<double> bestCoins;
    for (int i = 0; i < coinsCombinations[bestCombinationIndex].size(); ++i) {
        for (int j = 0; j < defaultDiameters.size(); ++j) {
            if (coinsCombinations[bestCombinationIndex][i] - defaultDiameters[j] == 0) {
                bestCoins.push_back(coinsNominals[j]);
                coinsCounter[j] += 1;
            }
        }
    }

    drawContours(img, circles, -1, Scalar(255, 0, 255), 7);
    showImg("IMG", img);

    for (int i = 0; i < bestCoins.size(); ++i) {
        result += bestCoins[i];
    }

    cout << "\n\nThe total value of the coins in the photo: " << result << " RUB" << endl << endl;
    printResult(coinsNominals, coinsCounter, 8);

    waitKey(0);

    return 0;
}
