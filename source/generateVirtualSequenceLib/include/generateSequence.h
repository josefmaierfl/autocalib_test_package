/**********************************************************************************************************
FILE: generateSequence.h

PLATFORM: Windows 7, MS Visual Studio 2015, OpenCV 3.2

CODE: C++

AUTOR: Josef Maier, AIT Austrian Institute of Technology

DATE: March 2018

LOCATION: TechGate Vienna, Donau-City-Stra�e 1, 1220 Vienna

VERSION: 1.0

DISCRIPTION: This file provides functionalities for generating stereo sequences with correspondences given
a view restrictions like depth ranges, moving objects, ...
**********************************************************************************************************/

#pragma once

#include "glob_includes.h"
#include "helper_funcs.h"
#include "opencv2/highgui/highgui.hpp"
#include <random>
#include <exception>

#include "pcl/point_cloud.h"
#include "pcl/point_types.h"

#include "generateVirtualSequenceLib/generateVirtualSequenceLib_api.h"

/* --------------------------- Defines --------------------------- */

struct GENERATEVIRTUALSEQUENCELIB_API depthPortion
{
	depthPortion()
	{
		near = 1.0 / 3.0;
		mid = 1.0 / 3.0;
		far = 1.0 / 3.0;
	}

	depthPortion(double near_, double mid_, double far_): near(near_), mid(mid_), far(far_)	
	{
		sumTo1();
	}

	void sumTo1()
	{
		checkCorrectValidity();
		double dsum = near + mid + far;
		near /= dsum;
		mid /= dsum;
		far /= dsum;
	}

	void checkCorrectValidity()
	{
		if (nearZero(near + mid + far))
		{
			near = 1.0 / 3.0;
			mid = 1.0 / 3.0;
			far = 1.0 / 3.0;
		}
	}

	double near;
	double mid;
	double far;
};

enum GENERATEVIRTUALSEQUENCELIB_API depthClass
{
	NEAR,
	MID,
	FAR
};

enum GENERATEVIRTUALSEQUENCELIB_API vorboseType
{
	SHOW_INIT_CAM_PATH = 0x01,
	SHOW_BUILD_PROC_MOV_OBJ = 0x02,
	SHOW_MOV_OBJ_DISTANCES = 0x04,
	SHOW_MOV_OBJ_3D_PTS = 0x08,
	SHOW_MOV_OBJ_CORRS_GEN = 0x10,
	SHOW_BUILD_PROC_STATIC_OBJ = 0x20,
	SHOW_STATIC_OBJ_DISTANCES = 0x40,
	SHOW_STATIC_OBJ_CORRS_GEN = 0x80,
	SHOW_STATIC_OBJ_3D_PTS = 0x100,
	SHOW_MOV_OBJ_MOVEMENT = 0x200
};

