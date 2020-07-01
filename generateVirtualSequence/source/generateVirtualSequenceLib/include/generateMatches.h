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

#ifndef GENERATEVIRTUALSEQUENCE_GENERATEMATCHES_H
#define GENERATEVIRTUALSEQUENCE_GENERATEMATCHES_H

#include "generateSequence.h"
#include <opencv2/core/types.hpp>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include "GTM/base_matcher.h"

struct GENERATEVIRTUALSEQUENCELIB_API GenMatchSequParameters {
    std::string mainStorePath;//Path for storing results. If empty and the 3D correspondences are loaded from file, the path for loading these correspondences is also used for storing the matches
    std::string imgPath;//Path containing the images for producing keypoint patches
    std::string imgPrePostFix;//image pre- and/or postfix (supports wildcards & subfolders) for images within imgPath
    std::string keyPointType;//Name of keypoint detector
    std::string descriptorType;//Name of descriptor extractor
    bool keypPosErrType;//Keypoint detector error (true) or error normal distribution (false)
    std::pair<double, double> keypErrDistr;//Keypoint error distribution (mean, std) for the matching keypoint location
    std::pair<double, double> imgIntNoise;//Noise (mean, std) on the image intensity for descriptor calculation
//    double lostCorrPor;//Portion (0 to 0.9) of lost correspondences from frame to frame. It corresponds to the portion of backprojected 3D-points that should not be visible in the frame.
    bool storePtClouds;//If true, all PCL point clouds and necessary information to load a cam sequence with correspondences are stored to disk
    bool rwXMLinfo;//If true, the parameters and information are stored and read in XML format. Otherwise it is stored or read in YAML format
    bool compressedWrittenInfo;//If true, the stored information and parameters are compressed (appends .gz) and it is assumed that paramter files to be read are aslo compressed
    bool takeLessFramesIfLessKeyP;//If true and too less images images are provided (resulting in too less keypoints), only as many frames with GT matches are provided as keypoints are available.
    bool checkDescriptorDist;//If true, TP and TN descriptors are only accepted if their descriptor distances between correspondences match the distribution calculated on the given images.
    std::pair<double, double> repeatPatternPortStereo;//Minimal and maximal percentage (0 to 1.0) of repeated patterns (image patches) between stereo cameras.
    std::pair<double, double> repeatPatternPortFToF;//Minimal and maximal percentage (0 to 1.0) of repeated patterns (image patches) from frame to frame.
    bool distortPatchCam1;//If true, tracked image patch in the first stereo image are distorted
    double oxfordGTMportion;//Portion of GT matches from Oxford dataset (GT = homographies)
    double kittiGTMportion;//Portion of GT matches from KITTI dataset (GT = flow, disparity)
    double megadepthGTMportion;//Portion of GT matches from MegaDepth dataset (GT = depth)
    double GTMportion;//Portion of GT matches (GTM) compared to warped patch correspondences if multiple datasets are used as source (Oxford, KITTI, MegaDepth)
    double WarpedPortionTN;//Portion of TN that should be drawn from warped image patches (and not from GTM).
    double portionGrossTN;//Portion of TN that should be from GTM or from different image patches (first <-> second stereo camera).
    int CeresCPUcnt;//Number of CPUs used by CERES for relative pose refinement of MegaDepth data. A value of -1 indicates to use all available CPUs.
    std::string execPath;//Path of this executable (main)
    std::vector<std::string> imageNetIDs;//List of WNIDs from ImageNet to use for download
    std::vector<std::string> imageNetBuzzWrds;//List of buzzwords to search for on ImageNet
    int nrImgsFromImageNet;//Number of images that should be downloaded from ImageNet
    bool parsValid;//Specifies, if the stored values within this struct are valid

    GenMatchSequParameters(std::string mainStorePath_,
                           std::string imgPath_,
                           std::string imgPrePostFix_,
                           std::string keyPointType_,
                           std::string descriptorType_,
                           bool keypPosErrType_ = false,
                           std::pair<double, double> keypErrDistr_ = std::make_pair(0, 0.5),
                           std::pair<double, double> imgIntNoise_ = std::make_pair(0, 5.0),
//                           double lostCorrPor_ = 0,
                           bool storePtClouds_ = false,
                           bool rwXMLinfo_ = false,
                           bool compressedWrittenInfo_ = false,
                           bool takeLessFramesIfLessKeyP_ = false,
                           bool checkDescriptorDist_ = true,
                           std::pair<double, double> repeatPatternPortStereo_ = std::make_pair(0, 0),
                           std::pair<double, double> repeatPatternPortFToF_ = std::make_pair(0, 0),
                           bool distortPatchCam1_ = false,
                           double oxfordGTMportion_ = 0,
                           double kittiGTMportion_ = 0,
                           double megadepthGTMportion_ = 0,
                           double GTMportion_ = 0,
                           double WarpedPortionTN_ = 1.0,
                           double portionGrossTN_ = 0,
                           int CeresCPUcnt_ = -1,
                           std::string execPath_ = "",
                           std::vector<std::string> imageNetIDs_ = std::vector<std::string>(),
                           std::vector<std::string> imageNetBuzzWrds_ = std::vector<std::string>(),
                           int nrImgsFromImageNet_ = 0):
            mainStorePath(std::move(mainStorePath_)),
            imgPath(std::move(imgPath_)),
            imgPrePostFix(std::move(imgPrePostFix_)),
            keyPointType(std::move(keyPointType_)),
            descriptorType(std::move(descriptorType_)),
            keypPosErrType(keypPosErrType_),
            keypErrDistr(std::move(keypErrDistr_)),
            imgIntNoise(std::move(imgIntNoise_)),
//            lostCorrPor(lostCorrPor_),
            storePtClouds(storePtClouds_),
            rwXMLinfo(rwXMLinfo_),
            compressedWrittenInfo(compressedWrittenInfo_),
            takeLessFramesIfLessKeyP(takeLessFramesIfLessKeyP_),
            checkDescriptorDist(checkDescriptorDist_),
            repeatPatternPortStereo(std::move(repeatPatternPortStereo_)),
            repeatPatternPortFToF(std::move(repeatPatternPortFToF_)),
            distortPatchCam1(distortPatchCam1_),
            oxfordGTMportion(oxfordGTMportion_),
            kittiGTMportion(kittiGTMportion_),
            megadepthGTMportion(megadepthGTMportion_),
            GTMportion(GTMportion_),
            WarpedPortionTN(WarpedPortionTN_),
            portionGrossTN(portionGrossTN_),
            CeresCPUcnt(CeresCPUcnt_),
            execPath(std::move(execPath_)),
            imageNetIDs(std::move(imageNetIDs_)),
            imageNetBuzzWrds(std::move(imageNetBuzzWrds_)),
            nrImgsFromImageNet(nrImgsFromImageNet_),
            parsValid(true){
        keypErrDistr.first = abs(keypErrDistr.first);
        keypErrDistr.second = abs(keypErrDistr.second);
        CV_Assert(keypPosErrType || (!keypPosErrType && (keypErrDistr.first < 5.0) &&
                                     (keypErrDistr.second < 5.0)
                                     && (keypErrDistr.first + 3.0 * keypErrDistr.second < 10.0)));
        imgIntNoise.second = abs(imgIntNoise.second);
        CV_Assert((imgIntNoise.first > -25.0) && (imgIntNoise.first < 25.0) && (imgIntNoise.second < 25.0));
//        CV_Assert((lostCorrPor >= 0) && (lostCorrPor <= 0.9));
        CV_Assert(!imgPath.empty());
    }

