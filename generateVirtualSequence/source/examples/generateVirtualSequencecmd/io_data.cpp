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

//#include "..\include\glob_includes.h"
#include "io_data.h"
//#include "atlstr.h"
#include <stdint.h>
#include <fstream>
#include "dirent.h"
#include <algorithm>
#include <functional>
//#include <boost/lambda/bind.hpp>

#define OLDVERSIONCHECKDIR 0
#if OLDVERSIONCHECKDIR
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <string>
#endif

//#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem.hpp>
//#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/dll/runtime_symbol_info.hpp>
#include "alphanum.hpp"

//#include "PfeImgFileIO.h"
//#include "PfeConv.h"

using namespace std;
using namespace cv;

/* --------------------- Function prototypes --------------------- */

int makeFrameIdConsistent1(std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr, std::size_t prefxPos1, std::size_t prefxPos2, bool bVerbose = false);

/* --------------------- Functions --------------------- */

/* This function reads all stereo or 2 subsequent images from a given directory and stores their names into two vectors.
 *
 * string filepath				Input  -> Path to the directory
 * string fileprefl				Input  -> File prefix for the left or first images (last character must be "_" which is the
 *										  last character in the filename before the image number)
 * string fileprefr				Input  -> File prefix for the right or second images (last character must be "_" which is the
 *										  last character in the filename before the image number)
 * vector<string> filenamesl	Output -> Vector with sorted filenames of the left or first images in the given directory 
 *										  that correspond to the image numbers of filenamesr
 * vector<string> filenamesr	Output -> Vector with sorted filenames of the right or second images in the given directory 
 *										  that correspond to the image numbers of filenamesl
 *
 * Return value:				 0:		  Everything ok
 *								-1:		  Could not open directory
 *								-2:		  No corresponding images available
 *								-3:		  No images available
 */
int loadStereoSequence(std::string filepath, std::string fileprefl, std::string fileprefr,
					   std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr)
{
	DIR *dir;
	struct dirent *ent;
	if((dir = opendir(filepath.c_str())) != nullptr)
	{
		while ((ent = readdir(dir)) != nullptr)
		{
			string filename;
			filename = string(ent->d_name);
			if(filename.compare(0,fileprefl.size(),fileprefl) == 0)
				filenamesl.push_back(filename);
			else if(filename.compare(0,fileprefr.size(),fileprefr) == 0)
				filenamesr.push_back(filename);
		}
		closedir(dir);

		if(filenamesl.empty())
		{
			perror("No left images available");
			return -3;
		}

		if(filenamesr.empty())
		{
			perror("No right images available");
			return -3;
		}

		sort(filenamesl.begin(),filenamesl.end(),
			 [](string const &first, string const &second){return stoi(first.substr(first.find_last_of('_')+1)) <
			 stoi(second.substr(second.find_last_of('_')+1));});

		sort(filenamesr.begin(),filenamesr.end(),
			 [](string const &first, string const &second){return stoi(first.substr(first.find_last_of('_')+1)) <
			 stoi(second.substr(second.find_last_of('_')+1));});

		size_t i = 0;
		while((i < filenamesr.size()) && (i < filenamesl.size()))
		{
			if(stoi(filenamesl[i].substr(filenamesl[i].find_last_of('_')+1)) <
			   stoi(filenamesr[i].substr(filenamesr[i].find_last_of('_')+1)))
			   filenamesl.erase(filenamesl.begin()+i,filenamesl.begin()+i+1);
			else if(stoi(filenamesl[i].substr(filenamesl[i].find_last_of('_')+1)) >
					stoi(filenamesr[i].substr(filenamesr[i].find_last_of('_')+1)))
					filenamesr.erase(filenamesr.begin()+i,filenamesr.begin()+i+1);
			else
				i++;
		}

		while(filenamesl.size() < filenamesr.size())
			filenamesr.pop_back();

		while(filenamesl.size() > filenamesr.size())
			filenamesl.pop_back();

		if(filenamesl.empty())
		{
			perror("No corresponding images available");
			return -2;
		}
	}
	else
	{
		perror("Could not open directory");
		return -1;
	}

	return 0;
}


/* This function reads all images from a given directory and stores their names into a vector.
 *
 * string filepath				Input  -> Path to the directory
 * string fileprefl				Input  -> File prefix for the left or first images (last character must be "_" which is the
 *										  last character in the filename before the image number)
 * vector<string> filenamesl	Output -> Vector with sorted filenames of the images in the given directory
 */
bool loadImageSequence(const std::string &filepath, const std::string &fileprefl, std::vector<std::string> &filenamesl)
{
	DIR *dir;
	struct dirent *ent;
	if((dir = opendir(filepath.c_str())) != nullptr)
	{
		while ((ent = readdir(dir)) != nullptr)
		{
			string filename;
			filename = string(ent->d_name);
			if(filename.compare(0,fileprefl.size(),fileprefl) == 0)
				filenamesl.push_back(filename);
		}
		closedir(dir);

		if(filenamesl.empty())
		{
            std::cerr << "No images available" << endl;
			return false;
		}

		sort(filenamesl.begin(),filenamesl.end(),
			 [](string const &first, string const &second){return stoi(first.substr(first.find_last_of('_')+1)) <
			 stoi(second.substr(second.find_last_of('_')+1));});
	}
	else
	{
		std::cerr << "Could not open directory" << endl;
		return false;
	}

	return true;
}

