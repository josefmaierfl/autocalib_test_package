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

For each of afore mentioned sce properties, parameter sweeps are performed on [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence) to find advantages and disadvantages of different pose estimation and refinement algorithms in addition to combinations thereof.

For a detailed description of the used GT-generation framework take a look at [SemiRealSequence](https://github.com/josefmaierfl/SemiRealSequence).

For a detailed description of the used pose estimation and refinement algorithms take a look at [MATCHING- AND POSELIB](https://github.com/josefmaierfl/matchinglib_poselib).

## Dependencies

As all our evaluations were performed on [AWS](https://aws.amazon.com/), this framework is optimized for [Docker](https://docs.docker.com/get-docker/) usage.
You can build a Docker image by cloning this repository and calling `./build_docker_base.sh`.