    GenMatchSequParameters():
            mainStorePath(""),
            imgPath(""),
            imgPrePostFix(""),
            keyPointType(""),
            descriptorType(""),
            keypPosErrType(false),
            keypErrDistr(std::make_pair(0, 0.5)),
            imgIntNoise(std::make_pair(0, 5.0)),
//            lostCorrPor(lostCorrPor_),
            storePtClouds(false),
            rwXMLinfo(false),
            compressedWrittenInfo(false),
            takeLessFramesIfLessKeyP(false),
            checkDescriptorDist(true),
            repeatPatternPortStereo(std::make_pair(0, 0)),
            repeatPatternPortFToF(std::make_pair(0, 0)),
            distortPatchCam1(false),
            oxfordGTMportion(0),
            kittiGTMportion(0),
            megadepthGTMportion(0),
            GTMportion(0),
            WarpedPortionTN(1.0),
            portionGrossTN(0),
            CeresCPUcnt(-1),
            execPath(""),
            imageNetIDs(std::vector<std::string>()),
            imageNetBuzzWrds(std::vector<std::string>()),
            nrImgsFromImageNet(0),
            parsValid(false){}

    bool checkParameters(){
        keypErrDistr.first = abs(keypErrDistr.first);
        keypErrDistr.second = abs(keypErrDistr.second);
        if(!(keypPosErrType || (!keypPosErrType && (keypErrDistr.first < 5.0) &&
                                     (keypErrDistr.second < 5.0)
                                     && (keypErrDistr.first + 3.0 * keypErrDistr.second < 15.0)))){
            std::cerr << "Invalid keypErrDistr." << std::endl;
            return false;
        }
        imgIntNoise.second = abs(imgIntNoise.second);
        if(!((imgIntNoise.first > -25.0) && (imgIntNoise.first < 25.0) && (imgIntNoise.second < 25.0))){//A minimum sum of mean and std of 25 is advisable
            std::cerr << "Invalid imgIntNoise." << std::endl;
            return false;
        }
        if(imgPath.empty()){
            std::cerr << "imgPath cannot be empty." << std::endl;
            return false;
        }
        repeatPatternPortStereo.first = std::round(abs(repeatPatternPortStereo.first) * 1e4) / 1e4;
        repeatPatternPortStereo.second = std::round(abs(repeatPatternPortStereo.second) * 1e4) / 1e4;
        if((repeatPatternPortStereo.first > (1. + DBL_EPSILON)) || (repeatPatternPortStereo.second > (1. + DBL_EPSILON)) ||
                (repeatPatternPortStereo.first > (repeatPatternPortStereo.second + DBL_EPSILON))){
            std::cerr << "Invalid repeatPatternPortStereo." << std::endl;
            return false;
        }
        repeatPatternPortFToF.first = std::round(abs(repeatPatternPortFToF.first) * 1e4) / 1e4;
        repeatPatternPortFToF.second = std::round(abs(repeatPatternPortFToF.second) * 1e4) / 1e4;
        if((repeatPatternPortFToF.first > (1. + DBL_EPSILON)) || (repeatPatternPortFToF.second > (1. + DBL_EPSILON)) ||
           (repeatPatternPortFToF.first > (repeatPatternPortFToF.second + DBL_EPSILON))){
            std::cerr << "Invalid repeatPatternPortFToF." << std::endl;
            return false;
        }
        oxfordGTMportion = std::round(abs(oxfordGTMportion) * 1e4) / 1e4;
        if(oxfordGTMportion > (1. + DBL_EPSILON)){
            std::cerr << "Invalid oxfordGTMportion." << std::endl;
            return false;
        }
        kittiGTMportion = std::round(abs(kittiGTMportion) * 1e4) / 1e4;
        if(kittiGTMportion > (1. + DBL_EPSILON)){
            std::cerr << "Invalid kittiGTMportion." << std::endl;
            return false;
        }
        megadepthGTMportion = std::round(abs(megadepthGTMportion) * 1e4) / 1e4;
        if(megadepthGTMportion > (1. + DBL_EPSILON)){
            std::cerr << "Invalid megadepthGTMportion." << std::endl;
            return false;
        }
        GTMportion = std::round(abs(GTMportion) * 1e4) / 1e4;
        if(GTMportion > (1. + DBL_EPSILON)){
            std::cerr << "Invalid oxfordGTMportion." << std::endl;
            return false;
        }
        WarpedPortionTN = std::round(abs(WarpedPortionTN) * 1e4) / 1e4;
        if(WarpedPortionTN > (1. + DBL_EPSILON)){
            std::cerr << "Invalid WarpedPortionTN." << std::endl;
            return false;
        }
        portionGrossTN = std::round(abs(portionGrossTN) * 1e4) / 1e4;
        if(portionGrossTN > (1. + DBL_EPSILON)){
            std::cerr << "Invalid portionGrossTN." << std::endl;
            return false;
        }
        return true;
    }
};

