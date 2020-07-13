//Released under the MIT License - https://opensource.org/licenses/MIT
//
//Copyright (c) 2019 AIT Austrian Institute of Technology GmbH
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
/**********************************************************************************************************
FILE: stereo_pose_refinement.h

PLATFORM: Windows 7, MS Visual Studio 2010, OpenCV 3.2

CODE: C++

AUTOR: Josef Maier, AIT Austrian Institute of Technology

DATE: October 2017

LOCATION: TechGate Vienna, Donau-City-Strasse 1, 1220 Vienna

VERSION: 1.0

DISCRIPTION: This file provides functions for pose refinement with multiple stereo pairs
**********************************************************************************************************/

#pragma once

#include "opencv2/highgui/highgui.hpp"
#include "poselib/glob_includes.h"
#include "poselib/pose_estim.h"
#include "poselib/pose_helper.h"
#include "poselib/stereo_pose_types.h"
#include "poselib/nanoflannInterface.h"
#include <list>
#include <unordered_map>
#include <memory>
#include <Eigen/Core>

#include "poselib/poselib_api.h"

namespace poselib
{

    /* --------------------------- Defines --------------------------- */

    /*struct POSELIB_API ConfigMatching
    {
        ConfigMatching() : img1(NULL),
            img2(NULL),
            histEqual(false),
            f_detect("FAST"),
            d_extr("FREAK"),
            matcher("GMBSOF"),
            nmsIdx(""),
            nmsQry(""),
            f_nr(8000),
            noRatiot(false),
            refineVFC(false),
            DynKeyP(false),
            subPixRef(1),
            showNr_matches(50),
            verbose(7)
        {}

        cv::Mat* img1;
        cv::Mat* img2;
        bool histEqual;
        std::string f_detect;
        std::string d_extr;
        std::string matcher;
        std::string nmsIdx;
        std::string nmsQry;
        int f_nr;
        bool noRatiot;
        bool refineVFC;
        bool refineSOF;
        bool DynKeyP;
        int subPixRef;
        int showNr_matches;
        int verbose;
    };*/

    // problem specific/data-related parameters
    struct POSELIB_API ConfigPoseEstimation
    {
        ConfigPoseEstimation() : dist0_8(nullptr),
            dist1_8(nullptr),
            K0(nullptr),
            K1(nullptr),
            keypointType("FAST"),
            descriptorType("FREAK"),
            th_pix_user(0.8),
            autoTH(false),
            Halign(0),
            RobMethod("USAC"),
            refineMethod(poselib::RefinePostAlg::PR_NO_REFINEMENT),
            refineRTold(false),
            kneipInsteadBA(false),
            BART(0),
            refineMethod_CorrPool(poselib::RefinePostAlg::PR_STEWENIUS | poselib::RefinePostAlg::PR_PSEUDOHUBER_WEIGHTS),
            refineRTold_CorrPool(false),
            kneipInsteadBA_CorrPool(false),
            BART_CorrPool(0),
            verbose(7),
            minStartAggInlRat(0.2),
            relInlRatThLast(0.35),
            relInlRatThNew(0.20),
            minInlierRatSkip(0.38),
            relMinInlierRatSkip(0.7),
            maxSkipPairs(5),
            minInlierRatioReInit(0.6),
            minPtsDistance(3.f),
            maxPoolCorrespondences(30000),
            minContStablePoses(3),
            absThRankingStable(0.075),
            useRANSAC_fewMatches(false),
            checkPoolPoseRobust(3),
            minNormDistStable(0.5),
			raiseSkipCnt(0),
			maxRat3DPtsFar(0.5),
			maxDist3DPtsZ(50.0)
        {}