/* This function reads all stereo or 2 subsequent images from a given directory and stores their names into two vectors.
*
* string filepath				Input  -> Path to the directory
* string fileprefl				Input  -> Prefix for the left or first images. It can include a folder structure that follows after the
*										  filepath, a file prefix, a '*' indicating the position of the number and a postfix. If it is empty,
*										  all files from the folder filepath are used (also if fileprefl only contains a folder ending with '/',
*										  every file within this folder is used). It is possible to specify only a prefix with or
*										  without '*' at the end. If a prefix is used, all characters until the first number (excluding) must
*										  be provided. For a postfix, '*' must be placed before the postfix.
*										  Valid examples: folder/pre_*post, *post, pre_*, pre_, folder/*post, folder/pre_*, folder/pre_, folder/,
*										  folder/folder/, folder/folder/pre_*post, ...
*										  For non stereo images (consecutive images), fileprefl must be equal to fileprefr.
* string fileprefr				Input  -> Prefix for the right or second images. It can include a folder structure that follows after the
*										  filepath, a file prefix, a '*' indicating the position of the number and a postfix. If it is empty,
*										  all files from the folder filepath are used (also if fileprefl only contains a folder ending with '/',
*										  every file within this folder is used). It is possible to specify only a prefix with or
*										  without '*' at the end. If a prefix is used, all characters until the first number (excluding) must
*										  be provided. For a postfix, '*' must be placed before the postfix.
*										  Valid examples: folder/pre_*post, *post, pre_*, pre_, folder/*post, folder/pre_*, folder/pre_, folder/,
*										  folder/folder/, folder/folder/pre_*post, ...
*										  For non stereo images (consecutive images), fileprefl must be equal to fileprefr.
* vector<string> filenamesl	Output -> Vector with sorted filenames of the left or first images in the given directory
*										  that correspond to the image numbers of filenamesr
* vector<string> filenamesr	Output -> Vector with sorted filenames of the right or second images in the given directory
*										  that correspond to the image numbers of filenamesl
*
* Return value:				 0:		  Everything ok
*								-1:		  Could not open directory
*								-2:		  No corresponding images available
*								-3:		  No images available
*/
int loadStereoSequenceNew(std::string filepath, std::string fileprefl, std::string fileprefr,
	std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr)
{
	DIR *dir;
	struct dirent *ent;
	std::string l_frameUsed;
	size_t prefxPos1 = 0, prefxPos2 = 0;
	bool bInputIdent = false;
	if (filepath.find('\\') != std::string::npos)
		std::replace(filepath.begin(), filepath.end(), '\\', '/');
	if (fileprefl.find('\\') != std::string::npos)
		std::replace(fileprefl.begin(), fileprefl.end(), '\\', '/');
	if (fileprefr.find('\\') != std::string::npos)
		std::replace(fileprefr.begin(), fileprefr.end(), '\\', '/');
	if (filepath.rfind('/') == filepath.size() - 1)
		filepath = filepath.substr(0, filepath.size() - 1);
	for (int i = 0; i<2; ++i)
	{
		std::string fileprefl_use = (i == 0) ? fileprefl : fileprefr, filedir_use = filepath;
		std::string filepostfx;
		int posLastSl = (int)fileprefl_use.rfind('/');
		if (posLastSl >= 0)
		{
			if (fileprefl_use.find('/') == 0)
				filedir_use += fileprefl_use.substr(0, (size_t)posLastSl);
			else
				filedir_use += "/" + fileprefl_use.substr(0, (size_t)posLastSl);
			fileprefl_use = fileprefl_use.substr((size_t)posLastSl + 1);
		}
		std::string cmp_is_ident = filedir_use + "/" + fileprefl_use;
		if (i == 0)
			l_frameUsed = cmp_is_ident;
		else if ((cmp_is_ident == l_frameUsed) && filenamesl.size() > 1)
		{
			bInputIdent = true;
			break;
		}


		int nFuzzyPos = (int)fileprefl_use.find('*');
		bool bCmpFuzzy = (nFuzzyPos >= 0);
		if (bCmpFuzzy)
		{
			//fileprefl_use = fileprefl_use.substr(0, nFuzzyPos) + fileprefl_use.substr(nFuzzyPos + 1);
			std::string fileprefl_use_tmp = fileprefl_use;
			fileprefl_use = fileprefl_use.substr(0, (size_t)nFuzzyPos);
			filepostfx = fileprefl_use_tmp.substr((size_t)nFuzzyPos + 1);
		}
		if ((dir = opendir(filedir_use.c_str())) != nullptr)
		{
			while ((ent = readdir(dir)) != nullptr)
			{
				string filename;
				filename = string(ent->d_name);
				if ((fileprefl_use.empty() && filepostfx.empty() && filename.size() > 2 && filename.find(".db") == std::string::npos)
					|| (bCmpFuzzy && !fileprefl_use.empty() && !filepostfx.empty()
						&& (filename.size() >= fileprefl_use.size()) && (filename.find(fileprefl_use) != std::string::npos)
						&& (filename.size() >= filepostfx.size()) && (filename.find(filepostfx) != std::string::npos))
					|| (bCmpFuzzy && !fileprefl_use.empty() && filepostfx.empty()
						&& (filename.size() >= fileprefl_use.size()) && filename.find(fileprefl_use) != std::string::npos)
					|| (bCmpFuzzy && fileprefl_use.empty() && !filepostfx.empty()
						&& (filename.size() >= filepostfx.size()) && filename.find(filepostfx) != std::string::npos)
					|| (!fileprefl_use.empty() && filename.compare(0, fileprefl_use.size(), fileprefl_use) == 0)) {
                    if (i == 0) {
                        filenamesl.push_back(filedir_use + "/" + filename);
                    } else {
                        filenamesr.push_back(filedir_use + "/" + filename);
                    }
                }
			}
			closedir(dir);
		}
		else
		{
			perror("Could not open directory");
			return -1;
		}

		if (!fileprefl_use.empty() && i == 0)
		{
			prefxPos1 = filenamesl.back().rfind(fileprefl_use);
			prefxPos1 += fileprefl_use.size();
		}
		else if (i == 0)
		{
			prefxPos1 = filedir_use.size() + 1;
			size_t nrpos = std::string::npos, nrpos1 = prefxPos1;
			bool firstchar = false;
			for (int j = 0; j < 10; j++)
			{
				nrpos = filenamesl.back().find_first_of(std::to_string(j), prefxPos1);

				if (!firstchar && nrpos != std::string::npos)
					firstchar = true;
				if (firstchar && nrpos == prefxPos1)
					break;

				if (nrpos != std::string::npos && ((nrpos < nrpos1) || ((nrpos >= prefxPos1) && (nrpos1 == prefxPos1))))
					nrpos1 = nrpos;
			}
			prefxPos1 = nrpos1;
		}

		if (!fileprefl_use.empty() && i == 1)
		{
			prefxPos2 = filenamesr.back().rfind(fileprefl_use);
			prefxPos2 += fileprefl_use.size();
		}
		else if (i == 1)
		{
			prefxPos2 = filedir_use.size() + 1;
			size_t nrpos = std::string::npos, nrpos1 = prefxPos2;
			bool firstchar = false;
			for (int j = 0; j < 10; j++)
			{
				nrpos = filenamesr.back().find_first_of(std::to_string(j), prefxPos2);

				if (!firstchar && nrpos != std::string::npos)
					firstchar = true;
				if (firstchar && nrpos == prefxPos2)
					break;

				if (nrpos != std::string::npos && ((nrpos < nrpos1) || ((nrpos >= prefxPos2) && (nrpos1 == prefxPos2))))
					nrpos1 = nrpos;
			}
			prefxPos2 = nrpos1;
		}
	}

	{
		if (filenamesl.empty())
		{
			perror("No left images available");
			return -3;
		}

		if (bInputIdent)
		{
			sort(filenamesl.begin(), filenamesl.end(), doj::alphanum_less<std::string>());
			filenamesr = filenamesl;   //r==l (remove first/last frame)
			filenamesl.pop_back();
			filenamesr.erase(filenamesr.begin());
		}
		else
		{
			if (filenamesr.empty())
			{
				perror("No right images available");
				return -3;
			}

			using namespace std::placeholders;
			sort(filenamesl.begin(), filenamesl.end(),
				std::bind([](string const &first, string const &second, std::size_t prefxPos)
				{return stoi(first.substr(prefxPos)) <
					stoi(second.substr(prefxPos)); }, _1, _2, prefxPos1));

			sort(filenamesr.begin(), filenamesr.end(),
				std::bind([](string const &first, string const &second, std::size_t prefxPos)
				{return stoi(first.substr(prefxPos)) <
					stoi(second.substr(prefxPos)); }, _1, _2, prefxPos2));

			makeFrameIdConsistent1(filenamesl, filenamesr, prefxPos1, prefxPos2);
		}

		if (filenamesl.empty())
		{
			perror("No corresponding images available");
			return -2;
		}
	}

	return 0;
}