struct GENERATEVIRTUALSEQUENCELIB_API StereoSequParameters
{
	StereoSequParameters(std::vector<cv::Mat> camTrack_,
		size_t nFramesPerCamConf_ = 5,
		std::pair<double, double> inlRatRange_ = std::make_pair(0.1, 1.0),
		double inlRatChanges_ = 0,
		std::pair<size_t, size_t> truePosRange_ = std::make_pair(100, 2000),
		double truePosChanges_ = 0,
		bool keypPosErrType_ = false,
		std::pair<double, double> keypErrDistr_ = std::make_pair(0, 0.5),
		std::pair<double, double> imgIntNoise_ = std::make_pair(0, 5.0),
		double minKeypDist_ = 3.0,
		depthPortion corrsPerDepth_ = depthPortion(),
		std::vector<cv::Mat> corrsPerRegion_ = std::vector<cv::Mat>(),
		size_t corrsPerRegRepRate_ = 1,
		std::vector<std::vector<depthPortion>> depthsPerRegion_ = std::vector<std::vector<depthPortion>>(),
		std::vector<std::vector<std::pair<size_t, size_t>>> nrDepthAreasPReg_ = std::vector<std::vector<std::pair<size_t, size_t>>>(),
		//double lostCorrPor_ = 0,
		double relCamVelocity_ = 0.5,
		cv::InputArray R_ = cv::noArray(),
		size_t nrMovObjs_ = 0,
		cv::InputArray startPosMovObjs_ =  cv::noArray(),
		std::pair<double, double> relAreaRangeMovObjs_ = std::make_pair(0.01, 0.1),
		std::vector<depthClass> movObjDepth_ = std::vector<depthClass>(),
		cv::InputArray movObjDir_ = cv::noArray(),
		std::pair<double, double> relMovObjVelRange_ = std::make_pair(0.5, 1.5),
		double minMovObjCorrPortion_ = 0.5,
		double CorrMovObjPort_ = 0.25,
		size_t minNrMovObjs_ = 0
		): 
	nFramesPerCamConf(nFramesPerCamConf_), 
		inlRatRange(inlRatRange_),
		inlRatChanges(inlRatChanges_),
		truePosRange(truePosRange_),
		truePosChanges(truePosChanges_),
		keypPosErrType(keypPosErrType_),
		keypErrDistr(keypErrDistr_),
		imgIntNoise(imgIntNoise_),
		minKeypDist(minKeypDist_),
		corrsPerDepth(corrsPerDepth_),
		corrsPerRegion(corrsPerRegion_),
		corrsPerRegRepRate(corrsPerRegRepRate_),
		depthsPerRegion(depthsPerRegion_),
		nrDepthAreasPReg(nrDepthAreasPReg_),
		//lostCorrPor(lostCorrPor_),
		camTrack(camTrack_),
		relCamVelocity(relCamVelocity_),
//		R(R_),
		nrMovObjs(nrMovObjs_),
//		startPosMovObjs(startPosMovObjs_),
		relAreaRangeMovObjs(relAreaRangeMovObjs_),
		movObjDepth(movObjDepth_),
//		movObjDir(movObjDir_),
		relMovObjVelRange(relMovObjVelRange_),
		minMovObjCorrPortion(minMovObjCorrPortion_),
		CorrMovObjPort(CorrMovObjPort_),
		minNrMovObjs(minNrMovObjs_)
	{
		CV_Assert(nFramesPerCamConf > 0);
		CV_Assert((inlRatRange.first < 1.0) && (inlRatRange.first >= 0) && (inlRatRange.second <= 1.0) && (inlRatRange.second > 0));
		CV_Assert((inlRatChanges <= 100.0) && (inlRatChanges >= 0));
		CV_Assert((truePosRange.first > 0) && (truePosRange.second > 0) && (truePosRange.second >= truePosRange.first));
		CV_Assert((truePosChanges <= 100.0) && (truePosChanges >= 0));
		CV_Assert(keypPosErrType || (!keypPosErrType && (keypErrDistr.first > -5.0) && (keypErrDistr.first < 5.0) && (keypErrDistr.second > -5.0) && (keypErrDistr.second < 5.0)));
		CV_Assert((imgIntNoise.first > -25.0) && (imgIntNoise.first < 25.0) && (imgIntNoise.second > -25.0) && (imgIntNoise.second < 25.0));
		CV_Assert((minKeypDist >= 1.0) && (minKeypDist < 100.0));
		CV_Assert((corrsPerDepth.near >= 0) && (corrsPerDepth.mid >= 0) && (corrsPerDepth.far >= 0));
		CV_Assert(corrsPerRegion.empty() || ((corrsPerRegion[0].rows == 3) && (corrsPerRegion[0].cols == 3) && (corrsPerRegion[0].type() == CV_64FC1)));
		CV_Assert(depthsPerRegion.empty() || ((depthsPerRegion.size() == 3) && (depthsPerRegion[0].size() == 3) && (depthsPerRegion[1].size() == 3) && (depthsPerRegion[2].size() == 3)));
		CV_Assert(nrDepthAreasPReg.empty() || ((nrDepthAreasPReg.size() == 3) && (nrDepthAreasPReg[0].size() == 3) && (nrDepthAreasPReg[1].size() == 3) && (nrDepthAreasPReg[2].size() == 3)));
		//CV_Assert((lostCorrPor >= 0) && (lostCorrPor <= 1.0));

		CV_Assert(!camTrack.empty() && (camTrack[0].rows == 3) && (camTrack[0].cols == 1) && (camTrack[0].type() == CV_64FC1));
		CV_Assert((relCamVelocity > 0) && (relCamVelocity <= 10.0));
		CV_Assert(R_.empty() || ((R_.getMat().rows == 3) && (R_.getMat().cols == 3) && (R_.type() == CV_64FC1)));
		CV_Assert(nrMovObjs < 20);
		CV_Assert(startPosMovObjs_.empty() || ((startPosMovObjs_.getMat().rows == 3) && (startPosMovObjs_.getMat().cols == 3) && (startPosMovObjs_.type() == CV_8UC1)));
		CV_Assert((relAreaRangeMovObjs.first <= 1.0) && (relAreaRangeMovObjs.first >= 0) && (relAreaRangeMovObjs.second <= 1.0) && (relAreaRangeMovObjs.second > 0) && (relAreaRangeMovObjs.first <= relAreaRangeMovObjs.second));
		CV_Assert(movObjDir_.empty() || ((movObjDir_.getMat().rows == 3) && (movObjDir_.getMat().cols == 1) && (movObjDir_.type() == CV_64FC1)));
		CV_Assert((relMovObjVelRange.first < 100.0) && (relAreaRangeMovObjs.first >= 0) && (relAreaRangeMovObjs.second <= 100.0) && (relAreaRangeMovObjs.second > 0));
		CV_Assert((minMovObjCorrPortion <= 1.0) && (minMovObjCorrPortion >= 0));
		CV_Assert((CorrMovObjPort > 0) && (CorrMovObjPort <= 1.0));
		CV_Assert(minNrMovObjs <= nrMovObjs);

		if(!R_.empty())
		    R = R_.getMat();
        if(!startPosMovObjs_.empty())
            startPosMovObjs = startPosMovObjs_.getMat();
        if(!movObjDir_.empty())
            movObjDir = movObjDir_.getMat();
	}