enum GT_DATASETS{
    OXFORD = 0x1,
    KITTI = 0x2,
    MEGADEPTH = 0x4
};

struct stats{
    double median;
    double mean;
    double standardDev;
    double minVal;
    double maxVal;
};

struct PatchCInfo{
    int show_cnt;
    const int show_interval;
    size_t featureIdx_tmp;
    double stdNoiseTN;
    double meanIntTNNoise;
    bool kpCalcNeeded;
    double ThTp;
    const double minDescrDistTP;
    double ThTn;
    double ThTnNear;
    std::normal_distribution<double> distr;
    bool visualize;
    bool useTN;
    bool succ;
    int i;
    bool takeImg2FallBack;
    cv::Rect patchROIimg1;
    cv::Rect patchROIimg2;
    cv::Mat patch1;
    cv::Mat patch2;

    PatchCInfo(const double &minDescrDistTP_,
               bool &useTN_,
               double &stdNoiseTN_,
               double &meanIntTNNoise_,
               bool &kpCalcNeeded_,
               double &ThTp_,
               double &ThTn_,
               double &ThTnNear_,
               bool visualize_ = false,
               const int show_interval_ = 40):
            show_cnt(0),
            show_interval(show_interval_),
            featureIdx_tmp(0),
            stdNoiseTN(stdNoiseTN_),
            meanIntTNNoise(meanIntTNNoise_),
            kpCalcNeeded(kpCalcNeeded_),
            ThTp(ThTp_),
            minDescrDistTP(minDescrDistTP_),
            ThTn(ThTn_),
            ThTnNear(ThTnNear_),
            visualize(visualize_),
            useTN(useTN_),
            succ(true),
            i(0),
            takeImg2FallBack(false){};
};

//template <typename ImageID>
class KeypointIndexer{
public:
    //Constructor for normal warped TN or TP (single image patch for both correspondences)
    KeypointIndexer(const bool &isTN_,
                    const std::string* idxImg1_,
                    const cv::Mat &descr1_,
                    const int &descr1Row_,
                    const cv::KeyPoint* kp1_,
                    const size_t &id_,
                    const size_t &imgId_):
            isGrossTN(false),
            isTN(isTN_),
            fromGTM(false),
            idxImg1(idxImg1_),
            idxImg2(nullptr),
            descr1(descr1_),
            descr1Row(descr1Row_),
            descr2Row(-1),
            kp1(kp1_),
            kp2(nullptr),
            match(nullptr),
            gtmIdx(std::numeric_limits<size_t>::max()),
            id(id_),
            imgId1(imgId_),
            imgId2(imgId_),
            uniqueImgId1(imgId_),
            uniqueImgId2(imgId_){}

    //Constructor for warped gross TN (2 image patches for correspondences)
    KeypointIndexer(const std::string* idxImg1_,
                    const std::string* idxImg2_,
                    const cv::Mat &descr1_,
                    const cv::Mat &descr2_,
                    const int &descr1Row_,
                    const int &descr2Row_,
                    const cv::KeyPoint* kp1_,
                    const cv::KeyPoint* kp2_,
                    const size_t &id_,
                    const size_t &imgId1_,
                    const size_t &imgId2_):
            isGrossTN(true),
            isTN(true),
            fromGTM(false),
            idxImg1(idxImg1_),
            idxImg2(idxImg2_),
            descr1(descr1_),
            descr2(descr2_),
            descr1Row(descr1Row_),
            descr2Row(descr2Row_),
            kp1(kp1_),
            kp2(kp2_),
            match(nullptr),
            gtmIdx(std::numeric_limits<size_t>::max()),
            id(id_),
            imgId1(imgId1_),
            imgId2(imgId2_),
            uniqueImgId1(imgId1_),
            uniqueImgId2(imgId2_){}

    //Constructor for GTM correspondences
    KeypointIndexer(const bool &isTN_,
                    const std::string* idxImg1_,
                    const std::string* idxImg2_,
                    const cv::Mat &descr1_,
                    const cv::Mat &descr2_,
                    const int &descr1Row_,
                    const int &descr2Row_,
                    const cv::KeyPoint* kp1_,
                    const cv::KeyPoint* kp2_,
                    const cv::DMatch* match_,
                    const size_t &gtmIdx_,
                    const size_t &id_,
                    const size_t &imgId1_,
                    const size_t &imgId2_,
                    const size_t &uniqueImgId1_,
                    const size_t &uniqueImgId2_):
            isGrossTN(false),
            isTN(isTN_),
            fromGTM(true),
            idxImg1(idxImg1_),
            idxImg2(idxImg2_),
            descr1(descr1_),
            descr2(descr2_),
            descr1Row(descr1Row_),
            descr2Row(descr2Row_),
            kp1(kp1_),
            kp2(kp2_),
            match(match_),
            gtmIdx(gtmIdx_),
            id(id_),
            imgId1(imgId1_),
            imgId2(imgId2_),
            uniqueImgId1(uniqueImgId1_),
            uniqueImgId2(uniqueImgId2_){}

