"""
Released under the MIT License - https://opensource.org/licenses/MIT

Copyright (c) 2020 Josef Maier

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Josef Maier (josefjohann-dot-maier-at-gmail-dot-at)

Description: Generate configuration files and overview files for scenes by varying the used inlier ratio and keypoint accuracy
"""
import sys, re, numpy as np, argparse, os, pandas as pd
#from tabulate import tabulate as tab
#from copy import deepcopy


def gen_configs(input_file_name, img_overlap_range, kp_min_dist_range, kpAccRange, img_path, store_path,
                both_kp_err_types, imgIntNoiseMeanRange, imgIntNoiseStdRange, load_path):
    path, fname = os.path.split(input_file_name)
    base = ''
    ending = ''
    fnObj = re.match(r'(.*)_initial\.(.*)', fname, re.I)
    if fnObj:
        base = fnObj.group(1)
        ending = fnObj.group(2)
    else:
        raise ValueError('Filename must include _initial at the end before the file type')

    #nr_inliers = (inlier_range[1] - inlier_range[0])/inlier_range[2] + 1
    #nr_accs = (kpAccRange[1] - kpAccRange[0]) / kpAccRange[2] + 1

    datac = {'conf_file': [], 'img_path': [], 'img_pref': [], 'store_path': [],
             'scene_exists': [], 'load_path': [], 'parSetNr': []}
    cnt = 0
    if len(img_overlap_range) == 1 or img_overlap_range[1] == 0:
        imr = np.arange(img_overlap_range[0], img_overlap_range[0] + 0.1, 0.2)
    else:
        imr = np.arange(img_overlap_range[0], img_overlap_range[1] + img_overlap_range[2] / 2, img_overlap_range[2])
    if len(kp_min_dist_range) == 1 or kp_min_dist_range[1] == 0:
        kdr = np.arange(kp_min_dist_range[0], kp_min_dist_range[0] + 0.1, 0.2)
    else:
        kdr = np.arange(kp_min_dist_range[0], kp_min_dist_range[1] + kp_min_dist_range[2] / 2, kp_min_dist_range[2])
    if len(kpAccRange) == 1 or kpAccRange[1] == 0:
        kar = np.arange(kpAccRange[0], 0.2, kpAccRange[0] + 0.1)
    else:
        kar = np.arange(kpAccRange[0], kpAccRange[1] + kpAccRange[2] / 2, kpAccRange[2])
    if imgIntNoiseMeanRange is not None and imgIntNoiseStdRange is not None:
        if len(imgIntNoiseMeanRange) == 1 or imgIntNoiseMeanRange[1] == 0:
            inmr = np.arange(imgIntNoiseMeanRange[0], imgIntNoiseMeanRange[0] + 0.1, 0.2)
        else:
            inmr = np.arange(imgIntNoiseMeanRange[0], imgIntNoiseMeanRange[1] + imgIntNoiseMeanRange[2] / 2, imgIntNoiseMeanRange[2])
        if len(imgIntNoiseStdRange) == 1 or imgIntNoiseStdRange[1] == 0:
            insr = np.arange(imgIntNoiseStdRange[0], imgIntNoiseStdRange[0] + 0.1, 0.2)
        else:
            insr = np.arange(imgIntNoiseStdRange[0], imgIntNoiseStdRange[1] + imgIntNoiseStdRange[2] / 2,
                             imgIntNoiseStdRange[2])
    else:
        inmr = None
        insr = None
    for imrv in imr:
        for kdrv in kdr:
            for karv in kar:
                if both_kp_err_types:
                    if inmr is None:
                        raise ValueError('Image noise range must be specified.')
                    for inmrv in inmr:
                        for insrv in insr:
                            fnew = 'imov_%.2f' % imrv
                            fnew += '_kpdist_%.2f' % kdrv
                            fnew += '_kpacc_%.2f' % karv
                            fnew += '_imNoiMe_%.2f' % inmrv
                            fnew += '_imNoiSD_%.2f' % insrv
                            fnew = fnew.replace('.', '_') + '.' + ending
                            pfnew = os.path.join(path, fnew)
                            if os.path.isfile(pfnew):
                                raise ValueError('File ' + pfnew + ' already exists.')
                            write_config_file(input_file_name, pfnew, round(imrv, 2), round(karv / 2, 3),
                                              round(kdrv, 2), 0, round(inmrv), round(insrv))
                            datac['conf_file'].append(pfnew)
                            datac['img_path'].append(img_path)
                            datac['img_pref'].append('/')
                            datac['store_path'].append(store_path)
                            datac['scene_exists'].append(0)
                            datac['load_path'].append(load_path)
                            datac['parSetNr'].append(cnt)
                            cnt = cnt + 1
                    fnew = 'imov_%.2f' % imrv
                    fnew += '_kpdist_%.2f' % kdrv
                    fnew += '_kpacc_%.2f' % karv
                    fnew = fnew.replace('.', '_') + '.' + ending
                    pfnew = os.path.join(path, fnew)
                    if os.path.isfile(pfnew):
                        raise ValueError('File ' + pfnew + ' already exists.')
                    write_config_file(input_file_name, pfnew, round(imrv, 2), round(karv / 2, 3),
                                      round(kdrv, 2), 1, None, None)
                    datac['conf_file'].append(pfnew)
                    datac['img_path'].append(img_path)
                    datac['img_pref'].append('/')
                    datac['store_path'].append(store_path)
                    datac['scene_exists'].append(0)
                    datac['load_path'].append(load_path)
                    datac['parSetNr'].append(cnt)
                    cnt = cnt + 1
                elif inmr is not None:
                    for inmrv in inmr:
                        for insrv in insr:
                            fnew = 'imov_%.2f' % imrv
                            fnew += '_kpdist_%.2f' % kdrv
                            fnew += '_kpacc_%.2f' % karv
                            fnew += '_imNoiMe_%.2f' % inmrv
                            fnew += '_imNoiSD_%.2f' % insrv
                            fnew = fnew.replace('.', '_') + '.' + ending
                            pfnew = os.path.join(path, fnew)
                            if os.path.isfile(pfnew):
                                raise ValueError('File ' + pfnew + ' already exists.')
                            write_config_file(input_file_name, pfnew, round(imrv, 2), round(karv / 2, 3),
                                              round(kdrv, 2), 0, round(inmrv), round(insrv))
                            datac['conf_file'].append(pfnew)
                            datac['img_path'].append(img_path)
                            datac['img_pref'].append('/')
                            datac['store_path'].append(store_path)
                            datac['scene_exists'].append(0)
                            datac['load_path'].append(load_path)
                            datac['parSetNr'].append(cnt)
                            cnt = cnt + 1
                else:
                    fnew = 'imov_%.2f' % imrv
                    fnew += '_kpdist_%.2f' % kdrv
                    fnew += '_kpacc_%.2f' % karv
                    fnew = fnew.replace('.', '_') + '.' + ending
                    pfnew = os.path.join(path, fnew)
                    if os.path.isfile(pfnew):
                        raise ValueError('File ' + pfnew + ' already exists.')
                    write_config_file(input_file_name, pfnew, round(imrv, 2), round(karv / 2, 3),
                                      round(kdrv, 2), 1, None, None)
                    datac['conf_file'].append(pfnew)
                    datac['img_path'].append(img_path)
                    datac['img_pref'].append('/')
                    datac['store_path'].append(store_path)
                    datac['scene_exists'].append(0)
                    datac['load_path'].append(load_path)
                    datac['parSetNr'].append(cnt)
                    cnt = cnt + 1

    df = pd.DataFrame(data=datac)
    ov_file = os.path.join(path, 'config_files.csv')
    df.to_csv(index=True, sep=';', path_or_buf=ov_file)
    #cf = pandas.read_csv(input_file_name, delimiter=';')
    return 0