	//Parameters for generating correspondences
	size_t nFramesPerCamConf;//# of Frames per camera configuration
	std::pair<double, double> inlRatRange;//Inlier ratio range
	double inlRatChanges;//Inlier ratio change rate from pair to pair. If 0, the inlier ratio within the given range is always the same for every image pair. If 100, the inlier ratio is chosen completely random within the given range.For values between 0 and 100, the inlier ratio selected is not allowed to change more than this factor from the last inlier ratio.
	std::pair<size_t, size_t> truePosRange;//# true positives range
	double truePosChanges;//True positives change rate from pair to pair. If 0, the true positives within the given range are always the same for every image pair. If 100, the true positives are chosen completely random within the given range.For values between 0 and 100, the true positives selected are not allowed to change more than this factor from the true positives.
	bool keypPosErrType;//Keypoint detector error (true) or error normal distribution (false)
	std::pair<double, double> keypErrDistr;//Keypoint error distribution (mean, std)
	std::pair<double, double> imgIntNoise;//Noise (mean, std) on the image intensity for descriptor calculation
	double minKeypDist;//min. distance between keypoints
	depthPortion corrsPerDepth;//portion of correspondences at depths
	std::vector<cv::Mat> corrsPerRegion;//List of portions of image correspondences at regions (Matrix must be 3x3). Maybe doesnt hold: Also depends on 3D-points from prior frames.
	size_t corrsPerRegRepRate;//Repeat rate of portion of correspondences at regions. If more than one matrix of portions of correspondences at regions is provided, this number specifies the number of frames for which such a matrix is valid. After all matrices are used, the first one is used again. If 0 and no matrix of portions of correspondences at regions is provided, as many random matrizes as frames are randomly generated.
	std::vector<std::vector<depthPortion>> depthsPerRegion;//Portion of depths per region (must be 3x3). For each of the 3x3=9 image regions, the portion of near, mid, and far depths can be specified. If the overall depth definition is not met, this tensor is adapted.Maybe doesnt hold: Also depends on 3D - points from prior frames.
	std::vector<std::vector<std::pair<size_t, size_t>>> nrDepthAreasPReg;//Min and Max number of connected depth areas per region (must be 3x3). The minimum number (first) must be larger 0. The maximum number is bounded by the minimum area which is 16 pixels. Maybe doesnt hold: Also depends on 3D - points from prior frames.
	//double lostCorrPor;//Portion of lost correspondences from frame to frame. It corresponds to the portion of 3D-points that would be visible in the next frame.

	//Paramters for camera and object movements
	std::vector<cv::Mat> camTrack;//Movement direction or track of the cameras (Mat must be 3x1). If 1 vector: Direction in the form [tx, ty, tz]. If more vectors: absolute position edges on a track.  The scaling of the track is calculated using the velocity information(The last frame is located at the last edge); tz is the main viewing direction of the first camera which can be changed using the rotation vector for the camera centre.The camera rotation during movement is based on the relative movement direction(like a fixed stereo rig mounted on a car).
	double relCamVelocity;//Relative velocity of the camera movement (value between 0 and 10; must be larger 0). The velocity is relative to the baseline length between the stereo cameras
	cv::Mat R;//Rotation matrix of the first camera centre. This rotation can change the camera orientation for which without rotation the z - component of the relative movement vector coincides with the principal axis of the camera. Rotation matrix must be generated using the form R_y * R_z * R_x.
	size_t nrMovObjs;//Number of moving objects in the scene
	cv::Mat startPosMovObjs;//Possible starting positions of moving objects in the image (must be 3x3 boolean (CV_8UC1))
	std::pair<double, double> relAreaRangeMovObjs;//Relative area range of moving objects. Area range relative to the image area at the beginning.
	std::vector<depthClass> movObjDepth;//Depth of moving objects. Moving objects are always visible and not covered by other static objects. If the number of paramters is 1, this depth is used for every object. If the number of paramters is equal "nrMovObjs", the corresponding depth is used for every object. If the number of parameters is smaller and between 2 and 3, the depths for the moving objects are selected uniformly distributed from the given depths. For a number of paramters larger 3 and unequal to "nrMovObjs", a portion for every depth that should be used can be defined (e.g. 3 x far, 2 x near, 1 x mid -> 3 / 6 x far, 2 / 6 x near, 1 / 6 x mid).
	cv::Mat movObjDir;//Movement direction of moving objects relative to camera movementm (must be 3x1). The movement direction is linear and does not change if the movement direction of the camera changes.The moving object is removed, if it is no longer visible in both stereo cameras.
	std::pair<double, double> relMovObjVelRange;//Relative velocity range of moving objects based on relative camera velocity. Values between 0 and 100; Must be larger 0;
	double minMovObjCorrPortion;//Minimal portion of correspondences on moving objects for removing them. If the portion of visible correspondences drops below this value, the whole moving object is removed. Zero means, that the moving object is only removed if there is no visible correspondence in the stereo pair. One means, that a single missing correspondence leads to deletion. Values between 0 and 1;
	double CorrMovObjPort;//Portion of correspondences on moving object (compared to static objects). It is limited by the size of the objects visible in the images and the minimal distance between correspondences.
	size_t minNrMovObjs;//Minimum number of moving objects over the whole track. If the number of moving obects drops below this number during camera movement, as many new moving objects are inserted until "nrMovObjs" is reached. If 0, no new moving objects are inserted if every preceding object is out of sight.
};

struct Poses
{
	Poses()
	{
		R = cv::Mat::eye(3, 3, CV_64FC1);
		t = cv::Mat::zeros(3, 1, CV_64FC1);
	}
	Poses(cv::Mat R_, cv::Mat t_) : R(R_), t(t_) {}

	cv::Mat R;
	cv::Mat t;
};


/* --------------------------- Classes --------------------------- */

class SequenceException : public std::exception
{
    std::string _msg;

public:
    SequenceException(const std::string &msg) : _msg(msg) {}