int makeFrameIdConsistent1(std::vector<std::string> & filenamesl, std::vector<std::string> & filenamesr, std::size_t prefxPos1, std::size_t prefxPos2, bool bVerbose)
{
	int num_rem = 0;
	size_t i = 0;
	while ((i < filenamesr.size()) && (i < filenamesl.size()))
	{
		if (stoi(filenamesl[i].substr(prefxPos1)) <
			stoi(filenamesr[i].substr(prefxPos2)))
		{
			if (bVerbose)
				cout << "Warning: removing inconsistent frame " << filenamesl[i] << " vs. " << filenamesr[i] << endl;
			num_rem++;
			filenamesl.erase(filenamesl.begin() + i, filenamesl.begin() + i + 1);
		}
		else if (stoi(filenamesl[i].substr(prefxPos1)) >
			stoi(filenamesr[i].substr(prefxPos2)))
		{
			num_rem++;
			filenamesr.erase(filenamesr.begin() + i, filenamesr.begin() + i + 1);
		}
		else
			i++;
	}

	while (filenamesl.size() < filenamesr.size())
	{
		num_rem++;
		filenamesr.pop_back();
	}

	while (filenamesl.size() > filenamesr.size())
	{
		if (bVerbose)
			cout << "Warning: removing inconsistent frame " << filenamesl[filenamesl.size() - 1] << " at end (numbers mismatch)" << endl;
		num_rem++;
		filenamesl.pop_back();
	}

	return num_rem;
}