        cv::Mat* dist0_8;//Distortion paramters in OpenCV format with 8 parameters for the first/left image
        cv::Mat* dist1_8;//Distortion paramters in OpenCV format with 8 parameters for the second/right image
        cv::Mat* K0;//Camera matrix of the first/left camera
        cv::Mat* K1;//Camera matrix of the second/right camera
        std::string keypointType;//Name of the used keypoints
        std::string descriptorType;//Name of the used descriptor
        double th_pix_user;//Threshold for robust refinement in pixels
        bool autoTH;//If the essentila matrix should be estimated using an automatic threshold estimation and ARRSAC
        int Halign;//If the essentila matrix should be estimated using homography alignment
        std::string RobMethod;//USAC,RANSAC,ARRSAC,LMEDS
        int refineMethod;//Choose the refinement algorithm (not necessary for USAC) after robust estimation. For details see enum RefinePostAlg
        bool refineRTold;//If true, the old refinement algorithm based on the 8pt algorithm and pseudo-huber weights is used after robust estimation.
        bool kneipInsteadBA;//If true, Kneips Eigen solver is used instead of bundle adjustment (no additional refinement beforehand) after robust estimation.
        int BART;//Optional bundle adjustment (BA) after robust estimation: 0: no BA, 1: BA for extrinsics and structure only, 2: BA for intrinsics, extrinsics, and structure
        int refineMethod_CorrPool;//Choose the refinement algorithm (not necessary for USAC) for all correspondences. For details see enum RefinePostAlg
        bool refineRTold_CorrPool;//If true, the old refinement algorithm based on the 8pt algorithm and pseudo-huber weights is used for all correspondences.
        bool kneipInsteadBA_CorrPool;//If true, Kneips Eigen solver is used instead of bundle adjustment (no additional refinement beforehand) for all correspondences.
        int BART_CorrPool;//Optional bundle adjustment (BA) for all correspondences: 0: no BA, 1: BA for extrinsics and structure only, 2: BA for intrinsics, extrinsics, and structure
        int verbose;//Verbosity level (max:7)
        double minStartAggInlRat;//Threshold on the inlier ratio for the first pose estimation. Below this threshold the correspondences will be discarded and not used in the next iteration.
        double relInlRatThLast;//Relative threshold (th = (1 - relInlRatThLast) * last_inl_rat) on the inlier ratios to decide if a new robust estimation is necessary (for comparison, the inlier ratio of the last image pair "last_inl_rat" is used)
        double relInlRatThNew;//Relative threshold (th = (1 - relInlRatThNew) * old_inl_rat) on the inlier ratios of the new image pair with the old and new (with robust estimation) Essential matrix. Is only used if a new robust estimation was performed (see relInlRatThLast)
        double minInlierRatSkip;//Absolute threshold on the inlier ratio for new image pairs if the old essential matrix E differs from the new one (estimated with a robust method). Below this threshold, a fall-back threshold estimated by relMinInlierRatSkip can be used, if the resulting threshold is smaller minInlierRatSkip.
        double relMinInlierRatSkip;//Relative threshold (th =  relMinInlierRatSkip * last_valid_inlier_rat) on the inlier ratio for new image pairs compared to the last valid (estimation of E with multiple image pairs) if the old essential matrix E differs from the new one (estimated with a robust method). Below this threshold the old pose is restored.
        size_t maxSkipPairs;//Maximum number of times the new Essential matrix E is discarded and restored by the old one (see minInlierRatSkip). If more E's are discarded, the whole system is reinitialized.
        double minInlierRatioReInit;//If the new pose differs from the old, the whole system is reinitialized if the inlier ratio with the new pose is above this value
        float minPtsDistance;//Minimum distance between points for insertion into the correspondence pool
        size_t maxPoolCorrespondences;//Maximum number of correspondences in the correspondence pool after concatenating correspondences from multiple image pairs
        size_t minContStablePoses;//Number of consecutive estimated poses that must be stable based on a pose distance ranking
        double absThRankingStable;//Threshold on the ranking over the last minContStablePoses poses to decide if the pose is stable (actual_ranking +/- absThRankingStable). Is only used if no stability is recognized using only the pose.
        bool useRANSAC_fewMatches;//If true, RANSAC is used for the robust estimation if the number of provided matches is below 150
        size_t checkPoolPoseRobust;//If not 0, the pose is robustly (RANSAC, ...) estimated from the pool correspondences after reaching checkPoolPoseRobust times the number of initial inliers. A value of 1 means robust estimation is used instead of refinement. For a value >1, the value is exponetially increased after every robust estimation from the pool.
        double minNormDistStable;//Minimum normalized distance of a stable pose to the center of gravity of all stored poses
		int raiseSkipCnt;//If not 0, the value of maxSkipPairs is increased to std::ceil(maxSkipPairs * (1.0 + (raiseSkipCnt & 0xF) * 0.25)) if a stable pose was detected. Thus, the first 4 bits define the multiplication factor. The 2nd 4bits define, how many consecutive estimated poses must be stable to perform the multiplication (nr = ((raiseSkipCnt & 0xF0) >> 4) + 1).
		double maxRat3DPtsFar;//Maximum ratio of 3D points for which their z-value is very large (e.g. 50 x baseline) compared to the number of all 3D points. Above this threshold, a pose cannot be marked as stable using only a threshold on the Sampson error ranges (see absThRankingStable)
		double maxDist3DPtsZ;//Maximum value for the z-coordinates of 3D points to be included into BA. Moreover, this value influences the decision if a pose is marked as stable during stereo refinement (see maxRat3DPtsFar).
    };