    bool has2Imgs() const{ return idxImg2 != nullptr; }

    bool isTP() const{ return !isTN; }

    std::string getImgName1() const{ return *idxImg1; }

    std::string getImgName2() const{ return *idxImg2; }

    size_t getCorrespID() const{ return id; }

    size_t getImgID1() const{ return imgId1; }

    size_t getImgID2() const{ return imgId2; }

    bool isImgID2Valid() const{ return imgId2 != imgId1; }

    size_t getUniqueImgID1() const{ return uniqueImgId1; }

    size_t getUniqueImgID2() const{ return uniqueImgId2; }

    bool isUniqueImgID2Valid() const{ return uniqueImgId2 != uniqueImgId1; }

    cv::Mat getDescriptor1() const{ return descr1.row(descr1Row); }

    cv::Mat getDescriptor2() const{
        if(descr2.empty()){
            throw SequenceException("No second descriptor available.");
        }
        return descr2.row(descr2Row);
    }

    cv::KeyPoint getKeypoint1() const{ return *kp1; }

    cv::KeyPoint getKeypoint2() const{
        if(!fromGTM && !isGrossTN){
            throw SequenceException("No second keypoint available.");
        }
        return *kp2;
    }

    double getDescriptorDist(cv::InputArray descr2_ = cv::noArray()) const{
        if(!fromGTM && !isGrossTN && descr2_.empty()){
            throw SequenceException("Cannot calculate descriptor distance. Provide a second descriptor.");
        }else if(fromGTM && descr2_.empty()){
            return static_cast<double>(match->distance);
        }
        cv::Mat sec_descr;
        if(!descr2_.empty()){
            sec_descr = descr2_.getMat();
        }else{
            sec_descr = descr2.row(descr2Row);
        }
        if(descr1.type() == CV_8U) {
            return norm(descr1.row(descr1Row), sec_descr, cv::NORM_HAMMING);
        }
        return norm(descr1.row(descr1Row), sec_descr, cv::NORM_L2);
    }

    size_t getGtmIdx() const{ return gtmIdx; }

    bool isGTM() const{ return fromGTM; }

private:
    const bool isGrossTN;//Specifies if correspondence uses 2 different image patches
    bool isTN;//Specifies if correspondence is a TN
    const bool fromGTM;//True, if from GTM, else from warped patches
    const std::string* idxImg1;//Pointer to first image name
    const std::string* idxImg2;//Pointer to second image name
    cv::Mat descr1;//Descriptor array/Pointer to first descriptors
    cv::Mat descr2;//Descriptor array/Pointer to second descriptors
    const int descr1Row;//Row of first descriptor in descr1
    int descr2Row;//Row of second descriptor in descr2; If -1, the second descriptor is calculated by warping and distorting
    const cv::KeyPoint* kp1;//Pointer to first keypoint if correspondence stems from warped patch
    const cv::KeyPoint* kp2;//Pointer to second keypoint if correspondence stems from warped gross TN patch or GTM
    const cv::DMatch* match;//Pointer to GTM match
    const size_t gtmIdx;//Index to GTM vector elements of an image pair
    const size_t id;//Identifier of the correspondence
    const size_t imgId1;//Identifier of the underlying first image
    const size_t imgId2;//Identifier of the underlying second image
    const size_t uniqueImgId1;//Unique image ID pointing to the same image name as imgId1 (for different imgId1 pointing to the same image, this ID stays the same)
    const size_t uniqueImgId2;//Unique image ID pointing to the same image name as imgId2 (for different imgId2 pointing to the same image, this ID stays the same)
};

class TNTPindexer{
public:
    explicit TNTPindexer(std::mt19937 *rand2_ = nullptr): tn_idx(0), tp_idx(0), nrCorrs(0), rand2ptr(rand2_){
        if(rand2ptr == nullptr){
            long int seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            rand_obj = std::mt19937(seed);
        }
    }

    size_t getNextTPID(const size_t &tp_repPatt_idx, const size_t &min_frm_idx);

    size_t getNextTNID(const size_t &tn_repPatt_idx, const size_t &min_frm_idx);

    size_t getCorrID(const size_t &nr, const bool &isTN, const bool &tryBoth = false);

    void addTPID(const size_t &id){ tp_ids.push_back(id); }

    void addTNID(const size_t &id){ tn_ids.push_back(id); }

    size_t size() const{ return nrCorrs; }

private:
    inline size_t rand2(){
        if(rand2ptr == nullptr){
            return rand_obj();
        }
        return (*rand2ptr)();
    }

    size_t tn_idx = 0;
    size_t tp_idx = 0;
    size_t nrCorrs = 0;
    std::vector<size_t> tn_ids;
    std::vector<size_t> tp_ids;
//    std::unordered_map<size_t, size_t*> tptn_ids;
    std::map<size_t, std::pair<size_t, size_t>> tp_repPatt_idxs;//repeated pattern index, running TP index, minimum index of corresponding frame
    std::map<size_t, std::pair<size_t, size_t>> tn_repPatt_idxs;//repeated pattern index, running TN index, minimum index of corresponding frame
    std::mt19937 *rand2ptr;
    std::mt19937 rand_obj;
};

class KeypointSearch{
public:
    explicit KeypointSearch(const std::vector<cv::KeyPoint> &keypoints){
        int idx = 0;
        for (auto &i: keypoints) {
            kpMap.emplace(i.pt, idx++);
        }
    }

    void getMissingIdxs(const std::vector<cv::KeyPoint> &keypoints, std::vector<int> &idxsMissing){
        idxsMissing.clear();
        int idx = 0;
        for (auto &i: keypoints) {
            if(kpMap.find(i.pt) == kpMap.end()){
                idxsMissing.emplace_back(idx);
            }
            idx++;
        }
        if(idxsMissing.empty()) return;
        std::sort(idxsMissing.begin(), idxsMissing.end(), [](const int &first, const int &second){return first > second;});
    }