def write_config_file(finput, foutput, imoverlap, kpacc, kpdist, kperrType, noiseMean, noiseSD):
    #kperrType ... 0 error distribution, 1 repeatability error
    with open(foutput, 'w') as fo:
        with open(finput, 'r') as fi:
            li = fi.readline()
            foundi = 0
            founda = 0
            while li:
                ovlap_obj = re.match(r'(\s*imageOverlap:(?:\s*))[0-9.]+', li)
                if ovlap_obj:
                    fo.write(ovlap_obj.group(1) + str(imoverlap) + '\n')
                    li = fi.readline()
                    continue
                kpdist_obj = re.match(r'(\s*minKeypDist:(?:\s*))[0-9.]+', li)
                if kpdist_obj:
                    fo.write(kpdist_obj.group(1) + str(kpdist) + '\n')
                    li = fi.readline()
                    continue
                kperrt_obj = re.match(r'(\s*keypPosErrType:(?:\s*))[0-9]+', li)
                if kperrt_obj:
                    fo.write(kperrt_obj.group(1) + str(int(kperrType)) + '\n')
                    li = fi.readline()
                    continue
                aobj = re.search('keypErrDistr:', li)
                if kperrType == 0:
                    imNoise_obj = re.search('imgIntNoise:', li)
                else:
                    imNoise_obj = None
                if imNoise_obj:
                    foundi = 1
                    fo.write(li)
                elif aobj:
                    founda = 1
                    fo.write(li)
                elif foundi == 1:
                    if noiseMean is None:
                        fo.write(li)
                        foundi = 2
                    else:
                        lobj = re.match(r'(\s*first:(?:\s*))[0-9.]+', li)
                        if lobj:
                            fo.write(lobj.group(1) + str(noiseMean) + '\n')
                            foundi = 2
                        else:
                            raise  ValueError('Unable to write first line of imgIntNoise')
                elif foundi == 2:
                    if noiseMean is None:
                        fo.write(li)
                        foundi = 0
                    else:
                        lobj = re.match(r'(\s*second:(?:\s*))[0-9.]+', li)
                        if lobj:
                            fo.write(lobj.group(1) + str(noiseSD) + '\n')
                            foundi = 0
                        else:
                            raise ValueError('Unable to write second line of imgIntNoise')
                elif founda == 1:
                    aobj = re.match(r'(\s*first:(?:\s*))[0-9.]+', li)
                    if aobj:
                        #fo.write(aobj.group(1) + str(acc) + '\n')
                        fo.write(li)
                        founda = 2
                    else:
                        raise ValueError('Unable to write first line of keypoint accuracy')
                elif founda == 2:
                    aobj = re.match(r'(\s*second:(?:\s*))[0-9.]+', li)
                    if aobj:
                        fo.write(aobj.group(1) + str(kpacc) + '\n')
                        founda = 0
                    else:
                        raise ValueError('Unable to write second line of keypoint accuracy')
                else:
                    fo.write(li)
                li = fi.readline()