    //typedef Eigen::Matrix<double, Eigen::Dynamic, 2, Eigen::RowMajor> EMatDouble2;

    /* --------------------- Function prototypes --------------------- */


    /* --------------------------- Classes --------------------------- */

    class POSELIB_API StereoRefine
    {
    private:
        ConfigPoseEstimation cfg_pose;
        poselib::ConfigUSAC cfg_usac;
        double pixToCamFact = 0;
        size_t nrEstimation = 0;
        size_t skipCount = 0;
        std::vector<cv::Point2f> points1new, points2new;//Undistorted point correspondences in the camera coordinate system
        cv::Mat points1newMat, points2newMat;//The same as points1new and points2new but double values in cv::Mat format
        cv::Mat points1newMat_tmp, points2newMat_tmp;//The same as points1newMat and points2newMat but holds also the correspondences that were filtered out
        cv::Mat points1Cam, points2Cam;//Undistorted point correspondences in the camera coordinate system of all valid image pairs
        size_t newAddedPoolCorrs = 0;//Number of newly added correspondences
        double th = 0;//Inlier threshold
        double th2 = 0;//Squared inlier threshold
        double t_mea = 0; //Time measurement
        double t_oa = 0; //Overall time
        float descrDist_max = 0;//Maximum observed descriptor distance in the data
        float keyPRespons_max = 0; //Maximum observed keypoint response in the data
        cv::Mat mask_E_new; //Newest mask using E only for the newest image pair
        cv::Mat mask_Q_new; //Newest mask using E and triangulated 3D points (excludes correspondences too far away) for the newest image pair
        size_t nr_inliers_new = 0;//Number of inliers for the newest image pair
        size_t nr_corrs_new = 0;//Number of initial correspondences of the newest image pair
        bool maxPoolSizeReached = false;//Is true if the maximum pool size was reached
        size_t checkPoolPoseRobust_tmp = 0;//After approximately this number multiplied by the initial number of inliers, a robust estimation is performed on the pool correspondences instead of refinement
        size_t initNumberInliers = 0;//Initial number of inliers after robust estimation
		size_t nrConsecStablePoses = 0;//Number of consecutive stable poses
		size_t maxSkipPairsNew = 0;//Maximum number of times the new Essential matrix E is discarded and restored by the old one (see minInlierRatSkip). If more E's are discarded, the whole system is reinitialized. This value may change if stability of the pose is reached.