    void getNewToOldOrdering(const std::vector<cv::KeyPoint> &keypoints, std::vector<int> &idxsNew){
        idxsNew.clear();
        std::unordered_map<cv::Point2f, int, KeyHasher, EqualTo>::iterator got;
        for (auto &i: keypoints) {
            got = kpMap.find(i.pt);
            if(got != kpMap.end()){
                idxsNew.emplace_back(got->second);
            }
        }
    }

private:
    struct KeyHasher
    {
        std::size_t operator()(const cv::Point2f& pt) const
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, pt.x);
            boost::hash_combine(seed, pt.y);
            return seed;
        }
    };
    struct EqualTo
    {
        bool operator()(const cv::Point2f& pt1, const cv::Point2f& pt2) const
        {
            return nearZero(static_cast<double>(pt1.x) - static_cast<double>(pt2.x)) &&
                   nearZero(static_cast<double>(pt1.y) - static_cast<double>(pt2.y));
        }
    };
    std::unordered_map<cv::Point2f, int, KeyHasher, EqualTo> kpMap;
};

class GENERATEVIRTUALSEQUENCELIB_API genMatchSequ : genStereoSequ {
public:
    genMatchSequ(cv::Size &imgSize_,
                 cv::Mat &K1_,
                 cv::Mat &K2_,
                 std::vector<cv::Mat> &R_,
                 std::vector<cv::Mat> &t_,
                 StereoSequParameters pars3D_,
                 GenMatchSequParameters &parsMtch_,
                 bool filter_occluded_points_,
                 uint32_t verbose_ = 0,
                 const std::string &writeIntermRes_path_ = "") :
            genStereoSequ(imgSize_, K1_, K2_, R_, t_, pars3D_, filter_occluded_points_, verbose_, writeIntermRes_path_),
            parsMtch(parsMtch_),
            pars3D(pars3D_),
            imgSize(imgSize_),
            K1(K1_),
            K2(K2_),
            sequParsLoaded(false),
            tntpindexer(&rand2){
        CV_Assert(!parsMtch.mainStorePath.empty());
        CV_Assert(parsMtch.parsValid);
        genSequenceParsFileName();
        K1i = K1.inv();
        K2i = K2.inv();
        kpErrors.clear();
        featureIdxBegin = 0;
        adaptPatchSize();
    };

    genMatchSequ(const std::string &sequLoadFolder,
                 GenMatchSequParameters &parsMtch_,
                 uint32_t verboseMatch_ = 0,
                 const std::string &writeIntermRes_path_ = "");

    bool generateMatches();

private:
    //Loads the image names (including folders) of all specified images (used to generate matches) within a given folder
    bool getImageList();
    //Download and load image names from ImageNet
    bool getImageNetImgs(std::vector<std::string> &filenames);
    //Check if feature matches should be used from a 3rd party GT dataset
    bool check_3rdPty_GT();
    //Generate GTM from 3rd party datasets
    bool calcGTM();
    //Resets all GTM related variables to only use warped patches
    void resetGTMuse();
    //Update image IDs by adapting. Used if image was not be able to be loaded.
    void updateImageIDs(const size_t &fromPos, const size_t &toPos, const size_t &len);
    //Calculate GTM descriptors and update variables within gtmdata
    bool getGtmDescriptors();
    //Recalculate match indices if the descriptor extraction removed keypoints
    bool recalcMatchIdxs(const std::vector<cv::KeyPoint> &kpsBefore, const size_t &idx, bool isLeftKp);
    //Build index corrToIdxMap which holds information for and pointers to correspondence data (warped and GTM correspondences)
    void buildCorrsIdx();
    //Generates a hash value from the parameters used to generate a scene and 3D correspondences, respectively
    size_t hashFromSequPars();
    //Generates a hash value from the parameters used to generate matches from 3D correspondences
    size_t hashFromMtchPars();
    //Calculates the total number of correspondences (Tp+TN) within a whole scene
    void totalNrCorrs();
    //Get images that are most often used within a single frame to calculate correspondences
    void getMostUsedImgs();
    //Returns an list of image names sorted by their global ID
    std::vector<std::string> getIDSortedImgNameList();
    //Update the linear index (tntpindexer) used by functions to calculate correspondences using the global correspondence index (corrToIdxMap)
    void updateLinearIdx();
    //Loads images and extracts keypoints and descriptors from them to generate matches later on
    bool getFeatures();
    //Writes the parameters used to generate 3D scenes to disk
    bool writeSequenceParameters(const std::string &filename);
    //Writes a subset of parameters used to generate a 3D scene to disk (used within an overview file which holds basic information about all sub-folders that contain parameters and different 3D scenes)
    void writeSomeSequenceParameters(cv::FileStorage &fs);
    //Generates a file or appends information to a file which holds an overview with basic information about all sub-folders that contain parameters and different 3D scenes (within a base-path)
    bool writeSequenceOverviewPars();
    //Reads parameters from a generated file that where used to generate a 3D sequence
    bool readSequenceParameters(const std::string &filename);
    //Writes information and correspondences for a single stereo frame to disk
    bool write3DInfoSingleFrame(const std::string &filename);
    //Reads information and correspondences for a single stereo frame from disk
    bool read3DInfoSingleFrame(const std::string &filename);
    //Writes PCL point clouds for the static scene and PCL point clouds for all moving objects to disk
    bool writePointClouds(const std::string &path, const std::string &basename, bool &overwrite);
    //Reads PCL point clouds for the static scene and PCL point clouds for all moving objects from disk
    bool readPointClouds(const std::string &path, const std::string &basename);
    //Generates the filename used to store/load parameters for generating a 3D sequence
    void genSequenceParsFileName();
    //Generates a path for storing results (matches (in sub-folder(s) of generated path) and if desired, the generated 3D scene. The last folder of this path might be a hash value if a dedicated storing-path is provided or corresponds to a given path used to load a 3D scene.
    bool genSequenceParsStorePath();
    //Generates a folder inside the folder of the 3D scene for storing matches
    bool genMatchDataStorePath();
    //Get the filename for storing an overview of the sequence parameters
    bool getSequenceOverviewParsFileName(std::string &filename) const;
    //Generates a YAML/XML file containing parameters for generating matches from 3D scenes for every sub-folder (For every run with the same 3D scene, the parameter set for the matches is appended at the end of the file)
    bool writeMatchingParameters();
    //Generates a new file inside the folder of the matches which holds the mean and standard deviation of keypoint position errors of the whole scene in addition to a list of images (including their folder structure) that were used to extract patches for calculating descriptors and keypoints
    bool writeKeyPointErrorAndSrcImgs(double &meanErr, double &sdErr);
    //Writes matches and features in addition to other information for every frame to disk
    bool writeMatchesToDisk();
    //Generates the user specified file extension (xml and yaml in combination with gz)
    std::string genSequFileExtension(const std::string &basename) const;

