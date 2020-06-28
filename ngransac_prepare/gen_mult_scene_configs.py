"""
Loads initial configuration files and builds a folder structure and an overview file for generating
specific configuration and overview files for scenes by varying the used inlier ratio and keypoint accuracy
"""
import sys, re, numpy as np, argparse, os, subprocess as sp, warnings
from shutil import copyfile


def gen_configs(input_path, path_confs_out, img_overlap_range, kp_min_dist_range, kpAccRange, img_path, store_path,
                both_kp_err_types, imgIntNoiseMeanRange, imgIntNoiseStdRange):
    #Load file names
    files_i = os.listdir(input_path)
    if len(files_i) == 0:
        raise ValueError('No files found.')
    files = []
    for i in files_i:
        fnObj = re.search(r'_initial\.', i, re.I)
        if fnObj:
            files.append(i)
    if len(files) == 0:
        raise ValueError('No files including _init found.')
    if len(files) < 4:
        if len(img_overlap_range) == 1 or img_overlap_range[1] == 0:
            imr = np.arange(img_overlap_range[0], 0.2, img_overlap_range[0] + 0.1)
        else:
            imr = np.arange(img_overlap_range[0], img_overlap_range[1] + img_overlap_range[2] / 2, img_overlap_range[2])
        if len(kp_min_dist_range) == 1 or kp_min_dist_range[1] == 0:
            kdr = np.arange(kp_min_dist_range[0], 0.2, kp_min_dist_range[0] + 0.1)
        else:
            kdr = np.arange(kp_min_dist_range[0], kp_min_dist_range[1] + kp_min_dist_range[2] / 2, kp_min_dist_range[2])
    else:
        imr = np.array([0])
        kdr = np.array([0])

    ovfile_new = os.path.join(path_confs_out, 'generated_dirs_config.txt')
    if os.path.exists(ovfile_new):
        raise FileExistsError("File containing configuration file names already exists: " + ovfile_new)
    with open(ovfile_new, 'w') as fo:
        for i in files:
            filen = os.path.basename(i)
            fnObj = re.match(r'(.*)_initial\.(.*)', filen, re.I)
            if fnObj:
                base = fnObj.group(1)
                ending = fnObj.group(2)
            else:
                raise ValueError('Cannot extract part of file name')
            for imrv in imr:
                if imrv != 0:
                    base1 = base + '_imov_%.2f' % imrv
                else:
                    base1 = base
                for kdrv in kdr:
                    if kdrv != 0:
                        base2 = base1 + '_kpdist_%.2f' % kdrv
                    else:
                        base2 = base1
                    pathnew = os.path.join(path_confs_out, base2)
                    try:
                        os.mkdir(pathnew)
                    except FileExistsError:
                        raise ValueError('Directory ' + pathnew + ' already exists')
                    #Copy init file
                    try:
                        filenew = os.path.join(pathnew, filen)
                        copyfile(os.path.join(input_path, i), filenew)
                    except IOError:
                        raise ValueError('Unable to copy template file')
                    #Generate new store path
                    store_path_new = os.path.join(store_path, base2)
                    try:
                        os.mkdir(store_path_new)
                    except FileExistsError:
                        raise ValueError('Directory ' + store_path_new + ' already exists')
                    #Generate new config files
                    pyfilepath = os.path.dirname(os.path.realpath(__file__))
                    pyfilename = os.path.join(pyfilepath, 'gen_scene_configs.py')
                    #pyfilename = 'gen_scene_configs.py'
                    try:
                        cmdline = ['python', pyfilename, '--filename', filenew]
                        if imrv != 0:
                            cmdline += ['--img_overlap_range', '%.2f' % imrv]
                        else:
                            if len(img_overlap_range) == 1:
                                cmdline += ['--img_overlap_range', '%.2f' % img_overlap_range[0]]
                            else:
                                cmdline += ['--img_overlap_range', '%.2f' % img_overlap_range[0], '%.2f' % img_overlap_range[1],
                                            '%.2f' % img_overlap_range[2]]
                        if kdrv != 0:
                            cmdline += ['--kp_min_dist_range', '%.2f' % kdrv]
                        else:
                            if len(kp_min_dist_range) == 1:
                                cmdline += ['--kp_min_dist_range', '%.2f' % kp_min_dist_range[0]]
                            else:
                                cmdline += ['--kp_min_dist_range', '%.2f' % kp_min_dist_range[0], '%.2f' % kp_min_dist_range[1],
                                            '%.2f' % kp_min_dist_range[2]]
                        if len(kpAccRange) == 1:
                            cmdline += ['--kpAccRange', '%.2f' % kpAccRange[0]]
                        else:
                            cmdline += ['--kpAccRange', '%.2f' % kpAccRange[0], '%.2f' % kpAccRange[1],
                                        '%.2f' % kpAccRange[2]]
                        if len(imgIntNoiseMeanRange) == 1:
                            cmdline += ['--imgIntNoiseMeanRange', '%.2f' % imgIntNoiseMeanRange[0]]
                        else:
                            cmdline += ['--imgIntNoiseMeanRange', '%.2f' % imgIntNoiseMeanRange[0],
                                        '%.2f' % imgIntNoiseMeanRange[1], '%.2f' % imgIntNoiseMeanRange[2]]
                        if len(imgIntNoiseStdRange) == 1:
                            cmdline += ['--imgIntNoiseStdRange', '%.2f' % imgIntNoiseStdRange[0]]
                        else:
                            cmdline += ['--imgIntNoiseStdRange', '%.2f' % imgIntNoiseStdRange[0],
                                        '%.2f' % imgIntNoiseStdRange[1], '%.2f' % imgIntNoiseStdRange[2]]
                        if both_kp_err_types:
                            cmdline += ['--both_kp_err_types']
                        cmdline += ['--img_path', img_path,
                                    '--store_path', store_path_new]
                        retcode = sp.run(cmdline, shell=False, check=True).returncode
                        if retcode < 0:
                            print("Child was terminated by signal", -retcode, file=sys.stderr)
                        else:
                            print("Child returned", retcode, file=sys.stderr)
                    except OSError as e:
                        print("Execution failed:", e, file=sys.stderr)
                    except sp.CalledProcessError as e:
                        print("Execution failed:", e, file=sys.stderr)
                    #Delete copied template file
                    os.remove(filenew)
                    #Write directory name holding config files to overview file
                    fo.write(pathnew + '\n')
    return 0


