# Testing Framework for the Pose Estimation Pipeline including NG-RANSAC using GT Data from SemiRealSequence

- [Introduction](#introduction)
- [Dependencies](#dependencies)
- [Usage](#usage)
  - [Testing of NG-RANSAC](#ngransac-test)
  - [Training of NG-RANSAC](#ngransac-train)
- [Publication](#publication)

## Introduction <a name="introduction"></a>

This testing framework is similar to the testing framework within the [master branch of this repository](https://github.com/josefmaierfl/autocalib_test_package/tree/master).
A detailed description of the framework can be found [here](https://github.com/josefmaierfl/autocalib_test_package/tree/master).

This testing framework version includes an interface to [NG-RANSAC](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac) using [Boost.Python](https://www.boost.org/doc/libs/1_74_0/libs/python/doc/html/index.html) within [./matchinglib_poselib/source/tests/noMatch_poselib-test/main.cpp](./matchinglib_poselib/source/tests/noMatch_poselib-test/main.cpp) of executable `noMatch_poselib-test` which is used to read data generated by [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence), perform relative pose estimations and refinements utilizing functions of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library, and calculate differences to ground truth (GT) data.

Folder [./ngransac_train](./ngransac_train) holds Python files for training [NG-RANSAC](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac) using data from [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) that was converted into a format used by authors of NG-RANSAC for training.
A framework for converting data can be found in [branch conversion](https://github.com/josefmaierfl/autocalib_test_package/tree/conversion).

## Dependencies <a name="dependencies"></a>

As all our evaluations were performed on [AWS](https://aws.amazon.com/), this framework is optimized for [Docker](https://docs.docker.com/get-docker/) usage.
You can build a Docker image by cloning this repository and calling `./build_docker_base.sh`.

In contrast to the [master branch of this repository](https://github.com/josefmaierfl/autocalib_test_package/tree/master), the Dockerfile also includes build steps for [PyTorch](https://pytorch.org/), [CUDA](https://developer.nvidia.com/cuda-zone), and [NG-RANSAC](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac).
Thus, at least one NVIDIA graphics card is required.
Testing can be parallelized (default) using all available GPUs and CPUs.

## Usage <a name="usage"></a>

For testing without NG-RANSAC, please use the [master branch of this repository](https://github.com/josefmaierfl/autocalib_test_package/tree/master).

For comparison of RANSAC, USAC, and NG-RANSAC (using one or more different models) without testing different USAC parameters, file `optimal_autocalib_pars.yml` (within the main directory of this repository) must be copied into your results directory: e.g. `cd [cloned_repo_dir] && cp optimal_autocalib_pars.yml ../results/testing_results/`

### Testing of NG-RANSAC <a name="ngransac-test"></a>

To start testing NG-RANSAC only perform the following steps:
1. Create a folder `res_save_compressed` outside of this repository's main directory (`../res_save_compressed`) if it does not already exist
2. Create a folder `results` outside of this repository's main directory (`../results`) if it does not already exist or provide option `RESDIR <your_directory>`  to script file `run_docker_base.sh` as second argument
3. Edit option `--ngransacModel` after line `196: elif test_name == 'ngransac'` in file [./py_test_scripts/start_test_cases.py](./py_test_scripts/start_test_cases.py) to specify your NG-RANSAC models that should be used for testing. The models must be placed in folder `[cloned_repo_dir]/matchinglib_poselib/source/poselib/thirdparty/ngransac/models`.
4. Execute the following command in the main directory of this repository
  * In case data generated by [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) exists (no evaluation and PDF generation): `./run_docker_base.sh [live/shutdown] --skip_use_test_name_nr use ngransac --skip_use_eval_name_nr usac_vs_ransac --skip_tests usac-testing refinement_ba vfc_gms_sof refinement_ba_stereo correspondence_pool robustness usac_vs_autocalib`
  * In case data generated by [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) exists and you only want to compare different NG-RANSAC models: `./run_docker_base.sh [live/shutdown] --skip_use_test_name_nr use ngransac --skip_use_eval_name_nr use usac_vs_ransac --skip_tests usac-testing refinement_ba vfc_gms_sof refinement_ba_stereo correspondence_pool robustness usac_vs_autocalib`
  * In case data generated by [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) does not exist and you do not want to compare the results to USAC and RANSAC: `./run_docker_base.sh [live/shutdown] --skip_use_test_name_nr use ngransac --skip_use_eval_name_nr usac-testing usac_vs_ransac --skip_tests refinement_ba vfc_gms_sof refinement_ba_stereo correspondence_pool robustness usac_vs_autocalib`

The first argument of script file `run_docker_base.sh` must be `live` or `shutdown`.
The latter shuts the system down after evaluations are finished or if an error occurred.

All errors are logged to disk.
Evaluations can be resumed at any point as long as processes were not killed by the user.
For additional options run `./run_docker_base.sh live -h`

### Training of NG-RANSAC <a name="ngransac-train"></a>