/* This function reads all images from a given directory and stores their names into a vector.
*
* string filepath				Input  -> Path to the directory
* string fileprefl				Input  -> Prefix images. It can include a folder structure that follows after the
*										  filepath, a file prefix, a '*' indicating the position of the number and a postfix. If it is empty,
*										  all files from the folder filepath are used (also if fileprefl only contains a folder ending with '/',
*										  every file within this folder is used). It is possible to specify only a prefix with or
*										  without '*' at the end. If a prefix is used, all characters until the first number (excluding) must
*										  be provided. For a postfix, '*' must be placed before the postfix.
*										  Valid examples: folder/pre_*post, *post, pre_*, pre_, folder/*post, folder/pre_*, folder/pre_, folder/,
*										  folder/folder/, folder/folder/pre_*post, ...
* vector<string> filenamesl		Output -> Vector with sorted filenames of the images in the given directory
*
* Return value:					 0:		  Everything ok
*								-1:		  Could not open directory
*								-2:		  No images available
*/
bool loadImageSequenceNew(std::string filepath, std::string fileprefl, std::vector<std::string> & filenamesl, bool isNoImg)
{
	DIR *dir;
	struct dirent *ent;
	std::string fileprefl_use = std::move(fileprefl), filedir_use = move(filepath);
	std::string filepostfx;
	if (filedir_use.find('\\') != std::string::npos)
		std::replace(filedir_use.begin(), filedir_use.end(), '\\', '/');
	if (fileprefl_use.find('\\') != std::string::npos)
		std::replace(fileprefl_use.begin(), fileprefl_use.end(), '\\', '/');
	if (filedir_use.rfind('/') == filedir_use.size() - 1)
		filedir_use = filedir_use.substr(0, filedir_use.size() - 1);
	int posLastSl = (int)fileprefl_use.rfind('/');
	if (posLastSl >= 0)
	{
		if (fileprefl_use.find('/') == 0)
			filedir_use += fileprefl_use.substr(0, posLastSl);
		else
			filedir_use += "/" + fileprefl_use.substr(0, posLastSl);
		fileprefl_use = fileprefl_use.substr(posLastSl + 1);
	}
	int nFuzzyPos = (int)fileprefl_use.find('*');
	bool bCmpFuzzy = (nFuzzyPos >= 0);
	if (bCmpFuzzy)
	{
		//fileprefl_use = fileprefl_use.substr(0, nFuzzyPos) + fileprefl_use.substr(nFuzzyPos + 1);
		std::string fileprefl_use_tmp = fileprefl_use;
		fileprefl_use = fileprefl_use.substr(0, (size_t)nFuzzyPos);
		filepostfx = fileprefl_use_tmp.substr((size_t)nFuzzyPos + 1);
	}
	if ((dir = opendir(filedir_use.c_str())) != nullptr)
	{
		while ((ent = readdir(dir)) != nullptr)
		{
			string filename;
			filename = string(ent->d_name);
			if ((fileprefl_use.empty() && filepostfx.empty() && filename.size() > 2 && filename.find(".db") == std::string::npos)
				|| (bCmpFuzzy && !fileprefl_use.empty() && !filepostfx.empty()
					&& (filename.size() >= fileprefl_use.size()) && (filename.find(fileprefl_use) != std::string::npos)
					&& (filename.size() >= filepostfx.size()) && (filename.find(filepostfx) != std::string::npos))
				|| (bCmpFuzzy && !fileprefl_use.empty() && filepostfx.empty()
					&& (filename.size() >= fileprefl_use.size()) && filename.find(fileprefl_use) != std::string::npos)
				|| (bCmpFuzzy && fileprefl_use.empty() && !filepostfx.empty()
					&& (filename.size() >= filepostfx.size()) && filename.find(filepostfx) != std::string::npos)
				|| (!fileprefl_use.empty() && filename.compare(0, fileprefl_use.size(), fileprefl_use) == 0)) {
				if(isNoImg || IsImgTypeSupported(filename)) {
					filenamesl.push_back(filedir_use + "/" + filename);
				}
			}
		}
		closedir(dir);
		std::sort(filenamesl.begin(), filenamesl.end(), doj::alphanum_less<std::string>());
	}
	else
	{
		perror("Could not open directory");
		return false;
	}

    return !filenamesl.empty();

}

bool IsImgTypeSupported(std::string const& filename){
	std::vector<std::string> vecSupportedTypes = GetSupportedImgTypes();

	size_t ptPos = filename.rfind('.');
	if(ptPos == string::npos){
		return false;
	}
	std::string ending = filename.substr(ptPos + 1);

	std::transform(ending.begin(), ending.end(), ending.begin(), ::tolower);

	if(std::find(vecSupportedTypes.begin(), vecSupportedTypes.end(), ending) != vecSupportedTypes.end())
	{
		return true;
	}

	return false;
}

std::vector<std::string> GetSupportedImgTypes(){
	int const nrSupportedTypes = 20;
	static std::string types [] = {"bmp",
								   "dib",
								   "jpeg",
								   "jpg",
								   "jpe",
								   "jp2",
								   "png",
								   "webp",
								   "pbm",
								   "pgm",
								   "ppm",
								   "pxm",
								   "pnm",
								   "sr",
								   "ras",
								   "tiff",
								   "tif",
								   "exr",
								   "hdr",
								   "pic"
	};
	return std::vector<std::string>(types, types + nrSupportedTypes);
}

#if OLDVERSIONCHECKDIR
bool checkPathExists(const std::string &path){
    struct stat info;
    if( stat( path.c_str() , &info ) != 0 )//Cannot access directory
        return false;
    else if( info.st_mode & S_IFDIR )  //It is a directory
        return true;

    return false;
}
#else
bool checkPathExists(const std::string &path){
	boost::filesystem::path path2 = path;
	return boost::filesystem::is_directory(path2);
}
#endif

bool checkFileExists(const std::string &filename){
	return boost::filesystem::exists(filename);
}

bool createDirectory(const std::string &path){
	return boost::filesystem::create_directories(path);
}

bool deleteDirectory(const std::string &path){
    return boost::filesystem::remove_all(path) > 0;
}

bool deleteFile(const std::string &filename){
	return boost::filesystem::remove(filename);
}

std::string remFileExt(const std::string &name){
    size_t lastindex = name.find_last_of('.');
    string fname_new;
    if(lastindex == std::string::npos){
        fname_new = name;
    }else {
        fname_new = name.substr(0, lastindex);
    }
    return fname_new;
}

//Return execution path without filename of the current executable
bool getCurrentExecPath(std::string &path){
    boost::filesystem::path p;
    try {
       p = boost::dll::program_location();
    } catch (...) {
        return false;
    }

    if(p.empty() || !boost::filesystem::exists(p)){
        return false;
    }
    path = p.parent_path().string();
    return true;
}

std::string concatPath(const std::string &mainPath, const std::string &subPath){
	std::string filedir1 = mainPath;
	std::string filedir2 = subPath;
	if (filedir1.find('\\') != std::string::npos)
		std::replace(filedir1.begin(), filedir1.end(), '\\', '/');
	if (filedir2.find('\\') != std::string::npos)
		std::replace(filedir2.begin(), filedir2.end(), '\\', '/');
	if (filedir1.rfind('/') == filedir1.size() - 1)
		filedir1 = filedir1.substr(0, filedir1.size() - 1);
	if (filedir2.find('/') == 0)
		filedir2 = filedir2.substr(1, filedir1.size());

	return filedir1 + '/' + filedir2;
}

