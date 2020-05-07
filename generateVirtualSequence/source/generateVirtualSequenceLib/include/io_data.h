/**********************************************************************************************************
 FILE: io_data.h

 PLATFORM: Windows 7, MS Visual Studio 2010, OpenCV 2.4.2

 CODE: C++
 
 AUTOR: Josef Maier, AIT Austrian Institute of Technology

 DATE: September 2015

 LOCATION: TechGate Vienna, Donau-City-Stra�e 1, 1220 Vienna

 VERSION: 1.0

 DISCRIPTION: This file holds functionalities for loading image data, flow data from the KITTI dataset,
 flow data from O. Zendel's generated virtuel images and homographys from files of the test data
 from K. Mikolajczyk. Moreover, functions are provided to generate the different quality measurements
 for the matching algorithms.
**********************************************************************************************************/

#pragma once

#include "glob_includes.h"
#include "opencv2/highgui/highgui.hpp"

#include "generateVirtualSequenceLib/generateVirtualSequenceLib_api.h"

//#include "PfeImgFileIO.h"

//This function reads all stereo or 2 subsequent images from a given directory and stores their names into two vectors.
int GENERATEVIRTUALSEQUENCELIB_API loadStereoSequence(std::string filepath, std::string fileprefl, std::string fileprefr,
					   std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr);
//This function reads all images from a given directory and stores their names into a vector.
bool GENERATEVIRTUALSEQUENCELIB_API loadImageSequence(const std::string &filepath, const std::string &fileprefl, std::vector<std::string> &filenamesl);
//This function reads all stereo or 2 subsequent images from a given directory and stores their names into two vectors. Search patterns can be used.
int GENERATEVIRTUALSEQUENCELIB_API loadStereoSequenceNew(std::string filepath, std::string fileprefl, std::string fileprefr,
	std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr);
//This function reads all images from a given directory and stores their names into a vector. Search patterns can be used.
int GENERATEVIRTUALSEQUENCELIB_API loadImageSequenceNew(std::string filepath, std::string fileprefl, std::vector<std::string> & filenamesl);

//Check for supported types if cv::imread
bool IsImgTypeSupported(std::string const& type);
std::vector<std::string> GetSupportedImgTypes();

//Check if the given directory exists
bool GENERATEVIRTUALSEQUENCELIB_API checkPathExists(const std::string &path);

//Check if a given file exists
bool GENERATEVIRTUALSEQUENCELIB_API checkFileExists(const std::string &filename);

//Delete a file
bool GENERATEVIRTUALSEQUENCELIB_API deleteFile(const std::string &filename);

//Create a new directory
bool GENERATEVIRTUALSEQUENCELIB_API createDirectory(const std::string &path);

//Delete directory tree
bool GENERATEVIRTUALSEQUENCELIB_API deleteDirectory(const std::string &path);

//Concatenate a main and a sub-path
std::string GENERATEVIRTUALSEQUENCELIB_API concatPath(const std::string &mainPath, const std::string &subPath);

//Return only the filename with extension from a string made of path + filename
std::string getFilenameFromPath(const std::string &name);

//Remove extension from filename
std::string remFileExt(const std::string &name);

//Reads all homography file names (Oxford dataset: www.robots.ox.ac.uk/~vgg/research/affine/) from a given directory and stores their names into a vector.
bool GENERATEVIRTUALSEQUENCELIB_API readHomographyFiles(const std::string& filepath, const std::string& fileprefl, std::vector<std::string> & filenamesl);

//Reads a homography from a given file (Oxford dataset: www.robots.ox.ac.uk/~vgg/research/affine/)
bool GENERATEVIRTUALSEQUENCELIB_API readHomographyFromFile(const std::string& filepath, const std::string& filename, cv::OutputArray H);

//Read a 3-channel uint16 image and convert to flow
bool GENERATEVIRTUALSEQUENCELIB_API convertImageFlowFile(const std::string &filename, std::vector<cv::Point2f> &positionI1,
                                                         std::vector<cv::Point2f> &positionI2, cv::OutputArray flow3 = cv::noArray(), float precision = 64.f,
                                                         bool useBoolValidity = true, float validityPrecision = 64.f, float minConfidence = 1.f);

//Read a 1- or 3-channel uint16 image and convert to disparity using the same output format as for flow
bool GENERATEVIRTUALSEQUENCELIB_API convertImageDisparityFile(const std::string &filename, std::vector<cv::Point2f> &positionI1,
                                                              std::vector<cv::Point2f> &positionI2, cv::OutputArray flow3 = cv::noArray(),
                                                              bool useFLowStyle = false, float precision = 256.f, bool use0Invalid = true);

/*!***************************************************************************
* PURPOSE: returns cv::Mat datastructure with PfePixImgStruct data
*
* DESCRIPTION:
*
* RETURN:
*     cv::Mat
****************************************************************************!*/
//static cv::Mat PfeConvToMat
//  ( PfePixImgStruct *   pPfe
//  )
//{
//  int type = CV_8UC1;
//
//  if ( pPfe->u16BitsPerPixel == 8 )
//  {
//    type = CV_8UC1;
//  }
//  else if ( pPfe->u16BitsPerPixel == 16 )
//  {
//    type = CV_16UC1;
//  }
//  else if ( pPfe->u16BitsPerPixel == 24 )
//  {
//    type = CV_8UC3;
//  }
//  else if ( pPfe->u16BitsPerPixel == 32 )
//  {
//    type = CV_32FC1;
//  }
//  else if ( pPfe->u16BitsPerPixel == 48 )
//  {
//	  type = CV_16UC3;
//  }
//
//  return cv::Mat
//      ( pPfe->u32Height         // _rows
//      , pPfe->u32Width          // _cols
//      , type                    // _type
//      , pPfe->pBits             // _data
//      , pPfe->u32BytesPerLine   // _step
//      );
//}