def main():
    parser = argparse.ArgumentParser(description='Generate configuration files and overview files for scenes by '
                                                 'varying the used inlier ratio and keypoint accuracy')
    parser.add_argument('--filename', type=str, required=True,
                        help='Path and filename of the template configuration file')
    parser.add_argument('--img_overlap_range', type=float, nargs='+', required=True,
                        help='Range for the desired image overlap between stereo frames. Format: min max step_size')
    parser.add_argument('--kp_min_dist_range', type=float, nargs='+', required=True,
                        help='Range for the minimal distance between keypoints in the image plane. '
                             'Format: min max step_size')
    parser.add_argument('--kpAccRange', type=float, nargs='+', required=True,
                        help='Range for the keypoint accuracy. The entered value is devided by 2 ro reach an '
                             'approximate maximum keypoint accuracy based on the given value as the value in '
                             'the configuration file corresponds to the standard deviation. '
                             'Format: min max step_size')
    parser.add_argument('--imgIntNoiseMeanRange', type=float, nargs='+', required=False, default=None,
                        help='Range for the mean gaussian intensity noise applied to image patches. '
                             'Format: min max step_size')
    parser.add_argument('--imgIntNoiseStdRange', type=float, nargs='+', required=False, default=None,
                        help='Range for the standard deviation of gaussian intensity noise applied to image patches. '
                             'Format: min max step_size')
    parser.add_argument('--both_kp_err_types', type=bool, nargs='?', required=False, default=False, const=True,
                        help='If provided, both keypoint location error models (based on keypoint repeatability error '
                             'and parameter kpAccRange) are used.')
    parser.add_argument('--img_path', type=str, required=True,
                        help='Path to images')
    parser.add_argument('--store_path', type=str, required=True,
                        help='Storing path for generated scenes and matches')
    parser.add_argument('--load_path', type=str, required=False,
                        help='Optional loading path for generated scenes and matches. '
                             'If not provided, store_path is used.')
    args = parser.parse_args()
    if not os.path.isfile(args.filename):
        raise ValueError('File ' + args.filename + ' holding scene configuration file names does not exist')
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
            elif not round(float((args.img_overlap_range[1] - args.img_overlap_range[0]) / args.img_overlap_range[2]),
                           6).is_integer():
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
            elif not round(float((args.kp_min_dist_range[1] - args.kp_min_dist_range[0]) / args.kp_min_dist_range[2]),
                           6).is_integer():
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
        elif not round(
                float((args.imgIntNoiseMeanRange[1] - args.imgIntNoiseMeanRange[0]) / args.imgIntNoiseMeanRange[2]),
                6).is_integer():
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
        elif not round(float((args.imgIntNoiseStdRange[1] - args.imgIntNoiseStdRange[0]) / args.imgIntNoiseStdRange[2]),
                       6).is_integer():
            raise ValueError("imgIntNoiseStdRange step size is wrong")
    if not os.path.exists(args.img_path):
        raise ValueError("Image path does not exist")
    if not os.path.exists(args.store_path):
        raise ValueError("Path for storing sequences does not exist")
    if len(os.listdir(args.store_path)) != 0:
        raise ValueError("Path for storing sequences is not empty")
    if args.load_path:
        if not os.path.exists(args.load_path):
            raise ValueError("Path for loading sequences does not exist")
    else:
        args.load_path = args.store_path
    ret = gen_configs(args.filename, args.img_overlap_range, args.kp_min_dist_range, args.kpAccRange, args.img_path, args.store_path,
                      args.both_kp_err_types, args.imgIntNoiseMeanRange, args.imgIntNoiseStdRange, args.load_path)
    sys.exit(ret)


if __name__ == "__main__":
    main()