/* This function reads all homography file names from a given directory and stores their names into a vector.
 *
 * string filepath				Input  -> Path to the directory
 * string fileprefl				Input  -> File prefix for the left or first images (for the dataset of
 *										  www.robots.ox.ac.uk/~vgg/research/affine/ this must be H1to because
 *										  the file names look like H1to2, H1to3, ...)
 * vector<string> filenamesl	Output -> Vector with sorted filenames of the images in the given directory
 */
bool readHomographyFiles(const std::string& filepath, const std::string& fileprefl, std::vector<std::string> & filenamesl)
{
    DIR *dir;
    struct dirent *ent;
    if((dir = opendir(filepath.c_str())) != nullptr)
    {
        while ((ent = readdir(dir)) != nullptr)
        {
            string filename;
            filename = string(ent->d_name);
            if(filename.compare(0,fileprefl.size(),fileprefl) == 0)
                filenamesl.push_back(filename);
        }
        closedir(dir);

        if(filenamesl.empty())
        {
            std::cerr << "No homography files available" << endl;
            return false;
        }

        sort(filenamesl.begin(),filenamesl.end(),
             [](string const &first, string const &second){return stoi(first.substr(first.find_last_of('o')+1)) <
                     stoi(second.substr(second.find_last_of('o')+1));});
    }
    else
    {
        std::cerr << "Could not open directory" << endl;
        return false;
    }

    return true;
}

/* This function reads a homography from a given file.
 *
 * string filepath				Input  -> Path to the directory
 * string filename				Input  -> Filename of a stored homography (from www.robots.ox.ac.uk/~vgg/research/affine/ )
 * Mat* H						Output -> Pointer to the homography
 */
bool readHomographyFromFile(const std::string& filepath, const std::string& filename, cv::OutputArray H)
{
    ifstream ifs;
    char stringline[100];
    char* pEnd;
    Mat H_ = Mat(3,3,CV_64FC1);
    size_t i = 0, j;
    string fpn = concatPath(filepath, filename);
    ifs.open(fpn, ifstream::in);

    while(ifs.getline(stringline,100) && (i < 3))
    {
        H_.at<double>(i,0) = strtod(stringline, &pEnd);
        for(j = 1; j < 3; j++)
        {
            H_.at<double>(i,j) = strtod(pEnd, &pEnd);
        }
        i++;
    }
    ifs.close();

    if((i < 3) || (j < 3))
        return false; //Reading homography failed
    if (H.needed())
    {
        if (H.empty())
            H.create(3, 3, CV_64F);
        H_.copyTo(H.getMat());
    }else{
        return false;
    }

    return true;
}

std::string getFilenameFromPath(const std::string &name){
    return boost::filesystem::path(name).filename().string();
}

//Get all directoies within a folder
std::vector<std::string> getDirs(const std::string &path){
    boost::filesystem::path p(path);
    if(!boost::filesystem::is_directory(p)){
        return std::vector<std::string>();
    }
    std::vector<std::string> dirs;
    for(auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(p), {})){
        if(boost::filesystem::is_directory(entry)){
            dirs.emplace_back(entry.path().filename().string());
        }
    }
    return dirs;
}

//Check if a given directory is empty
bool dirIsEmpty(const std::string &path){
    boost::filesystem::path p(path);
    return boost::filesystem::is_empty(p);
}

//Returns the parent path
std::string getParentPath(const std::string &path){
    return boost::filesystem::path(path).parent_path().string();
}

//Count the number of files in the folder
size_t getNumberFilesInFolder(const std::string &path){
    boost::filesystem::path p(path);

    size_t cnt = std::count_if(
            boost::filesystem::directory_iterator(p),
            boost::filesystem::directory_iterator(),
            static_cast<bool(*)(const boost::filesystem::path&)>(boost::filesystem::is_regular_file) );
    return cnt;
}

/* This function takes an 16Bit RGB integer image and converts it to a 3-channel float flow matrix where R specifies the
 * flow in u, G the flow in v and B if the flow is valid.
 *
 * string filename				Input  -> File name of the flow file
 * vector<Point2f> positionI1   Output -> Points in first image corresponding to positionI2
 * vector<Point2f> positionI2   Output -> Points in second image corresponding to positionI1
 * OutputArray flow3			Output -> Pointer to the resulting 3-channel flow matrix (floats) where channel 1 specifies
 *										  the flow in u, channel 2 the flow in v and channel 3 if the flow is valid (=1).
 * float precision				Input  -> Used precision in the given flow image file after the decimal point
 *										  (e.g. a precision of 64 yields a resolution of 1/64). [Default = 64]
 * bool useBoolValidity			Input  -> If true, it is asumed that the given validity in B is boolean (0 or 1) as e.g.
 *										  used within the KITTI database. Otherwise it can be a float number with
 *										  with precision validityPrecision (validity = B/validityPrecision). [Default = true]
 * float validityPrecision		Input  -> If useBoolValidity = false, this value specifies the precision of the
 *										  validity B (validity = B/validityPrecision). [Default = 64]
 * float minConfidence			Input  -> If useBoolValidity = false, minConfidence specifies the treshold used to decide
 *										  if a flow value is marked as valid or invalid (for validities between 0 and 1).
 *										  [Default = 1.0]
 */