    virtual const char *what() const noexcept override
    {
        return _msg.c_str();
    }
};

class GENERATEVIRTUALSEQUENCELIB_API genStereoSequ
{
public:
	genStereoSequ(cv::Size imgSize_,
			cv::Mat K1_,
			cv::Mat K2_,
			std::vector<cv::Mat> R_,
			std::vector<cv::Mat> t_,
			StereoSequParameters & pars_,
			uint32_t verbose = 0);
	void startCalc();

private:
	void constructCamPath();
	cv::Mat getTrackRot(const cv::Mat tdiff, cv::InputArray R_old = cv::noArray());
	void genMasks();
	bool getDepthRanges();
	void adaptDepthsPerRegion();
	void updDepthReg(bool isNear, std::vector<std::vector<depthPortion>> &depthPerRegion, cv::Mat &cpr);
	void genInlierRatios();
	void genNrCorrsImg();
	bool initFracCorrImgReg();
	void initNrCorrespondences();
	void checkDepthAreas();
	void calcPixAreaPerDepth();
	void checkDepthSeeds();
	void checkMovObjDirection();
	void backProject3D();
	void adaptIndicesNoDel(std::vector<size_t> &idxVec, std::vector<size_t> &delListSortedAsc);
	void adaptIndicesCVPtNoDel(std::vector<cv::Point3_<int32_t>> &seedVec, std::vector<size_t> &delListSortedAsc);
	void genRegMasks();
	void genDepthMaps();
	bool addAdditionalDepth(unsigned char pixVal,
		cv::Mat &imgD,
		cv::Mat &imgSD,
		cv::Mat &mask,
		cv::Mat &regMask,
		cv::Point_<int32_t> &startpos,
		cv::Point_<int32_t> &endpos,
		int32_t &addArea,
		int32_t &maxAreaReg,
		cv::Size &siM1,
		cv::Point_<int32_t> initSeed,
		cv::Rect &vROI,
		size_t &nrAdds,
		unsigned char &usedDilate,
		cv::InputOutputArray neighborRegMask = cv::noArray(),
		unsigned char regIdx = 0);
	std::vector<int32_t> getPossibleDirections(cv::Point_<int32_t> &startpos, cv::Mat &mask, cv::Mat &regMask, cv::Mat &imgD, cv::Size &siM1, cv::Mat &imgSD, bool escArea = true);
    void nextPosition(cv::Point_<int32_t> &position, int32_t direction);
	void getRandDepthFuncPars(std::vector<std::vector<double>> &pars1, size_t n_pars);
	void getDepthVals(cv::OutputArray dout, const cv::Mat &din, double dmin, double dmax, std::vector<cv::Point3_<int32_t>> &initSeedInArea);
	inline double getDepthFuncVal(std::vector<double> &pars1, double x, double y);
	void getDepthMaps(cv::OutputArray dout, cv::Mat &din, double dmin, double dmax, std::vector<std::vector<std::vector<cv::Point3_<int32_t>>>> &initSeeds, int dNr);
	void getKeypoints();
	int32_t genTrueNegCorrs(int32_t nrTN,
		std::uniform_int_distribution<int32_t> &distributionX,
		std::uniform_int_distribution<int32_t> &distributionY,
		std::uniform_int_distribution<int32_t> &distributionX2,
		std::uniform_int_distribution<int32_t> &distributionY2,
		std::vector<cv::Point2d> &x1TN,
		std::vector<cv::Point2d> &x2TN,
		std::vector<double> &x2TNdistCorr,
		cv::Mat &img1Mask,
		cv::Mat &img2Mask,
		cv::Mat &usedDepthMap);/*,
		cv::InputArray labelMask = cv::noArray());*/
	bool checkLKPInlier(cv::Point_<int32_t> pt, cv::Point2d &pt2, cv::Point3d &pCam, cv::Mat &usedDepthMap);
	void getNrSizePosMovObj();
	void buildDistributionRanges(std::vector<int> &xposes,
								 std::vector<int> &yposes,
								 int &x,
								 int &y,
								 std::vector<double> &xInterVals,
								 std::vector<double> &xWeights,
								 std::vector<double> &yInterVals,
								 std::vector<double> &yWeights);
	void generateMovObjLabels(cv::Mat &mask, std::vector<cv::Point_<int32_t>> &seeds, std::vector<int32_t> &areas, int32_t corrsOnMovObjLF);
	void genNewDepthMovObj();
	void backProjectMovObj();
	void genMovObjHulls(cv::Mat &corrMask, std::vector<cv::Point> &kps, cv::Mat &finalMask);
	void genHullFromMask(cv::Mat &mask, std::vector<cv::Point> &finalHull);
	void getSeedsAreasMovObj();
	bool getSeedAreaListFromReg(std::vector<cv::Point_<int32_t>> &seeds, std::vector<int32_t> &areas);
	bool getNewMovObjs();
	void getMovObjCorrs();
	void combineCorrespondences();
	void updateFrameParameters();
	void getNewCorrs();
	void transPtsToWorld();
	void transMovObjPtsToWorld();
	void updateMovObjPositions();
	void getActEigenCamPose();
	bool getVisibleCamPointCloud(pcl::PointCloud<pcl::PointXYZ>::Ptr cloudIn, pcl::PointCloud<pcl::PointXYZ>::Ptr cloudOut);
	bool filterNotVisiblePts(pcl::PointCloud<pcl::PointXYZ>::Ptr cloudIn, pcl::PointCloud<pcl::PointXYZ>::Ptr cloudOut, bool useNearLeafSize = false);
	void getMovObjPtsCam();
	void getCamPtsFromWorld();
	void visualizeCamPath();
	void visualizeMovObjPtCloud();
    void visualizeStaticObjPtCloud();
    void visualizeMovingAndStaticObjPtCloud();
	void visualizeMovObjMovement(std::vector<pcl::PointXYZ> &cloudCentroids_old,
                                 std::vector<pcl::PointXYZ> &cloudCentroids_new,
                                 std::vector<float> &cloudExtensions);
    int32_t getRandMask(cv::Mat &mask, int32_t area, int32_t useRad, int32_t midR);
    bool fillRemainingAreas(cv::Mat &depthArea,
                            const cv::Mat &usedAreas,
                            int32_t areaToFill,
                            int32_t &actualArea,
                            cv::InputArray otherDepthA1 = cv::noArray(),
                            cv::InputArray otherDepthA2 = cv::noArray());
    void removeNrFilledPixels(cv::Size delElementSi, cv::Size matSize, cv::Mat &targetMat, int32_t nrToDel);
    void delOverlaps2(cv::Mat &depthArea1, cv::Mat &depthArea2);
    void adaptNRCorrespondences(int32_t nrTP,
                                int32_t nrTN,
                                size_t corrsNotVisible,
                                size_t foundTPCorrs,
                                int idx_x,
                                int32_t nr_movObj = 0,
                                int y = 0);

public:
	uint32_t verbose = 0;

private:
	std::default_random_engine rand_gen;

