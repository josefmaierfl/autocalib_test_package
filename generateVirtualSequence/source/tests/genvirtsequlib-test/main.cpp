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

#include "argvparser.h"

#include "io_data.h"

#include "getStereoCameraExtr.h"
#include "generateSequence.h"
#include "generateMatches.h"
#include "helper_funcs.h"
#include <time.h>
#include <chrono>
#include <iomanip>

using namespace std;
using namespace cv;
using namespace CommandLineProcessing;

double getRandDoubleVal(std::default_random_engine rand_generator, double lowerBound, double upperBound);
void initStarVal(default_random_engine rand_generator, double *range, vector<double>& startVec);
void initRangeVals(default_random_engine rand_generator,
		const double *range,
		const double *relMinMaxCh,
		const vector<double> &startVec,
		vector<double>& actVec);
int startEvaluation(ArgvParser& cmd);
int testStereoCamGeneration(int verbose = 0,
							bool genSequence = true,
							bool genMatches = false,
                            bool use_kitti = false,
                            bool use_oxford = false,
                            bool use_mega = false,
							std::string *mainStorePath = nullptr,
							std::string *imgPath = nullptr,
							std::string *imgPrePostFix = nullptr,
							std::string *sequLoadFolder = nullptr,
							bool rwXMLinfo = false,
							bool compressedWrittenInfo = false);
int genNewSequence(std::vector<cv::Mat>& Rv,
				   std::vector<cv::Mat>& tv,
				   cv::Mat& K_1,
				   cv::Mat& K_2,
				   cv::Size &imgSize,
				   bool genMatches = false,
                   bool use_kitti = false,
                   bool use_oxford = false,
                   bool use_mega = false,
				   std::string *mainStorePath = nullptr,
				   std::string *imgPath = nullptr,
				   std::string *imgPrePostFix = nullptr,
				   std::string *sequLoadFolder = nullptr,
				   bool rwXMLinfo = false,
				   bool compressedWrittenInfo = false);
int genNewMatches(std::vector<cv::Mat>& Rv,
				  std::vector<cv::Mat>& tv,
				  cv::Mat& K_1,
				  cv::Mat& K_2,
				  cv::Size &imgSize,
				  StereoSequParameters &stereoSequPars,
				  const std::string &mainStorePath,
				  const std::string &imgPath,
				  const std::string &imgPrePostFix,
				  const std::string &sequLoadFolder,
				  bool rwXMLinfo = false,
				  bool compressedWrittenInfo = false,
                  bool use_kitti = false,
                  bool use_oxford = false,
                  bool use_mega = false,
				  uint32_t verbose = 0);

depthClass getRandDepthClass();
std::string getKeyPointType(int idx);
std::string getDescriptorType(int idx);
bool checkKpDescrCompability(const std::string &keypointType, const std::string &descriptorType);

void SetupCommandlineParser(ArgvParser& cmd, int argc, char* argv[])
{
	cmd.setIntroductoryDescription("Test interface for generating randomized 3D scenes and matches");
	//define error codes
	cmd.addErrorCode(0, "Success");
	cmd.addErrorCode(1, "Error");

	cmd.setHelpOption("h", "help","If no option is specified, only the generation of 3D scenes without matches is tested and nothing is stored to disk.");
	cmd.defineOption("img_path", "<Path to the images where the features should be extracted from (all required in one folder)>", ArgvParser::OptionRequiresValue);
	cmd.defineOption("img_pref", "<Prefix and/or postfix for the used images.\n "
								 "It can include a folder structure that follows after the filepath, a file prefix, a '*' indicating the position of the number and a postfix. "
								 "If it is empty, all files from the folder img_path are used (also if img_pref only contains a folder ending with '/', every file within this folder is used). "
								 "It is possible to specify only a prefix with or without '*' at the end. "
								 "If a prefix is used, all characters until the first number (excluding) must be provided. "
								 "For a postfix, '*' must be placed before the postfix.\n "
								 "Valid examples : folder/pre_*post, *post, pre_*, pre_, folder/*post, folder/pre_*, folder/pre_, folder/, folder/folder/, folder/folder/pre_*post, ...>", ArgvParser::OptionRequiresValue);
	cmd.defineOption("store_path", "<Path for storing the generated 3D scenes and matches>", ArgvParser::OptionRequiresValue);
	cmd.defineOption("load_folder", "<Path for loading an existing 3D scene for generating new matches. load_type must also be specified.>", ArgvParser::OptionRequiresValue);
	cmd.defineOption("load_type", "<File type of the stored 3D scene that must be provided if it is loaded. Options: "
					 "\n 0\t YAML without compression"
					 "\n 1\t YAML with compression (.yaml.gz)"
					 "\n 2\t XML without compression"
					 "\n 3\t XML with compression (.xml.gz)",
			ArgvParser::OptionRequiresValue);
    cmd.defineOption("kitti", "<If provided, GTM from KITTI are included.>", ArgvParser::NoOptionAttribute);
    cmd.defineOption("oxford", "<If provided, GTM from Oxford are included.>", ArgvParser::NoOptionAttribute);
    cmd.defineOption("mega", "<If provided, GTM from MegaDepth are included.>", ArgvParser::NoOptionAttribute);
	
	/// finally parse and handle return codes (display help etc...)
	int result = -1;
	result = cmd.parse(argc, argv);
	if (result != ArgvParser::NoParserError)
	{
		cout << cmd.parseErrorDescription(result);
		exit(1);
	}
}

int startEvaluation(ArgvParser& cmd)
{
	bool option_found = false;
	option_found = cmd.foundOption("img_path") & cmd.foundOption("img_pref") & cmd.foundOption("store_path");
    bool use_kitti = cmd.foundOption("kitti");
    bool use_oxf = cmd.foundOption("oxford");
    bool use_mega = cmd.foundOption("mega");
	int err = 0;
	if(!option_found){
		cout << "Did not find necessary options to test generation of matches. Performing only tests for generating 3D sequences." << endl;
		err = testStereoCamGeneration(0);
		if(err){
			return -1;
		}
	}
	else{
		string img_path, store_path, img_pref, load_folder, load_type;
		img_path = cmd.optionValue("img_path");
		img_pref = cmd.optionValue("img_pref");
		store_path = cmd.optionValue("store_path");
		if(cmd.foundOption("load_folder")){
			if(!cmd.foundOption("load_type")){
				cerr << "Unable to load 3D scene as the file type is missing (parameter load_type)." << endl;
				return -1;
			}
			load_folder = cmd.optionValue("load_folder");
			load_type = cmd.optionValue("load_type");
			int load_type_val = stoi(load_type);
			bool rwXMLinfo = false;
			bool compressedWrittenInfo = false;
			switch(load_type_val){
				case 0:
					rwXMLinfo = false;
					compressedWrittenInfo = false;
					break;
				case 1:
					rwXMLinfo = false;
					compressedWrittenInfo = true;
					break;
				case 2:
					rwXMLinfo = true;
					compressedWrittenInfo = false;
					break;
				case 3:
					rwXMLinfo = true;
					compressedWrittenInfo = true;
					break;
				default:
					cerr << "Wrong parameter value for the file type." << endl;
					return -1;
			}
            err = testStereoCamGeneration(0,
                                          true,
                                          true,
                                          use_kitti,
                                          use_oxf,
                                          use_mega,
                                          &store_path,
                                          &img_path,
                                          &img_pref,
                                          &load_folder,
                                          rwXMLinfo,
                                          compressedWrittenInfo);
		}else {
            err = testStereoCamGeneration(0,
                                          true,
                                          true,
                                          use_kitti,
                                          use_oxf,
                                          use_mega,
                                          &store_path,
                                          &img_path,
                                          &img_pref);
		}
		if(err){
			return -1;
		}
	}

	return 0;
}