bool convertImageFlowFile(const std::string &filename, std::vector<cv::Point2f> *positionI1, std::vector<cv::Point2f> *positionI2, cv::OutputArray flow3,
                          float precision, bool useBoolValidity, float validityPrecision, float minConfidence)
{
    Mat intflow = imread(filename, cv::IMREAD_UNCHANGED);

    if(intflow.data  == nullptr){
        cerr <<"Error reading flow file" << endl;
        return false;
    }
    if(intflow.type() != CV_16UC3){
        cerr <<"Wrong format after reading flow file" << endl;
        return false;
    }

    //flow3->create(intflow.rows, intflow.cols, CV_32FC3);

    vector<Mat> channels(3), channels_fin;
    if(flow3.needed()) {
        channels_fin.emplace_back(intflow.rows, intflow.cols, CV_32FC1);
        channels_fin.emplace_back(intflow.rows, intflow.cols, CV_32FC1);
        channels_fin.emplace_back(intflow.rows, intflow.cols, CV_32FC1);
    }
    cv::split(intflow, channels);
    auto nr_elems = static_cast<size_t>(intflow.rows * intflow.cols);
    if(useBoolValidity)
    {
        if(flow3.needed()) {
            for (int u = 0; u < intflow.rows; u++) {
                for (int v = 0; v < intflow.cols; v++) {
                    if (channels[2].at<uint16_t>(u, v) > 0) {
                        channels_fin[0].at<float>(u, v) =
                                ((float) channels[0].at<uint16_t>(u, v) - 32768.0f) / precision;
                        channels_fin[1].at<float>(u, v) =
                                ((float) channels[1].at<uint16_t>(u, v) - 32768.0f) / precision;
                        channels_fin[2].at<float>(u, v) = (float) channels[2].at<uint16_t>(u, v);
                    } else {
                        channels_fin[0].at<float>(u, v) = 0.0f;
                        channels_fin[1].at<float>(u, v) = 0.0f;
                        channels_fin[2].at<float>(u, v) = 0.0f;
                    }
                }
            }
        }
        if((positionI1 != nullptr) && (positionI2 != nullptr)){
            *positionI1 = std::vector<cv::Point2f>(nr_elems);
            *positionI2 = std::vector<cv::Point2f>(nr_elems);
            size_t used = 0;
            cv::Point2f flow;
            for (int u = 0; u < intflow.rows; u++) {
                for (int v = 0; v < intflow.cols; v++) {
                    if (channels[2].at<uint16_t>(u, v) > 0) {
                        flow.x = (static_cast<float>(channels[0].at<uint16_t>(u, v)) - 32768.0f) / precision;
                        flow.y = (static_cast<float>(channels[1].at<uint16_t>(u, v)) - 32768.0f) / precision;
                        (*positionI1)[used].x = static_cast<float>(v);
                        (*positionI1)[used].y = static_cast<float>(u);
                        (*positionI2)[used].x = static_cast<float>(v) + flow.x;
                        (*positionI2)[used++].y = static_cast<float>(u) + flow.y;
                    }
                }
            }
            positionI1->erase(positionI1->begin() + used, positionI1->end());
            positionI2->erase(positionI2->begin() + used, positionI2->end());
        }
    }else{
        if(flow3.needed()) {
            for (int u = 0; u < intflow.rows; u++) {
                for (int v = 0; v < intflow.cols; v++) {
                    if (channels[2].at<uint16_t>(u, v) > 0) {
                        float conf = (float) channels[2].at<uint16_t>(u, v) / validityPrecision;
                        if (conf >= minConfidence) {
                            channels_fin[0].at<float>(u, v) =
                                    ((float) channels[0].at<uint16_t>(u, v) - 32768.0f) / precision;
                            channels_fin[1].at<float>(u, v) =
                                    ((float) channels[1].at<uint16_t>(u, v) - 32768.0f) / precision;
                            channels_fin[2].at<float>(u, v) = 1.0f;
                        } else {
                            channels_fin[0].at<float>(u, v) = 0.0f;
                            channels_fin[1].at<float>(u, v) = 0.0f;
                            channels_fin[2].at<float>(u, v) = 0.0f;
                        }
                    } else {
                        channels_fin[0].at<float>(u, v) = 0.0f;
                        channels_fin[1].at<float>(u, v) = 0.0f;
                        channels_fin[2].at<float>(u, v) = 0.0f;
                    }
                }
            }
        }
        if((positionI1 != nullptr) && (positionI2 != nullptr)){
            *positionI1 = std::vector<cv::Point2f>(nr_elems);
            *positionI2 = std::vector<cv::Point2f>(nr_elems);
            size_t used = 0;
            cv::Point2f flow;
            for (int u = 0; u < intflow.rows; u++) {
                for (int v = 0; v < intflow.cols; v++) {
                    if (channels[2].at<uint16_t>(u, v) > 0) {
                        const float conf = static_cast<float>(channels[2].at<uint16_t>(u, v)) / validityPrecision;
                        if (conf >= minConfidence) {
                            flow.x = (static_cast<float>(channels[0].at<uint16_t>(u, v)) - 32768.0f) / precision;
                            flow.y = (static_cast<float>(channels[1].at<uint16_t>(u, v)) - 32768.0f) / precision;
                            (*positionI1)[used].x = static_cast<float>(v);
                            (*positionI1)[used].y = static_cast<float>(u);
                            (*positionI2)[used].x = static_cast<float>(v) + flow.x;
                            (*positionI2)[used++].y = static_cast<float>(u) + flow.y;
                        }
                    }
                }
            }
            positionI1->erase(positionI1->begin() + used, positionI1->end());
            positionI2->erase(positionI2->begin() + used, positionI2->end());
        }
    }

    if(flow3.needed()) {
        if (flow3.empty())
            flow3.create(intflow.rows, intflow.cols, CV_32FC3);
        cv::merge(channels_fin, flow3.getMat());
    }

    return true;
}

