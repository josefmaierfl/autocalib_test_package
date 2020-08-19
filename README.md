# Framework for Generating Ground Truth Sparse Feature Correspondences, 3D Data and Camera Poses using SemiRealSequence and Conversion into NG-RANSAC Training Format

- [Introduction](#introduction)
- [Dependencies](#dependencies)
- [Usage](#usage)
- [Publication](#publication)

## Introduction <a name="introduction"></a>

This framework provides functionality to generate GT data (sparse feature correspondences, poses, ...) using [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) for training and testing [NG-RANSAC](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac).

For details on how to use [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence), see [here](https://github.com/josefmaierfl/SemiRealSequence).

For details on how to train and test [NG-RANSAC](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac), see [branch ngransac](https://github.com/josefmaierfl/autocalib_test_package/tree/ngransac).

For details on how to convert data from SemiRealSequence into NG-RANSAC training format, take a look at [./ngransac_prepare/readSequMatches.py](./ngransac_prepare/readSequMatches.py).
For available options call `cd [cloned_repo_dir]/ngransac_prepare && python readSequMatches.py -h`.

For an example on how to create bulk configuration files, take a look at files [./ngransac_prepare/gen_mult_scene_configs.py](./ngransac_prepare/gen_mult_scene_configs.py) and [./ngransac_prepare/gen_scene_configs.py](./ngransac_prepare/gen_scene_configs.py).

## Dependencies <a name="dependencies"></a>

As all our evaluations were performed on [AWS](https://aws.amazon.com/), this framework is optimized for [Docker](https://docs.docker.com/get-docker/) usage.
You can build a Docker image by cloning this repository and calling `./build_docker_base.sh`.

## Usage <a name="usage"></a>

To start generating data:
1. Create a folder `res_save_compressed` outside of this repository's main directory (`../res_save_compressed`)
2. Create a folder `results_train` outside of this repository's main directory (`../results_train`) or provide option `RESDIR <your_relative_directory>` (relative directory to `[cloned_repo_dir]/..`) to script file `run_docker_base.sh` as second argument
3. Copy images and/or datasets used within [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) to directory `[cloned_repo_dir]/images` as described [here](https://github.com/josefmaierfl/SemiRealSequence#image-folder)
4. Edit scene parameter sweep values within function `get_config_file_parameters()` (line 30) of file [./ngransac_prepare/main.py](./ngransac_prepare/main.py).
Format for parameter sweeps: `[first_parameter_value last_including_parameter_value step_size]`.
Additional parameters of [SemiRealSequence configuration files](https://github.com/josefmaierfl/SemiRealSequence#config-file) (which are currently not supported within this framework) can easily by added to files [./ngransac_prepare/gen_mult_scene_configs.py](./ngransac_prepare/gen_mult_scene_configs.py) and [./ngransac_prepare/gen_scene_configs.py](./ngransac_prepare/gen_scene_configs.py)
5. Execute `./run_docker_base.sh [live/shutdown] [RESDIR <your_relative_directory>]` to start generating data

The first argument of script file `run_docker_base.sh` must be `live` or `shutdown`.
The latter shuts the system down after data generation is finished or if an error occurred.

All errors are logged to disk.
Data generation can be resumed at any point as long as processes were not killed by the user.
For additional options run `./run_docker_base.sh live -h`

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