	const int32_t minDArea = 36;//6*6: minimum area for a depth region in the image
	const double maxFarDistMultiplier = 20.0;//the maximum possible depth used is 

	cv::Size imgSize;
	cv::Mat K1, K1i;//Camera matrix 1 and its inverse
	cv::Mat K2, K2i;//Camera matrix 2 and its inverse
	std::vector<cv::Mat> R;
	std::vector<cv::Mat> t;
	StereoSequParameters pars;
	size_t nrStereoConfs;//Number of different stereo camera configurations

	size_t totalNrFrames = 0;//Total number of frames
	double absCamVelocity;//in baselines from frame to frame
	std::vector<Poses> absCamCoordinates;//Absolute coordinates of the camera centres (left or bottom cam of stereo rig) for every frame; Includes the rotation from the camera into world and the position of the camera centre C in the world: X_world  = R * X_cam + t (t corresponds to C in this case); X_cam = R^T * X_world - R^T * t

	std::vector<double> inlRat;//Inlier ratio for every frame
	std::vector<size_t> nrTruePos;//Absolute number of true positive correspondences per frame
	std::vector<size_t> nrCorrs;//Absolute number of correspondences (TP+TN) per frame
	std::vector<size_t> nrTrueNeg;//Absolute number of true negative correspondences per frame
	bool fixedNrCorrs = false;//If the inlier ratio and the absolute number of true positive correspondences are constant over all frames, the # of correspondences are as well const. and fixedNrCorrs = true
	std::vector<cv::Mat> nrTruePosRegs;//Absolute number of true positive correspondences per image region and frame; Type CV_32SC1
	std::vector<cv::Mat> nrCorrsRegs;//Absolute number of correspondences (TP+TN) per image region and frame; Type CV_32SC1
	std::vector<cv::Mat> nrTrueNegRegs;//Absolute number of true negative correspondences per image region and frame; Type CV_32SC1

	std::vector<std::vector<cv::Mat>> regmasks;//Mask for every of the 3x3 regions with the same size as the image and a specific percentage of validity outside every region (overlap areas). Used for generating depth maps. See genRegMasks() for details on the overlap area.
	std::vector<std::vector<cv::Rect>> regmasksROIs;//ROIs used to generate regmasks.

	cv::Mat depthMap;//Mat of the same size as the image holding a depth value for every pixel (double)
	cv::Mat depthAreaMap;//Type CV_8UC1 and same size as image; Near depth areas are marked with 1, mid with 2 and far with 3
	std::vector<double> depthNear;//Lower border of near depths for every camera configuration
	std::vector<double> depthMid;//Upper border of near and lower border of mid depths for every camera configuration
	std::vector<double> depthFar;//Upper border of far depths for every camera configuration
	std::vector<std::vector<std::vector<depthPortion>>> depthsPerRegion;//same size as pars.corrsPerRegion: m x 3 x 3
	std::vector<cv::Mat> nrDepthAreasPRegNear;//Number of depth areas ord seeds holding near depth values per region; Type CV_32SC1, same size as depthsPerRegion
	std::vector<cv::Mat> nrDepthAreasPRegMid;//Number of depth areas ord seeds holding mid depth values per region; Type CV_32SC1, same size as depthsPerRegion
	std::vector<cv::Mat> nrDepthAreasPRegFar;//Number of depth areas ord seeds holding far depth values per region; Type CV_32SC1, same size as depthsPerRegion
	std::vector<cv::Mat> areaPRegNear;//Area in pixels per region that should hold near depth values; Type CV_32SC1, same size as depthsPerRegion
	std::vector<cv::Mat> areaPRegMid;//Area in pixels per region that should hold mid depth values; Type CV_32SC1, same size as depthsPerRegion
	std::vector<cv::Mat> areaPRegFar;//Area in pixels per region that should hold far depth values; Type CV_32SC1, same size as depthsPerRegion
	std::vector<std::vector<cv::Rect>> regROIs;//ROIs of every of the 9x9 image regions

