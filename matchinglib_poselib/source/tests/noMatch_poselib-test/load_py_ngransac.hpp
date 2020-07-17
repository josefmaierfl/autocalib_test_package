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

#ifndef MATCHING_LOAD_PY_NGRANSAC_HPP
#define MATCHING_LOAD_PY_NGRANSAC_HPP
#include <iostream>
#include <vector>
#include <memory>

#include "boost/shared_ptr.hpp"
#include <boost/python.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include "boost/python/stl_iterator.hpp"

#include "opencv2/core/core.hpp"
//#include <boost/shared_ptr.hpp>

namespace bp = boost::python;

template<typename T>
inline
std::vector< T > pyList2Vec( const bp::object& iterable )
{
    std::vector< T > tmp = std::vector< T >( bp::stl_input_iterator< T >( iterable ),
                                             bp::stl_input_iterator< T >( ) );
    return tmp;
}


template <class T>
inline
bp::list std_vector_to_py_list(std::vector<T> vector) {
    typename std::vector<T>::iterator iter;
    bp::list list;
    for (iter = vector.begin(); iter != vector.end(); ++iter) {
        list.append(*iter);
    }
    return list;
}

template<typename T, typename T1>
inline
cv::Mat vecToMat(std::vector<T> &vec, int nr_rows, int nr_cols){
    cv::Mat data = cv::Mat_<T1>(nr_rows, nr_cols);
    for(int i=0; i < nr_cols; i++){
        for(int j=0; j < nr_rows; j++){
            int idx = i * nr_rows + j;
            data.at<T1>(j, i) = static_cast<T1>(vec[idx]);
        }
    }
    return data;
}

inline
std::vector<bp::tuple> vecCvPoints2vecPyTuple(const std::vector<cv::Point2f> &pts){
    std::vector<bp::tuple> tuples;
    for(auto &i: pts){
        tuples.emplace_back(bp::make_tuple((double)i.x, (double)i.y));
    }
    return tuples;
}

inline
std::vector<double> mat2vec(const cv::Mat &data){
    CV_Assert(data.type() == CV_64FC1);
    std::vector<double> tmp = std::vector<double>(data.begin<double>(), data.end<double>());
    return tmp;
}

struct Py_input{
    std::string model_file_name;
    double threshold;
    bp::list pts1, pts2;
    bp::list ratios;
    bp::list K1, K2;
    int gpu_nr;

    Py_input(const std::string &model_file_name_,
            const double &threshold_,
            const std::vector<cv::Point2f> &pts1_,
            const std::vector<cv::Point2f> &pts2_,
            const std::vector<double> &ratios_,
            const int &gpu_nr_,
            cv::InputArray &K1_ = cv::noArray(),
            cv::InputArray &K2_ = cv::noArray()):
            model_file_name(model_file_name_),
            threshold(threshold_),
            gpu_nr(gpu_nr_){
        std::vector<bp::tuple> pts1__ = vecCvPoints2vecPyTuple(pts1_);
        std::vector<bp::tuple> pts2__ = vecCvPoints2vecPyTuple(pts2_);
        pts1 = std_vector_to_py_list(pts1__);
        pts2 = std_vector_to_py_list(pts2__);
        ratios = std_vector_to_py_list(ratios_);

        if(!K1_.empty() && !K2_.empty()) {
            std::vector<double> K1__ = mat2vec(K1_.getMat());
            std::vector<double> K2__ = mat2vec(K2_.getMat());
            K1 = std_vector_to_py_list(K1__);
            K2 = std_vector_to_py_list(K2__);
        }
    }

    Py_input(){
        model_file_name = "";
        threshold = 0.001;
        gpu_nr = -1;
    }
};

struct Py_output{
    unsigned int nr_inliers;
    cv::Mat mask;
    cv::Mat model;
    int gpu_nr;

    Py_output(const unsigned int &nr_inliers_, cv::Mat mask_, cv::Mat model_, const int &gpu_nr_):
            nr_inliers(nr_inliers_),
            mask(mask_.clone()),
            model(model_.clone()),
            gpu_nr(gpu_nr_){
        CV_Assert((mask.type() == CV_8UC1) && (mask.rows == 1) && (mask.cols > mask.rows));
        CV_Assert((model.type() == CV_64FC1) && (model.rows == 3) && (model.cols == model.rows));
    };

    Py_output(){
        nr_inliers = 0;
        gpu_nr = -1;
    };
};

class ComputeInstance{
public:
    virtual ~ComputeInstance(){}
    explicit ComputeInstance() = default;

    virtual void compute(const Py_input& data) = 0;
};

class PyComputeInstance final
        : public ComputeInstance,
          public bp::wrapper<ComputeInstance>
{
    using ComputeInstance::ComputeInstance;

    void compute(const Py_input& data) override
    {
        this->get_override("compute")(data);
    }
};

class ngransacInterface{
public:
    ngransacInterface(const std::string& module, const std::string& path, const std::string& workdir){

        int res = initialize(module, path, workdir);
        if(res != 0){
            throw "Unable to initialize Python interface";
        }
        is_init = true;
    }

    ngransacInterface(){
        is_init = false;
    }
    int initialize(const std::string& module, const std::string& path, const std::string& workdir);
    int call_ngransac(const std::string &model_file_name,
                      const double &threshold,
                      const std::vector<cv::Point2f> &points1,
                      const std::vector<cv::Point2f> &points2,
                      const std::vector<double> &ratios,
                      cv::Mat &model,
                      cv::Mat &mask,
                      int &gpu_nr,
                      cv::InputArray &K1 = cv::noArray(),
                      cv::InputArray &K2 = cv::noArray());
    void transferModel(bp::list &model, bp::list &inlier_mask, int nr_inliers, int gpu_nr);
private:
    bool is_init = false;
    Py_input data;
    Py_output result;
    bp::object main, globals, module, compute_module;
};
#endif //MATCHING_LOAD_PY_NGRANSAC_HPP
