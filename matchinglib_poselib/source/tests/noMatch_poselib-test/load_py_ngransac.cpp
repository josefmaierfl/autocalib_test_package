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
#include "load_py_ngransac.hpp"

using namespace std;

BOOST_PYTHON_MODULE(ComputeFramework)
{
    bp::class_<Py_input>("Py_input")
            .def_readonly("model_file_name", &Py_input::model_file_name)
            .def_readonly("threshold", &Py_input::threshold)
            .def_readonly("pts1", &Py_input::pts1)
            .def_readonly("pts2", &Py_input::pts2)
            .def_readonly("ratios", &Py_input::ratios)
            .def_readonly("gpu_nr", &Py_input::gpu_nr)
            .def_readonly("K1", &Py_input::K1)
            .def_readonly("K2", &Py_input::K2)
            ;

    bp::class_<PyComputeInstance, boost::noncopyable>("ComputeInstance");
//            .def("transferModel", &ComputeInstance::transferModel);
}

bp::object import(const std::string& module, const std::string& path, bp::object& globals)
{
    bp::dict locals;
    locals["module_name"] = module;
    locals["path"]        = path;

    bp::exec("import imp\n"
             "new_module = imp.load_module(module_name, open(path), path, ('py', 'U', imp.PY_SOURCE))\n",
             globals,
             locals);
    return locals["new_module"];
}

void ngransacInterface::transferModel(bp::list &model, bp::list &inlier_mask, int nr_inliers, int gpu_nr)
{
    std::vector<double> model_c = pyList2Vec<double>(model);
    std::vector<int> mask = pyList2Vec<int>(inlier_mask);
    result = Py_output((unsigned int)nr_inliers, vecToMat<int, unsigned char>(mask, 1, (int)mask.size()), vecToMat<double, double>(model_c, 3, 3).t(), gpu_nr);
}

int ngransacInterface::initialize(const std::string& module_name, const std::string& path, const std::string& workdir){
    try {
        // register the python module we created, so our script can import it
        PyImport_AppendInittab("ComputeFramework", &PyInit_ComputeFramework);
        Py_Initialize();

        // import the __main__ module and obtain the globals dict
        main = bp::import("__main__");
        globals = main.attr("__dict__");
        // import our *.py file
        PyObject* sysPath = PySys_GetObject("path");
        PyList_Insert( sysPath, 0, PyUnicode_FromString(workdir.c_str()));
        module = import(module_name, path, globals);
        compute_module = module.attr("Compute");//Python class name
        is_init = true;
    }catch(const bp::error_already_set&)
    {
        is_init = false;
        std::cerr << ">>> Error! Exception when trying to initialize python interface:\n";
        PyErr_Print();
        return -1;
    }
    return 0;
}

int ngransacInterface::call_ngransac(const std::string &model_file_name,
                                     const double &threshold,
                                     const std::vector<cv::Point2f> &points1,
                                     const std::vector<cv::Point2f> &points2,
                                     const std::vector<double> &ratios,
                                     cv::Mat &model,
                                     cv::Mat &mask,
                                     int &gpu_nr,
                                     cv::InputArray &K1,
                                     cv::InputArray &K2){
    if(!is_init){
        return -1;
    }
    try {
        data = Py_input(model_file_name, threshold, points1, points2, ratios, gpu_nr, K1, K2);
        bp::object compute = compute_module();
        bp::object py_out = compute.attr("compute")(data);
        bp::list model_ = bp::extract<bp::list>(py_out[0]);
        bp::list inlier_mask_ = bp::extract<bp::list>(py_out[1]);
        int nr_inliers = bp::extract<int>(py_out[2]);
        gpu_nr = bp::extract<int>(py_out[3]);
        transferModel(model_, inlier_mask_, nr_inliers, gpu_nr);
        result.model.copyTo(model);
        result.mask.copyTo(mask);
        return nr_inliers;
    }catch(...){
        bp::handle_exception();
        return -1;
    }
}