	pcl::PointCloud<pcl::PointXYZ> staticWorld3DPts;//Point cloud in the world coordinate system holding all generated 3D points
	std::vector<cv::Point3d> actImgPointCloudFromLast;//3D coordiantes that were generated with a different frame. Coordinates are in the camera coordinate system.
	std::vector<cv::Point3d> actImgPointCloud;//All newly generated 3D coordiantes excluding actImgPointCloudFromLast. Coordinates are in the camera coordinate system.
	cv::Mat actCorrsImg1TPFromLast, actCorrsImg2TPFromLast;//TP correspondences in the stereo rig from actImgPointCloudFromLast in homogeneous image coordinates. Size: 3xn; Last row should be 1.0; Both Mat must have the same size.
	std::vector<size_t> actCorrsImg12TPFromLast_Idx;//Index to the corresponding 3D point within actImgPointCloudFromLast of correspondences in actCorrsImg1TPFromLast and actCorrsImg2TPFromLast
	cv::Mat actCorrsImg1TP, actCorrsImg2TP;//TP orrespondences in the stereo rig from actImgPointCloud in homogeneous image coordinates. Size: 3xn; Last row should be 1.0; Both Mat must have the same size.
	cv::Mat actCorrsImg1TNFromLast;//TN keypoint in the first stereo rig image from actImgPointCloudFromLast in homogeneous image coordinates. Size: 3xn; Last row should be 1.0
	std::vector<size_t> actCorrsImg1TNFromLast_Idx;//Index to the corresponding 3D point within actImgPointCloudFromLast of actCorrsImg1TNFromLast
	cv::Mat actCorrsImg2TNFromLast;//TN keypoint in the second stereo rig image from actImgPointCloudFromLast in homogeneous image coordinates. Size: 3xn; Last row should be 1.0
	std::vector<size_t> actCorrsImg2TNFromLast_Idx;//Index to the corresponding 3D point within actImgPointCloudFromLast of actCorrsImg2TNFromLast
	cv::Mat actCorrsImg1TN;//Newly created TN keypoint in the first stereo rig image (no 3D points were created for them)
	cv::Mat actCorrsImg2TN;//Newly created TN keypoint in the second stereo rig image (no 3D points were created for them)
	std::vector<double> distTNtoReal;//Distance values of the TN keypoint locations in the 2nd image to the location that would be a perfect correspondence to the TN in image 1. If the value is >= 50, the "perfect location" would be outside the image
	cv::Mat corrsIMG;//Every keypoint location is marked within this Mat with a square (ones) of the size of the minimal keypoint distance. The size is img-size plus 2 * ceil(min. keypoint dist)
	cv::Mat csurr;//Mat of ones with the size 2 * ceil(min. keypoint dist) + 1

	cv::Mat actR;//actual rotation matrix of the stereo rig: x2 = actR * x1 + actT
	cv::Mat actT;//actual translation vector of the stereo rig: x2 = actR * x1 + actT
	size_t actFrameCnt = 0;
	size_t actCorrsPRIdx = 0;//actual index (corresponding to the actual frame) for pars.corrsPerRegion, depthsPerRegion, nrDepthAreasPRegNear, ...
	size_t actStereoCIdx = 0;//actual index (corresponding to the actual frame) for R, t, depthNear, depthMid, depthFar
	double actDepthNear;//Lower border of near depths for the actual camera configuration
	double actDepthMid;//Upper border of near and lower border of mid depths for the actual camera configuration
	double actDepthFar;//Upper border of mid and lower border of far depths for the actual camera configuration
	Eigen::Matrix4f actCamPose;//actual camera pose in camera coordinates in a different camera coordinate system (X forward, Y is up, and Z is right) to use the PCL filter FrustumCulling
	Eigen::Quaternionf actCamRot;//actual camerea rotation from world to camera (rotation w.r.t the origin)

	std::vector<std::vector<std::vector<cv::Point3_<int32_t>>>> seedsNear;//Holds the actual near seeds for every region; Size 3x3xn; Point3 holds the seed coordinate (x,y) and a possible index (=z) to actCorrsImg12TPFromLast_Idx if the seed was generated from an existing 3D point (otherwise z=-1).
	std::vector<std::vector<std::vector<cv::Point3_<int32_t>>>> seedsMid;//Holds the actual mid seeds for every region; Size 3x3xn; Point3 holds the seed coordinate (x,y) and a possible index (=z) to actCorrsImg12TPFromLast_Idx if the seed was generated from an existing 3D point (otherwise z=-1).
	std::vector<std::vector<std::vector<cv::Point3_<int32_t>>>> seedsFar;//Holds the actual far seeds for every region; Size 3x3xn; Point3 holds the seed coordinate (x,y) and a possible index (=z) to actCorrsImg12TPFromLast_Idx if the seed was generated from an existing 3D point (otherwise z=-1).
	std::vector<std::vector<std::vector<cv::Point_<int32_t>>>> seedsNearFromLast;//Holds the actual near seeds of backprojected 3D points for every region; Size 3x3xn;
	std::vector<std::vector<std::vector<cv::Point_<int32_t>>>> seedsMidFromLast;//Holds the actual mid seeds of backprojected 3D points for every region; Size 3x3xn;
	std::vector<std::vector<std::vector<cv::Point_<int32_t>>>> seedsFarFromLast;//Holds the actual far seeds of backprojected 3D points for every region; Size 3x3xn;
    std::vector<std::vector<std::vector<int32_t>>> seedsNearNNDist;//Holds the distance to the nearest neighbor for every seed of seedsNear
    std::vector<std::vector<std::vector<int32_t>>> seedsMidNNDist;//Holds the distance to the nearest neighbor for every seed of seedsMid
    std::vector<std::vector<std::vector<int32_t>>> seedsFarNNDist;//Holds the distance to the nearest neighbor for every seed of seedsFar