        std::list<CoordinateProps> correspondencePool;//Holds all correspondences and their properties over the last valid image pairs
        std::unordered_map<size_t, std::list<CoordinateProps>::iterator> correspondencePoolIdx;//Stores the iterator to every correspondencePool element. The key value is an continous index necessary for nanoflann
        size_t corrIdx = 0;//Continous index starting with 0 counting all correspondences that were ever inserted into correspondencePool. The index is resetted when the KD tree is resetted.
        std::unique_ptr<keyPointTreeInterface> kdTreeLeft;//KD-tree using nanoflann holding the keypoint coordinates of the left valid keypoints
        bool verbose = false;

        struct CoordinatePropsNew
        {
            CoordinatePropsNew(const cv::Point2f &pt1_,
                    const cv::Point2f &pt2_,
                    float descrDist_,
                    float response1,
                    float response2,
                    double sampsonError_) : pt1(pt1_),
                pt2(pt2_),
                descrDist(descrDist_),
                keyPResponses{ response1, response2 },
                sampsonError(sampsonError_)
            {}

            cv::Point2f pt1;//Keypoint position in first/left image
            cv::Point2f pt2;//Keypoint position in second/right image
            float descrDist;//Descriptor distance
            float keyPResponses[2];//Response of corresponding keypoints in the first and second image
            double sampsonError;//Sampson Error
        };

        struct poseHist
        {
            poseHist(cv::Mat E_, cv::Mat R_, cv::Mat t_) : E(std::move(E_)),
                R(std::move(R_)),
                t(std::move(t_))
            {}

            cv::Mat E;
            cv::Mat R;
            cv::Mat t;
        };
        std::vector<poseHist> pose_history;//Holds all valid poses over the last estimations
        std::vector<double> pose_history_rating;//Holds a rating for every pose in the history based on the distance to the centre of gravity of all poses
        std::vector<size_t> mostLikelyPoseIdxs;//Holds the indexes over the last chosen poses within the history that are most likely the best
        std::vector<double> inlier_ratio_history;//Holds the inlier ratios for the last image pairs
        std::vector<statVals> errorStatistic_history;//Holds statics of the Sampson errors for the last image pairs
		size_t nr_Q_tooFar = 0;//Holds the number of 3D points for which their z-coordinate is too far away for calculating a reliable translation vector
		size_t nr_Qs = 0;//Number of 3D points stored

    public:

        cv::Mat E_new; //Newest essential matrix
        cv::Mat Q;//3D points of latest image pair
        cv::Mat R_new; //Newest rotation matrix
        cv::Mat t_new; //Newest translation vector
        cv::Mat E_mostLikely;//Essential matrix that is most likely the best over all stored in the history
        cv::Mat R_mostLikely; //Rotation matrix that is most likely the best over all stored in the history
        cv::Mat t_mostLikely; //Translation vector that is most likely the best over all stored in the history
        bool poseIsStable = false;//True, if the difference of the last poses or their error ranges is very small
        bool mostLikelyPose_stable = false;//True, if the chosen most likely best pose is the same over the last minContStablePoses iterations

        explicit StereoRefine(ConfigPoseEstimation cfg_pose_, bool verbose_ = false) :
            cfg_pose(cfg_pose_),
            verbose(verbose_){
            CV_Assert((cfg_pose_.K0 != nullptr)
            && (cfg_pose_.K1 != nullptr)
            && (cfg_pose_.dist0_8 != nullptr)
            && (cfg_pose_.dist1_8 != nullptr));

            pixToCamFact = 4.0 / (std::sqrt(2.0)
                    * (cfg_pose_.K0->at<double>(0, 0) + cfg_pose_.K0->at<double>(1, 1)
                            + cfg_pose_.K1->at<double>(0, 0) + cfg_pose_.K1->at<double>(1, 1)));
            nrEstimation = 0;
            skipCount = 0;
            th = cfg_pose_.th_pix_user * pixToCamFact; //Inlier threshold
            th2 = th * th;
            checkInputParamters();
            corrIdx = 0;
            t_mea = 0;
            t_oa = 0;
            descrDist_max = 0;
            keyPRespons_max = 0;
            poseIsStable = false;
            maxPoolSizeReached = false;
            mostLikelyPose_stable = false;
            initNumberInliers = 0;
			nrConsecStablePoses = 0;
			maxSkipPairsNew = cfg_pose_.maxSkipPairs;
			nr_Q_tooFar = 0;
			nr_Qs = 0;
        }//Constructor

