//
// Created by maierj on 10.02.20.
//
#include "load_py_ngransac.hpp"

using namespace std;

class ComputeInstance{
public:
    explicit ComputeInstance(ComputeServer&);
    virtual ~ComputeInstance() = default;

    virtual void compute(const Py_input& data) = 0;
    void transferModel(bp::list model, bp::list inlier_mask, unsigned int nr_inliers);

private:
    ComputeServer& _server;
};

class PyComputeInstance final
        : public ComputeInstance
                , public bp::wrapper<ComputeInstance>
{
    using ComputeInstance::ComputeInstance;

    void compute(const Py_input& data) override
    {
        get_override("compute")(data);
    }
};

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

    bp::class_<ComputeServer>("ComputeServer");

    bp::class_<PyComputeInstance, boost::noncopyable>("ComputeInstance", bp::init<ComputeServer&>())
            .def("transferModel", &ComputeInstance::transferModel)
            ;
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

ComputeInstance::ComputeInstance(ComputeServer& server)
        : _server(server)
{}

void ComputeInstance::transferModel(bp::list model, bp::list inlier_mask, unsigned int nr_inliers)
{
    _server.transferModel(*this, model, inlier_mask, nr_inliers);
}

void ComputeServer::transferModel(ComputeInstance&, bp::list model, bp::list inlier_mask, unsigned int nr_inliers)
{
    std::vector<double> model_c = pyList2Vec<double>(model);
    std::vector<int> mask = pyList2Vec<int>(inlier_mask);
    result = Py_output(nr_inliers, vecToMat<int, bool>(mask, 1, (int)mask.size()), vecToMat<double, double>(model_c, 3, 3));
}

unsigned int ComputeServer::get_parameters(cv::Mat &model_, cv::Mat &mask_){
    if (result.mask.empty() || result.model.empty()){
        std::cout << "No data from NGRANSAC available." << std::endl;
        return 0;
    }
    result.mask.copyTo(mask_);
    result.model.copyTo(model_);
    return result.nr_inliers;
}

int ngransacInterface::initialize(const std::string& module_name, const std::string& path){
    try {
        // register the python module we created, so our script can import it
        PyImport_AppendInittab("ComputeFramework", &PyInit_ComputeFramework);
        Py_Initialize();

        // import the __main__ module and obtain the globals dict
        main = bp::import("__main__");
        globals = main.attr("__dict__");
        // import our *.py file
//        module = import("compute", "compute.py", globals);
        module = import(module_name, path, globals);
        Compute = module.attr("Compute");//Python class name
        compute = Compute(server);
    }catch(const bp::error_already_set&)
    {
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
                                     cv::Mat &model,
                                     cv::Mat &mask,
                                     cv::InputArray &K1,
                                     cv::InputArray &K2){
    if(!is_init){
        return -1;
    }
    data = Py_input(model_file_name, threshold, points1, points2, K1, K2);
    compute.attr("compute")(data);
    return (int)server.get_parameters(model, mask);
}