	cv::Mat startPosMovObjs; //Possible starting positions of moving objects in the image (must be 3x3 boolean (CV_8UC1))
	cv::Mat movObjDir;//Movement direction of moving objects relative to camera movementm (must be 3x1 double). The movement direction is linear and does not change if the movement direction of the camera changes.
	int minOArea, maxOArea;//Minimum and maximum area of single moving objects in the image
	int minODist;//Minimum distance between seeding positions (for areas) of moving objects
	int maxOPerReg;//Maximum number of moving objects or seeds per region
	std::vector<std::vector<std::vector<int32_t>>> movObjAreas;//Area for every new moving object per image region (must be 3x3xn)
	std::vector<std::vector<std::vector<cv::Point_<int32_t>>>> movObjSeeds;//Seeding positions for every new moving object per image region (must be 3x3xn and the same size as movObjAreas)
	std::vector<std::vector<cv::Point>> convhullPtsObj;//Every vector element (size corresponds to number of moving objects) holds the convex hull of backprojected (into image) 3D-points from a moving object
	std::vector<std::vector<cv::Point3d>> movObj3DPtsCam;//Every vector element (size corresponds to number of existing moving objects) holds the 3D-points from a moving object in camera coordinates
	std::vector<pcl::PointCloud<pcl::PointXYZ>> movObj3DPtsWorld;//Every vector element (size corresponds to number of existing moving objects) holds the 3D-points from a moving object in world coordinates
	std::vector<cv::Mat> movObjWorldMovement;//Holds the absolute 3x1 movement vector scaled by velocity for every moving object 
	std::vector<std::vector<cv::Point3d>> movObj3DPtsCamNew;//Every vector element (size corresponds to number of new generated moving objects) holds the 3D-points from a moving object in camera coordinates
	std::vector<depthClass> movObjDepthClass;//Every vector element (size corresponds to number of moving objects) holds the depth class (near, mid, or far) for its corresponding object
	std::vector<cv::Mat> movObjLabels;//Every vector element (size corresponds to number of newly to add moving objects) holds a mask with the size of the image marking the area of the moving object
	std::vector<cv::Rect> movObjLabelsROIs;////Every vector element (size corresponds to number of newly to add moving objects) holds a bounding rectangle of the new labels stored in movObjLabels
	std::vector<depthClass> movObjDepthClassNew;//Every vector element (size corresponds to number of newly to add moving objects) holds the depth class (near, mid, or far) for its corresponding object
	cv::Mat combMovObjLabels;//Combination of all masks stored in movObjLabels. Every label has a different value.
	cv::Mat combMovObjDepths;//Every position marked by combMovObjLabels holds an individual depth value
	cv::Mat combMovObjLabelsAll;//Combination of all masks marking moving objects (New generated elements (combMovObjLabels) and existing elements (movObjMaskFromLast)). Every label has a different value.
	int32_t actCorrsOnMovObj;//Actual number of new correspondences on moving objects (excluding backprojected correspondences from moving objects that were created one or more frames ago) including true negatives
	int32_t actTruePosOnMovObj;//Number of new true positive correspondences on moving objects: actTruePosOnMovObj = actCorrsOnMovObj - actTrueNegOnMovObj
	int32_t actTrueNegOnMovObj;//Number of new true negative correspondences on moving objects: actTrueNegOnMovObj = actCorrsOnMovObj - actTruePosOnMovObj
	int32_t actCorrsOnMovObjFromLast;//Actual number of backprojected correspondences on moving objects (from moving objects that were created one or more frames ago)  including true negatives
	int32_t actTruePosOnMovObjFromLast;//Number of backprojected true positive correspondences on moving objects: actTruePosOnMovObjFromLast = actCorrsOnMovObjFromLast - actTrueNegOnMovObjFromLast
	int32_t actTrueNegOnMovObjFromLast;//Number of backprojected true negative correspondences on moving objects: actTrueNegOnMovObjFromLast = actCorrsOnMovObjFromLast - actTruePosOnMovObjFromLast
	std::vector<int32_t> actTPPerMovObj;//Number of true positive correspondences on every single moving object (size corresponds to number of newly to add moving objects)
	std::vector<int32_t> actTNPerMovObj;//Number of true negative correspondences on every single moving object (size corresponds to number of newly to add moving objects)
	std::vector<int32_t> actTNPerMovObjFromLast;//Number of true negative correspondences for every single backprojected moving object (size corresponds to number of already existing moving objects)
	std::vector<cv::Mat> movObjLabelsFromLast;//Every vector element (size corresponds to number of backprojected moving objects) holds a mask with the size of the image marking the area of the moving object
	cv::Mat movObjMaskFromLast;//Mask with the same size as the image masking areas with moving objects that were backprojected (mask for first stereo image)
	cv::Mat movObjMaskFromLast2;//Mask with the same size as the image masking correspondences of moving objects that were backprojected (mask for second stereo image)
	/*noch nicht angelegt*///cv::Mat movObjMask2;//Mask with the same size as the image masking correspondences of new moving objects (mask for second stereo image)
	cv::Mat movObjMask2All;//Combination of movObjMaskFromLast2 and movObjMask2. Mask with the same size as the image masking correspondences of moving objects (mask for second stereo image)
	std::vector<std::vector<bool>> movObjHasArea;//Indicates for every region if it is completely occupied by a moving object
	std::vector<cv::Mat> movObjCorrsImg1TPFromLast, movObjCorrsImg2TPFromLast;//Every vector element (size corresponds to number of moving objects) holds correspondences within a moving object. Every vector element: Size: 3xn; Last row should be 1.0; Both Mat (same vector index) must have the same size.
	std::vector<std::vector<size_t>> movObjCorrsImg12TPFromLast_Idx;//Index to the corresponding 3D point within movObj3DPtsCam of correspondences in movObjCorrsImg1TPFromLast and movObjCorrsImg2TPFromLast
	std::vector<cv::Mat> movObjCorrsImg1TNFromLast;//Every vector element (size corresponds to number of moving objects) holds TN correspondences within a moving object. Every vector element: TN keypoint in the first stereo rig image from movObj3DPtsCam in homogeneous image coordinates. Size: 3xn; Last row should be 1.0
	std::vector<cv::Mat> movObjCorrsImg2TNFromLast;//Every vector element (size corresponds to number of moving objects) holds TN correspondences within a moving object. Every vector element: TN keypoint in the second stereo rig image from movObj3DPtsCam in homogeneous image coordinates. Size: 3xn; Last row should be 1.0
	std::vector<cv::Mat> movObjCorrsImg1TP, movObjCorrsImg2TP;//Every vector element (size corresponds to number of newly to add moving objects) holds correspondences within a new moving object. Every vector element: Size: 3xn; Last row should be 1.0; Both Mat (same vector index) must have the same size.
	std::vector<cv::Mat> movObjCorrsImg1TN;//Every vector element (size corresponds to number of newly to add moving objects) holds TN correspondences within a new moving object. Every vector element: Newly created TN keypoint in the first stereo rig image (no 3D points were created for them)
	std::vector<cv::Mat> movObjCorrsImg2TN;//Every vector element (size corresponds to number of newly to add moving objects) holds TN correspondences within a new moving object. Every vector element: Newly created TN keypoint in the second stereo rig image (no 3D points were created for them)
	std::vector<std::vector<double>> movObjDistTNtoReal;//Distance values of the TN keypoint locations for the already existing moving objects in the 2nd image to the location that would be a perfect correspondence to the TN in image 1. If the value is >= 50, the "perfect location" would be outside the image
	std::vector<std::vector<double>> movObjDistTNtoRealNew;//Distance values of the TN keypoint locations for new generated moving objects in the 2nd image to the location that would be a perfect correspondence to the TN in image 1. If the value is >= 50, the "perfect location" would be outside the image