/** @function main */
int main( int argc, char* argv[])
{
	ArgvParser cmd;
	SetupCommandlineParser(cmd, argc, argv);
	int err = startEvaluation(cmd);
	if(err){
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int testStereoCamGeneration(int verbose,
							bool genSequence,
							bool genMatches,
                            bool use_kitti,
							bool use_oxford,
							bool use_mega,
							std::string *mainStorePath,
							std::string *imgPath,
							std::string *imgPrePostFix,
							std::string *sequLoadFolder,
							bool rwXMLinfo,
							bool compressedWrittenInfo)
{
	srand(time(NULL));
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine rand_generator(seed);
    std::mt19937 rand2(seed);

	const bool testGenericAlignment = false;

	//Parameter ranges for tx and ty are down in the for-loop
	double roll_minmax[2] = { -25.0, 25.0 };
	double pitch_minmax[2] = { -25.0, 25.0 };
	double yaw_minmax[2] = { -20.0, 20.0 };
	double tx_minmax_change[2] = { 0.7, 1.3 };
	double ty_minmax_change[2] = { 0.7, 1.3 };
	double tz_minmax_change[2] = { 0.7, 1.3 };
	double roll_minmax_change[2] = { 0.7, 1.3 };
	double pitch_minmax_change[2] = { 0.7, 1.3 };
	double yaw_minmax_change[2] = { 0.7, 1.3 };
	double txy_relmax = 0.7;//0.25;// 0.7; //must be between 0 and 1
	double tz_relxymax = 0.5;//0.1;// 0.5; //should be between 0 and 1
	double hlp = std::max(tx_minmax_change[1] * txy_relmax, ty_minmax_change[1] * txy_relmax);
	txy_relmax = hlp >= 1.0 ? (0.99 / std::max(tx_minmax_change[1], ty_minmax_change[1])) : txy_relmax;

	CV_Assert(txy_relmax < 1.0);

	for (size_t j = 0; j < 1000; j++) //Number of performed test runs
	{
		double tx_minmax[2] = { 0.1, 5.0 };
		double ty_minmax[2] = { 0, 5.0 };

		int nrCams = (int)(rand2() % 10) + 1;//1 //Number of different stereo camera configurations (extrinsic) per test

		//Calculate, for which extrinsic (tx, ty, tz, roll, ...) its value or range should remain the same for all nrCams different stereo camera configuratuions
		int roll_equRanges = (int)((rand2() % 2) & (rand2() % 2));
		int pitch_equRanges = (int)((rand2() % 2) & (rand2() % 2));
		int yaw_equRanges = (int)((rand2() % 2) & (rand2() % 2));
		int tx_equRanges = (int)((rand2() % 2) & (rand2() % 2));
		int ty_equRanges = (int)((rand2() % 2) & (rand2() % 2));
		int tz_equRanges = (int)((rand2() % 2) & (rand2() % 2));

		vector<double> txi_start, tyi_start, tzi_start, rolli_start, pitchi_start, yawi_start;
		if (!testGenericAlignment)//If restrictions apply for the relative distances between cameras in x and y in a specific camera alignment (horizontal or vertical)
		{
			int align = (int)(rand2() % 2);//0 //Choose either vertical or horizontal camera alignment
			if (align)//Vertical camera alignment
			{
				initStarVal(rand_generator, ty_minmax, tyi_start);//Generate a random value or range (chosen ramdomly) between ranges for the distance in y
				tx_minmax[0] = -txy_relmax * tyi_start.back();//Calculate the range of the x-distance based on the maximum y-distance
				tx_minmax[1] = txy_relmax * tyi_start.back();
				double tmp = -ty_minmax[0];//Negate to make sure that the reference camera is the top camera (y-component of translation vector must be negative)
				ty_minmax[0] = -ty_minmax[1];
				ty_minmax[1] = tmp;
				initStarVal(rand_generator, tx_minmax, txi_start);//Generate a random value or range (chosen ramdomly) between ranges (smaller or equal than for y) for the distance in x
				if (tyi_start.size() == 1)
				{
					tyi_start[0] = -tyi_start[0];
				}
				else
				{
					tmp = -tyi_start[0];
					tyi_start[0] = -tyi_start[1];
					tyi_start[1] = tmp;
				}
			}
			else //Horizontal camera alignment
			{
				initStarVal(rand_generator, tx_minmax, txi_start);//Generate a random value or range (chosen ramdomly) between ranges for the distance in x
				//txi_start.push_back(0.9);//REMOVE

				ty_minmax[0] = -txy_relmax * txi_start.back();//Calculate the range of the y-distance based on the maximum x-distance
				ty_minmax[1] = txy_relmax * txi_start.back();
				double tmp = -tx_minmax[0];//Negate to make sure that the reference camera is the left camera (x-component of translation vector must be negative)
				tx_minmax[0] = -tx_minmax[1];
				tx_minmax[1] = tmp;
				initStarVal(rand_generator, ty_minmax, tyi_start);//Generate a random value or range (chosen ramdomly) between ranges (smaller or equal than for x) for the distance in y
				//tyi_start.push_back(-0.5);//REMOVE
				//tyi_start.push_back(0.5);//REMOVE

				if (txi_start.size() == 1)
				{
					txi_start[0] = -txi_start[0];
				}
				else
				{
					tmp = -txi_start[0];
					txi_start[0] = -txi_start[1];
					txi_start[1] = tmp;
				}
			}
		}
		else //Complete random alignment (x- and y-direction of translation vector) of the stereo cameras without any restrictions about relations between x and y translation
		{
			tx_minmax[0] = -tx_minmax[1];
			ty_minmax[0] = -ty_minmax[1];
			initStarVal(rand_generator, tx_minmax, txi_start);
			initStarVal(rand_generator, ty_minmax, tyi_start);
		}

		//Calculate maximum z-distance between cameras based on maximum x- and y-distance (z-distance must be smaller)
		double tzr = tz_relxymax * min(std::max(abs(*max_element(txi_start.begin(), txi_start.end())),
                                            abs(*min_element(txi_start.begin(), txi_start.end()))),
                                       std::max(abs(*max_element(tyi_start.begin(), tyi_start.end())),
                                                abs(*min_element(tyi_start.begin(), tyi_start.end()))));
		double tz_minmax[2];
		tz_minmax[0] = -tzr;
		tz_minmax[1] = tzr;
		initStarVal(rand_generator, tz_minmax, tzi_start); //Calculate value or range (chosen ramdomly) for the z-distance

		initStarVal(rand_generator, roll_minmax, rolli_start); //Calculate value or range (chosen ramdomly) for the roll
		initStarVal(rand_generator, pitch_minmax, pitchi_start); //Calculate value or range (chosen ramdomly) for the pitch
		initStarVal(rand_generator, yaw_minmax, yawi_start); //Calculate value or range (chosen ramdomly) for the yaw

		//tzi_start.push_back(0.02);//REMOVE
		//rolli_start.push_back(-15.0);//REMOVE
		//rolli_start.push_back(15.0);//REMOVE
		//pitchi_start.push_back(-15.0);//REMOVE
		//pitchi_start.push_back(15.0);//REMOVE
		//yawi_start.push_back(-0.2);//REMOVE

		std::vector<std::vector<double>> tx;
		std::vector<std::vector<double>> ty;
		std::vector<std::vector<double>> tz;
		std::vector<std::vector<double>> roll;
		std::vector<std::vector<double>> pitch;
		std::vector<std::vector<double>> yaw;

		//Store the initial values or ranges of the extrinsics in the vector that holds the configurations of every of the nrCams stereo configurations
		tx.push_back(txi_start);
		ty.push_back(tyi_start);
		tz.push_back(tzi_start);
		roll.push_back(rolli_start);
		pitch.push_back(pitchi_start);
		yaw.push_back(yawi_start);

		//Generate the remaining ranges (or values) of extrinsics for the other stereo camera configurations
		//The other ranges must be within the maximum specified ranges and are only allowed to deviate from the initial calculated ranges (..._start) by a relative value (..._minmax_change)
		for (int i = 1; i < nrCams; i++)
		{
			vector<double> txi, tyi, tzi, rolli, pitchi, yawi;
			size_t wrongsignCnt = 0;
			bool cngTxy = false;
			while (txi.empty() || ((wrongsignCnt < 100) && cngTxy))
			{
				if (tx_equRanges)//If true, the tx value or range stays the same for all stereo camera configurations
					txi = txi_start;
				else
					initRangeVals(rand_generator, tx_minmax, tx_minmax_change, txi_start, txi);//Calculate the new range or value based on the first range (within maximum specified range tx_minmax)
				if (ty_equRanges)//If true, the ty value or range stays the same for all stereo camera configurations
					tyi = tyi_start;
				else
					initRangeVals(rand_generator, ty_minmax, ty_minmax_change, tyi_start, tyi);//Calculate the new range or value based on the first range (within maximum specified range ty_minmax)

				//Check if the largest absolute x- or y-value (from x-,y-values and/or ranges) is negative to ensure that the reference camera is the top or left camera
				//This is necessary, as all optimization algorithms (im LM) are based on this assumption
				double maxX, maxY;
				if (txi.size() > 1)
				{
					if (((txi[0] >= 0) && (txi[1] > 0)) ||
						(abs(txi[1]) > abs(txi[0])))
					{
						maxX = txi[1];
					}
					else
					{
						maxX = txi[0];
					}
				}
				else
				{
					maxX = txi[0];
				}
				if (tyi.size() > 1)
				{
					if (((tyi[0] >= 0) && (tyi[1] > 0)) ||
						(abs(tyi[1]) > abs(tyi[0])))
					{
						maxY = tyi[1];
					}
					else
					{
						maxY = tyi[0];
					}
				}
				else
				{
					maxY = tyi[0];
				}

				if (((abs(maxX) >= abs(maxY)) && (maxX > 0)) ||
					((abs(maxY) >= abs(maxX)) && (maxY > 0)))
				{
					cngTxy = true;//Try again if the reference camera is not the top or left camera
					txi.clear();
					tyi.clear();
				}
				else
				{
					cngTxy = false;
				}
				wrongsignCnt++;
			}
			if (cngTxy)//Abort if no valid extrinsic values or ranges were created for this stereo configuration (reference camera must be top or left camera)
				break;

			if (tz_equRanges) {//If true, the tz value or range stays the same for all stereo camera configurations
                double maxTzi = max(abs(*max_element(tzi_start.begin(), tzi_start.end())),
                        abs(*min_element(tzi_start.begin(), tzi_start.end())));
                double maxTxi = max(abs(*max_element(txi.begin(), txi.end())),
                                    abs(*min_element(txi.begin(), txi.end())));
                double maxTyi = max(abs(*max_element(tyi.begin(), tyi.end())),
                                    abs(*min_element(tyi.begin(), tyi.end())));
                double maxTxyi = max(maxTxi, maxTyi);
                if(maxTzi >= maxTxyi){
                    if(tx_equRanges && ty_equRanges){
                        return -1;
                    }
                    i--;
                    continue;
                }
			    tzi = tzi_start;
            }
			else{
                //Calculate the new range or value based on the first range (within maximum specified range tz_minmax)
                int tryCnt = 20;
                double maxTzi = 0;
                double maxTxyi = 0;
                do {
                    tzi.clear();
                    initRangeVals(rand_generator, tz_minmax, tz_minmax_change, tzi_start, tzi);
                    maxTzi = max(abs(*max_element(tzi.begin(), tzi.end())),
                                        abs(*min_element(tzi.begin(), tzi.end())));
                    double maxTxi = max(abs(*max_element(txi.begin(), txi.end())),
                                        abs(*min_element(txi.begin(), txi.end())));
                    double maxTyi = max(abs(*max_element(tyi.begin(), tyi.end())),
                                        abs(*min_element(tyi.begin(), tyi.end())));
                    maxTxyi = max(maxTxi, maxTyi);
                    tryCnt--;
                }while((tryCnt > 0) && (maxTzi >= maxTxyi));
                if(tryCnt == 0){
                    i--;
                    continue;
                }
			}
			if (roll_equRanges)//If true, the roll value or range stays the same for all stereo camera configurations
				rolli = rolli_start;
			else
				initRangeVals(rand_generator, roll_minmax, roll_minmax_change, rolli_start, rolli);//Calculate the new range or value based on the first range (within maximum specified range roll_minmax)
			if (pitch_equRanges) //If true, the pitch value or range stays the same for all stereo camera configurations
				pitchi = pitchi_start;
			else
				initRangeVals(rand_generator, pitch_minmax, pitch_minmax_change, pitchi_start, pitchi);//Calculate the new range or value based on the first range (within maximum specified range pitch_minmax)
			if (yaw_equRanges) //If true, the yaw value or range stays the same for all stereo camera configurations
				yawi = yawi_start;
			else
				initRangeVals(rand_generator, yaw_minmax, yaw_minmax_change, yawi_start, yawi);//Calculate the new range or value based on the first range (within maximum specified range yaw_minmax)

			//Add the new camera configuration to the vector that should hold all of them in the end
			tx.push_back(txi);
			ty.push_back(tyi);
			tz.push_back(tzi);
			roll.push_back(rolli);
			pitch.push_back(pitchi);
			yaw.push_back(yawi);
		}

		//If for one or more stereo configurations no valid configuration was found (reference camera must be top or left camera), skip this test
		if ((int)tx.size() < nrCams)
			continue;

		//Generate a random target image overlap between the stereo cameras that sould be present after LM optimization of the extrinsic parameters between the given ranges (that were generated above)
		double approxImgOverlap = getRandDoubleVal(rand_generator, 0.2, 1.0);//0.1;
		cv::Size imgSize = cv::Size(1280, 720);//Select an image size (can be arbitrary)

		//Calculate the extrinsic parameters for all given stereo configurations within the given ranges to achieve an image overlap nearest to the above generated value (in addition to a few other constraints)
		GenStereoPars newStereoPars(tx, ty, tz, roll, pitch, yaw, approxImgOverlap, imgSize, 0);
		int err = newStereoPars.optimizeRtf(verbose);

		//Write the results to std out
		vector<cv::Mat> t_new;
		Mat K;
		vector<double> roll_new, pitch_new, yaw_new;
		newStereoPars.getEulerAngles(roll_new, pitch_new, yaw_new);
		t_new = newStereoPars.tis;
		K = newStereoPars.K1;
		cout << endl;
		cout << "**************************************************************************" << endl;
		cout << "User specified image overlap = " << std::setprecision(3) << approxImgOverlap << endl;
		cout << "f= " << std::setprecision(2) << K.at<double>(0, 0) << " cx= " << K.at<double>(0, 2) << " cy= " << K.at<double>(1, 2) << endl;
		for (size_t i = 0; i < roll_new.size(); i++)
		{
			cout << "tx= " << std::setprecision(6) << t_new[i].at<double>(0) << " ty= " << t_new[i].at<double>(1) << " tz= " << t_new[i].at<double>(2) << endl;
			cout << "roll= " << std::setprecision(3) << roll_new[i] << " pitch= " << pitch_new[i] << " yaw= " << yaw_new[i] << endl;
		}
		cout << "**************************************************************************" << endl << endl;

		//Generate a trajectory/image sequence using the above calculated stereo camera configurations
		if ((err == 0) && genSequence)
		{
			std::vector<cv::Mat> Rv, tv;
			cv::Mat K_1, K_2;
			if (newStereoPars.getCamPars(Rv, tv, K_1, K_2))
			{
				for (size_t i = 0; i < 10; i++)
				{
					err = genNewSequence(Rv,
										 tv,
										 K_1,
										 K_2,
										 imgSize,
										 genMatches,
                                         use_kitti,
                                         use_oxford,
                                         use_mega,
										 mainStorePath,
										 imgPath,
										 imgPrePostFix,
										 sequLoadFolder,
										 rwXMLinfo,
										 compressedWrittenInfo);
					if(err == -1){
						/*cerr << "Existing." << endl;
						return err;*/
						break;
					}
				}
			}
		}
	}

	return 0;
}

int genNewSequence(std::vector<cv::Mat>& Rv,
				   std::vector<cv::Mat>& tv,
				   cv::Mat& K_1,
				   cv::Mat& K_2,
				   cv::Size &imgSize,
				   bool genMatches,
                   bool use_kitti,
                   bool use_oxford,
                   bool use_mega,
				   std::string *mainStorePath,
				   std::string *imgPath,
				   std::string *imgPrePostFix,
				   std::string *sequLoadFolder,
				   bool rwXMLinfo,
				   bool compressedWrittenInfo)
{
	StereoSequParameters stereoSequPars;
	if(!sequLoadFolder) {
		auto seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::default_random_engine rand_generator(seed);
		std::mt19937 rand2(seed);

		const int maxTrackElements = 100;
		double closedLoopMaxXYRatioRange[2] = {1.0,
											   3.0}; //Multiplier range that specifies how much larger the expension of a track in x-direction can be compared to y (y = x / value from this range).
		double closedLoopMaxXZRatioRange[2] = {0.1,
											   1.0}; //Multiplier range that specifies how much smaller/larger the expension of a track in z-direction can be compared to x.
		double closedLoopMaxElevationAngleRange[2] = {0, 3.14 /
														 16.0}; //Only for closed loop. Angle range for the z-component (y in the camera coordinate system) of the ellipsoide (must be in the range -pi/2 <= angle <= pi/2). For a fixed angle, it defines the ellipse on the ellipsoide with a fixed value of z (y in the camera coordinate system).
		const bool enableFlightMode = true; //Only for closed loop. If enabled, the elevation angle teta of the ellipsoide is continuously changed within the range closedLoopMaxElevationAngleRange to get different height values (z-component of ellipsoide (y in the camera coordinate system)) along the track

		size_t nFramesPerCamConf = 1 + (size_t) (rand2() %
												 20);//5;//Number of consecutive frames on a track with the same stereo configuration
		if ((Rv.size() == 1) && (nFramesPerCamConf == 1)) {
			nFramesPerCamConf++;
		}
        int nTotalNrFrames = (int)(tv.size() * nFramesPerCamConf);
        if((rand2() % 2) == 0){
            nTotalNrFrames -= (int)(rand2() % nFramesPerCamConf);
        }else if((rand2() % 7) == 0){
            nTotalNrFrames += (int)pow(-1.0, (double)(rand2() % 2))
                              * min(tv.size() - 1, static_cast<size_t>(2))
                              * (int)nFramesPerCamConf;
        }

		double minInlierRange[2] = {0.1, 0.5};//Range of the minimum inlier ratio
		double maxInlierRange[2] = {0.55, 1.0};//Range of the maximum inlier ratio bounded by minInlierRange

		size_t minNrInlierRange[2] = {1, 50};//Range of the minimum number of true positive correspondences
		size_t maxNrInlierRange[2] = {100, 10000};//Range of the maximum number of true positive correspondences

		double minKeypDistRange[2] = {1.0, 10.0};//Range of the minimum distance between keypoints

		size_t MaxNrDepthAreasPReg = 50;//Maximum number of depth areas per image region

		double relCamVelocityRange[2] = {0.1, 5.0};//Relative camera velocity compared to the basline length

		double rollRange[2] = {-3.0,
							   3.0};//Roll angle (x-axis) for the first camera centre. This rotation can change the camera orientation for which without rotation the z - component of the relative movement vector coincides with the principal axis of the camera
		double pitchRange[2] = {-10.0,
								10.0};//Pitch angle (y-axis) for the first camera centre. This rotation can change the camera orientation for which without rotation the z - component of the relative movement vector coincides with the principal axis of the camera
		double yawRange[2] = {-2.0,
							  2.0};//Yaw angle (z-axis) for the first camera centre. This rotation can change the camera orientation for which without rotation the z - component of the relative movement vector coincides with the principal axis of the camera

		double minRelAreaRangeMovObjsRange[2] = {0,
												 0.1};//Range of the lower border of the relative area range of moving objects. Minimum area range of moving objects relative to the image area at the beginning (when a moving object is first introduced).
		double maxRelAreaRangeMovObjsRange[2] = {0.15,
												 0.99};//Range of the upper border of the relative area range of moving objects. Maximum area range of moving objects relative to the image area at the beginning (when a moving object is first introduced).

		double minRelVelocityMovObj = 0.1;//Minimum relative object velocity compared to camera velocity
		double maxRelVelocityMovObj = 20.0;//Maximum relative object velocity compared to camera velocity

		double minPortionMovObj = 0.1;//Minimal portion of used correspondences by moving objects
		double maxPortionMovObj = 0.85;//Maximal portion of used correspondences by moving objects

		double minCamMatDistortion = 0;//Minimal percentage for distorting the camera matrices
		double maxCamMatDistortion = 0.6;//Maximal percentage for distorting the camera matrices

		//Generate a random camera track
		int closedLoop = (int) (rand2() % 2);
		int staticCamMovement = (int) (rand2() % 2);
		int nrTrackElements = (int) (rand2() % static_cast<size_t>(maxTrackElements)) + 1;
		if (nrTrackElements == 1)
			staticCamMovement = 1;
		std::vector<cv::Mat> camTrack;
		if ((closedLoop == 0) || (staticCamMovement == 1)) {
			double xy, xz;
			Mat tr1 = Mat(3, 1, CV_64FC1);
			//Generate a random camera movement vector based on the ranges given. The scaling is not important. The scaling and track segments for every image pair are caluclated within the class genStereoSequ
			tr1.at<double>(0) = 1.0;
			xy = getRandDoubleVal(rand_generator, closedLoopMaxXYRatioRange[0], closedLoopMaxXYRatioRange[1]);
			xz = getRandDoubleVal(rand_generator, closedLoopMaxXZRatioRange[0], closedLoopMaxXZRatioRange[1]);
			tr1.at<double>(1) = 1.0 / xy;
			tr1.at<double>(2) = xz;
			camTrack.push_back(tr1.clone());
			if (staticCamMovement !=
				1)//Add more track segments (ignoring scale of the whole track). Otherwise, the first generated vector defines the not changing direction of the track.
			{
				std::normal_distribution<double> distributionNX2(1.0, 0.25);
				for (int i = 0; i < nrTrackElements; i++) {
					xy = getRandDoubleVal(rand_generator, closedLoopMaxXYRatioRange[0], closedLoopMaxXYRatioRange[1]);
					xz = getRandDoubleVal(rand_generator, closedLoopMaxXZRatioRange[0], closedLoopMaxXZRatioRange[1]);
					tr1.at<double>(0) *= distributionNX2(rand_generator);//New x-direction depends on old x-direction
					tr1.at<double>(1) =
							(2.0 * (tr1.at<double>(1) * distributionNX2(rand_generator)) + tr1.at<double>(0) / xy) /
							3.0;//New y-direction mainly depends on old y-direction but also on a random part which depends on the x-direction
					tr1.at<double>(2) =
							(2.0 * (tr1.at<double>(2) * distributionNX2(rand_generator)) + tr1.at<double>(0) * xz) /
							3.0;//New z-direction mainly depends on old z-direction but also on a random part which depends on the x-direction
					camTrack.push_back(camTrack.back() + tr1);
				}
			}
		} else //Track with loop closure (last track position is near the first but not the same)
		{
			//Take an ellipsoide with random parameters
			double b, a = 1.0, c;//, ab, ac;
			b = a / getRandDoubleVal(rand_generator, closedLoopMaxXYRatioRange[0], closedLoopMaxXYRatioRange[1]);
			c = a * getRandDoubleVal(rand_generator, closedLoopMaxXZRatioRange[0], closedLoopMaxXZRatioRange[1]);
			double tmp1 = getRandDoubleVal(rand_generator, closedLoopMaxElevationAngleRange[0],
										   closedLoopMaxElevationAngleRange[1]);
			double tmp2 = getRandDoubleVal(rand_generator, closedLoopMaxElevationAngleRange[0],
										   closedLoopMaxElevationAngleRange[1]);
			double theta = min(tmp1, tmp2);
			double theta2 = max(tmp1, tmp2);
			double thetaPiece = (theta2 - theta) / (double) (nrTrackElements -
															 1);//Variation in height from one track element to the next based on the angle theta. Only used in enableFlightMode.
			double phiPiece = (2.0 * M_PI - M_PI / (double) (10 * maxTrackElements)) / (double) (nrTrackElements -
																								 1);//The track is nearly a loop closure (phi from 0 to nearly 2 pi)
			//camTrack.push_back(Mat::zeros(3, 1, CV_64FC1));
			double phi = 0;// phiPiece;
			for (int i = 0; i < nrTrackElements; i++) {
				Mat tr1 = Mat(3, 1, CV_64FC1);
				tr1.at<double>(0) = a * cos(theta) * cos(phi);
				tr1.at<double>(2) = b * cos(theta) * sin(phi);
				tr1.at<double>(1) = c * sin(theta);
				phi += phiPiece;
				if (enableFlightMode)
					theta += thetaPiece;
				camTrack.push_back(tr1.clone());
			}
		}

		int onlyOneInlierRat = (int) (rand2() % 2);
		//Inlier ratio range
		std::pair<double, double> inlRatRange;
		if (onlyOneInlierRat == 0)//Generate a random inlier ratio range within a given range for the image pairs
		{
			inlRatRange = std::make_pair(getRandDoubleVal(rand_generator, minInlierRange[0], minInlierRange[1]), 1.0);
			inlRatRange.second = getRandDoubleVal(rand_generator, max(inlRatRange.first + 0.01, maxInlierRange[0]),
												  max(min(inlRatRange.first + 0.02, 1.0), maxInlierRange[1]));
		} else//Only 1 fixed inlier ratio over all image pairs
		{
			inlRatRange = std::make_pair(getRandDoubleVal(rand_generator, minInlierRange[0], maxInlierRange[1]), 1.0);
			inlRatRange.second = inlRatRange.first;
		}

		int inlChangeRate = (int) (rand2() % 3);
		//Inlier ratio change rate from pair to pair. If 0, the inlier ratio within the given range is always the same for every image pair.
		//If 100, the inlier ratio is chosen completely random within the given range.
		//For values between 0 and 100, the inlier ratio selected is not allowed to change more than this factor (not percentage) from the last inlier ratio.
		double inlRatChanges = 0;
		if (inlChangeRate == 1) {
			inlRatChanges = getRandDoubleVal(rand_generator, 0, 100.0);
		} else if (inlChangeRate == 2) {
			inlRatChanges = 100.0;
		}

		int onlyOneTPNr = (int) (rand2() % 2);
		//# true positives range
		std::pair<size_t, size_t> truePosRange;
		if (onlyOneTPNr == 0) {
			std::uniform_int_distribution<size_t> distribution1(minNrInlierRange[0], minNrInlierRange[1]);
			truePosRange = std::make_pair(distribution1(rand_generator), 1);
			std::uniform_int_distribution<size_t> distribution2(max(truePosRange.first, maxNrInlierRange[0]),
																max(truePosRange.first + 1, maxNrInlierRange[1]));
			truePosRange.second = distribution2(rand_generator);
		} else {
			std::uniform_int_distribution<size_t> distribution1(minNrInlierRange[0], maxNrInlierRange[1]);
			truePosRange = std::make_pair(distribution1(rand_generator), 1);
			truePosRange.second = truePosRange.first;
		}

		int tpChangeRate = (int) (rand2() % 3);
		//True positives change rate from pair to pair. If 0, the true positives within the given range are always the same for every image pair.
		//If 100, the true positives are chosen completely random within the given range.
		//For values between 0 and 100, the true positives selected are not allowed to change more than this factor from the true positives.
		double truePosChanges = 0;
		if (tpChangeRate == 1) {
			truePosChanges = getRandDoubleVal(rand_generator, 0, 100.0);
		} else if (tpChangeRate == 2) {
			truePosChanges = 100.0;
		}

		//min. distance between keypoints
		double minKeypDist = getRandDoubleVal(rand_generator, minKeypDistRange[0], minKeypDistRange[1]);

		//portion of correspondences at depths
		depthPortion corrsPerDepth = depthPortion(getRandDoubleVal(rand_generator, 0, 1.0),
												  getRandDoubleVal(rand_generator, 0, 1.0),
												  getRandDoubleVal(rand_generator, 0, 1.0));

		int useMultcorrsPerRegion = (int) (rand2() % 3);
		//List of portions of image correspondences at regions (Matrix must be 3x3). This is the desired distribution of features in the image. Maybe doesnt hold: Also depends on 3D-points from prior frames.
		std::vector<cv::Mat> corrsPerRegion;
		//If useMultcorrsPerRegion==0, the portions are randomly initialized. There are as many matrices generated as the number of image pairs divided by corrsPerRegRepRate
		if (useMultcorrsPerRegion == 1)//The same correspondence distribution over the image is used for all image pairs
		{
			cv::Mat corrsPReg(3, 3, CV_64FC1);
			cv::randu(corrsPReg, Scalar(0), Scalar(1.0));
			corrsPReg /= sum(corrsPReg)[0];
			corrsPerRegion.push_back(corrsPReg.clone());
		} else if (useMultcorrsPerRegion ==
				   2)//Generates a random number (between 1 and the maximum number of image pairs) of different correspondence distributions
		{
			int nr_elems = (int) (rand2() % (nFramesPerCamConf * Rv.size() - 1)) + 1;
			for (int i = 0; i < nr_elems; i++) {
				cv::Mat corrsPReg(3, 3, CV_64FC1);
				cv::randu(corrsPReg, Scalar(0), Scalar(1.0));
				corrsPReg /= sum(corrsPReg)[0];
				corrsPerRegion.push_back(corrsPReg.clone());
			}
		}

		int setcorrsPerRegRepRate0 = (int) (rand2() % 2);
		//Repeat rate of portion of correspondences at regions.
		//If more than one matrix of portions of correspondences at regions is provided, this number specifies the number of frames for which such a matrix is valid.
		//After all matrices are used, the first one is used again. If 0 and no matrix of portions of correspondences at regions is provided, as many random matrizes as frames are randomly generated.
		size_t corrsPerRegRepRate = 0;
		if (setcorrsPerRegRepRate0 == 0) {
			corrsPerRegRepRate = (size_t) rand2() % (nFramesPerCamConf * Rv.size() - 1);
		}

		int depthsPerRegionEmpty = (int) (rand2() % 2);
		//Portion of depths per region (must be 3x3). For each of the 3x3=9 image regions, the portion of near, mid, and far depths can be specified. If the overall depth definition is not met, this tensor is adapted.Maybe doesnt hold: Also depends on 3D - points from prior frames.
		std::vector<std::vector<depthPortion>> depthsPerRegion;
		//If depthsPerRegionEmpty==1, depthsPerRegion is initialized randomly within class genStereoSequ
		if (depthsPerRegionEmpty == 0) {
			depthsPerRegion.resize(3, std::vector<depthPortion>(3));
			for (size_t y = 0; y < 3; y++) {
				for (size_t x = 0; x < 3; x++) {
					depthsPerRegion[y][x] = depthPortion(getRandDoubleVal(rand_generator, 0, 1.0),
														 getRandDoubleVal(rand_generator, 0, 1.0),
														 getRandDoubleVal(rand_generator, 0, 1.0));
				}
			}
		}

		int nrDepthAreasPRegEmpty = (int) (rand2() % 2);
		//Min and Max number of connected depth areas per region (must be 3x3). The minimum number (first) must be larger 0.
		//The maximum number is bounded by a minimum area of 16 pixels. Maybe doesnt hold: Also depends on 3D - points from prior frames.
		std::vector<std::vector<std::pair<size_t, size_t>>> nrDepthAreasPReg;
		//if nrDepthAreasPRegEmpty==1, nrDepthAreasPReg is initialized randomly within class genStereoSequ
		if (nrDepthAreasPRegEmpty == 0) {
			nrDepthAreasPReg.resize(3, std::vector<std::pair<size_t, size_t>>(3));
			for (size_t y = 0; y < 3; y++) {
				for (size_t x = 0; x < 3; x++) {
					nrDepthAreasPReg[y][x] = std::pair<size_t, size_t>(1, (size_t) rand2() % MaxNrDepthAreasPReg + 1);
					int nrDepthAreasPRegEqu = (int) (rand2() % 2);
					if (nrDepthAreasPRegEqu == 1) {
						nrDepthAreasPReg[y][x].first = nrDepthAreasPReg[y][x].second;
					}
				}
			}
		}

		//Functionality not implemented:
//	double lostCorrPor = 0;
		//end

		//Relative velocity of the camera movement (value between 0 and 10; must be larger 0). The velocity is relative to the baseline length between the stereo cameras
		double relCamVelocity = getRandDoubleVal(rand_generator, relCamVelocityRange[0], relCamVelocityRange[1]);

		//Rotation matrix of the first camera centre.
		//This rotation can change the camera orientation for which without rotation the z - component of the relative movement vector coincides with the principal axis of the camera
		cv::Mat R = eulerAnglesToRotationMatrix(
				getRandDoubleVal(rand_generator, rollRange[0], rollRange[1]) * M_PI / 180.0,
				getRandDoubleVal(rand_generator, pitchRange[0], pitchRange[1]) * M_PI / 180.0,
				getRandDoubleVal(rand_generator, yawRange[0], yawRange[1]) * M_PI / 180.0);
		/*cv::Mat R = eulerAnglesToRotationMatrix(0,
                                                -90.0 * M_PI / 180.0,
                                                0);*/

		//Number of moving objects in the scene
		size_t nrMovObjs = (size_t) rand2() % 20;

		int startPosMovObjsEmpty = (int) (rand2() % 2);
		//Possible starting positions of moving objects in the image (must be 3x3 boolean (CV_8UC1))
		cv::Mat startPosMovObjs;
		//if startPosMovObjsEmpty==1, nrDepthAreasPReg is initialized randomly within class genStereoSequ
		if (startPosMovObjsEmpty == 0) {
			startPosMovObjs = Mat(3, 3, CV_8UC1);
			for (int y = 0; y < 3; y++) {
				for (int x = 0; x < 3; x++) {
					startPosMovObjs.at<unsigned char>(y, x) = (unsigned char) (rand2() % 2);
				}
			}
		}

		//Relative area range of moving objects. Area range relative to the image area at the beginning.
		std::pair<double, double> relAreaRangeMovObjs = std::make_pair(
				getRandDoubleVal(rand_generator, minRelAreaRangeMovObjsRange[0], minRelAreaRangeMovObjsRange[1]), 0.1);
		relAreaRangeMovObjs.second = getRandDoubleVal(rand_generator,
													  max(maxRelAreaRangeMovObjsRange[0], relAreaRangeMovObjs.first),
													  maxRelAreaRangeMovObjsRange[1]);

		int movObjDepthVersion = (int) (rand2() % 4);
		//Depth of moving objects. Moving objects are always visible and not covered by other static objects.
		//If the number of paramters is 1, this depth class (near, mid, or far) is used for every object.
		//If the number of paramters is equal "nrMovObjs", the corresponding depth is used for every object.
		//If the number of parameters is smaller and between 2 and 3, the depths for the moving objects are selected uniformly distributed from the given depths.
		//For a number of paramters larger 3 and unequal to "nrMovObjs", a portion for every depth that should be used can be defined
		//(e.g. 3 x far, 2 x near, 1 x mid -> 3 / 6 x far, 2 / 6 x near, 1 / 6 x mid).
		std::vector<depthClass> movObjDepth;
		if (movObjDepthVersion == 0)//Use the same random depth for every moving object
		{
			movObjDepth.push_back(getRandDepthClass());
		}
		if (movObjDepthVersion == 1)//Use a random depth for every moving object
		{
			movObjDepth.resize(nrMovObjs);
			for (size_t i = 0; i < nrMovObjs; i++) {
				movObjDepth[i] = getRandDepthClass();
			}
		} else if (movObjDepthVersion ==
				   2)//Specify 2 or 3 random depth classes that are used by every moving object (uniformly distributed)
		{
			int nrClasses = std::max(min((int) (rand2() % 2) + 2, (int) nrMovObjs - 1), 1);
			for (int i = 0; i < nrClasses; i++) {
				movObjDepth.push_back(getRandDepthClass());
			}
		} else//Generates a random distribution of the 3 depth classes which is used to select a depth class for a moving object
		{
			int nrClasses = (int) (rand2() % 1000) + 1;
			for (int i = 0; i < nrClasses; i++) {
				movObjDepth.push_back(getRandDepthClass());
			}
		}

		int movObjDirEmpty = (int) (rand2() % 2);
		//Movement direction of moving objects relative to camera movement (must be 3x1).
		//The movement direction is linear and does not change if the movement direction of the camera changes. The moving object is removed, if it is no longer visible in both stereo cameras.
		cv::Mat movObjDir;
		if (movObjDirEmpty == 0) {
			movObjDir = Mat(3, 1, CV_64FC1);
			cv::randu(movObjDir, Scalar(0), Scalar(1.0));
		}

		//Relative velocity range of moving objects based on relative camera velocity. Values between 0 and 100; Must be larger 0;
		std::pair<double, double> relMovObjVelRange = std::make_pair(minRelVelocityMovObj,
																	 getRandDoubleVal(rand_generator,
																					  minRelVelocityMovObj,
																					  maxRelVelocityMovObj));

		int minMovObjCorrPortionType = (int) (rand2() % 3);
		int minMovObjCorrPortionType1 = ((int) (rand2() % 3) | minMovObjCorrPortionType) & 2;
		minMovObjCorrPortionType = minMovObjCorrPortionType1 == 2 ? minMovObjCorrPortionType1
																  : minMovObjCorrPortionType;//Higher propability to get a 2
		//Minimal portion of correspondences on moving objects for removing them.
		//If the portion of visible correspondences drops below this value, the whole moving object is removed.
		//Zero means, that the moving object is only removed if there is no visible correspondence in the stereo pair.
		//One means, that a single missing correspondence leads to deletion. Values between 0 and 1;
		double minMovObjCorrPortion = 0;
		if (minMovObjCorrPortionType == 1) {
			minMovObjCorrPortion = 1.0;
		} else if (minMovObjCorrPortionType == 2) {
			minMovObjCorrPortion = getRandDoubleVal(rand_generator, 0, 1.0);
		}

		//Portion of correspondences on moving object (compared to static objects). It is limited by the size of the objects visible in the images and the minimal distance between correspondences.
		double CorrMovObjPort = getRandDoubleVal(rand_generator, minPortionMovObj, maxPortionMovObj);

		//Minimum number of moving objects over the whole track.
		//If the number of moving obects drops below this number during camera movement, as many new moving objects are inserted until "nrMovObjs" is reached.
		//If 0, no new moving objects are inserted if every preceding object is out of sight.
		size_t minNrMovObjs = (size_t) rand2() % (nrMovObjs + 1);

		//Distortion of camera matrices (internally the correct intrinsics are used, but a distorted version is generated on the output)
		//This is mainly for testing the BA capabilities for finding the correct intrinsics
		int useSpecificCamMatDistortion = (int) (rand2() % 4);
		double camMatDistLowerBound = getRandDoubleVal(rand_generator, minCamMatDistortion, maxCamMatDistortion);
		std::pair<double, double> camMatDist;
		if (useSpecificCamMatDistortion == 0) {
			camMatDist = make_pair(camMatDistLowerBound, camMatDistLowerBound);
		} else {
			double camMatDistUpperBound = getRandDoubleVal(rand_generator, camMatDistLowerBound, maxCamMatDistortion);
			camMatDist = make_pair(camMatDistLowerBound, camMatDistUpperBound);
		}

		//Set parameters
		stereoSequPars = StereoSequParameters(camTrack,
											nFramesPerCamConf,
                                              nTotalNrFrames,
											inlRatRange,
											inlRatChanges,
											truePosRange,
											truePosChanges,
											minKeypDist,
											corrsPerDepth,
											corrsPerRegion,
											corrsPerRegRepRate,
											depthsPerRegion,
											nrDepthAreasPReg,
											relCamVelocity,
											R.clone(),
											nrMovObjs,
											startPosMovObjs.clone(),
											relAreaRangeMovObjs,
											movObjDepth,
											movObjDir.clone(),
											relMovObjVelRange,
											minMovObjCorrPortion,
											CorrMovObjPort,
											minNrMovObjs,
											camMatDist
		);
	}

	//Initialize system
	uint32_t verbose = PRINT_WARNING_MESSAGES | SHOW_IMGS_AT_ERROR;

	//Only create the 3D scene or additionally create matches for the scene
	if(!genMatches) {
		genStereoSequ stereoSequ(imgSize, K_1, K_2, Rv, tv, stereoSequPars, false, verbose);

		//Generate Sequence
		stereoSequ.startCalc();
	}else{
		if(!mainStorePath || !imgPath || !imgPrePostFix){
			cerr << "Image path, image pre/postfix and path for storing matches must be provided!" << endl;
			return -1;
		}
		int err = 0;
		if(!sequLoadFolder) {
            err = genNewMatches(Rv,
                                tv,
                                K_1,
                                K_2,
                                imgSize,
                                stereoSequPars,
                                *mainStorePath,
                                *imgPath,
                                *imgPrePostFix,
                                "",
                                rwXMLinfo,
                                compressedWrittenInfo,
                                use_kitti,
                                use_oxford,
                                use_mega,
                                verbose);
		}else{
			err = genNewMatches(Rv,
								tv,
								K_1,
								K_2,
								imgSize,
								stereoSequPars,
								*mainStorePath,
								*imgPath,
								*imgPrePostFix,
								*sequLoadFolder,
								rwXMLinfo,
								compressedWrittenInfo,
                                use_kitti,
                                use_oxford,
                                use_mega,
								verbose);
		}
		if(err){
			return err;
		}
	}

	return 0;
}

int genNewMatches(std::vector<cv::Mat>& Rv,
				  std::vector<cv::Mat>& tv,
				  cv::Mat& K_1,
				  cv::Mat& K_2,
				  cv::Size &imgSize,
				  StereoSequParameters &stereoSequPars,
				  const std::string &mainStorePath,
				  const std::string &imgPath,
				  const std::string &imgPrePostFix,
				  const std::string &sequLoadFolder,
				  bool rwXMLinfo,
				  bool compressedWrittenInfo,
                  bool use_kitti,
                  bool use_oxford,
                  bool use_mega,
				  uint32_t verbose){
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine rand_generator(seed);
	std::mt19937 rand2(seed);

	string execPath;
    if(!getCurrentExecPath(execPath)){
        execPath = "";
    }

	std::string mainStorePath_ = mainStorePath;
	bool rwXMLinfo_ = rwXMLinfo;
	bool compressedWrittenInfo_ = compressedWrittenInfo;

	const double minKeypErrDistrMean = 0;
	const double maxKeypErrDistrMean = 4.0;
	const double minKeypErrDistrSD = 0;
	const double maxKeypErrDistrSD = 4.0;
	const double maxKeypErrDistr = 9.0;

	const double minIntNoiseMean = -24.0;
	const double maxIntNoiseMean = 24.0;
	const double minIntNoiseSD = -24.0;
	const double maxIntNoiseSD = 24.0;

	//Fix parameters for generating matches
	int fixpars = (int) (rand2() % 2);

	string kpType, descType;
	bool keypPosErrType = false;
	double keypErrDistr_mean = 0;
	double keypErrDistr_SD = 0;
	std::pair<double, double> keypErrDistr = std::make_pair(0, 0.5);

	double IntNoise_mean = 0;
	double IntNoise_SD = 0;
	std::pair<double, double> imgIntNoise = std::make_pair(0, 5.0);

	//Calculates matches for only a part of the frames if too less images for extracting keypoints (too less overall features) are within the given folder
	bool takeLessFramesIfLessKeyP = true;

	bool storePtClouds = false;

	bool filter_occluded_points = false;

	bool checkDescriptorDist = true;

    double repeatPatternPortStereo_min = 0;
    double repeatPatternPortStereo_max = 0.5;
    std::pair<double, double> repeatPatternPortStereo = std::make_pair(0, 0.5);

    double repeatPatternPortFToF_min = 0;
    double repeatPatternPortFToF_max = 0.5;
    std::pair<double, double> repeatPatternPortFToF = std::make_pair(0, 0.5);

    bool distortPatchCam1 = false;

    double oxfordGTMportion = 0;
    double kittiGTMportion = 0;
    double megadepthGTMportion = 0;
    double GTMportion = 0;
    double WarpedPortionTN = 1.0;
    double portionGrossTN = 0;

    int nrImgsFromImageNet = 0;
    bool rand_imagenet = false;
    string wnids[25] = {"n11530860", "n11531090", "n11837204", "n11926976", "n11966215",
                        "n03939062", "n04094438", "n03024882", "n03342529", "n03941231",
                        "n03068707", "n03830835", "n03162080", "n04508804", "n07804771",
                        "n07565945", "n07591162", "n07583865", "n07873198", "n12560775",
                        "n11868814", "n09678917", "n10343554", "n07880458", "n06479494"};
    string words[25] = {"flower", "car", "truck", "sign", "road",
                        "sports", "chair", "house", "drink", "plant",
                        "person", "fruit", "shoe", "love", "room",
                        "table", "food", "door", "cloths", "market",
                        "instrument", "animal", "water", "liquid", "dance"};
    std::vector<std::string> imageNetIDs;
    std::vector<std::string> imageNetBuzzWrds;

	//Repeat generation of matches with the same 3D sequence parameters multiple times
	GenMatchSequParameters matchPars;
	int useSameSequence = (int)(rand2() % 3) + 1;
    bool fixp = true;
	while(useSameSequence > 0){
		if(fixp){
			int idx = 0;

			//Get the used keypoint and descriptor types
			do {
				idx = (int)(rand2() % 10);
				kpType = getKeyPointType(idx);
				idx = (int) (rand2() % 22);
				descType = getDescriptorType(idx);
			}while(!checkKpDescrCompability(kpType, descType));

			//Use a keypoint position error based on keypoint detection or a given error distribution
			idx = (int)(rand2() % 6);
			if(idx){
				//Use an error distribution
				//Get mean and standard deviation of positioning error
				do{
					keypErrDistr_mean = getRandDoubleVal(rand_generator, minKeypErrDistrMean, maxKeypErrDistrMean);
					keypErrDistr_SD = getRandDoubleVal(rand_generator, minKeypErrDistrSD, maxKeypErrDistrSD);
				}while((keypErrDistr_mean + 3.0 * keypErrDistr_SD) > maxKeypErrDistr);
				keypErrDistr = std::make_pair(keypErrDistr_mean, keypErrDistr_SD);
				keypPosErrType = false;
			}else{
				keypPosErrType = true;
			}

			//Get parameters for gaussian noise on the image intensity for generating the matching descriptors
			idx = (int)(rand2() % 4);
			//Take only small or large range of noise
			if(idx){
				IntNoise_mean = getRandDoubleVal(rand_generator, minIntNoiseMean / 4.0, maxIntNoiseMean / 4.0);
				IntNoise_SD = getRandDoubleVal(rand_generator, minIntNoiseSD / 2.0, maxIntNoiseSD / 2.0);
			}else{
				IntNoise_mean = getRandDoubleVal(rand_generator, minIntNoiseMean, maxIntNoiseMean);
				IntNoise_SD = getRandDoubleVal(rand_generator, minIntNoiseSD, maxIntNoiseSD);
			}
			imgIntNoise = std::make_pair(IntNoise_mean, IntNoise_SD);

			if(sequLoadFolder.empty()) {
				//Randomly select the storage format
				idx = (int) (rand2() % 4);
				rwXMLinfo_ = true;
				if (idx) {
					rwXMLinfo_ = false;
				}

				//Randomly select if the output should be compressed
				idx = (int) (rand2() % 6);
				compressedWrittenInfo_ = true;
				if (idx) {
					compressedWrittenInfo_ = false;
				}
			}

			idx = (int)(rand2() % 2);
			if(idx) {
				storePtClouds = true;
			}
			if(!sequLoadFolder.empty()){
				idx = (int)(rand2() % 2);
				//Resulting matches will either be stored to the location where the sequence is loaded from or to the given store location
				if(idx){
					mainStorePath_ = "";
				}
			}

            idx = (int)(rand2() % 10);
            filter_occluded_points = idx == 0;

            idx = (int)(rand2() % 4);
            checkDescriptorDist = idx != 0;

            //Get parameters for repeated pattern between stereo pairs
            idx = (int)(rand2() % 4);
            if(idx){
                idx = (int)(rand2() % 10);
                repeatPatternPortStereo_min = getRandDoubleVal(rand_generator, 0, 1.0);
                if(idx) {
                    if (repeatPatternPortStereo_min < 0.98) {
                        repeatPatternPortStereo_max = getRandDoubleVal(rand_generator, repeatPatternPortStereo_min, 1.0);
                    }else{
                        repeatPatternPortStereo_max = 1.0;
                    }
                }
            }else{
                repeatPatternPortStereo_min = 0;
                repeatPatternPortStereo_max = 0;
            }
            repeatPatternPortStereo = std::make_pair(repeatPatternPortStereo_min, repeatPatternPortStereo_max);

            //Get parameters for repeated pattern from frame to frame
            idx = (int)(rand2() % 4);
            if(idx){
                idx = (int)(rand2() % 10);
                repeatPatternPortFToF_min = getRandDoubleVal(rand_generator, 0, 1.0);
                if(idx) {
                    if (repeatPatternPortFToF_min < 0.98) {
                        repeatPatternPortFToF_max = getRandDoubleVal(rand_generator, repeatPatternPortFToF_min, 1.0);
                    }else{
                        repeatPatternPortFToF_max = 1.0;
                    }
                }
            }else{
                repeatPatternPortFToF_min = 0;
                repeatPatternPortFToF_max = 0;
            }
            repeatPatternPortFToF = std::make_pair(repeatPatternPortFToF_min, repeatPatternPortFToF_max);

            idx = (int)(rand2() % 4);
            distortPatchCam1 = idx != 0;

            int mult_data = 0;
            if(use_kitti){
                idx = (int)(rand2() % 2);
                if(idx){
                    kittiGTMportion = getRandDoubleVal(rand_generator, 0, 1.0);
                    if(!nearZero(kittiGTMportion)) mult_data++;
                }
            }

            if(use_oxford){
                idx = (int)(rand2() % 2);
                if(idx){
                    oxfordGTMportion = getRandDoubleVal(rand_generator, 0, 1.0);
                    if(!nearZero(oxfordGTMportion)) mult_data++;
                }
            }

            if(use_mega){
                idx = (int)(rand2() % 2);
                if(idx){
                    megadepthGTMportion = getRandDoubleVal(rand_generator, 0, 1.0);
                    if(!nearZero(megadepthGTMportion)) mult_data++;
                }
            }

            if(mult_data > 1){
                GTMportion = getRandDoubleVal(rand_generator, 0, 1.0);
            }

            WarpedPortionTN = getRandDoubleVal(rand_generator, 0, 1.0);

            portionGrossTN = getRandDoubleVal(rand_generator, 0, 1.0);

            idx = (int)(rand2() % 2);
            imageNetIDs.clear();
            imageNetBuzzWrds.clear();
            if(idx == 0) {
                nrImgsFromImageNet = static_cast<int>(rand2() % 1500);
                idx = (int)(rand2() % 10);
                rand_imagenet = idx == 0;
                if(!rand_imagenet){
                    idx = (int)(rand2() % 3);
                    auto nr_entries1 = (size_t)(rand2() % 25);
                    vector<size_t> index_nr(25);
                    iota(index_nr.begin(), index_nr.end(), 0);
                    shuffle(index_nr.begin(), index_nr.end(), rand2);
                    index_nr.resize(nr_entries1);
                    switch (idx) {
                        case 0:
                            for(auto &nr: index_nr) {
                                imageNetIDs.push_back(wnids[nr]);
                            }
                            break;
                        case 1:
                            for(auto &nr: index_nr) {
                                imageNetBuzzWrds.push_back(words[nr]);
                            }
                            break;
                        default:
                            auto nr_entries2 = (size_t)(rand2() % 25);
                            vector<size_t> index_nr1(25);
                            iota(index_nr1.begin(), index_nr1.end(), 0);
                            shuffle(index_nr1.begin(), index_nr1.end(), rand2);
                            index_nr1.resize(nr_entries2);
                            for(auto &nr: index_nr) {
                                imageNetIDs.push_back(wnids[nr]);
                            }
                            for(auto &nr: index_nr1) {
                                imageNetBuzzWrds.push_back(words[nr]);
                            }
                            break;
                    }
                }
            }else{
                nrImgsFromImageNet = 0;
            }

			if(fixpars){
				fixp = false;
			}

			matchPars = GenMatchSequParameters(mainStorePath_,
											   imgPath,
											   imgPrePostFix,
											   kpType,
											   descType,
											   keypPosErrType,
											   keypErrDistr,
											   imgIntNoise,
											   storePtClouds,
											   rwXMLinfo_,
											   compressedWrittenInfo_,
											   takeLessFramesIfLessKeyP,
											   checkDescriptorDist,
                                               repeatPatternPortStereo,
                                               repeatPatternPortFToF,
                                               distortPatchCam1,
                                               oxfordGTMportion,
                                               kittiGTMportion,
                                               megadepthGTMportion,
                                               GTMportion,
                                               WarpedPortionTN,
                                               portionGrossTN,
                                               -1,
                                               execPath,
                                               imageNetIDs,
                                               imageNetBuzzWrds,
                                               nrImgsFromImageNet);
		}

		bool matchSucc = true;
		if(sequLoadFolder.empty()){
			genMatchSequ sequ(imgSize,
						 K_1,
						 K_2,
						 Rv,
						 tv,
						 stereoSequPars,
					matchPars,
						 filter_occluded_points,
					verbose);
			matchSucc = sequ.generateMatches();
		}else{
			genMatchSequ sequ(sequLoadFolder,
						 matchPars,
						 verbose);
			matchSucc = sequ.generateMatches();
		}

		if(!matchSucc){
			cerr << "Failed to calculate matches!" << endl;
			return -1;
		}

		useSameSequence--;
	}

	return 0;
}

bool checkKpDescrCompability(const std::string &keypointType, const std::string &descriptorType){
	if ((descriptorType == "KAZE") && (keypointType != "KAZE"))
	{
//		cout << "KAZE descriptors are only compatible with KAZE keypoints!" << endl;
		return false;
	}

	if ((descriptorType == "AKAZE") && (keypointType != "AKAZE"))
	{
//		cout << "AKAZE descriptors are only compatible with AKAZE keypoints!" << endl;
		return false;
	}

	if ((descriptorType == "ORB") && (keypointType == "SIFT"))
	{
//		cout << "ORB descriptors are not compatible with SIFT keypoints!" << endl;
		return false;
	}

	if ((descriptorType == "SIFT") && (keypointType == "MSD"))
	{
//		cout << "SIFT descriptors are not compatible with MSD keypoints!" << endl;
		return false;
	}

	return true;
}

std::string getKeyPointType(int idx){
	int const nrSupportedTypes =
#if defined(USE_NON_FREE_CODE)
			10;
#else
			7;//8;
#endif

	static std::string types [] = {"FAST",
								   "MSER",
								   "ORB",
								   //"BRISK",
								   "KAZE",
								   "AKAZE",
#if defined(USE_NON_FREE_CODE)
	"SIFT",
                                       "SURF",
#endif
								   "STAR",
								   "MSD"
	};
	return types[idx % nrSupportedTypes];
}

std::string getDescriptorType(int idx){
	int const nrSupportedTypes =
#if defined(USE_NON_FREE_CODE)
			22;
#else
			19;//20;
#endif
	static std::string types [] = {//"BRISK",
								   "ORB",
								   "KAZE",
								   "AKAZE",
								   "FREAK",
#if defined(USE_NON_FREE_CODE)
	"SIFT",
                                       "SURF",
#endif
								   "DAISY",
								   "LATCH",
								   "BGM",
								   "BGM_HARD",
								   "BGM_BILINEAR",
								   "LBGM",
								   "BINBOOST_64",
								   "BINBOOST_128",
								   "BINBOOST_256",
								   "VGG_120",
								   "VGG_80",
								   "VGG_64",
								   "VGG_48",
								   "RIFF",
								   "BOLD"
	};
	return types[idx % nrSupportedTypes];
}

depthClass getRandDepthClass()
{
	int selDepthClass = rand() % 3;
	depthClass res = depthClass::MID;
	switch (selDepthClass)
	{
	case 0:
		res = depthClass::NEAR;
		break;
	case 1:
		res = depthClass::MID;
		break;
	case 2:
		res = depthClass::FAR;
		break;
	default:
		break;
	}
	return res;
}

double getRandDoubleVal(std::default_random_engine rand_generator, double lowerBound, double upperBound)
{
	rand_generator = std::default_random_engine((unsigned int)std::rand());
	std::uniform_real_distribution<double> distribution(lowerBound, upperBound);
	return distribution(rand_generator);
}

void initStarVal(default_random_engine rand_generator, double *range, vector<double>& startVec)
{
	int isRange = std::rand() % 4;
	/*for (size_t i = 0; i < 1; i++)
	{
		isRange |= std::rand() % 2;
	}*/
	startVec.push_back(getRandDoubleVal(rand_generator, range[0], range[1]));
	if (isRange)
		startVec.push_back(getRandDoubleVal(rand_generator, startVec.back(), range[1]));
}

void initRangeVals(default_random_engine rand_generator,
		const double *range,
		const double *relMinMaxCh,
		const vector<double> &startVec,
		vector<double>& actVec)
{
	double newRange[2];
	if (startVec.size() == 1)
	{
		if (startVec[0] >= 0)
		{
			newRange[0] = std::min(std::max(startVec[0] * relMinMaxCh[0], range[0]), range[1]);
			newRange[1] = std::min(startVec[0] * relMinMaxCh[1], range[1]);
		}
		else
		{
			newRange[0] = std::min(std::max(startVec[0] * relMinMaxCh[1], range[0]), range[1]);
			newRange[1] = std::min(startVec[0] * relMinMaxCh[0], range[1]);
		}
	}
	else//2
	{
		if (startVec[0] >= 0)
			newRange[0] = std::max(startVec[0] * relMinMaxCh[0], range[0]);
		else
			newRange[0] = std::max(startVec[0] * relMinMaxCh[1], range[0]);
		if (startVec[1] >= 0)
			newRange[1] = std::min(startVec[1] * relMinMaxCh[1], range[1]);
		else
			newRange[1] = std::min(startVec[1] * relMinMaxCh[0], range[1]);
	}
	initStarVal(rand_generator, newRange, actVec);
}