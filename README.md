# Testing Framework for the Pose Estimation Pipeline using GT Data from SemiRealSequence

- [Introduction](#introduction)
- [Dependencies](#dependencies)
- [Usage](#usage)
- [Python File Descriptions](#files)
- [Testing Results](#results)
- [Publication](#publication)

## Introduction <a name="introduction"></a>

This framework is used to generate multiple semi-real GT datasets utilizing [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) (a not installable version is included in this repository within [./generateVirtualSequence](./generateVirtualSequence)) which are further used to test various relative pose estimation and refinement algorithms from the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library (a not installable version is included in this repository within [./matchinglib_poselib](./matchinglib_poselib)).

[SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) is used to generate 3D scenes and sparse feature matches with specific properties like depth ranges of 3D point clouds, number of inliers, inlier ratios, spatial distributions of correspondences within the 2D image domain, keypoint repeatability errors, linear and abrupt changes in different GT camera pose parameters, ...

For each of afore mentioned scene properties, parameter sweeps are performed on [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) to find advantages and disadvantages of different pose estimation and refinement algorithms in addition to combinations thereof.

Branch [ngransac](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac) also includes functionality for testing [NG-RANSAC](https://github.com/vislearn/ngransac).

For a detailed description of the used GT-generation framework take a look at [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence).

For a detailed description of the used pose estimation and refinement algorithms take a look at [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib).

The testing framework makes use of parallel computing using all available CPU cores (can be adjusted using option `--nrCPUs`) to speed up testing.
We used 72 CPUs for generating GT data and testing.

## Dependencies <a name="dependencies"></a>

As all our evaluations were performed on [AWS](https://aws.amazon.com/), this framework is optimized for [Docker](https://docs.docker.com/get-docker/) usage.
You can build a Docker image by cloning this repository and calling `./build_docker_base.sh`.

## Usage <a name="usage"></a>

Evaluations were performed in the following order:
1. Parameter sweep on USAC parameters
2. Comparison of [USAC](https://ieeexplore.ieee.org/document/6365642), vanilla RANSAC, and [NG-RANSAC](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac)
3. Comparison of various pose refinement algorithms and cost functions using estimated poses from USAC
4. Evaluation on correspondence filtering methods [GMS](https://github.com/JiawangBian/GMS-Feature-Matcher), [VFC](https://github.com/jiayi-ma/VFC), and [SOF](https://link.springer.com/chapter/10.1007/978-3-319-46478-7_7)
5. Comparison of various pose refinement algorithms and cost functions on aggregated feature correspondences using the continuous high accuracy stereo pose estimation and refinement functionality of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library
6. Comparison of various parameter settings on the feature match aggregation functionality of the continuous high accuracy stereo pose estimation and refinement framework of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library
7. Parameter sweep on robustness parameters of the continuous high accuracy stereo pose estimation and refinement framework of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library
8. Comparison of USAC with simple correspondence aggregation using a windowing approach and the continuous high accuracy stereo pose estimation and refinement framework of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library

For each of afore mentioned evaluations, best performing parameter sets were used for the next evaluation (e.g. USAC parameters were fixed for evaluations on refinement algorithms) to reduce parameter space.

To start testing (all evaluations) perform the following steps:
1. Create a folder `res_save_compressed` outside of this repository's main directory (`../res_save_compressed`)
2. Create a folder `results` outside of this repository's main directory (`../results`) or provide option `RESDIR <your_relative_directory>` (relative directory to `[cloned_repo_dir]/..`) to script file `run_docker_base.sh` as second argument
3. Execute `./run_docker_base.sh live` in the main directory of this repository

The first argument of script file `run_docker_base.sh` must be `live` or `shutdown`.
The latter shuts the system down after evaluations are finished or if an error occurred.

All errors are logged to disk.
Evaluations can be resumed at any point as long as processes were not killed by the user.
For additional options run `./run_docker_base.sh live -h`

## Python File Descriptions <a name="files"></a>

Python files used for testing are located within [./py_test_scripts](./py_test_scripts).
Most Python files offer their own command line interface for independent usage.

### main.py

[Main test file](./py_test_scripts/main.py) for starting and resuming evaluations (including generation of configuration files for parameter sweeps and scene creations with [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)).
It provides functionality for performing only specific tests, compressing results and logging information, specifying different working directories, ...
For available options call `./run_docker_base.sh live -h`.

### gen_init_scenes.py <a name="gen_init_scenes"></a>

Used to load initial configuration files (which define parameters that are not used for scene parameter sweeping but will be kept fixed) for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) and creates scenes which can be further used to extract information (relative poses of stereo cameras that should be equal for evaluations) that is needed for the final configuration files.

The following steps should be performed:
1. Create a directory that should hold your config files
2. Generate a template configuration file following instructions [here](https://github.com/josefmaierfl/SemiRealSequence#config-file)
3. Rename the configuration file to `[your_file_name]_initial.[extension]`.
4. Change configuration file parameters (excluding sweep parameters) based on your needs
5. Repeat steps 2 to 5 for additional parameter sets
6. Call `cd [cloned_repo_dir]/py_test_scripts && python gen_init_scenes.py -h` to show necessary options or take a look at file [./py_test_scripts/gen_init_scenes.py](./py_test_scripts/gen_init_scenes.py).
7. Call `cd [cloned_repo_dir]/py_test_scripts && python gen_init_scenes.py [options]` to start generating initial sequences
8. Use Python file [extract_Rt.py](#extract_Rt) to copy calculated relative poses into final initial configuration files

### extract_Rt.py <a name="extract_Rt"></a>

Copies all stereo configurations from a generated scene (by [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)) into a configuration file.
Python file [gen_init_scenes.py](#gen_init_scenes) must be used in advance for generating scenes.

Call `cd [cloned_repo_dir]/py_test_scripts && python extract_Rt.py -h` to show necessary options or take a look at file [./py_test_scripts/extract_Rt.py](./py_test_scripts/extract_Rt.py).

### change_distCamMat_config.py

Changes configuration file parameter `distortCamMat` for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) within all configuration files for which the file name in a given directory contains the pattern `*_initial.[extension]`.

Call `cd [cloned_repo_dir]/py_test_scripts && python change_distCamMat_config.py -h` to show options or take a look at file [./py_test_scripts/change_distCamMat_config.py](./py_test_scripts/change_distCamMat_config.py).

### change_keyp_distr_config.py

Changes configuration file parameter `corrsPerRegion` for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) within all configuration files for which the file name in a given directory contains the pattern `*_initial.[extension]`.
Used parameter values must be edited within [./py_test_scripts/change_keyp_distr_config.py](./py_test_scripts/change_keyp_distr_config.py)

Call `cd [cloned_repo_dir]/py_test_scripts && python change_keyp_distr_config.py -h` to show options or take a look at file [./py_test_scripts/change_keyp_distr_config.py](./py_test_scripts/change_keyp_distr_config.py).

### change_nrTP_config.py

Changes configuration file parameter `truePosRange` for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) within all configuration files for which the file name in a given directory contains the pattern `[.*_kp-distr-(?:(?:half-img)|(?:1corn)|(?:equ))_depth-(?:F|(?:NM)|(?:NMF))_TP-([0-9to]+).*]_initial.[extension]`.
The values for `truePosRange` are extracted from the file name using the regular expression shown above within brackets `[...]`.

Call `cd [cloned_repo_dir]/py_test_scripts && python change_nrTP_config.py -h` to show options or take a look at file [./py_test_scripts/change_nrTP_config.py](./py_test_scripts/change_nrTP_config.py).

### change_user_interrupt_config.py

Changes configuration file parameter `imageOverlap` for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) within all configuration files for which the file name in a given directory contains the pattern `*_initial.[extension]`.
Used parameter values must be edited within [./py_test_scripts/change_user_interrupt_config.py](./py_test_scripts/change_user_interrupt_config.py)

To change parameter `acceptBadStereoPars` instead of `imageOverlap`, comment lines (`59: lobj = re.search('imageOverlap:', li)` and `62: fo.write('imageOverlap: 8.5000000000000004e-01\n')`) and uncomment lines (`58: #lobj = re.search('acceptBadStereoPars:', li)` and `61: #fo.write('acceptBadStereoPars: 1\n')`) within [./py_test_scripts/change_user_interrupt_config.py](./py_test_scripts/change_user_interrupt_config.py).

Call `cd [cloned_repo_dir]/py_test_scripts && python change_user_interrupt_config.py -h` to show options or take a look at file [./py_test_scripts/change_user_interrupt_config.py](./py_test_scripts/change_user_interrupt_config.py).

### gen_mult_scene_configs.py

Loads initial configuration files (based on them multiple configuration files are generated by performing scene parameter sweeps on some parameters) for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) and builds a folder structure and an overview file for generating specific configuration and overview files for scenes by varying the used inlier ratio or inlier ratio change rate and keypoint accuracy.
Depending on whether a parameter sweep on the inlier ratio or the inlier ratio change rate is performed, Python file [./py_test_scripts/gen_scene_configs.py](./py_test_scripts/gen_scene_configs.py) or [./py_test_scripts/gen_rob-test_scene_configs.py](./py_test_scripts/gen_rob-test_scene_configs.py) is called.

Call `cd [cloned_repo_dir]/py_test_scripts && python gen_mult_scene_configs.py -h` to show options or take a look at file [./py_test_scripts/gen_mult_scene_configs.py](./py_test_scripts/gen_mult_scene_configs.py).

### gen_scene_configs.py

Loads initial configuration files (based on them multiple configuration files are generated by performing scene parameter sweeps on some parameters) for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) and generates configuration files and overview files for scenes by varying the used inlier ratio and keypoint accuracy.
This file is typically called by [./py_test_scripts/gen_mult_scene_configs.py](./py_test_scripts/gen_mult_scene_configs.py)

Call `cd [cloned_repo_dir]/py_test_scripts && python gen_scene_configs.py -h` to show options or take a look at file [./py_test_scripts/gen_scene_configs.py](./py_test_scripts/gen_scene_configs.py).

### gen_rob-test_scene_configs.py

Loads initial configuration files (based on them multiple configuration files are generated by performing scene parameter sweeps on some parameters) for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) and generates configuration files and overview files for scenes by varying the used inlier ratio change rate and keypoint accuracy.
This file is typically called by [./py_test_scripts/gen_mult_scene_configs.py](./py_test_scripts/gen_mult_scene_configs.py)

Call `cd [cloned_repo_dir]/py_test_scripts && python gen_rob-test_scene_configs.py -h` to show options or take a look at file [./py_test_scripts/gen_rob-test_scene_configs.py](./py_test_scripts/gen_rob-test_scene_configs.py).

### create_scenes.py

Loads multiple configuration files and generates scenes using [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence).

Call `cd [cloned_repo_dir]/py_test_scripts && python create_scenes.py -h` to show options or take a look at file [./py_test_scripts/create_scenes.py](./py_test_scripts/create_scenes.py).

### start_test_cases.py

Holds parameter sweep values and filtering conditions for every main test listed [here](#usage) and every sub-test.
Moreover, functionality is included to check for available best performing parameter sets of preceding evaluations.
For every sub-test, Python file [exec_autocalib.py](#exec_autocalib) is called.

Call `cd [cloned_repo_dir]/py_test_scripts && python start_test_cases.py -h` to show options or take a look at file [./py_test_scripts/start_test_cases.py](./py_test_scripts/start_test_cases.py).

### exec_autocalib.py <a name="exec_autocalib"></a>

Script for performing parameter sweeps on parameters of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library.

Call `cd [cloned_repo_dir]/py_test_scripts && python exec_autocalib.py -h` to show options or take a look at file [./py_test_scripts/exec_autocalib.py](./py_test_scripts/exec_autocalib.py).

### retry_test_cases.py

Used to execute different test scenarios for the relative pose estimation framework within the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library that failed before.

Call `cd [cloned_repo_dir]/py_test_scripts && python retry_test_cases.py -h` to show options or take a look at file [./py_test_scripts/retry_test_cases.py](./py_test_scripts/retry_test_cases.py).

### eval_tests_main.py

Holds function calls for each evaluation including used parameter values, informative texts, ... that are used to calculate statistics and evaluation results in addition to generate Latex and PDF files.
In addition, testing results from multiple tests (resulting from parameter sweeps on [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) and the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library) are loaded and concatenated into single Pandas dataframes.
This file mainly calls functions from [statistics_and_plot.py](#statistics_and_plot)

Call `cd [cloned_repo_dir]/py_test_scripts && python eval_tests_main.py -h` to show options or take a look at file [./py_test_scripts/eval_tests_main.py](./py_test_scripts/eval_tests_main.py).

### statistics_and_plot.py <a name="statistics_and_plot"></a>

Holds functions for calculating statistics (or calling provided functions) and for generating Latex and PDF files.
This file provides functionalities for generating [pgfplots](https://ctan.org/pkg/pgfplots?lang=de) (bar plots, 2D & 3D plots) out of Pandas dataframes containing testing results.
Take a look at file [./py_test_scripts/statistics_and_plot.py](./py_test_scripts/statistics_and_plot.py).

### calc_opt_parameters.py

Used to estimate best performing parameter values based on evaluation results.
Take a look at file [./py_test_scripts/calc_opt_parameters.py](./py_test_scripts/calc_opt_parameters.py).

### evaluation_numbers.py

Holds number of evaluations, test names, folder and data structures, ... for all tests.
Take a look at file [./py_test_scripts/evaluation_numbers.py](./py_test_scripts/evaluation_numbers.py).

### usac_eval.py

Specific filtering and evaluation functions based on already calculated statistics for testing results on [USAC](https://ieeexplore.ieee.org/document/6365642) and comparisons of USAC and vanilla RANSAC within the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library.
Take a look at file [./py_test_scripts/usac_eval.py](./py_test_scripts/usac_eval.py).

### refinement_eval.py

Specific filtering and evaluation functions based on already calculated statistics for testing results of various refinement algorithms and cost functions within the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library.
Take a look at file [./py_test_scripts/refinement_eval.py](./py_test_scripts/refinement_eval.py).

### vfc_gms_sof_eval.py

Specific filtering and evaluation functions based on already calculated statistics for testing results on correspondence filtering methods [GMS](https://github.com/JiawangBian/GMS-Feature-Matcher), [VFC](https://github.com/jiayi-ma/VFC), and [SOF](https://link.springer.com/chapter/10.1007/978-3-319-46478-7_7).
Take a look at file [./py_test_scripts/vfc_gms_sof_eval.py](./py_test_scripts/vfc_gms_sof_eval.py).

### corr_pool_eval.py

Specific filtering and evaluation functions based on already calculated statistics for testing results of various parameter settings on the feature match aggregation functionality of the continuous high accuracy stereo pose estimation and refinement framework of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library.
Take a look at file [./py_test_scripts/corr_pool_eval.py](./py_test_scripts/corr_pool_eval.py).

### robustness_eval.py

Specific filtering and evaluation functions based on already calculated statistics for testing results of various parameter settings on the robustness functionality of the continuous high accuracy stereo pose estimation and refinement framework of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library.
Take a look at file [./py_test_scripts/robustness_eval.py](./py_test_scripts/robustness_eval.py).

### usac_vs_autocalib_eval.py

Specific filtering and evaluation functions based on already calculated statistics for comparisons of [USAC](https://ieeexplore.ieee.org/document/6365642) with simple correspondence aggregation using a windowing approach and the continuous high accuracy stereo pose estimation and refinement framework of the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library.
Take a look at file [./py_test_scripts/usac_vs_autocalib_eval.py](./py_test_scripts/usac_vs_autocalib_eval.py).

### communication.py

Interface for sending SMS in case an error occurred or testing finished:
1. To use the interface, you need a [twilio](https://www.twilio.com/) account.
2. If you have one, call function `gen_ciphered_text` providing your twilio account token and set a password. You can copy your token into the function argument of line 87, change line 85 to `encr = True` and call `cd [cloned_repo_dir]/py_test_scripts && python communication.py`. Don't forget the delete your inserted token later on.
3. Set variable `hash` in line 52 of file [./py_test_scripts/communication.py](./py_test_scripts/communication.py) to the hash value of output `Hash value for later verification: [your_hash_value]`.
4. Set variable `ciphered_text` in line 67 of file [./py_test_scripts/communication.py](./py_test_scripts/communication.py) to the encrypted text of output `Encrypted PW: [your_encrypted_text]`.
5. Set your accound SID in line 74: `client = Client("accountSID", token)`
6. Insert your phone number and your twilio account phone number in lines 79-80 in fields `your_phone_number` and `your_twilio_account_phone_number`. You also need to specify the country code like +1..., +49...
7. To test if it works, change line 85 to `encr = False` and execute `cd [cloned_repo_dir]/py_test_scripts && python communication.py`

### calc_scene_extrinsic_distribution.py

Reads stereo configurations from a generated scene (by [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)) and calculate statistics on their parameters.
Take a look at file [./py_test_scripts/calc_scene_extrinsic_distribution.py](./py_test_scripts/calc_scene_extrinsic_distribution.py).

### recompile_tex_no_author.py

Loads all tex-files within a given directory including all sub-directories, deletes author information and builds PDFs.
If option `--copy_only` is specified, all and only PDF documents are copied to the destination folder while preserving the folder structure.

Call `cd [cloned_repo_dir]/py_test_scripts && python recompile_tex_no_author.py -h` to show options or take a look at file [./py_test_scripts/recompile_tex_no_author.py](./py_test_scripts/recompile_tex_no_author.py).

## Testing Results <a name="results"></a>

Coming soon

## Publication <a name="publication"></a>

Coming soon

<!--
```
@inproceedings{maier2020semireal,
  title={Unlimited Semi-Real-World Ground Truth Generation for Feature-Based Applications},
  author={Maier, Josef},
  booktitle={ACCV},
  year={2020}
}
```
-->