    //Rotates a line 'b' about a line 'a' (only direction vector) using the given angle
    static cv::Mat rotateAboutLine(const cv::Mat &a, const double &angle, const cv::Mat &b);

    //Calculates a homography by rotating a plane in 3D (which was generated using a 3D point and its projections into camera 1 & 2) and backprojection of corresponding points on that plane into the second image
    cv::Mat getHomographyForDistortion(const cv::Mat &X,
                                       const cv::Mat &x1,
                                       const cv::Mat &x2,
                                       int64_t idx3D,
                                       size_t keyPIdx,
                                       cv::InputArray planeNVec,
                                       bool visualize,
                                       bool forCam1 = false);
    //Checks if the 3D point of the given correspondence was already used before in a different stereo frame to calculate a homography (in this case the same 3D plane is used to calculate a homography) and if not, calculates a new homography
    cv::Mat getHomographyForDistortionChkOld(const cv::Mat& X,
                                             const cv::Mat& x1,
                                             const cv::Mat& x2,
                                             int64_t idx3D,
                                             int64_t idx3D2,
                                             size_t keyPIdx,
                                             bool visualize,
                                             bool forCam1 = false);

    //Create a homography for a TN correspondence
    cv::Mat getHomographyForDistortionTN(const cv::Mat& x1,
                                         bool visualize);
    //Visualizes the planes in 3D used to calculate a homography
    void visualizePlanes(std::vector<cv::Mat> &pts3D,
                         const cv::Mat &plane1,
                         const cv::Mat &plane2);
    //Adds gaussian noise to an image patch
    static void addImgNoiseGauss(const cv::Mat &patchIn,
            cv::Mat &patchOut,
                          double meanNoise,
                          double stdNoise,
                          bool visualize = false);

    //Adds salt and pepper noise to an image patch
    static void addImgNoiseSaltAndPepper(const cv::Mat &patchIn,
            cv::Mat &patchOut,
            int minTH = 30,
            int maxTH = 225,
            bool visualize = false);

