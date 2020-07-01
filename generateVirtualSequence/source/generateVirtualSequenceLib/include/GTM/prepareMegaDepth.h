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
//
// Created by maierj on 5/11/20.
//

#ifndef GENERATEVIRTUALSEQUENCE_PREPAREMEGADEPTH_H
#define GENERATEVIRTUALSEQUENCE_PREPAREMEGADEPTH_H

#include <glob_includes.h>
#include <opencv2/highgui.hpp>

struct megaDepthFolders{
    std::string mdImgF;//Folder holding images of a MegaDepth sub-set
    std::string mdDepth;//Folder holding depth files of a MegaDepth sub-set
    std::string sfmF;//Folder holding SfM files of a MegaDepth sub-set
    std::string sfmImgF;//Folder holding original images used within SfM of a MegaDepth sub-set
    std::string depthExt;//Extension of depth files

    megaDepthFolders(std::string &&mdImgF_, std::string &&mdDepth_, std::string &&sfmF_, std::string sfmImgF_, std::string depthExt_):
            mdImgF(move(mdImgF_)), mdDepth(move(mdDepth_)), sfmF(move(sfmF_)), sfmImgF(move(sfmImgF_)), depthExt(move(depthExt_)){}

    megaDepthFolders()= default;
};

struct megaDepthData{
    std::string img1_name;//Name and path to first image
    std::string img2_name;//Name and path to first image
    cv::Mat flow;//Optical flow data

    megaDepthData(std::string &&img1_name_, std::string &&img2_name_, cv::Mat &&flow_):
            img1_name(std::move(img1_name_)), img2_name(std::move(img2_name_)), flow(std::move(flow_)){};
};

bool convertMegaDepthData(const megaDepthFolders& folders,
                          const std::string &flowSubFolder,
                          std::vector<megaDepthData> &data,
                          uint32_t verbose_ = 0,
                          int CeresCPUcnt_ = -1);

#endif //GENERATEVIRTUALSEQUENCE_PREPAREMEGADEPTH_H
