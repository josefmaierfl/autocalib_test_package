//
// Created by maierj on 10.02.20.
//
#include "load_py_ngransac.hpp"

using namespace std;

ComputeInstance::ComputeInstance(boost::shared_ptr<ComputeServer>& server)
        : _server(server)
{
    std::cout << "In ComputeInstance constructor" << std::endl;
}

BOOST_PYTHON_MODULE(ComputeFramework)
{
    bp::class_<Py_input>("Py_input")
            .def_readonly("model_file_name", &Py_input::model_file_name)
            .def_readonly("threshold", &Py_input::threshold)
            .def_readonly("pts1", &Py_input::pts1)
            .def_readonly("pts2", &Py_input::pts2)
            .def_readonly("K1", &Py_input::K1)
            .def_readonly("K2", &Py_input::K2)
            ;

    bp::class_<ComputeServer, boost::shared_ptr<ComputeServer>, boost::noncopyable>("ComputeServer", bp::no_init);

//    bp::class_<PyComputeInstance, boost::noncopyable>("ComputeInstance", bp::init<ComputeServer&>())
//            .def("transferModel", &ComputeInstance::transferModel)
//            ;
    bp::class_<PyComputeInstance, boost::noncopyable>("ComputeInstance", bp::init<boost::shared_ptr<ComputeServer>&>())
            .def("transferModel", &ComputeInstance::transferModel);
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

void ComputeInstance::transferModel(bp::list model, bp::list inlier_mask, int nr_inliers)
{
    std::cout << "Transferring" << std::endl;
    _server->transferModel(model, inlier_mask, (unsigned int)nr_inliers);
}

void ComputeServer::transferModel(bp::list &model, bp::list &inlier_mask, unsigned int nr_inliers)
{
    std::cout << "In server" << std::endl;
    std::vector<double> model_c = pyList2Vec<double>(model);
    std::vector<int> mask = pyList2Vec<int>(inlier_mask);
    std::cout << "After list convert" << std::endl;
    result = Py_output(nr_inliers, vecToMat<int, unsigned char>(mask, 1, (int)mask.size()), vecToMat<double, double>(model_c, 3, 3));
    if (result.mask.empty() || result.model.empty()){
        std::cout << "It is empty!!!" << std::endl;
    }
}

//void ComputeInstance::transferModel(bp::list &model, bp::list &inlier_mask, int nr_inliers)
//{
//    std::cout << "In server" << std::endl;
//    std::vector<double> model_c = pyList2Vec<double>(model);
//    std::vector<int> mask = pyList2Vec<int>(inlier_mask);
//    std::cout << "After list convert" << std::endl;
//    result = Py_output((unsigned int)nr_inliers, vecToMat<int, unsigned char>(mask, 1, (int)mask.size()), vecToMat<double, double>(model_c, 3, 3));
//    if (result.mask.empty() || result.model.empty()){
//        std::cout << "It is empty!!!" << std::endl;
//    }
//}

unsigned int ComputeServer::get_parameters(cv::Mat &model_, cv::Mat &mask_){
    if (result.mask.empty() || result.model.empty()){
        std::cout << "No data from NGRANSAC available." << std::endl;
        return 0;
    }
    std::cout << "Cloning final mats" << std::endl;
    result.mask.copyTo(mask_);
    result.model.copyTo(model_);
    return result.nr_inliers;
}

//unsigned int ComputeInstance::get_parameters(cv::Mat &model_, cv::Mat &mask_){
//    if (result.mask.empty() || result.model.empty()){
//        std::cout << "No data from NGRANSAC available." << std::endl;
//        return 0;
//    }
//    std::cout << "Cloning final mats" << std::endl;
//    result.mask.copyTo(mask_);
//    result.model.copyTo(model_);
//    return result.nr_inliers;
//}

int ngransacInterface::initialize(const std::string& module_name, const std::string& path, const std::string& workdir){
    try {
        server.reset(new ComputeServer);
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
        std::cout << "Before transferring server" << std::endl;
        compute = compute_module(server);
        std::cout << "After transferring server" << std::endl;
//        compute_module = bp::object((bp::handle<>(PyImport_ImportModule("ComputeFramework"))));
//        module["ComputeFramework"] = compute_module(server);
//        bp::scope(compute_module).attr("Compute") = bp::ptr(&compute);
        is_init = true;
    }catch(const bp::error_already_set&)
    {
        is_init = false;
        std::cerr << ">>> Error! Exception when trying to initialize python interface:\n";
        PyErr_Print();
        return -1;
    }
    std::cout << "Init finished" << std::endl;
    return 0;
}

int ngransacInterface::call_ngransac(const std::string &model_file_name,
                                     const double &threshold,
                                     const std::vector<cv::Point2f> &points1,
                                     const std::vector<cv::Point2f> &points2,
                                     cv::Mat &model,
                                     cv::Mat &mask,
                                     cv::InputArray &K1,
                                     cv::InputArray &K2){
    if(!is_init){
        return -1;
    }
    cout << "Calling" << endl;
    data = Py_input(model_file_name, threshold, points1, points2, K1, K2);
    compute.attr("compute")(data);
    return (int)server->get_parameters(model, mask);
}