	cv::Mat combCorrsImg1TP, combCorrsImg2TP;//Combined TP correspondences (static and moving objects). Size: 3xn; Last row should be 1.0; Both Mat must have the same size.
	std::vector<cv::Point3d> comb3DPts;//Combined 3D points corresponding to matches combCorrsImg1TP and combCorrsImg2TP
	cv::Mat combCorrsImg1TN, combCorrsImg2TN;//Combined TN correspondences (static and moving objects). Size: 3xn; Last row should be 1.0; Both Mat must have the same size.
	int combNrCorrsTP, combNrCorrsTN;//Number of overall TP and TN correspondences (static and moving objects)
	std::vector<double> combDistTNtoReal;//Distance values of all (static and moving objects) TN keypoint locations in the 2nd image to the location that would be a perfect correspondence to the TN in image 1. If the value is >= 50, the "perfect location" would be outside the image
};


/* --------------------- Function prototypes --------------------- */

template<typename T, typename A, typename T1, typename A1>
void deleteVecEntriesbyIdx(std::vector<T, A> &editVec, std::vector<T1, A1> const& delVec);

template<typename T, typename A>
void deleteMatEntriesByIdx(cv::Mat &editMat, std::vector<T, A> const& delVec, bool rowOrder);

/*Rounds a rotation matrix to its nearest integer values and checks if it is still a rotation matrix and does not change more than 22.5deg from the original rotation matrix.
As an option, the error of the rounded rotation matrix can be compared to an angular difference of a second given rotation matrix R_fixed to R_old.
The rotation matrix with the smaller angular difference is selected.
This function is used to select a proper rotation matrix if the "look at" and "up vector" are nearly equal. I trys to find the nearest rotation matrix aligened to the
"look at" vector taking into account the rotation matrix calculated from the old/last "look at" vector
*/
bool roundR(const cv::Mat R_old, cv::Mat & R_round, cv::InputArray R_fixed = cv::noArray());

/* -------------------------- Functions -------------------------- */