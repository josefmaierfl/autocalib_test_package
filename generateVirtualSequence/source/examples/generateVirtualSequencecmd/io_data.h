//Released under the MIT License - https://opensource.org/licenses/MIT
//
//Copyright (c) 2020 Josef Maier
//
//Permission is hereby granted, free of charge, to any person obtaining
//a copy of this software and associated documentation files (the "Software"),
//to deal in the Software without restriction, including without limitation
//the rights to use, copy, modify, merge, publish, distribute, sublicense,
//and/or sell copies of the Software, and to permit persons to whom the
//Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included
//in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
//OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//Author: Josef Maier (josefjohann-dot-maier-at-gmail-dot-at)

#pragma once

#include <iostream>
#include <string>
#include <iostream>
//#include <stdio.h>
//#include <stdlib.h>
#include <vector>
//#include <cmath>
//#define _USE_MATH_DEFINES
//#include <math.h>       /* isnan, sqrt */
#include <utility>
#include "opencv2/highgui/highgui.hpp"

//#include "PfeImgFileIO.h"

//This function reads all stereo or 2 subsequent images from a given directory and stores their names into two vectors.
int loadStereoSequence(std::string filepath, std::string fileprefl, std::string fileprefr,
					   std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr);
//This function reads all images from a given directory and stores their names into a vector.
bool loadImageSequence(const std::string &filepath, const std::string &fileprefl, std::vector<std::string> &filenamesl);
//This function reads all stereo or 2 subsequent images from a given directory and stores their names into two vectors. Search patterns can be used.
int loadStereoSequenceNew(std::string filepath, std::string fileprefl, std::string fileprefr,
	std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr);
//This function reads all images from a given directory and stores their names into a vector. Search patterns can be used.
bool loadImageSequenceNew(std::string filepath, std::string fileprefl, std::vector<std::string> & filenamesl, bool isNoImg = false);

//Check for supported types of cv::imread
bool IsImgTypeSupported(std::string const& type);
std::vector<std::string> GetSupportedImgTypes();

//Check if the given directory exists
bool checkPathExists(const std::string &path);

//Check if a given file exists
bool checkFileExists(const std::string &filename);

//Delete a file
bool deleteFile(const std::string &filename);

//Create a new directory
bool createDirectory(const std::string &path);

//Delete directory tree
bool deleteDirectory(const std::string &path);

//Get all directoies within a folder
std::vector<std::string> getDirs(const std::string &path);

//Check if a given directory is empty
bool dirIsEmpty(const std::string &path);

//Count the number of files in the folder
size_t getNumberFilesInFolder(const std::string &path);

//Concatenate a main and a sub-path
std::string concatPath(const std::string &mainPath, const std::string &subPath);

//Return only the filename with extension from a string made of path + filename
std::string getFilenameFromPath(const std::string &name);

//Returns the parent path
std::string getParentPath(const std::string &path);

//Remove extension from filename
std::string remFileExt(const std::string &name);

//Return execution path without filename of the current executable
bool getCurrentExecPath(std::string &path);

//Reads all homography file names (Oxford dataset: www.robots.ox.ac.uk/~vgg/research/affine/) from a given directory and stores their names into a vector.
bool readHomographyFiles(const std::string& filepath, const std::string& fileprefl, std::vector<std::string> & filenamesl);

//Reads a homography from a given file (Oxford dataset: www.robots.ox.ac.uk/~vgg/research/affine/)
bool readHomographyFromFile(const std::string& filepath, const std::string& filename, cv::OutputArray H);

//Read a 3-channel uint16 image and convert to flow
bool convertImageFlowFile(const std::string &filename, std::vector<cv::Point2f> *positionI1 = nullptr,
                                                         std::vector<cv::Point2f> *positionI2 = nullptr, cv::OutputArray flow3 = cv::noArray(), float precision = 64.f,
                                                         bool useBoolValidity = true, float validityPrecision = 64.f, float minConfidence = 1.f);

//Read a 1- or 3-channel uint16 image and convert to disparity using the same output format as for flow
bool convertImageDisparityFile(const std::string &filename, std::vector<cv::Point2f> *positionI1 = nullptr,
                                                              std::vector<cv::Point2f> *positionI2 = nullptr, cv::OutputArray flow3 = cv::noArray(),
                                                              bool useFLowStyle = false, float precision = 256.f, bool use0Invalid = true);
//Convert a 3 channel floating point flow matrix (x, y, last channel corresp. to validity) to a 3-channel uint16 png image (same format as KITTI)
bool writeKittiFlowFile(const std::string &filename, const cv::Mat &flow, float precision = 64.f,
                                                       bool useBoolValidity = true, float validityPrecision = 64.f);