    //Generates features and matches for correspondences of a given stereo frame (TN and TP) and stores them to disk
    bool generateCorrespondingFeatures();
    //Generates features and matches based on image patches and calculated homographies for either TN or TP
    void generateCorrespondingFeaturesTPTN(size_t featureIdxBegin_,
                                           bool useTN,
                                           std::vector<cv::KeyPoint> &frameKPs1,
                                           std::vector<cv::KeyPoint> &frameKPs2,
                                           cv::Mat &frameDescr1,
                                           cv::Mat &frameDescr2,
                                           std::vector<cv::DMatch> &frameMatchesTNTP,
                                           std::vector<cv::Mat> &homo,
                                           std::vector<std::pair<std::pair<size_t,cv::KeyPoint>,
                                                   std::pair<size_t,cv::KeyPoint>>> &srcImgIdxAndKp,
                                           std::vector<cv::Mat> *homoCam1 = nullptr);
    //Calculate warped patches and corresponding descriptors
    cv::Mat calculateDescriptorWarped(const cv::Mat &img,
                                      const cv::KeyPoint &kp,
                                      cv::Mat &H,
                                      std::vector<cv::Mat> &homo,
                                      PatchCInfo &patchInfos,
                                      cv::KeyPoint &kp2,
                                      cv::Point2f &kp2err,
                                      double &descrDist,
                                      bool forCam1,
                                      cv::InputArray H_cam1 = cv::noArray(),
                                      cv::InputArray descr_cam1 = cv::noArray());
    //Calculates the size of a patch that should be extracted from the source image to get a minimum square patch size after warping with the given homography based on the shape of the ellipse which emerges after warping a circle with the given keypoint diameter
    bool getRectFitsInEllipse(const cv::Mat &H,
                              const cv::KeyPoint &kp,
                              cv::Rect &patchROIimg1,
                              cv::Rect &patchROIimg2,
                              cv::Rect &patchROIimg21,
                              cv::Point2d &ellipseCenter,
                              double &ellipseRot,
                              cv::Size2d &axes,
                              bool &reflectionX,
                              bool &reflectionY,
                              cv::Size &imgFeatureSi);
    //Calculate statistics on descriptor distances for not matching descriptors
    bool calcGoodBadDescriptorTH();
    //Calculate statistics on descriptor distances for not matching descriptors of GTM if available
    bool getGTMDescrDistStat(std::vector<double> &descrDistsTN, const double &minDist = 0);
    //Estimate number of warped TN+TP and number of GTM TN+TP
    void EstimateNrTPTN(bool reestimate = false);
    //Calculate image IDs for GTM
    void calcGTMimgIDs();
    //Distorts the keypoint position
    void distortKeyPointPosition(cv::KeyPoint &kp2,
                                 const cv::Rect &roi,
                                 std::normal_distribution<double> &distr);
    //Generates keypoints without a position error (Order of correspondences from generating the 3D scene must be the same as for the keypoints)
    void getErrorFreeKeypoints(const std::vector<cv::KeyPoint> &kpWithErr,
                               std::vector<cv::KeyPoint> &kpNoErr);
    //Calculate statistics about the difference in the camera position from frame to frame in addition to the overall length
    void getCamCoordinatesStats(double &lenght,
                                qualityParm &stats_DiffTx,
                                qualityParm &stats_DiffTy,
                                qualityParm &stats_DiffTz);
    void adaptPatchSize();
    bool getKazeProperties(cv::Mat& patch,
                           std::vector<cv::KeyPoint> &kp2_v,
                           cv::KeyPoint &kp2);
    bool check3DToIdxConsisty(const cv::Mat &X, const int64_t &idx3D, const int64_t &idx3D2);
    bool check2D3DConsistency(const cv::Mat &x1, const cv::Mat &x2, const cv::Mat &X, const int64_t &idx3D, const int64_t &idx3D2);
    //Calculate patch ROIs for displaying
    cv::Rect getNormalPatchROI(const cv::Size &imgSi, const cv::KeyPoint &kp) const;
    //Reorder keypoints, masks, and descriptors of one image based on different ordered keypoints
    void reOrderMatches1Img(const std::vector<cv::KeyPoint> &kpsBefore, const size_t &idx, bool isLeftKp);

public:
    GenMatchSequParameters parsMtch;

private:
    int minPatchSize2 = 85;//Corresponds to the minimal patch size (must be an odd number) we want after warping. It is also used to define the maximum patch size by multiplying it with maxPatchSizeMult2
    const int maxPatchSizeMult2 = 3;//Multiplication factor for minPatchSize2 to define the maximum allowed patch size of the warped image
    const size_t maxImgLoad = 200;//Defines the maximum number of images that are loaded and saved in a vector
    size_t minNrFramesMatch = 10;//Minimum number of required frames that should be generated if there are too less keypoints available
    size_t featureIdxBegin = 0;//Index for features at the beginning of every calculated frame
    std::vector<size_t> featureIdxRepPatt;//Index to keypoints, descriptors, and featureImgIdx with possible duplicates to simulate repeated patterns
    std::vector<cv::Mat> imgs;//If less than maxImgLoad images are in the specified folder, they are loaded into this vector. Otherwise, this vector holds only images for the current frame
    std::vector<std::string> imageList;//Holds the filenames of all images to extract keypoints
    size_t nrCorrsFullSequ = 0;//Number of predicted overall correspondences (TP+TN) for all frames
    size_t nrTNFullSequ = 0;//Number of predicted overall TN correspondences for all frames
    size_t nrTPFullSequ = 0;//Number of predicted overall TP correspondences for all frames
    size_t nrCorrsFullSequWarped = 0;//Number of predicted overall correspondences (TP+TN) for all frames that should be generated using warped patches
    size_t nrTNFullSequWarped = 0;//Number of predicted overall TN correspondences for all frames that should be generated using warped patches
    size_t nrGrossTNFullSequWarped = 0;//Number of predicted overall TN correspondences for all frames that should be from different image patches (first <-> second stereo camera)
    size_t nrGrossTNFullSequGTM = 0;//Number of predicted overall TN correspondences for all frames that should be from GTM
    size_t nrTPFullSequWarped = 0;//Number of predicted overall TP correspondences for all frames that should be generated using warped patches
    size_t nrCorrsExtractWarped = 0;//Number of features that should be extracted from given images to meet nrGrossTNFullSequWarped, nrTNFullSequWarped, and nrTPFullSequWarped
    std::vector<cv::KeyPoint> keypoints1;//Keypoints from all used images
    cv::Mat descriptors1;//Descriptors from all used images
    size_t nrFramesGenMatches = 0;//Number of frames used to calculate matches. If a smaller number of keypoints was found than necessary for the full sequence, this number corresponds to the number of frames for which enough features are available. Otherwise, it equals to totalNrFrames.
    size_t hash_Sequ = 0, hash_Matches = 0;//Hash values for the generated 3D sequence and the matches based on their input parameters.
    StereoSequParameters pars3D;//Holds all parameters for calculating a 3D sequence. Is manly used to load existing 3D sequences.
    cv::Size imgSize;//Size of the images
    cv::Mat K1, K1i;//Camera matrix 1 & its inverse
    cv::Mat K2, K2i;//Camera matrix 2 & its inverse
    size_t nrMovObjAllFrames = 0;//Sum over the number of moving objects in every frame
    std::string sequParFileName = "";//File name for storing and loading parameters of 3D sequences
    std::string sequParPath = "";//Path for storing and loading parameters of 3D sequences
    std::string matchDataPath = "";//Path for storing parameters for generating matches
    bool sequParsLoaded = false;//Indicates if the 3D sequence was/will be loaded or generated during execution of the program
    const std::string pclBaseFName = "pclCloud";//Base name for storing PCL point clouds. A specialization is added at the end of this string
    const std::string sequSingleFrameBaseFName = "sequSingleFrameData";//Base name for storing data of generated frames (correspondences)
    const std::string matchSingleFrameBaseFName = "matchSingleFrameData";//Base name for storing data of generated matches
    std::string sequLoadPath = "";//Holds the path for loading a 3D sequence
    std::vector<size_t> featureImgIdx;//Contains an index to the corresponding image for every keypoint and descriptor
    std::map<size_t, KeypointIndexer> corrToIdxMap;//Holds information for and pointers to correspondence data (warped and GTM correspondences)
    TNTPindexer tntpindexer;//Converts a running correspondence index to correspondence IDs of corrToIdxMap
    cv::Mat actTransGlobWorld;//Transformation for the actual frame to transform 3D camera coordinates to world coordinates
    cv::Mat actTransGlobWorldit;//Inverse and translated Transformation for the actual frame to transform 3D camera coordinates to world coordinates
    std::map<int64_t,std::tuple<cv::Mat,size_t,size_t>> planeTo3DIdx;//Holds the plane coefficients, keypoint index, and stereo frame number for every used keypoint in correspondence to the index of the 3D point in the point cloud
    double actNormT = 0;//Norm of the actual translation vector between the stereo cameras
    std::vector<std::pair<std::map<size_t,size_t>,std::vector<size_t>>> imgFrameIdxMap;//If more than maxImgLoad images to generate features are used, every map contains the most maxImgLoad used images (key = img idx, value = position in the vector holding the images) for keypoints per frame. The vector inside the pair holds a consecutive order of image indices for loading the images
    bool loadImgsEveryFrame = false;//Indicates if there are more than maxImgLoad images in the folder and the images used to extract patches must be loaded for every frame
    std::map<size_t, std::string*> uniqueImgIDToName;//Holds unique image IDs pointing to corresponding image names
    stats badDescrTH = {0,0,0,0,0};//Descriptor distance statistics for not matching descriptors. E.g. a descriptor distance larger the median could be considered as not matching descriptors
    std::vector<cv::KeyPoint> frameKeypoints1, frameKeypoints2;//Keypoints for the actual stereo frame (there is no 1:1 correspondence between these 2 as they are shuffled but the keypoint order of each of them is the same as in their corresponding descriptor Mat (rows))
    std::vector<cv::KeyPoint> frameKeypoints2NoErr;//Keypoints in the second stereo image without a positioning error (in general, keypoints in the first stereo image are without errors)
    cv::Mat frameDescriptors1, frameDescriptors2;//Descriptors for the actual stereo frame (there is no 1:1 correspondence between these 2 as they are shuffled but the descriptor order of each of them is the same as in their corresponding keypoint vector). Descriptors corresponding to the same static 3D point (not for moving objects) in different stereo frames are similar
    std::vector<cv::DMatch> frameMatches;//Matches between features of a single stereo frame. They are sorted based on the descriptor distance (smallest first)
    std::vector<bool> frameInliers;//Indicates if a feature (frameKeypoints1 and corresponding frameDescriptors1) is an inlier.
    std::vector<cv::Mat> frameHomographies;//Holds the homographies for all patches arround keypoints for warping the patch which is then used to calculate the matching descriptor. Homographies corresponding to the same static 3D point in different stereo frames are similar
    std::vector<cv::Mat> frameHomographiesCam1;//Holds homographies for all patches arround keypoints in the first camera (for tracked features) for warping the patch which is then used to calculate the matching descriptor. Homographies corresponding to the same static 3D point in different stereo frames are similar
    std::vector<std::pair<std::pair<size_t,cv::KeyPoint>, std::pair<size_t,cv::KeyPoint>>> srcImgPatchIdxAndKp; //Holds the keypoints and image indices of the images used to extract patches
    std::vector<int> corrType;//Specifies the type of a correspondence (TN from static (=4) or TN from moving (=5) object, or TP from a new static (=0), a new moving (=1), an old static (=2), or an old moving (=3) object (old means,that the corresponding 3D point emerged before this stereo frame and also has one or more correspondences in a different stereo frame))
    std::vector<double> kpErrors;//Holds distances from the original to the distorted keypoint locations for every correspondence of the whole sequence
    //std::string matchParsFileName = "";//File name (including path) of the parameter file to create matches
    bool overwriteMatchingFiles = false;//If true (after asking the user), data files holding matches (from a different call to this software) in the same folder (should not happen - only if the user manually copies data) are replaced by the new data files
    std::vector<std::pair<double,double>> timePerFrameMatch;//Holds time measurements for every frame in microseconds. The first value corresponds to the time for loading or calculating 3D information of one stereo frame. The second value holds the time for calculating the matches.
    qualityParm timeMatchStats = qualityParm();//Statistics for the execution time in microseconds for calculating matches based on all frames
    qualityParm time3DStats = qualityParm();//Statistics for the execution time in microseconds for calculating 3D correspondences based on all frames
    uint8_t use_3dPrtyGT = false;//Indicates if feature matches from 3rd party GT datasets should be used. The first 3 bits indicate which datasets should be used.
    bool onlyWarped = true;//Indicates, if GTM were disabled based on inputs
    GTMdata gtmdata;//Holds Ground Truth matches and related information
    std::vector<size_t> idxs_match23D1, idxs_match23D2;//Indices for final correspondences to point from reordered frameKeypoints1, frameKeypoints2, frameDescriptors1, frameDescriptors2, frameHomographies, ... to corresponding 3D information like combCorrsImg1TP, combCorrsImg2TP, combCorrsImg12TP_IdxWorld2, ...
};

//GENERATEVIRTUALSEQUENCELIB_API cv::FileStorage& operator << (cv::FileStorage& fs, bool &value);
////void GENERATEVIRTUALSEQUENCELIB_API operator >> (const cv::FileNode& n, bool& value);
//void GENERATEVIRTUALSEQUENCELIB_API operator >> (const cv::FileNode& n, int64_t& value);
//GENERATEVIRTUALSEQUENCELIB_API cv::FileStorage& operator << (cv::FileStorage& fs, int64_t &value);
//GENERATEVIRTUALSEQUENCELIB_API cv::FileNodeIterator& operator >> (cv::FileNodeIterator& it, int64_t & value);

#endif //GENERATEVIRTUALSEQUENCE_GENERATEMATCHES_H
