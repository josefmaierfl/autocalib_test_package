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

Description: Reads matches of a given directory
"""
import sys, numpy as np, argparse, os, warnings, math, time, psutil, gzip, logging
#import mgzip
import ruamel.yaml as yaml
from ruamel.yaml import CLoader as Loader
import cv2
from xml.etree import cElementTree as ElementTree
import multiprocessing
import multiprocessing.pool
import threading


class NoDaemonProcess(multiprocessing.Process):
    # make 'daemon' attribute always return False
    def _get_daemon(self):
        return False
    def _set_daemon(self, value):
        pass
    daemon = property(_get_daemon, _set_daemon)


# We sub-class multiprocessing.pool.Pool instead of multiprocessing.Pool
# because the latter is only a wrapper function, not a proper class.
class MyPool(multiprocessing.pool.Pool):
    Process = NoDaemonProcess


def configure_logging(message_path, base_name, already_set=False):
    err_trace_base_name = base_name
    base = err_trace_base_name
    excmess = os.path.join(message_path, base + '.txt')
    cnt = 1
    while os.path.exists(excmess):
        base = err_trace_base_name + '_-_' + str(cnt)
        excmess = os.path.join(message_path, base + '.txt')
        cnt += 1
    if already_set:
        global fileh
        fileh = logging.FileHandler(excmess, 'a')
        fileh.setLevel(level=logging.DEBUG)
        log = logging.getLogger()  # root logger
        for hdlr in log.handlers[:]:  # remove all old handlers
            hdlr.close()
            log.removeHandler(hdlr)
        log.addHandler(fileh)  # set the new handler
    else:
        logging.basicConfig(filename=excmess, level=logging.DEBUG)
        log = logging.getLogger()  # root logger
    return excmess, log


def get_list_from_str(value):
    if isinstance(value, str):
        tmp = value.strip()
        if ' ' in tmp or '\n' in tmp:
            tmp1 = tmp.split()
            for i, elem in enumerate(tmp1):
                tmp1[i] = convertTInitType(elem)
                if i > 0 and type(tmp1[i]) != type(tmp1[i - 1]):
                    if not ((isinstance(tmp1[i], float) or isinstance(tmp1[i], int)) and
                            (isinstance(tmp1[i - 1], float) or isinstance(tmp1[i - 1], int))):
                        return value
            return tmp1
        else:
            return tmp
    else:
        return value


def convertTInitType(value):
    if isinstance(value, str):
        if '"' in value:
            return get_list_from_str(value.replace('"', ''))
        else:
            try:
                tmp = int(value)
                return tmp
            except ValueError:
                try:
                    tmp = float(value)
                    return tmp
                except ValueError:
                    return get_list_from_str(value)
    else:
        return value


class XmlListConfig(list):
    def __init__(self, aList):
        for element in aList:
            if element:
                # treat like dict
                if len(element) == 1 or element[0].tag != element[1].tag:
                    aDict = XmlDictConfig(element)
                    if 'type_id' in aDict and aDict['type_id'] == 'opencv-matrix':
                        mat = np.array(aDict["data"])
                        # mat = np.fromstring(aDict["data"], dtype=float, sep=' ')
                        mat.resize(aDict["rows"], aDict["cols"])
                        self.append(mat)
                    else:
                        self.append(aDict)
                # treat like list
                elif element[0].tag == element[1].tag:
                    self.append(XmlListConfig(element))
            elif element.text:
                text = element.text.strip()
                if text:
                    self.append(convertTInitType(text))


class XmlDictConfig(dict):
    '''
    Example usage:

    >>> tree = ElementTree.parse('your_file.xml')
    >>> root = tree.getroot()
    >>> xmldict = XmlDictConfig(root)

    Or, if you want to use an XML string:

    >>> root = ElementTree.XML(xml_string)
    >>> xmldict = XmlDictConfig(root)

    And then use xmldict for what it is... a dict.
    '''
    def __init__(self, parent_element):
        if parent_element.items():
            self.update(dict(parent_element.items()))
        for element in parent_element:
            if element:
                # treat like dict - we assume that if the first two tags
                # in a series are different, then they are all different.
                is_list = False
                if len(element) == 1 or element[0].tag != element[1].tag:
                    aDict = XmlDictConfig(element)
                # treat like list - we assume that if the first two tags
                # in a series are the same, then the rest are the same.
                else:
                    # here, we put the list in dictionary; the key is the
                    # tag name the list elements all share in common, and
                    # the value is the list itself
                    if element[0].tag == '_':
                        aDict = XmlListConfig(element)
                        is_list = True
                    else:
                        aDict = {element[0].tag: XmlListConfig(element)}
                # if the tag has attributes, add those to the dict
                if element.items():
                    aDict.update(dict(element.items()))
                # Check for OpenCV matrices
                if not is_list and 'type_id' in aDict and aDict['type_id'] == 'opencv-matrix':
                    mat = np.array(aDict["data"])
                    # mat = np.fromstring(aDict["data"], dtype=float, sep=' ')
                    mat.resize(aDict["rows"], aDict["cols"])
                    self.update({element.tag: mat})
                else:
                    self.update({element.tag: aDict})
            # this assumes that if you've got an attribute in a tag,
            # you won't be having any text. This may or may not be a
            # good idea -- time will tell. It works for the way we are
            # currently doing XML configuration files...
            elif element.items():
                self.update({element.tag: dict(element.items())})
            # finally, if there are no child tags and no attributes, extract
            # the text
            else:
                self.update({element.tag: convertTInitType(element.text.strip())})


def opencv_matrix_constructor(loader, node):
    mapping = loader.construct_mapping(node, deep=True)
    mat = np.array(mapping["data"])
    mat.resize(mapping["rows"], mapping["cols"])
    return mat


yaml.add_constructor(u"tag:yaml.org,2002:opencv-matrix", opencv_matrix_constructor)
yaml.SafeLoader.add_constructor(u"tag:yaml.org,2002:opencv-matrix", opencv_matrix_constructor)

warnings.simplefilter('ignore', yaml.error.UnsafeLoaderWarning)


def readOpenCVYaml(file, isstr=False, isSingleWrite=False):
    if isstr:
        data = file.split('\n')
    else:
        with open(file, 'r') as fi:
            data = fi.readlines()
    data = [line for line in data if line and line[0] is not '%']
    try:
        if isSingleWrite:
            data = yaml.load("\n".join(data), Loader=Loader)
        else:
            data = yaml.load_all("\n".join(data))
            data1 = {}
            for d in data:
                data1.update(d)
            data = data1
    except:
        print('Exception during reading yaml.')
        e = sys.exc_info()
        print(str(e))
        sys.stdout.flush()
        raise BaseException
    return data


def readYamlOrXml(file, isSingleWrite=False):
    base, ending11 = os.path.splitext(file)
    _, ending12 = os.path.splitext(base)
    if ending12 and ('yaml' in ending12 or 'yml' in ending12 or 'xml' in ending12):
        ending = ending12 + ending11
    else:
        ending = ending11
    is_zipped = True if 'gz' in ending else False
    is_xml = True if 'xml' in ending else False
    if is_zipped:
        # nr_threads = estimate_available_cpus(32)
        # with mgzip.open(file, 'rt', thread=nr_threads) as fin:
        with gzip.open(file, 'rt') as fin:
            f_str = fin.read()
            #f_bytes = fin.read()
        #f_str = f_bytes.decode('utf-8')
        if is_xml:
            root = ElementTree.XML(f_str)
            data = XmlDictConfig(root)
        else:
            data = readOpenCVYaml(f_str, True, isSingleWrite)
    else:
        if is_xml:
            tree = ElementTree.parse(file)
            root = tree.getroot()
            data = XmlDictConfig(root)
        else:
            data = readOpenCVYaml(file, False, isSingleWrite)
    return data


def estimate_available_cpus(nr_tasks, nr_cpus=-1, mult_proc=True):
    av_cpus = os.cpu_count()
    if av_cpus:
        if nr_cpus < 1:
            cpu_use = av_cpus
        elif nr_cpus > av_cpus:
            print('Demanded ' + str(nr_cpus) + ' but only ' + str(av_cpus) + ' CPUs are available. Using '
                  + str(av_cpus) + ' CPUs.')
            cpu_use = av_cpus
        else:
            cpu_use = nr_cpus
        if mult_proc:
            time.sleep(.5)
            cpu_per = psutil.cpu_percent()
            if cpu_per > 10:
                if nr_tasks >= cpu_use:
                    cpu_rat = 100
                else:
                    cpu_rat = nr_tasks / cpu_use
                cpu_rem = 100 - cpu_per
                if cpu_rem < cpu_rat:
                    if cpu_rem >= (cpu_rat / 2):
                        cpu_use = int(math.ceil(cpu_use * cpu_rem / 100))
                    else:
                        wcnt = 0
                        while cpu_rem < (cpu_rat / 2) and wcnt < 600:
                            time.sleep(.5)
                            cpu_rem = 100 - psutil.cpu_percent()
                            wcnt += 1
                        if wcnt >= 600:
                            if nr_tasks > 50:
                                cpu_use = max(int(math.ceil(cpu_use * cpu_rem / 100)),
                                              int(math.ceil(0.25 * cpu_use)),
                                              int(min(4, cpu_use)))
                            else:
                                cpu_use = max(int(math.ceil(cpu_use * cpu_rem / 100)),
                                              int(math.ceil(0.1 * cpu_use)),
                                              int(min(2, cpu_use)))
                        else:
                            cpu_use = int(math.ceil(cpu_use * cpu_rem / 100))
        else:
            cpu_use = 1
    else:
        cpu_use = max(nr_cpus, 1)
    act_nr_threads = threading.active_count()
    if av_cpus:
        max_threads = av_cpus * 2
    else:
        max_threads = 100
    if cpu_use > 1 and cpu_use + act_nr_threads > max_threads:
        cpu_use = max(max_threads - act_nr_threads, 1)
    return cpu_use


def read_matches(output_path_train, output_path_validate, sequ_dirs2, nr_train, minInlRat):
    for elem in sequ_dirs2:
        fs = os.listdir(elem['sequDir'])
        ending = '.yaml'
        for i in fs:
            if 'sequSingleFrameData_0' in i:
                base, ending11 = os.path.splitext(i)
                _, ending12 = os.path.splitext(base)
                if ending12 and ('yaml' in ending12 or 'yml' in ending12 or 'xml' in ending12):
                    ending = ending12 + ending11
                else:
                    ending = ending11
                break
        cnt = 0
        hashSequ = os.path.basename(elem['sequDir'])
        sequ_data_full = []
        while(True):
            sequFile = 'sequSingleFrameData_' + str(cnt) + ending
            sequFile = os.path.join(elem['sequDir'], sequFile)
            if not os.path.exists(sequFile):
                break
            sequ_data = readYamlOrXml(sequFile, True)
            sequ_data_full.append({'pt3Didx': sequ_data['combCorrsImg12TP_IdxWorld2'],
                                   'hashSequ': hashSequ,
                                   'Rrel': sequ_data['actR'],
                                   'trel': sequ_data['actT'],
                                   'K1': sequ_data['K1'],
                                   'K2': sequ_data['K2']})
            cnt += 1
        sequ_len = len(sequ_data_full)
        match_data_full = []
        for midx, melem in enumerate(elem['matchDirs']):
            hashMtch = os.path.basename(melem)
            kpType = elem['keyPointType'][midx]
            cnt = 0
            match_data_full1 = []
            while (True):
                mtchFile = 'matchSingleFrameData_' + str(cnt) + ending
                mtchFile = os.path.join(melem, mtchFile)
                if not os.path.exists(mtchFile):
                    break
                match_data = readYamlOrXml(mtchFile, True)
                match_data_full1.append({'kp1': match_data['frameKeypoints1'],
                                         'kp2': match_data['frameKeypoints2'],
                                         'descr1': match_data['frameDescriptors1'],
                                         'descr2': match_data['frameDescriptors2'],
                                         'matches': match_data['frameMatches'],
                                         'idx1': match_data['idxs_match23D1'],
                                         'idx2': match_data['idxs_match23D2'],
                                         'hashMtch': hashMtch,
                                         'keyPointType': kpType})
                cnt += 1
            if len(match_data_full1) != sequ_len:
                raise ValueError("Number of frames with 3D information and matches are not consistent")
            match_data_full.append(match_data_full1)
        cnt_tv = 0
        for match in match_data_full:
            for i in range(0, sequ_len):
                # Get stereo correspondences
                pts1 = []
                pts2 = []
                descr1 = []
                descr2 = []
                dists = []
                if len(match[i]['matches']) < 10:
                    continue
                for m in match[i]['matches']:
                    pts2.append(match[i]['kp2'][m[1]][:2])
                    pts1.append(match[i]['kp1'][m[0]][:2])
                    descr1.append(match[i]['descr1'][m[0]])
                    descr2.append(match[i]['descr2'][m[1]])
                    dists.append(m[3])
                ratios = calcRatios(descr1, descr2, dists)
                isInt = descr_is_int(descr1)
                pts1 = np.array([pts1])
                pts2 = np.array([pts2])
                ratios = np.array([ratios])
                ratios = np.expand_dims(ratios, 2)
                K1 = np.array(sequ_data_full[i]['K1'])
                K2 = np.array(sequ_data_full[i]['K2'])
                GT_R_Rel = np.array(sequ_data_full[i]['Rrel'])
                GT_t_Rel = np.array(sequ_data_full[i]['trel'])

                # save data tensor and ground truth transformation
                name0 = 'sequ-' + sequ_data_full[i]['hashSequ'] + '_mtch-' + match[i]['hashMtch'] + '_' + \
                        match[i]['keyPointType'] + '_'
                if pts1.shape[1] > 10:
                    gt_F = getFundamentalMat(GT_R_Rel, GT_t_Rel, K1, K2)
                    gt_res = epipolar_error(pts1, pts2, gt_F)
                    gt_inliers = (gt_res < 10)
                    inl_cnt = float(gt_inliers.sum())
                    inlrat = inl_cnt / float(pts1.shape[1])
                    if inlrat < 0.6 * elem['inlRat'][i]:
                        print('Stereo correspondences are corrupt', sys.stderr)
                    elif inlrat > minInlRat:
                        name = name0 + 'pair_%d-1_%d-2.npy' % (i, i)
                        if cnt_tv < nr_train:
                            file = os.path.join(output_path_train, name)
                        else:
                            file = os.path.join(output_path_validate, name)
                        np.save(file, [
                            pts1.astype(np.float32),
                            pts2.astype(np.float32),
                            ratios.astype(np.float32),
                            elem['imgSize'],
                            elem['imgSize'],
                            K1.astype(np.float32),
                            K2.astype(np.float32),
                            GT_R_Rel.astype(np.float32),
                            GT_t_Rel.astype(np.float32)
                        ])
                        cnt_tv += 1

                #Get frame to frame correspondences for cam1
                if i > 0:
                    idx3D2 = {}
                    nr_TP2 = len(sequ_data_full[i]['pt3Didx'])
                    if nr_TP2 < 10:
                        continue
                    for idx, id in enumerate(match[i]['idx1']):
                        if id < nr_TP2:
                            idx3D2[sequ_data_full[i]['pt3Didx'][id]] = idx

                    idx3D1 = {}
                    nr_TP1 = len(sequ_data_full[i - 1]['pt3Didx'])
                    if nr_TP1 < 10:
                        continue
                    for idx, id in enumerate(match[i - 1]['idx1']):
                        if id < nr_TP1:
                            idx3D1[sequ_data_full[i - 1]['pt3Didx'][id]] = idx

                    pts1 = []
                    pts2 = []
                    descr1 = []
                    descr2 = []
                    dists = []
                    corrs1 = set()
                    corrs2 = set()
                    for item in idx3D1.items():
                        if item[0] in idx3D2:
                            try:
                                idx2 = idx3D2[item[0]]
                            except Exception:
                                continue
                            corrs1.add(item[1])
                            corrs2.add(idx2)
                            pts1.append(match[i - 1]['kp1'][item[1]][:2])
                            pts2.append(match[i]['kp1'][idx2][:2])
                            descr1.append(match[i - 1]['descr1'][item[1]])
                            descr2.append(match[i]['descr1'][idx2])
                            dists.append(getDescriptorDist(descr1[-1], descr2[-1], isInt))
                    if len(pts1) > 10:
                        ratios = calcRatios(descr1, descr2, dists)
                        K1 = np.array(sequ_data_full[i - 1]['K1'])
                        K2 = np.array(sequ_data_full[i]['K1'])
                        GT_R_Rel = np.array(elem['camPosesWrld'][i]['R']).transpose() @ \
                                   np.array(elem['camPosesWrld'][i - 1]['R'])
                        GT_t_Rel = np.matmul(np.array(elem['camPosesWrld'][i]['R']).transpose(),
                                             np.array(elem['camPosesWrld'][i - 1]['t']) -
                                             np.array(elem['camPosesWrld'][i]['t']))
                        # Check for correctness
                        gt_F = getFundamentalMat(GT_R_Rel, GT_t_Rel, K1, K2)
                        gt_res = epipolar_error(np.array([pts1]), np.array([pts2]), gt_F)
                        gt_inliers = (gt_res < 10)
                        inl_cnt = float(gt_inliers.sum())
                        inlrat = inl_cnt / float(len(pts1))
                        max_tn = max(int(round(float(len(pts1)) / minInlRat, 0)) - len(pts1), 0)
                        if inlrat < 0.6:
                            print('Frame to frame correspondences for cam1 are corrupt', sys.stderr)
                        else:
                            #Get TN
                            minlen = min(len(idx3D1), len(idx3D2)) - len(pts1)
                            tnidx1 = []
                            for j in range(0, len(idx3D1)):
                                if j not in corrs1:
                                    tnidx1.append(j)
                            tnidx2 = []
                            for j in range(0, len(idx3D2)):
                                if j not in corrs2:
                                    tnidx2.append(j)
                            if len(tnidx1) > 5 and len(tnidx2) > 5 and max_tn > 5:
                                np.random.shuffle(tnidx1)
                                np.random.shuffle(tnidx2)
                                kp1tn = []
                                kp2tn = []
                                descr1tn = []
                                descr2tn = []
                                minlen = min(minlen, max_tn)
                                for j in range(0, minlen):
                                    kp1tn.append(match[i - 1]['kp1'][tnidx1[j]])
                                    kp2tn.append(match[i]['kp1'][tnidx2[j]])
                                    descr1tn.append(match[i - 1]['descr1'][tnidx1[j]])
                                    descr2tn.append(match[i]['descr1'][tnidx2[j]])
                                bf = cv2.BFMatcher()
                                if isInt:
                                    tnmatches = bf.knnMatch(np.array(descr1tn).astype(np.uint8),
                                                            np.array(descr2tn).astype(np.uint8), k=2)
                                else:
                                    tnmatches = bf.knnMatch(np.array(descr1tn).astype(float),
                                                            np.array(descr2tn).astype(float), k=2)
                                if len(tnmatches) > 5:
                                    dellist = []
                                    for midx, m in enumerate(tnmatches):
                                        if len(m) != 2:
                                            dellist.append(midx)
                                    if dellist:
                                        dellist.sort(reverse=True)
                                        for dl in dellist:
                                            del tnmatches[dl]
                                    if len(tnmatches) > 5:
                                        for (m, n) in tnmatches:
                                            pts2.append(kp2tn[m.trainIdx][:2])
                                            pts1.append(kp1tn[m.queryIdx][:2])
                                            ratios.append((m.distance + 1e-8) / (n.distance + 1e-8))

                            shlist = list(range(0, len(pts1)))
                            np.random.shuffle(shlist)
                            shlist = list(shlist)
                            pts1_ = [[pt_idx, pt] for pt_idx, pt in zip(shlist, pts1)]
                            pts2_ = [[pt_idx, pt] for pt_idx, pt in zip(shlist, pts2)]
                            ratios_ = [[r_idx, r] for r_idx, r in zip(shlist, ratios)]
                            pts1_.sort(key=lambda k: k[0])
                            pts2_.sort(key=lambda k: k[0])
                            ratios_.sort(key=lambda k: k[0])
                            pts1 = [pt[1] for pt in pts1_]
                            pts2 = [pt[1] for pt in pts2_]
                            ratios = [pt[1] for pt in ratios_]
                            pts1 = np.array([pts1])
                            pts2 = np.array([pts2])
                            ratios = np.array([ratios])
                            ratios = np.expand_dims(ratios, 2)

                            name = name0 + 'pair_%d-1_%d-1.npy' % (i - 1, i)
                            if cnt_tv < nr_train:
                                file = os.path.join(output_path_train, name)
                            else:
                                file = os.path.join(output_path_validate, name)
                            np.save(file, [
                                pts1.astype(np.float32),
                                pts2.astype(np.float32),
                                ratios.astype(np.float32),
                                elem['imgSize'],
                                elem['imgSize'],
                                K1.astype(np.float32),
                                K2.astype(np.float32),
                                GT_R_Rel.astype(np.float32),
                                GT_t_Rel.astype(np.float32)
                            ])
                            cnt_tv += 1


                    # Get frame to frame correspondences for cam2
                    idx3D2 = {}
                    nr_TP2 = len(sequ_data_full[i]['pt3Didx'])
                    if nr_TP2 < 10:
                        continue
                    for idx, id in enumerate(match[i]['idx2']):
                        if id < nr_TP2:
                            idx3D2[sequ_data_full[i]['pt3Didx'][id]] = idx

                    idx3D1 = {}
                    nr_TP1 = len(sequ_data_full[i - 1]['pt3Didx'])
                    if nr_TP1 < 10:
                        continue
                    for idx, id in enumerate(match[i - 1]['idx2']):
                        if id < nr_TP1:
                            idx3D1[sequ_data_full[i - 1]['pt3Didx'][id]] = idx

                    pts1 = []
                    pts2 = []
                    descr1 = []
                    descr2 = []
                    dists = []
                    corrs1 = set()
                    corrs2 = set()
                    for item in idx3D1.items():
                        if item[0] in idx3D2:
                            try:
                                idx2 = idx3D2[item[0]]
                            except Exception:
                                continue
                            corrs1.add(item[1])
                            corrs2.add(idx2)
                            pts1.append(match[i - 1]['kp2'][item[1]][:2])
                            pts2.append(match[i]['kp2'][idx2][:2])
                            descr1.append(match[i - 1]['descr2'][item[1]])
                            descr2.append(match[i]['descr2'][idx2])
                            dists.append(getDescriptorDist(descr1[-1], descr2[-1], isInt))
                    if len(pts1) > 10:
                        ratios = calcRatios(descr1, descr2, dists)
                        K1 = np.array(sequ_data_full[i - 1]['K2'])
                        K2 = np.array(sequ_data_full[i]['K2'])
                        GT_R_Rel = np.array(sequ_data_full[i]['Rrel']) @ GT_R_Rel @ \
                                   np.array(sequ_data_full[i - 1]['Rrel']).transpose()
                        GT_t_Rel = np.array(sequ_data_full[i]['trel']) + \
                                   np.array(sequ_data_full[i]['Rrel']) @ GT_t_Rel - \
                                   GT_R_Rel @ np.array(sequ_data_full[i - 1]['trel'])

                        # Check for correctness
                        gt_F = getFundamentalMat(GT_R_Rel, GT_t_Rel, K1, K2)
                        gt_res = epipolar_error(np.array([pts1]), np.array([pts2]), gt_F)
                        gt_inliers = (gt_res < 10)
                        inl_cnt = float(gt_inliers.sum())
                        inlrat = inl_cnt / float(len(pts1))
                        max_tn = max(int(round(float(len(pts1)) / minInlRat, 0)) - len(pts1), 0)
                        if inlrat < 0.6:
                            print('Frame to frame correspondences for cam2 are corrupt', sys.stderr)
                        else:
                            # Get TN
                            minlen = min(len(idx3D1), len(idx3D2)) - len(pts1)
                            tnidx1 = []
                            for j in range(0, len(idx3D1)):
                                if j not in corrs1:
                                    tnidx1.append(j)
                            tnidx2 = []
                            for j in range(0, len(idx3D2)):
                                if j not in corrs2:
                                    tnidx2.append(j)
                            if len(tnidx1) < 5 or len(tnidx2) < 5 or max_tn < 5:
                                continue
                            np.random.shuffle(tnidx1)
                            np.random.shuffle(tnidx2)
                            kp1tn = []
                            kp2tn = []
                            descr1tn = []
                            descr2tn = []
                            minlen = min(minlen, max_tn)
                            for j in range(0, minlen):
                                kp1tn.append(match[i - 1]['kp2'][tnidx1[j]])
                                kp2tn.append(match[i]['kp2'][tnidx2[j]])
                                descr1tn.append(match[i - 1]['descr2'][tnidx1[j]])
                                descr2tn.append(match[i]['descr2'][tnidx2[j]])
                            bf = cv2.BFMatcher()
                            if isInt:
                                tnmatches = bf.knnMatch(np.array(descr1tn).astype(np.uint8),
                                                        np.array(descr2tn).astype(np.uint8), k=2)
                            else:
                                tnmatches = bf.knnMatch(np.array(descr1tn).astype(float),
                                                        np.array(descr2tn).astype(float), k=2)
                            if len(tnmatches) > 5:
                                dellist = []
                                for midx, m in enumerate(tnmatches):
                                    if len(m) != 2:
                                        dellist.append(midx)
                                if dellist:
                                    dellist.sort(reverse=True)
                                    for dl in dellist:
                                        del tnmatches[dl]
                                if len(tnmatches) > 5:
                                    for (m, n) in tnmatches:
                                        pts2.append(kp2tn[m.trainIdx][:2])
                                        pts1.append(kp1tn[m.queryIdx][:2])
                                        ratios.append((m.distance + 1e-8) / (n.distance + 1e-8))

                            shlist = list(range(0, len(pts1)))
                            np.random.shuffle(shlist)
                            shlist = list(shlist)
                            pts1_ = [[pt_idx, pt] for pt_idx, pt in zip(shlist, pts1)]
                            pts2_ = [[pt_idx, pt] for pt_idx, pt in zip(shlist, pts2)]
                            ratios_ = [[r_idx, r] for r_idx, r in zip(shlist, ratios)]
                            pts1_.sort(key=lambda k: k[0])
                            pts2_.sort(key=lambda k: k[0])
                            ratios_.sort(key=lambda k: k[0])
                            pts1 = [pt[1] for pt in pts1_]
                            pts2 = [pt[1] for pt in pts2_]
                            ratios = [pt[1] for pt in ratios_]
                            pts1 = np.array([pts1])
                            pts2 = np.array([pts2])
                            ratios = np.array([ratios])
                            ratios = np.expand_dims(ratios, 2)

                            name = name0 + 'pair_%d-2_%d-2.npy' % (i - 1, i)
                            if cnt_tv < nr_train:
                                file = os.path.join(output_path_train, name)
                            else:
                                file = os.path.join(output_path_validate, name)
                            np.save(file, [
                                pts1.astype(np.float32),
                                pts2.astype(np.float32),
                                ratios.astype(np.float32),
                                elem['imgSize'],
                                elem['imgSize'],
                                K1.astype(np.float32),
                                K2.astype(np.float32),
                                GT_R_Rel.astype(np.float32),
                                GT_t_Rel.astype(np.float32)
                            ])
                            cnt_tv += 1
    return True


def getEssentialMat(R, t):
    gt_E = np.zeros((3, 3))
    gt_E[0, 1] = -float(t[2, 0])
    gt_E[0, 2] = float(t[1, 0])
    gt_E[1, 0] = float(t[2, 0])
    gt_E[1, 2] = -float(t[0, 0])
    gt_E[2, 0] = -float(t[1, 0])
    gt_E[2, 1] = float(t[0, 0])

    gt_E = gt_E @ R
    return gt_E


def getFundamentalMat(R, t, K1, K2):
    gt_E = getEssentialMat(R, t)
    gt_F = np.linalg.inv(K2).transpose() @ gt_E @ np.linalg.inv(K1)
    return gt_F


def getHomogeneousCoords(pts1, pts2):
    num_pts = pts1.shape[1]
    hom_pts1 = np.transpose(np.concatenate((pts1[0, :, :], np.ones((num_pts, 1))), axis=1))
    hom_pts2 = np.transpose(np.concatenate((pts2[0, :, :], np.ones((num_pts, 1))), axis=1))
    return hom_pts1, hom_pts2


def epipolar_error_from_homo_coords(hom_pts1, hom_pts2, F):
    """Compute the symmetric epipolar error."""
    res = 1 / np.linalg.norm(F.T.dot(hom_pts2)[0:2], axis=0)
    res += 1 / np.linalg.norm(F.dot(hom_pts1)[0:2], axis=0)
    res *= abs(np.sum(hom_pts2 * np.matmul(F, hom_pts1), axis=0))
    return res


def epipolar_error(pts1, pts2, F):
    hom_pts1, hom_pts2 = getHomogeneousCoords(pts1, pts2)
    return epipolar_error_from_homo_coords(hom_pts1, hom_pts2, F)


def calcRatios(descr1, descr2, dists):
    isInt = descr_is_int(descr1)
    ratios = []
    for dv1, di1 in zip(descr1, dists):
        di2 = 1e+10
        for dv2 in descr2:
            di3 = getDescriptorDist(dv1, dv2, isInt)
            if di1 < di3 < di2:
                di2 = di3
        ratios.append((di1 + 1e-8) / (di2 + 1e-8))
    return ratios


def descr_is_int(descr):
    err_sum = 0
    for da in descr[0:2]:
        for d in da:
            err_sum += d - math.floor(d)
    isInt = np.isclose(0, err_sum, atol=1e-6, rtol=1e-4)
    return isInt


def getDescriptorDist(descr1, descr2, isInt):
    if isInt:
        return cv2.norm(descr1.astype(np.uint8), descr2.astype(np.uint8), normType=cv2.NORM_HAMMING)
    else:
        return cv2.norm(descr1.astype(float), descr2.astype(float), normType=cv2.NORM_L2)


def main():
    parser = argparse.ArgumentParser(description='Generate configuration files and overview files for scenes by '
                                                 'varying the used inlier ratio and keypoint accuracy')
    parser.add_argument('--path', type=str, required=True,
                        help='Directory holding sequences and file sequInfos.yaml/yml/xml')
    parser.add_argument('--path_out', type=str, required=False,
                        help='Directory for storing converted data')
    parser.add_argument('--validatePort', type=float, required=False, default=0.2,
                        help='Data portion that should be used for testing')
    parser.add_argument('--minInlRat', type=float, required=False, default=0.19,
                        help='Minimum inlier ratio')
    parser.add_argument('--nrCPUs', type=int, required=False, default=4,
                        help='Number of CPU cores for parallel processing. If a negative value is provided, '
                             'the program tries to find the number of available CPUs on the system - if it fails, '
                             'the absolute value of nrCPUs is used. Default: 4')
    args = parser.parse_args()
    if not os.path.exists(args.path):
        raise ValueError('Directory ' + args.path + ' does not exist')
    if args.nrCPUs > 72 or args.nrCPUs == 0:
        raise ValueError("Unable to use " + str(args.nrCPUs) + " CPU cores.")
    av_cpus = os.cpu_count()
    if av_cpus:
        if args.nrCPUs < 0:
            cpu_use = av_cpus
        elif args.nrCPUs > av_cpus:
            print('Demanded ' + str(args.nrCPUs) + ' but only ' + str(av_cpus) + ' CPUs are available. Using '
                  + str(av_cpus) + ' CPUs.')
            cpu_use = av_cpus
        else:
            cpu_use = args.nrCPUs
    elif args.nrCPUs < 0:
        print('Unable to determine # of CPUs. Using ' + str(abs(args.nrCPUs)) + ' CPUs.')
        cpu_use = abs(args.nrCPUs)
    else:
        cpu_use = args.nrCPUs
    fs = os.listdir(args.path)
    ending = ''
    for i in fs:
        if 'sequInfos' in i:
            _, ending = os.path.splitext(i)
            break
    if not ending:
        raise ValueError('Missing sequInfos file')
    sequFile = 'sequInfos' + ending
    sequf = os.path.join(args.path, sequFile)
    if not os.path.exists(sequf):
        raise ValueError('File ' + sequf + ' holding sequence information does not exist')
    data = readYamlOrXml(sequf)
    sequ_dirs = []
    for key in data:
        if 'parSetNr' not in key:
            raise ValueError('File ' + sequf + ' holding sequence information is corrupt')
        sequ_dirs.append(data[key]['hashSequencePars'])
    if len(sequ_dirs) == 0:
        raise ValueError('File ' + sequf + ' does not hold sequence information')

    sequ_dirs2 = []
    matchFile = 'matchInfos' + ending
    cnt = 0
    cnts = []
    for d in sequ_dirs:
        s_dir = os.path.join(args.path, d)
        if not os.path.exists(s_dir):
            raise ValueError('Missing folder ' + s_dir)
        fs = os.listdir(s_dir)
        ending1 = ''
        cnt1 = 0
        for i in fs:
            if 'sequPars' in i and not ending1:
                base, ending11 = os.path.splitext(i)
                _, ending12 = os.path.splitext(base)
                if ending12 and ('yaml' in ending12 or 'yml' in ending12 or 'xml' in ending12):
                    ending1 = ending12 + ending11
                else:
                    ending1 = ending11
            elif 'sequSingleFrameData_' in i:
                cnt1 += 1
        if not ending1:
            raise ValueError('Missing sequPars file')
        cnts.append(cnt1 + 2 * (cnt1 - 1))
        cnt += cnts[-1]
        sequ_parf = os.path.join(s_dir, 'sequPars' + ending1)
        if not os.path.exists(sequ_parf):
            raise ValueError('Missing file ' + sequ_parf)
        sequ_par_data = readYamlOrXml(sequ_parf, True)
        f_match = os.path.join(s_dir, matchFile)
        if not os.path.exists(f_match):
            raise ValueError('Missing file ' + f_match)
        data = readYamlOrXml(f_match)
        ms = {'sequDir': s_dir,
              'matchDirs': [],
              'keyPointType': [],
              'imgSize': (sequ_par_data['imgSize']['height'], sequ_par_data['imgSize']['width']),
              'camPosesWrld': sequ_par_data['absCamCoordinates'],
              'inlRat': sequ_par_data['inlRat']}
        for key in data:
            if 'parSetNr' not in key:
                raise ValueError('File ' + f_match + ' holding information about matches is corrupt')
            mhash = data[key]['hashMatchingPars']
            m_dir = os.path.join(s_dir, mhash)
            if not os.path.exists(m_dir):
                raise ValueError('Missing folder ' + m_dir)
            ms['matchDirs'].append(m_dir)
            ms['keyPointType'].append(data[key]['keyPointType'])
        if len(ms['matchDirs']) == 0:
            raise ValueError('File ' + f_match + ' does not hold matching information')
        sequ_dirs2.append(ms)
    if args.validatePort > 0.8:
        args.validatePort = 0.8
    elif args.validatePort < 0:
        args.validatePort = 0
    nr_validate = int(round(args.validatePort * cnt))
    nr_train = cnt - nr_validate
    if args.path_out:
        output_path = args.path_out
        if not os.path.exists(output_path):
            os.mkdir(output_path)
    else:
        output_path = os.path.join(args.path, 'matches_python')
    if os.path.exists(output_path):
        warnings.warn('Path ' + output_path + ' already exists. Old data might be overwritten.', UserWarning)
    else:
        os.mkdir(output_path)
    output_path_train = os.path.join(output_path, 'train')
    if not os.path.exists(output_path_train):
        os.mkdir(output_path_train)
    output_path_validate = os.path.join(output_path, 'validate')
    if not os.path.exists(output_path_validate):
        os.mkdir(output_path_validate)
    nr_dirs = len(sequ_dirs2)
    max_procs = min(nr_dirs, cpu_use)
    if max_procs == 1:
        try:
            read_matches(output_path_train, output_path_validate, sequ_dirs2, nr_train, args.minInlRat)
        except:
            e = sys.exc_info()
            print('Exception: ' + str(e))
            sys.stdout.flush()
            sys.exit(1)
    else:
        excmess, log = configure_logging(output_path, 'convert_except_')
        nr_dirs_sub = int(math.ceil(nr_dirs / max_procs))
        cmds = []
        il = 0
        for i in range(nr_dirs_sub, nr_dirs, nr_dirs_sub):
            cnt = sum(cnts[il:i])
            nr_validate = int(round(args.validatePort * cnt))
            nr_train = cnt - nr_validate
            cmds.append((output_path_train, output_path_validate, sequ_dirs2[il:i], nr_train, args.minInlRat))
            il = i
        if il < nr_dirs:
            cnt = sum(cnts[il:nr_dirs])
            nr_validate = int(round(args.validatePort * cnt))
            nr_train = cnt - nr_validate
            cmds.append((output_path_train, output_path_validate, sequ_dirs2[il:nr_dirs], nr_train, args.minInlRat))
        cmd_fails = []
        with MyPool(processes=max_procs) as pool:
            # results = pool.map(processDir, work_items)
            results = [pool.apply_async(read_matches, t) for t in cmds]
            cnt = 0
            cnt_dot = 0
            for r in results:
                while 1:
                    sys.stdout.flush()
                    try:
                        res = r.get(2.0)
                        print()
                        if not res:
                            cmd_fails = cmd_fails + [a['sequDir'] for a in cmds[cnt][2]]
                            print('Finished the following directories with errors:')
                        else:
                            print('Finished the following directories:')
                        print('\n'.join([a['sequDir'] for a in cmds[cnt][2]]))
                        print('\n')
                        break
                    except multiprocessing.TimeoutError:
                        if cnt_dot >= 90:
                            print()
                            cnt_dot = 0
                        sys.stdout.write('.')
                        cnt_dot = cnt_dot + 1
                    except Exception:
                        log.error('Exception during conversion of sequence data', exc_info=True)
                        print()
                        print('Exception in processing directories.')
                        e = sys.exc_info()
                        print(str(e))
                        sys.stdout.flush()
                        cmd_fails = cmd_fails + [a['sequDir'] for a in cmds[cnt][2]]
                        break
                cnt = cnt + 1
            pool.close()
            pool.join()
        logging.shutdown()
        if os.path.exists(excmess) and os.stat(excmess).st_size < 4:
            os.remove(excmess)
        if cmd_fails:
            res_file = os.path.join(output_path, 'error_overview.txt')
            cnt = 1
            while os.path.exists(res_file):
                res_file = os.path.join(output_path, 'error_overview' + str(cnt) + '.txt')
                cnt = cnt + 1

            with open(res_file, 'w') as fo:
                fo.write('\n'.join(cmd_fails))
            sys.exit(1)
        sys.exit(0)


if __name__ == "__main__":
    main()