/* This function takes an 16Bit 1-channel integer image (grey values) and converts it to a 3-channel (RGB) float flow matrix
 * where R specifies the disparity, G is always 0 (as the disparity only represents the flow in x-direction) and B specifies
 * if the flow/disparity is valid (0 or 1).
 * string filename				Input  -> File name of the disparity file
 * vector<Point2f> positionI1   Output -> Points in first image corresponding to positionI2
 * vector<Point2f> positionI2   Output -> Points in second image corresponding to positionI1
 * OutputArray flow3			Output -> Resulting 3-channel flow matrix (floats) where channel 1 specifies
 *										  the the disparity, channel 2 is always 0 (as the disparity only represents the
 *										  flow in x-direction) and channel 3 specifies if the disparity is valid (=1).
 * bool useFLowStyle			Input  -> If true [Default=false], the input file is expected to be a 3-channel 16bit file,
 *										  where the first channel includes the disparity values, the second channel is useless
 *										  and the third channel specifies if a disparity value is valid (valid >0, invalid 0)
 * float precision				Input  -> Used precision in the given disparity image file after the decimal point
 *										  (e.g. a precision of 64 yields a resolution of 1/64). [Default = 256]
 * bool use0Invalid				Input  -> If true, it is asumed that the given disparity is valid if the disparity is >0
 *										  (0 = invalid) as e.g. used within the KITTI database. Otherwise it is asumed that
 *										  invalid disparities have the value 0xFFFF. [Default = true]
 */
bool convertImageDisparityFile(const std::string &filename, std::vector<cv::Point2f> *positionI1, std::vector<cv::Point2f> *positionI2,
                               cv::OutputArray flow3, const bool useFLowStyle, const float precision, const bool use0Invalid)
{
    Mat intflow = imread(filename, cv::IMREAD_UNCHANGED);

    if(intflow.data  == nullptr){
        cerr <<"Error reading flow file" << endl;
        return false;
    }
    if(intflow.type() != CV_16UC1 && intflow.type() != CV_16UC3){
        cerr <<"Wrong format after reading flow file" << endl;
        return false;
    }

    vector<Mat> channels(3), channels_fin;
    if(flow3.needed()) {
        channels_fin.emplace_back(intflow.rows, intflow.cols, CV_32FC1);
        channels_fin.emplace_back(intflow.rows, intflow.cols, CV_32FC1);
        channels_fin.emplace_back(intflow.rows, intflow.cols, CV_32FC1);
    }
    auto nr_elems = static_cast<size_t>(intflow.rows * intflow.cols);
    if(useFLowStyle && flow3.needed())
    {
        cv::split(intflow, channels);
        //namedWindow( "Channel 1", WINDOW_AUTOSIZE );// Create a window for display.
        //imshow( "Channel 1", channels[0] );
        //namedWindow( "Channel 2", WINDOW_AUTOSIZE );// Create a window for display.
        //imshow( "Channel 2", channels[1] );
        //namedWindow( "Channel 3", WINDOW_AUTOSIZE );// Create a window for display.
        //imshow( "Channel 3", channels[2] );
        //cv::waitKey(0);
    }

    if(useFLowStyle)
    {
        if(flow3.needed()) {
            for (int u = 0; u < intflow.rows; u++) {
                for (int v = 0; v < intflow.cols; v++) {
                    if (channels[2].at<uint16_t>(u, v) > 0) {
                        channels_fin[0].at<float>(u, v) = -1.0f * (float) channels[0].at<uint16_t>(u, v) / precision;
                        channels_fin[1].at<float>(u, v) = 0.0f;
                        channels_fin[2].at<float>(u, v) = (float) channels[2].at<uint16_t>(u, v);
                    } else {
                        channels_fin[0].at<float>(u, v) = 0.0f;
                        channels_fin[1].at<float>(u, v) = 0.0f;
                        channels_fin[2].at<float>(u, v) = 0.0f;
                    }
                }
            }
        }
        if((positionI1 != nullptr) && (positionI2 != nullptr)){
            *positionI1 = std::vector<cv::Point2f>(nr_elems);
            *positionI2 = std::vector<cv::Point2f>(nr_elems);
            size_t used = 0;
            float disp = 0;
            for (int u = 0; u < intflow.rows; u++) {
                for (int v = 0; v < intflow.cols; v++) {
                    if (channels[2].at<uint16_t>(u, v) > 0) {
                        disp = -1.0f * static_cast<float>(channels[0].at<uint16_t>(u, v)) / precision;
                        (*positionI1)[used].x = static_cast<float>(v);
                        (*positionI1)[used].y = static_cast<float>(u);
                        (*positionI2)[used].x = static_cast<float>(v) + disp;
                        (*positionI2)[used++].y = static_cast<float>(u);
                    }
                }
            }
            positionI1->erase(positionI1->begin() + used, positionI1->end());
            positionI2->erase(positionI2->begin() + used, positionI2->end());
        }
    }else{
        if(use0Invalid){
            if(flow3.needed()) {
                for (int u = 0; u < intflow.rows; u++) {
                    for (int v = 0; v < intflow.cols; v++) {
                        if (intflow.at<uint16_t>(u, v) > 0) {
                            channels_fin[0].at<float>(u, v) = -1.0f * (float) intflow.at<uint16_t>(u, v) / precision;
                            channels_fin[1].at<float>(u, v) = 0.0f;
                            channels_fin[2].at<float>(u, v) = 1.0f;
                        } else {
                            channels_fin[0].at<float>(u, v) = 0.0f;
                            channels_fin[1].at<float>(u, v) = 0.0f;
                            channels_fin[2].at<float>(u, v) = 0.0f;
                        }
                    }
                }
            }
            if((positionI1 != nullptr) && (positionI2 != nullptr)){
                *positionI1 = std::vector<cv::Point2f>(nr_elems);
                *positionI2 = std::vector<cv::Point2f>(nr_elems);
                size_t used = 0;
                float disp = 0;
                for (int u = 0; u < intflow.rows; u++) {
                    for (int v = 0; v < intflow.cols; v++) {
                        if (intflow.at<uint16_t>(u, v) > 0) {
                            disp = -1.0f * static_cast<float>(intflow.at<uint16_t>(u, v)) / precision;
                            (*positionI1)[used].x = static_cast<float>(v);
                            (*positionI1)[used].y = static_cast<float>(u);
                            (*positionI2)[used].x = static_cast<float>(v) + disp;
                            (*positionI2)[used++].y = static_cast<float>(u);
                        }
                    }
                }
                positionI1->erase(positionI1->begin() + used, positionI1->end());
                positionI2->erase(positionI2->begin() + used, positionI2->end());
            }
        }else{
            if(flow3.needed()) {
                for (int u = 0; u < intflow.rows; u++) {
                    for (int v = 0; v < intflow.cols; v++) {
                        if (intflow.at<uint16_t>(u, v) == 0xFFFF) {
                            channels_fin[0].at<float>(u, v) = 0.0f;
                            channels_fin[1].at<float>(u, v) = 0.0f;
                            channels_fin[2].at<float>(u, v) = 0.0f;
                        } else {
                            channels_fin[0].at<float>(u, v) = -1.0f * (float) intflow.at<uint16_t>(u, v) / precision;
                            channels_fin[1].at<float>(u, v) = 0.0f;
                            channels_fin[2].at<float>(u, v) = 1.0f;
                        }
                    }
                }
            }
            if((positionI1 != nullptr) && (positionI2 != nullptr)){
                *positionI1 = std::vector<cv::Point2f>(nr_elems);
                *positionI2 = std::vector<cv::Point2f>(nr_elems);
                size_t used = 0;
                float disp = 0;
                for (int u = 0; u < intflow.rows; u++) {
                    for (int v = 0; v < intflow.cols; v++) {
                        if (intflow.at<uint16_t>(u, v) != 0xFFFF) {
                            disp = -1.0f * static_cast<float>(intflow.at<uint16_t>(u, v)) / precision;
                            (*positionI1)[used].x = static_cast<float>(v);
                            (*positionI1)[used].y = static_cast<float>(u);
                            (*positionI2)[used].x = static_cast<float>(v) + disp;
                            (*positionI2)[used++].y = static_cast<float>(u);
                        }
                    }
                }
                positionI1->erase(positionI1->begin() + used, positionI1->end());
                positionI2->erase(positionI2->begin() + used, positionI2->end());
            }
        }
    }

    if(flow3.needed()) {
        if (flow3.empty())
            flow3.create(intflow.rows, intflow.cols, CV_32FC3);
        cv::merge(channels_fin, flow3.getMat());
    }

    return true;
}