        void setNewParameters(ConfigPoseEstimation cfg_pose_);//Set new parameters for the stereo pose estimation.

        int addNewCorrespondences(std::vector<cv::DMatch> matches,
                std::vector<cv::KeyPoint> kp1,
                std::vector<cv::KeyPoint> kp2,
                const poselib::ConfigUSAC &cfg);//Add new correspondences for estimating a new pose and estimate the pose.

        size_t getCorrespondencePoolSize(){
            return correspondencePool.size();
        }

    private:

        int robustPoseEstimation();//Robust pose estimation with optional refinement and Bundle Adjustment
        int refinePoseFromPool();//Refinement of E using all correspondences from the pool
        int addCorrespondencesToPool(std::vector<cv::DMatch> matches, std::vector<cv::KeyPoint> kp1, std::vector<cv::KeyPoint> kp2);//Adds new correspondences and their properties to the pool
        size_t getInliers(const cv::Mat &E, cv::Mat & p1, cv::Mat & p2, cv::Mat & mask, std::vector<double> & error);//Get the inlier mask and error values for a given set of correspondences and an Essential matrix E
        void clearHistoryAndPool();//Clears all stored correspondences, data, and history necessary to reinitialize the whole system.
        void checkInputParamters();//Checks if the input paramters are in the correct range.
        int filterNewCorrespondences(std::vector<cv::DMatch> & matches, std::vector<cv::KeyPoint> kp1, std::vector<cv::KeyPoint> kp2, std::vector<double> error);//Filters the correspondences of the newest image pair and from the pool.
        bool compareCorrespondences(CoordinatePropsNew &newCorr, CoordinateProps &oldCorr);//Compares the properties of correspondences from a new image pair and the pool which are within the search radius "minPtsDistance"
        int poolCorrespondenceDelete(std::vector<size_t> delete_list);//Delets correspondences from the correspondence pool and KD-tree.
        int checkPoolSize(int64_t maxPoolSize);//Check if the number of correspondences within the pool ("correspondencePool") is too large.
        double computeCorrespondenceWeight(const double &error, const double &descrDist, const double &resp1, const double &resp2, const bool &z3DtooFar = false, const double &zValue3D = 0);//Compute a weight between 0 and 1 for a correspondence given its keypoint responses (left and right img), descriptor distance, and Sampson error
        int getNearToMeanPose();//Function for rating poses.
        int checkPoseStability();//Checks the stability of poses within the history.
        int robustInitialization(double & inlier_ratio, std::vector<cv::DMatch> & matches, std::vector<cv::KeyPoint> & kp1, std::vector<cv::KeyPoint> & kp2);//Estimate a pose using robust estimation on the correspondences of the newest image pair.
        bool reinitializeSystem(double & inlier_ratio, std::vector<cv::DMatch> & matches, std::vector<cv::KeyPoint> & kp1, std::vector<cv::KeyPoint> & kp2);//Clears all stored correspondences, data, and history and adds the new inliers to the correspondence pool and updates the history.
        bool initDataAfterReinitialization(double & inlier_ratio, std::vector<cv::DMatch> & matches, std::vector<cv::KeyPoint> & kp1, std::vector<cv::KeyPoint> & kp2);//Adds the new inliers after robust estimation to the correspondence pool and updates the history.
        int robustEstimationOnPool(std::vector<cv::DMatch> & matches, std::vector<cv::KeyPoint> & kp1, std::vector<cv::KeyPoint> & kp2);//Estimate a pose using robust estimation on all stored pool correspondences
		void updateMaxSkipPairs();//Performs an update on maxSkipPairsNew if cfg_pose.raiseSkipCnt was configured to increase cfg_pose.maxSkipPairs after pose stability
    };

}
