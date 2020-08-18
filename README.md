# Testing Framework for the Pose Estimation Pipeline using GT Data from SemiRealSequence

- [Introduction](#introduction)
- [Dependencies](#dependencies)
- [Usage](#usage)
- [Python File Descriptions](#files)
- [Testing Results](#results)
- [Publication](#publication)

## Introduction <a name="introduction"></a>

This framework is used to generate multiple semi-real GT datasets with [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) (a not installable version is included in this repository within [./generateVirtualSequence](./generateVirtualSequence)) which are further used to test various relative pose estimation and refinement algorithms from the [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib) library (a not installable version is included in this repository within [./matchinglib_poselib](./matchinglib_poselib)).

[SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) is used to generate 3D scenes and sparse feature matches with specific properties like depth ranges of 3D point clouds, number of inliers, inlier ratios, spatial distributions of correspondences within the 2D image domain, keypoint repeatability errors, linear and abrupt changes in different GT camera pose parameters, ...

For each of afore mentioned scene properties, parameter sweeps are performed on [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) to find advantages and disadvantages of different pose estimation and refinement algorithms in addition to combinations thereof.

Branch [ngransac](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac) also includes functionality for testing [NG-RANSAC](https://github.com/vislearn/ngransac).

For a detailed description of the used GT-generation framework take a look at [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence).

For a detailed description of the used pose estimation and refinement algorithms take a look at [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib).

## Dependencies <a name="dependencies"></a>

As all our evaluations were performed on [AWS](https://aws.amazon.com/), this framework is optimized for [Docker](https://docs.docker.com/get-docker/) usage.
You can build a Docker image by cloning this repository and calling `./build_docker_base.sh`.

## Usage <a name="usage"></a>

Evaluations were performed in the following order:
1. Parameter sweep on USAC parameters
2. Comparison of [USAC](https://ieeexplore.ieee.org/document/6365642), vanilla RANSAC, and [NG-RANSAC](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac)
3. Comparison of various pose refinement algorithms and cost functions using estimated poses from USAC
4. Evaluation on correspondence filtering methods [GMS](https://github.com/JiawangBian/GMS-Feature-Matcher), [VFC](https://github.com/jiayi-ma/VFC), and [SOF](https://link.springer.com/chapter/10.1007/978-3-319-46478-7_7)
5. Comparison of various pose refinement algorithms and cost functions on aggregated feature correspondences using the continuous high accuracy stereo pose estimation and refinement functionality of [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)
6. Comparison of various parameter settings on the feature match aggregation functionality of the continuous high accuracy stereo pose estimation and refinement framework of [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)
7. Parameter sweep on robustness parameters of the continuous high accuracy stereo pose estimation and refinement framework of [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)
8. Comparison of USAC with simple correspondence aggregation using a windowing approach and the continuous high accuracy stereo pose estimation and refinement framework of [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)

For each of afore mentioned evaluations, best performing parameter sets were used for the next evaluation (e.g. USAC parameters were fixed for evaluations on refinement algorithms) to reduce parameter space.

To start testing (all evaluations) perform the following steps:
1. Create a folder `res_save_compressed` outside of this repository's main directory (`../res_save_compressed`)
2. Create a folder `results` outside of this repository's main directory (`../results`) or provide option `RESDIR <your_directory>`  to script file `run_docker_base.sh` as second argument
3. Execute `./run_docker_base.sh live` in the main directory of this repository

The first argument of script file `run_docker_base.sh` must be `live` or `shutdown`.
The latter shuts the system down after evaluations are finished or if an error occurred.

All errors are logged to disk.
Evaluations can be resumed at any point.
For additional options run `./run_docker_base.sh live -h`

## Python File Descriptions <a name="files"></a>

Python files used for testing are located within [./py_test_scripts](./py_test_scripts).
Every Python file offers its own command line interface for independent usage.

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
6. Call `cd [cloned_repo_dir]/py_test_scripts && python gen_init_scenes.py -h` to show necessary options
7. Call `cd [cloned_repo_dir]/py_test_scripts && python gen_init_scenes.py [options]` to start generating initial sequences
8. Use Python file [extract_Rt.py](#extract_Rt) to copy calculated relative poses into final initial configuration files

### extract_Rt.py <a name="extract_Rt"></a>

Copies all stereo configurations from a generated scene (by [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence)) into a configuration file.
Python file [gen_init_scenes.py](#gen_init_scenes) must be used in advance for generating scenes.

Call `cd [cloned_repo_dir]/py_test_scripts && python extract_Rt.py -h` to show necessary options.

### change_distCamMat_config.py

Changes configuration file parameter `distortCamMat` for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) within all configuration files for which the file name in a given directory contains the pattern `*_initial.[extension]`.

Call `cd [cloned_repo_dir]/py_test_scripts && python change_distCamMat_config.py -h` to show options.

### change_keyp_distr_config.py

Changes configuration file parameter `corrsPerRegion` for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) within all configuration files for which the file name in a given directory contains the pattern `*_initial.[extension]`.
Used parameter values must be edited within [./py_test_scripts/change_keyp_distr_config.py](./py_test_scripts/change_keyp_distr_config.py)

Call `cd [cloned_repo_dir]/py_test_scripts && python change_keyp_distr_config.py -h` to show options.

### change_nrTP_config.py

Changes configuration file parameter `truePosRange` for [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) within all configuration files for which the file name in a given directory contains the pattern `[.*_kp-distr-(?:(?:half-img)|(?:1corn)|(?:equ))_depth-(?:F|(?:NM)|(?:NMF))_TP-([0-9to]+).*]_initial.[extension]`.
The values for `truePosRange` are extracted from the file name using the regular expression shown above within brackets `[...]`.

Call `cd [cloned_repo_dir]/py_test_scripts && python change_nrTP_config.py -h` to show options.

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