def main():
    parser = argparse.ArgumentParser(description='Generate configuration files and overview files for scenes by '
                                                 'varying the used inlier ratio and keypoint accuracy')
    parser.add_argument('--path', type=str, required=True,
                        help='Directory holding template configuration files')
    parser.add_argument('--path_confs_out', type=str, required=False,
                        help='Optional directory for writing configuration files. If not available, '
                             'the directory from argument \'path\' is used.')
    parser.add_argument('--img_overlap_range', type=float, nargs='+', required=False,
                        help='Range for the desired image overlap between stereo frames. Format: min max step_size')
    parser.add_argument('--kp_min_dist_range', type=float, nargs='+', required=False,
                        help='Range for the minimal distance between keypoints in the image plane. '
                             'Format: min max step_size')
    parser.add_argument('--kpAccRange', type=float, nargs='+', required=False,
                        help='Range for the keypoint accuracy. The entered value is devided by 2 ro reach an '
                             'approximate maximum keypoint accuracy based on the given value as the value in '
                             'the configuration file corresponds to the standard deviation. '
                             'Format: min max step_size')
    parser.add_argument('--imgIntNoiseMeanRange', type=float, nargs='+', required=False,
                        help='Range for the mean gaussian intensity noise applied to image patches. '
                             'Format: min max step_size')
    parser.add_argument('--imgIntNoiseStdRange', type=float, nargs='+', required=False,
                        help='Range for the standard deviation of gaussian intensity noise applied to image patches. '
                             'Format: min max step_size')
    parser.add_argument('--both_kp_err_types', type=bool, nargs='?', required=False, default=False, const=True,
                        help='If provided, both keypoint location error models (based on keypoint repeatability error '
                             'and parameter kpAccRange) are used.')
    parser.add_argument('--img_path', type=str, required=True,
                        help='Path to images')
    parser.add_argument('--store_path', type=str, required=True,
                        help='Storing path for generated scenes and matches')
    args = parser.parse_args()
    if not os.path.exists(args.path):
        raise ValueError('Directory ' + args.path + ' holding template scene configuration files does not exist')
    if not args.path_confs_out:
        args.path_confs_out = args.path
    elif not os.path.exists(args.path_confs_out):
        os.makedirs(args.path_confs_out, exist_ok=True)
    if args.img_overlap_range:
        if len(args.img_overlap_range) != 3 and len(args.img_overlap_range) != 1:
            raise ValueError("Only 1 or 3 image overlap parameters are accepted.")
        if len(args.img_overlap_range) == 3:
            if (args.img_overlap_range[0] > args.img_overlap_range[1]) or \
                    (args.img_overlap_range[2] > (args.img_overlap_range[1] - args.img_overlap_range[0])):
                raise ValueError("Image overlap parameters must have the following format: "
                                 "range_min range_max step_size")
            if args.img_overlap_range[2] == 0 and args.img_overlap_range[0] != args.img_overlap_range[1]:
                raise ValueError("Image overlap parameters are wrong")
            elif not round(float((args.img_overlap_range[1] - args.img_overlap_range[0]) / args.img_overlap_range[2]), 6).is_integer():
                raise ValueError("Image overlap step size is wrong")
    if args.kp_min_dist_range:
        if len(args.kp_min_dist_range) != 3 and len(args.kp_min_dist_range) != 1:
            raise ValueError("Only 1 or 3 minimum keypoint distance parameters are accepted.")
        if len(args.img_overlap_range) == 3:
            if (args.kp_min_dist_range[0] > args.kp_min_dist_range[1]) or \
                    (args.kp_min_dist_range[2] > (args.kp_min_dist_range[1] - args.kp_min_dist_range[0])):
                raise ValueError("Minimum keypoint distance parameters must have the following format: "
                                 "range_min range_max step_size")
            if args.kp_min_dist_range[2] == 0 and args.kp_min_dist_range[0] != args.kp_min_dist_range[1]:
                raise ValueError("Minimum keypoint distance parameters are wrong")
            elif not round(float((args.kp_min_dist_range[1] - args.kp_min_dist_range[0]) / args.kp_min_dist_range[2]), 6).is_integer():
                raise ValueError("Minimum keypoint distance step size is wrong")
    if args.kpAccRange:
        if len(args.kpAccRange) != 3 and len(args.kpAccRange) != 1:
            raise ValueError("Only 1 or 3 keypoint accuracy parameters are accepted.")
        if (args.kpAccRange[0] > args.kpAccRange[1]) or \
                (args.kpAccRange[2] > (args.kpAccRange[1] - args.kpAccRange[0])):
            raise ValueError("Keypoint accuracy parameters must have the following format: "
                             "range_min range_max step_size")
        if args.kpAccRange[2] == 0 and args.kpAccRange[0] != args.kpAccRange[1]:
            raise ValueError("Minimum keypoint distance parameters are wrong")
        elif not round(float((args.kpAccRange[1] - args.kpAccRange[0]) / args.kpAccRange[2]), 6).is_integer():
            raise ValueError("Keypoint accuracy step size is wrong")
    if args.imgIntNoiseMeanRange:
        if len(args.imgIntNoiseMeanRange) != 3 and len(args.imgIntNoiseMeanRange) != 1:
            raise ValueError("Only 1 or 3 imgIntNoiseMeanRange parameters are accepted.")
        if (args.imgIntNoiseMeanRange[0] > args.imgIntNoiseMeanRange[1]) or \
                (args.imgIntNoiseMeanRange[2] > (args.imgIntNoiseMeanRange[1] - args.imgIntNoiseMeanRange[0])):
            raise ValueError("imgIntNoiseMeanRange parameters must have the following format: "
                             "range_min range_max step_size")
        if args.imgIntNoiseMeanRange[2] == 0 and args.imgIntNoiseMeanRange[0] != args.imgIntNoiseMeanRange[1]:
            raise ValueError("imgIntNoiseMeanRange parameters are wrong")
        elif not round(float((args.imgIntNoiseMeanRange[1] - args.imgIntNoiseMeanRange[0]) / args.imgIntNoiseMeanRange[2]), 6).is_integer():
            raise ValueError("imgIntNoiseMeanRange step size is wrong")
    if args.imgIntNoiseStdRange:
        if len(args.imgIntNoiseStdRange) != 3 and len(args.imgIntNoiseStdRange) != 1:
            raise ValueError("Only 1 or 3 imgIntNoiseStdRange parameters are accepted.")
        if (args.imgIntNoiseStdRange[0] > args.imgIntNoiseStdRange[1]) or \
                (args.imgIntNoiseStdRange[2] > (args.imgIntNoiseStdRange[1] - args.imgIntNoiseStdRange[0])):
            raise ValueError("imgIntNoiseStdRange parameters must have the following format: "
                             "range_min range_max step_size")
        if args.imgIntNoiseStdRange[2] == 0 and args.imgIntNoiseStdRange[0] != args.imgIntNoiseStdRange[1]:
            raise ValueError("imgIntNoiseStdRange parameters are wrong")
        elif not round(float((args.imgIntNoiseStdRange[1] - args.imgIntNoiseStdRange[0]) / args.imgIntNoiseStdRange[2]), 6).is_integer():
            raise ValueError("imgIntNoiseStdRange step size is wrong")
    if not os.path.exists(args.img_path):
        raise ValueError("Image path does not exist")
    if not os.path.exists(args.store_path):
        raise ValueError("Path for storing sequences does not exist")
    try:
        ret = gen_configs(args.path, args.path_confs_out, args.img_overlap_range, args.kp_min_dist_range, args.kpAccRange, args.img_path,
                          args.store_path, args.both_kp_err_types, args.imgIntNoiseMeanRange, args.imgIntNoiseStdRange)
    except FileExistsError:
        warnings.warn(sys.exc_info()[0], UserWarning)
        sys.exit(1)
    except:
        print("Unexpected error: ", sys.exc_info()[0])
        raise
    sys.exit(ret)


if __name__ == "__main__":
    main()