//Convert a 3 channel floating point flow matrix (x, y, last channel corresp. to validity) to a 3-channel uint16 png image (same format as KITTI)
bool writeKittiFlowFile(const std::string &filename, const cv::Mat &flow, float precision,
                        bool useBoolValidity, float validityPrecision){
    if(checkFileExists(filename)){
        cerr << "Flow file " << filename << " already exists." << endl;
        return false;
    }
    if(flow.type() != CV_32FC3){
        cerr <<"Wrong format of flow matrix" << endl;
        return false;
    }

    vector<Mat> channels, channelsInt;
    for (int i = 0; i < 3; ++i) {
        channelsInt.emplace_back(Mat::zeros(flow.size(), CV_16UC1));
    }
    cv::split(flow, channels);
    if(useBoolValidity)
    {
        for (int u = 0; u < flow.rows; u++) {
            for (int v = 0; v < flow.cols; v++) {
                if (channels[2].at<float>(u, v) > 0) {
                    const float fx = round(channels[0].at<float>(u, v) * precision + 32768.0f);
                    if(fx < 0 || fx > static_cast<float>(std::numeric_limits<uint16_t>::max())){
                        continue;
                    }
                    const float fy = round(channels[1].at<float>(u, v) * precision + 32768.0f);
                    if(fy < 0 || fy > static_cast<float>(std::numeric_limits<uint16_t>::max())){
                        continue;
                    }
                    channelsInt[0].at<uint16_t>(u, v) = static_cast<uint16_t>(fx);
                    channelsInt[1].at<uint16_t>(u, v) = static_cast<uint16_t>(fy);
                    channelsInt[2].at<uint16_t>(u, v) = 1;
                }
            }
        }
    }else{
        for (int u = 0; u < flow.rows; u++) {
            for (int v = 0; v < flow.cols; v++) {
                if (channels[2].at<float>(u, v) > 0) {
                    const float fx = round(channels[0].at<float>(u, v) * precision + 32768.0f);
                    if(fx < 0 || fx > static_cast<float>(std::numeric_limits<uint16_t>::max())){
                        continue;
                    }
                    const float fy = round(channels[1].at<float>(u, v) * precision + 32768.0f);
                    if(fy < 0 || fy > static_cast<float>(std::numeric_limits<uint16_t>::max())){
                        continue;
                    }
                    channelsInt[0].at<uint16_t>(u, v) = static_cast<uint16_t>(fx);
                    channelsInt[1].at<uint16_t>(u, v) = static_cast<uint16_t>(fy);
                    channelsInt[2].at<float>(u, v) = static_cast<uint16_t>(round(channels[2].at<float>(u, v) * validityPrecision));
                }
            }
        }
    }
    Mat pngFlow;
    cv::merge(channelsInt, pngFlow);
    vector<int> compression_params;
    compression_params.push_back(IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(0);
    bool result = false;
    try {
        result = cv::imwrite(filename, pngFlow, compression_params);
    }catch (const cv::Exception& ex){
        cerr << "Exception converting image to PNG format: " << ex.what() << endl;
        return false;
    }
    if (!result) {
        cerr << "ERROR: Can't save PNG file." << endl;
        return false;
    }
    return true;
}