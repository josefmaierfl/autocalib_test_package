"""
Load configuration files and generate different scenes for testing using multiple CPU cores
"""
import sys, re, numpy as np, math, argparse, os, pandas, time, subprocess as sp
import ruamel.yaml as yaml
import multiprocessing
import warnings
# We must import this explicitly, it is not imported by the top-level
# multiprocessing module.
import multiprocessing.pool
#from tabulate import tabulate as tab
#from copy import deepcopy

def opencv_matrix_constructor(loader, node):
    mapping = loader.construct_mapping(node, deep=True)
    mat = np.array(mapping["data"])
    mat.resize(mapping["rows"], mapping["cols"])
    return mat
yaml.add_constructor(u"tag:yaml.org,2002:opencv-matrix", opencv_matrix_constructor)
yaml.SafeLoader.add_constructor(u"tag:yaml.org,2002:opencv-matrix", opencv_matrix_constructor)

warnings.simplefilter('ignore', yaml.error.UnsafeLoaderWarning)


def genScenes(input_path, executable, nr_cpus, message_path, retry_file=None):
    if retry_file:
        if not os.path.exists(retry_file):
            raise ValueError("Unable to load " + retry_file)
        dirs_f = retry_file
    else:
        dirs_f = os.path.join(input_path, 'generated_dirs_config.txt')
        if not os.path.exists(dirs_f):
            raise ValueError("Unable to load " + dirs_f)

    #data = readOpenCVYaml('/home/maierj/work/Sequence_Test/Test_load_save/USAC/usac_kp-distr-1corn_depth-NMF_TP-500/sequInfos.yaml')

    dirsc = []
    with open(dirs_f, 'r') as fi:
        #Load directory holding configuration files
        confd = fi.readline()
        cont = False
        if confd:
            confd = confd.rstrip()
            cont = True
        while cont:
            #Load list of configuration files and settings for generating scenes
            if retry_file:
                if not confd:
                    confd = fi.readline()
                    if confd:
                        confd = confd.rstrip()
                        cont = True
                    else:
                        cont = False
                    continue
                if not os.path.exists(confd):
                    raise ValueError("Directory " + confd + " does not exist.")
                ovcf = confd
            else:
                if not os.path.exists(confd):
                    raise ValueError("Directory " + confd + " does not exist.")
                ovcf = os.path.join(confd, 'config_files.csv')
            if not os.path.exists(ovcf):
                raise ValueError("File " + ovcf + " does not exist.")
            dirsc.append(ovcf)
            confd = fi.readline()
            if confd:
                confd = confd.rstrip()
                cont = True
            else:
                cont = False
    maxd_parallel = int(len(dirsc) / nr_cpus)
    if maxd_parallel == 0:
        maxd_parallel = 1
    nr_used_cpus = min(int(len(dirsc) / maxd_parallel), nr_cpus)
    dirscp = []
    rest = len(dirsc) - nr_used_cpus * maxd_parallel
    cr = 0
    rest_sv = rest
    for i in range(0, len(dirsc) - rest_sv, maxd_parallel):
        if rest > 0:
            dirscp.append(dirsc[(i + cr):(i + cr + maxd_parallel + 1)])
            cr = cr + 1
            rest = rest - 1
        else:
            dirscp.append(dirsc[(i + cr):(i + cr + maxd_parallel)])
    assert len(dirscp) == nr_used_cpus
    cpus_rest = nr_cpus - nr_used_cpus
    if cpus_rest == 0:
        cpus_rest = [1] * nr_used_cpus
    else:
        sub_cpus = nr_cpus / nr_used_cpus
        cpus_rest = [int(sub_cpus)] * nr_used_cpus
        for i in range(0, int(round((sub_cpus - math.floor(sub_cpus)) * nr_used_cpus))):
            cpus_rest[i] = cpus_rest[i] + 1
    message_path_new = os.path.join(message_path, 'scene_creation')
    try:
        os.mkdir(message_path_new)
    except FileExistsError:
        pass
    work_items = [(dirscp[x], cpus_rest[x], executable, message_path_new) for x in range(0, nr_used_cpus)]
    cnt_dot = 0
    cmd_fails = []
    cmd_fails_dir = []
    with MyPool(processes=nr_used_cpus) as pool:
        #results = pool.map(processDir, work_items)
        results = [pool.apply_async(processDir, t) for t in work_items]
        cnt = 0
        for r in results:
            while 1:
                sys.stdout.flush()
                try:
                    res = r.get(2.0)
                    print()
                    if res:
                        cmd_fails = cmd_fails + res
                        print('Finished the following directories with errors:')
                    else:
                        print('Finished the following directories:')
                    print('\n'.join(work_items[cnt][0]))
                    print('\n')
                    break
                except ValueError as e:
                    print()
                    print('Exception during processing a folder with multiple configuration files:')
                    print(str(e))
                    print('Some of the following directories might be not processed completely:')
                    print('\n'.join(work_items[cnt][0]))
                    print('\n')
                    cmd_fails_dir.append(work_items[cnt][0])
                    break
                except multiprocessing.TimeoutError:
                    if cnt_dot >= 90:
                        print()
                        cnt_dot = 0
                    sys.stdout.write('.')
                    cnt_dot = cnt_dot + 1
                except:
                    print()
                    print('Unknown exception in processing directories.')
                    e = sys.exc_info()
                    print(str(e))
                    sys.stdout.flush()
                    cmd_fails_dir.append(work_items[cnt][0])
                    break
            cnt = cnt + 1
        pool.close()
        pool.join()

    if cmd_fails or cmd_fails_dir:
        if cmd_fails:
            res_file = os.path.join(message_path_new, 'sequ_matches_cmds_error_overview.txt')
            cnt = 1
            while os.path.exists(res_file):
                res_file = os.path.join(message_path_new, 'sequ_matches_cmds_error_overview' + str(cnt) + '.txt')
                cnt = cnt + 1

            with open(res_file, 'w') as fo:
                cmd_fails_tmp = []
                for r in cmd_fails:
                    cmd_fails_tmp.append(' '.join(map(str, r)))
                fo.write('\n'.join(cmd_fails_tmp))
        if cmd_fails_dir:
            res_file = os.path.join(message_path_new, 'sequ_matches_dirs_error_overview.txt')
            cnt = 1
            while os.path.exists(res_file):
                res_file = os.path.join(message_path_new, 'sequ_matches_dirs_error_overview' + str(cnt) + '.txt')
                cnt = cnt + 1

            with open(res_file, 'w') as fo:
                cmd_fails_tmp = []
                for r in cmd_fails_dir:
                    cmd_fails_tmp.append('\n'.join(r))
                fo.write('\n\n'.join(cmd_fails_tmp))
        return 1
    return 0


def retry_dirs_from_file(file_name, executable, nr_cpus, message_path):
    if not os.path.exists(file_name):
        raise ValueError('File ' + file_name + ' not found')
    return genScenes(None, executable, nr_cpus, message_path, file_name)


def retry_cmds_from_file(file_name, nr_cpus, message_path):
    if not os.path.exists(file_name):
        raise ValueError('File ' + file_name + ' not found')
    message_path_new = os.path.join(message_path, 'scene_creation')
    try:
        os.mkdir(message_path_new)
    except FileExistsError:
        pass
    mess_new = os.path.join(message_path, 'retry_cmds')
    try:
        os.mkdir(mess_new)
    except FileExistsError:
        pass
    cmds = []
    with open(file_name, 'r') as fi:
        # Load directory holding configuration files
        confd = fi.readline()
        cont = False
        if confd:
            confd = confd.rstrip()
            cont = True
        while cont:
            if not confd:
                confd = fi.readline()
                if confd:
                    confd = confd.rstrip()
                    cont = True
                else:
                    cont = False
                continue
            if not os.path.exists(confd):
                raise ValueError("Directory " + confd + " does not exist.")
            cmds.append(confd.split(' '))
            confd = fi.readline()
            if confd:
                confd = confd.rstrip()
                cont = True
            else:
                cont = False

    cmds_sequ = []
    cmds_match = []
    cmd_fails = []
    for i in confd:
        if '--load_folder' in i:
            cmds_match.append(i)
        else:
            cmds_sequ.append(i)
    if cmds_match:
        cmds_ordered = {}
        for i in cmds_match:
            if not cmds_ordered or i[10] not in cmds_ordered.keys():
                cmds_ordered[i[10]] = [i]
            else:
                cmds_ordered[i[10]].append(i)

        # cpus_use = max(int(nr_cpus / len(cmds_ordered.keys())), 1)
        # cpus_rest = nr_cpus % cpus_use
        # longest = ('', 0)
        # cpu_list = {}
        # for i in cmds_ordered.keys():
        #     cpu_list[i] = cpus_use
        #     if len(cmds_ordered[i]) > longest[2]:
        #         longest = (i, len(cmds_ordered[i]))
        # cpu_list[longest[0]] += cpus_rest
        cmds = []
        for i in cmds_ordered.keys():
            # Generate unique path for storing messages
            dirn = os.path.dirname(cmds_ordered[i][8])
            sub_dirs = re.findall(r'[\w-]+', dirn)
            if sub_dirs:
                base = sub_dirs[-1]
            else:
                warnings.warn('Unable to extract last sub-directory name of ' + cmds_ordered[i][8], UserWarning)
                warnings.warn('Skipping calculations of matches for sequence folder ' + i, UserWarning)
                sys.stdout.flush()
                continue
            mess_new1 = os.path.join(mess_new, base)
            try:
                os.mkdir(mess_new1)
            except FileExistsError:
                print('Directory ', mess_new1, ' already exists.')
                print('Overwriting previous stderr and stdout message files if file names are equal')
                sys.stdout.flush()
            ov_file = os.path.join(i, 'matchInfos.yaml')
            if not os.path.exists(ov_file):
                raise ValueError('File ' + ov_file + ' does not exist')
            cnt = 0
            try:
                data = readOpenCVYaml(ov_file)
            except:
                raise ValueError("Unable to read yaml file " + ov_file)
            while 1:
                if 'parSetNr' + str(int(cnt)) in data.keys():
                    cnt += 1
                else:
                    break
            if cnt == 0:
                raise ValueError('No sequence available in ' + ov_file)
            for j in cmds_ordered[i]:
                cmds.append((j, int(cnt), mess_new1, nr_cpus, True))
                cnt += 1
        res2 = gen_matches_multiprocessing(cmds, cmd_fails, nr_cpus)
        if res2:
            res_file = os.path.join(mess_new, 'results.txt')
            if os.path.exists(res_file):
                print('results.txt already exists. Overwriting...')
                sys.stdout.flush()
            with open(res_file, 'w') as fo:
                res1_tmp = []
                for r in res2:
                    if len(r) == 1:
                        res1_tmp.append(r[0] + '\n')
                    else:
                        res2_tmp = []
                        for r1 in r:
                            if isinstance(r1, (list,)):
                                res2_tmp.append(' '.join(map(str, r1)))
                            else:
                                res2_tmp.append('Inner-parSetNr: ' + str(r1))
                        res1_tmp.append('\n'.join(res2_tmp))
                fo.write('\n\n'.join(res1_tmp))

    ret = 0
    if cmds_sequ:
        folders = []
        # Get cmds with same config folder
        cmds_ordered = {}
        for i in cmds_sequ:
            path = os.path.dirname(i[8])
            if not cmds_ordered or path not in cmds_ordered.keys():
                cmds_ordered[path] = [i]
            else:
                cmds_ordered[path].append(i)
        # Get overview file to be able to read which matches should be generated after creating the sequence
        for i in cmds_ordered.keys():
            # Get last parSetNr
            ovs_f = os.path.join(cmds_ordered[i][0][6], 'sequInfos.yaml')
            if not os.path.exists(ovs_f):
                raise ValueError("Sequence overview file " + ovs_f + " does not exist.")
            try:
                data = readOpenCVYaml(ovs_f)
            except:
                raise ValueError("Unable to read yaml file " + ovs_f)
            cnt = 0
            while 1:
                if 'parSetNr' + str(int(cnt)) in data.keys():
                    cnt += 1
                else:
                    break
            if cnt == 0:
                raise ValueError('No sequence available in ' + ovs_f + '. The whole directery should be processed.')
            ovcf = os.path.join(i, 'config_files.csv')
            confs_list = []
            if not os.path.exists(ovcf):
                raise ValueError("File " + ovcf + " does not exist.")
            cf = pandas.read_csv(ovcf, delimiter=';')
            if cf.empty:
                print("File " + ovcf + " is empty.")
                print('Skipping all configuration files listed in ', ovcf)
                sys.stdout.flush()
                continue

            for j in cmds_ordered[i]:
                # fname = os.path.basename(j[8])
                sequ_row = cf.loc[cf['conf_file'] == j[8]]
                if sequ_row.empty or sequ_row.shape[0] != 1:
                    raise ValueError('Unable to locate config file ' + j[8] + ' in overview file ' + ovcf)
                parSet = int(sequ_row['parSetNr'])
                small_set = cf.loc[cf['parSetNr'] == parSet].copy(deep=True)
                small_set.loc[:, 'parSetNr'] = [cnt] * int(small_set.shape[0])
                confs_list.append(small_set)
                cnt += 1
            cf_new = pandas.concat(confs_list, axis=0, ignore_index=True)
            # Check which new file name we can use
            f_name_new = os.path.join(i, 'config_files_retry_1.csv')
            cnt = 2
            while os.path.exists(f_name_new):
                f_name_new = os.path.join(i, 'config_files_retry_' + str(cnt) + '.csv')
                cnt += 1
            cf_new.to_csv(index=True, sep=';', path_or_buf=f_name_new)
            folders.append(f_name_new)

        cmd_fails_dir = []
        try:
            res = processDir(folders, nr_cpus, cmds_sequ[0][0], mess_new)
            print()
            if res:
                cmd_fails += res
                print('Finished the following directories with errors:')
            else:
                print('Finished the following directories:')
            print('\n'.join(folders))
            print('\n')
        except ValueError as e:
            print()
            print('Exception during processing a folder with multiple configuration files:')
            print(str(e))
            print('Some of the following directories might be not processed completely:')
            print('\n'.join(folders))
            print('\n')
            cmd_fails_dir.append(folders)
        except:
            print()
            print('Unknown exception in processing directories.')
            e = sys.exc_info()
            print(str(e))
            sys.stdout.flush()
            cmd_fails_dir.append(folders)

        if cmd_fails_dir:
            ret = 1
            res_file = os.path.join(mess_new, 'sequ_matches_dirs_error_overview.txt')
            cnt = 1
            while os.path.exists(res_file):
                res_file = os.path.join(mess_new, 'sequ_matches_dirs_error_overview' + str(cnt) + '.txt')
                cnt = cnt + 1

            with open(res_file, 'w') as fo:
                cmd_fails_tmp = []
                for r in cmd_fails_dir:
                    cmd_fails_tmp.append('\n'.join(r))
                fo.write('\n\n'.join(cmd_fails_tmp))

    if cmd_fails:
        ret += 1
        res_file = os.path.join(mess_new, 'sequ_matches_cmds_error_overview.txt')
        cnt = 1
        while os.path.exists(res_file):
            res_file = os.path.join(mess_new, 'sequ_matches_cmds_error_overview' + str(cnt) + '.txt')
            cnt = cnt + 1

        with open(res_file, 'w') as fo:
            cmd_fails_tmp = []
            for r in cmd_fails:
                cmd_fails_tmp.append(' '.join(map(str, r)))
            fo.write('\n'.join(cmd_fails_tmp))
    return ret


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

def processDir(dirs_list, cpus_rest, executable, message_path):
    cmd_fails = []
    for ovcf in dirs_list:
        cf = pandas.read_csv(ovcf, delimiter=';')
        if cf.empty:
            print("File " + ovcf + " is empty.")
            print('Skipping all configuration files listed in ', ovcf)
            sys.stdout.flush()
            continue
        #err_cnt = 0
        #Split entries into tasks for generating initial sequences and matches only
        c_sequ = cf.loc[cf['scene_exists'] == 0].copy(deep=True)
        c_match = cf.loc[cf['scene_exists'] == 1].copy(deep=True)
        c_match.sort_values('parSetNr', inplace=True)

        #Calculate sequences first using multiple CPUs
        maxd_parallel1 = int(c_sequ.shape[0] / cpus_rest)
        if maxd_parallel1 == 0:
            maxd_parallel1 = 1
        nr_used_cpus1 = min(int(c_sequ.shape[0] / maxd_parallel1), cpus_rest)

        #Generate unique path for storing messages
        dirn = os.path.dirname(ovcf)
        sub_dirs = re.findall(r'[\w-]+', dirn)
        if sub_dirs:
            base = sub_dirs[-1]
        else:
            print('Unable to extract last sub-directory name of ', ovcf)
            print('Skipping all configuration files listed in ', ovcf)
            sys.stdout.flush()
            continue
        mess_new = os.path.join(message_path, base)
        try:
            os.mkdir(mess_new)
        except FileExistsError:
            print('Directory ', mess_new, ' already exists.')
            print('Overwriting previous stderr and stdout message files if file names are equal')
            sys.stdout.flush()

        cmds = []
        for index, row in c_sequ.iterrows():
            if not os.path.exists(row['conf_file']):
                raise ValueError("Configuration file " + row['conf_file'] + " does not exist.")
            if not os.path.exists(row['store_path']):
                raise ValueError("Save path " + row['store_path'] + " does not exist.")
            cmds.append(([executable,
                         '--img_path', row['img_path'],
                         '--img_pref', row['img_pref'],
                         '--store_path', row['store_path'],
                         '--conf_file', row['conf_file']], int(row['parSetNr']), mess_new, nr_used_cpus1))

        cnt_start_fail = None
        with multiprocessing.Pool(processes=nr_used_cpus1) as pool:
            results = [pool.apply_async(processSequences, t) for t in cmds]
            res1 = []
            cnt = 0
            for r in results:
                try:
                    res1.append(r.get())
                    if len(res1[-1]) == 1:
                        if res1[-1][0] == 'noExe' or res1[-1][0] == 'badName':
                            res1[-1][0] = 'Failed to start ' + ' '.join(map(str, cmds[cnt][0]))
                            print('Unable to start a process. Stopping calculations.')
                            sys.stdout.flush()
                            pool.terminate()
                            cnt_start_fail = cnt
                            break
                except FileExistsError:
                    print('Folder for creating sequences is not empty. Stopping calculations.')
                    sys.stdout.flush()
                    pool.terminate()
                    cnt_start_fail = cnt
                    break
                except ChildProcessError:
                    print('Generation of sequence and matches failed.')
                    sys.stdout.flush()
                    # Check if the overview file contains the parameters
                    ov_file = os.path.join(cmds[cnt][0][6], 'sequInfos.yaml')
                    if searchParSetNr(ov_file, cmds[cnt][1]):
                        res1.append(['Failed to generate sequences or matches with parSetNr'
                                     + str(cmds[cnt][1]) + ' using ' + ' '.join(map(str, cmds[cnt][0]))])
                        cmd_fails.append(cmds[cnt][0])
                    else:
                        print('Stopping calculations.')
                        sys.stdout.flush()
                        pool.terminate()
                        cnt_start_fail = cnt
                        break
                except InterruptedError:
                    print('Unable to calculate extrinsics or some user inputs are wrong. Stopping calculations.')
                    sys.stdout.flush()
                    pool.terminate()
                    cnt_start_fail = cnt
                    break
                except TimeoutError:
                    #Check if the overview file contains the parameters
                    ov_file = os.path.join(cmds[cnt][0][6], 'sequInfos.yaml')
                    if searchParSetNr(ov_file, cmds[cnt][1]):
                        res1.append(['Failed to generate sequences or matches (timeout) with parSetNr'
                                     + str(cmds[cnt][1]) + ' using ' + ' '.join(map(str, cmds[cnt][0]))])
                        cmd_fails.append(cmds[cnt][0])
                    else:
                        print('Stopping calculations.')
                        sys.stdout.flush()
                        pool.terminate()
                        cnt_start_fail = cnt
                        break
                except:
                    print('Unknown exception in processing single sequences.')
                    e = sys.exc_info()
                    print(str(e))
                    sys.stdout.flush()
                    # Check if the overview file contains the parameters
                    ov_file = os.path.join(cmds[cnt][0][6], 'sequInfos.yaml')
                    if searchParSetNr(ov_file, cmds[cnt][1]):
                        res1.append(['Failed to generate sequences or matches (unknown exception) with parSetNr'
                                     + str(cmds[cnt][1]) + ' using ' + ' '.join(map(str, cmds[cnt][0]))])
                        cmd_fails.append(cmds[cnt][0])
                    else:
                        print('Stopping calculations.')
                        sys.stdout.flush()
                        pool.terminate()
                        cnt_start_fail = cnt
                        break
                cnt = cnt + 1
            pool.close()
            pool.join()
        if cnt_start_fail:
            for i in range(cnt_start_fail, len(cmds)):
                cmd_fails.append(cmds[i][0])

        cmds_m = []
        res1_2 = []
        if res1:
            res1_parsetnr = [nr for nr in res1 if not len(nr) == 1]
            if res1_parsetnr:
                res1_parsetnr_tmp = []
                for i in res1_parsetnr:
                    res1_parsetnr_tmp.append(i[1])
                res1_parsetnr = res1_parsetnr_tmp
                cnt_m = 1
                notSuccSequ = []
                parSetNr_sv = -1
                for index, row in c_match.iterrows():
                    if parSetNr_sv == -1:
                        parSetNr_sv = int(row['parSetNr'])
                    elif parSetNr_sv == int(row['parSetNr']):
                        cnt_m = cnt_m + 1
                    else:
                        cnt_m = 1
                        parSetNr_sv = int(row['parSetNr'])
                    if not os.path.exists(row['conf_file']):
                        raise ValueError("Configuration file " + row['conf_file'] + " does not exist.")
                    if not os.path.exists(row['store_path']):
                        raise ValueError("Save path " + row['store_path'] + " does not exist.")
                    if int(row['parSetNr']) in res1_parsetnr:
                        ovs_f = os.path.join(row['load_path'], 'sequInfos.yaml')
                        if not os.path.exists(ovs_f):
                            raise ValueError("Sequence overview file " + ovs_f + " does not exist.")
                        try:
                            data = readOpenCVYaml(ovs_f)
                        except:
                            raise ValueError("Unable to read yaml file " + ovs_f)
                        if data:
                            try:
                                data_set = data['parSetNr' + str(int(row['parSetNr']))]
                            except:
                                raise ValueError("Unable to locate parSetNr" + str(int(row['parSetNr']))
                                                 + " in yaml file " + ovs_f)
                            if data_set:
                                try:
                                    sub_path = data_set['hashSequencePars']
                                except:
                                    raise ValueError("Unable to read sequence path from yaml file " + ovs_f)
                                if sub_path:
                                    sub_path = str(sub_path)
                                else:
                                    raise ValueError("Unable to read sequence path from yaml file " + ovs_f)
                            else:
                                raise ValueError("Unable to locate parSetNr" + str(int(row['parSetNr']))
                                                 + " in yaml file " + ovs_f)
                        else:
                            raise ValueError("Unable to read yaml file " + ovs_f)
                        load_path = os.path.join(row['load_path'], sub_path)
                        if not os.path.exists(load_path):
                            raise ValueError("Sequence path " + load_path + " does not exist.")
                        cmds_m.append(([executable,
                                        '--img_path', row['img_path'],
                                        '--img_pref', row['img_pref'],
                                        '--store_path', '*',
                                        '--conf_file', row['conf_file'],
                                        '--load_folder', load_path], int(cnt_m), mess_new, cpus_rest, True))
                        #cnt_m = cnt_m + 1
                    else:
                        notSuccSequ.append(int(row['parSetNr']))
                if notSuccSequ:
                    notSuccSequ = list(dict.fromkeys(notSuccSequ)) #Removes duplicates
                    for i in notSuccSequ:
                        res1_2.append('Not able to generate matches for sequence with parSetNr'
                                       + str(i) + ' as the sequence was probably not generated.')

        if cmds_m:
            res2 = gen_matches_multiprocessing(cmds_m, cmd_fails, cpus_rest)
        else:
            res2 = []

        if res1:
            res_file = os.path.join(mess_new, 'results.txt')
            if os.path.exists(res_file):
                print('results.txt already exists. Overwriting...')
                sys.stdout.flush()

            with open(res_file, 'w') as fo:
                res1_tmp = []
                for r in res1:
                    if len(r) == 1:
                        res1_tmp.append(r[0] + '\n')
                    else:
                        res2_tmp = []
                        for r1 in r:
                            if isinstance(r1, (list, )):
                                res2_tmp.append(' '.join(map(str, r1)))
                            else:
                                res2_tmp.append('parSetNr' + str(r1))
                        res1_tmp.append('\n'.join(res2_tmp))
                fo.write('\n\n'.join(res1_tmp))
                if res1_2:
                    fo.write('\n\n')
                    fo.write('\n'.join(res1_2))
                if res2:
                    fo.write('\n\n')
                    res1_tmp = []
                    for r in res2:
                        if len(r) == 1:
                            res1_tmp.append(r[0] + '\n')
                        else:
                            res2_tmp = []
                            for r1 in r:
                                if isinstance(r1, (list,)):
                                    res2_tmp.append(' '.join(map(str, r1)))
                                else:
                                    res2_tmp.append('Inner-parSetNr: ' + str(r1))
                            res1_tmp.append('\n'.join(res2_tmp))
                    fo.write('\n\n'.join(res1_tmp))

    return cmd_fails


def gen_matches_multiprocessing(cmds_m, cmd_fails, cpus_rest):
    res2 = []
    # maxd_parallel2 = int(len(cmds_m) / cpus_rest)
    # if maxd_parallel2 == 0:
    #     maxd_parallel2 = 1
    # nr_used_cpus2 = min(int(len(cmds_m) / maxd_parallel2), cpus_rest)
    #
    # cmds_tmp = []
    # for t in cmds_m:
    #     lst = list(t)
    #     lst[3] = nr_used_cpus2
    #     cmds_tmp.append(tuple(lst))
    # cmds_m = cmds_tmp
    cnt_start_fail = None
    with multiprocessing.Pool(processes=cpus_rest) as pool:
        results = [pool.apply_async(processSequences, t) for t in cmds_m]
        cnt = 0
        for r in results:
            try:
                res2.append(r.get())
                if len(res2[-1]) == 1:
                    if res2[-1][0] == 'noExe' or res2[-1][0] == 'badName':
                        res2[-1][0] = 'Failed to start ' + ' '.join(map(str, cmds_m[cnt][0]))
                        print('Unable to start a process. Stopping calculations.')
                        sys.stdout.flush()
                        pool.terminate()
                        cnt_start_fail = cnt
                        break
            except FileExistsError:
                print('Something went wrong. Check first used parSetNr. Stopping calculations.')
                sys.stdout.flush()
                pool.terminate()
                cnt_start_fail = cnt
                break
            except ChildProcessError:
                print('Generation of matches failed.')
                sys.stdout.flush()
                res2.append(['Failed to generate ' + ' '.join(map(str, cmds_m[cnt][0]))])
                cmd_fails.append(cmds_m[cnt][0])
            except InterruptedError:
                print('Unable to generate matches as some user inputs are wrong. Stopping calculations.')
                pool.terminate()
                cnt_start_fail = cnt
                break
            except TimeoutError:
                print('Generation of matches failed due to timeout.')
                sys.stdout.flush()
                res2.append(['Failed (timeout) to generate ' + ' '.join(map(str, cmds_m[cnt][0]))])
                cmd_fails.append(cmds_m[cnt][0])
            except:
                print('Unknown exception in generating matches only.')
                e = sys.exc_info()
                print(str(e))
                sys.stdout.flush()
                res2.append(['Failed (Unknown exception) to generate ' + ' '.join(map(str, cmds_m[cnt][0]))])
                cmd_fails.append(cmds_m[cnt][0])
            cnt = cnt + 1
        pool.close()
        pool.join()
    if cnt_start_fail:
        for i in range(cnt_start_fail, len(cmds_m)):
            cmd_fails.append(cmds_m[i][0])
    return res2

def searchParSetNr(ov_file, parSetNr):
    # Check if the overview file contains the parameters
    if os.path.exists(ov_file):
        try:
            data = readOpenCVYaml(ov_file)
        except:
            return False
        if data:
            try:
                data_set = data['parSetNr' + str(parSetNr)]
            except:
                return False
            if data_set:
                return True
            else:
                print('Overview parameters not found after timeout.')
        else:
            print('Overview parameters not readable after timeout.')
    else:
        print('File with overview parameters not found after timeout.')
    return False


def processSequences(cmd_l, parSetNr, message_path, used_cpus, loaded = False):
    # np.random.seed()
    #Check if we have to wait until other sequence generation processes have finished writing into the overview file
    if loaded:
        ov_file = os.path.join(cmd_l[10], 'matchInfos.yaml')
    else:
        ov_file = os.path.join(cmd_l[6], 'sequInfos.yaml')

    if parSetNr != 0:
        if loaded and parSetNr > 1:
            time.sleep(float(min(used_cpus, (parSetNr - 1)) * 10))
        else:
            time.sleep(float(min(parSetNr, used_cpus) * 10))
        cnt = 0
        while not os.path.exists(ov_file) and cnt < 20 and not loaded:
            time.sleep(10)
            cnt = cnt + 1
        if not os.path.exists(ov_file):
            return ['noExe']
        try:
            data = readOpenCVYaml(ov_file)
        except:
            raise BaseException
        cnt = 0
        while not data and cnt < 20 and not loaded:
            time.sleep(1)
            try:
                data = readOpenCVYaml(ov_file)
            except:
                raise BaseException
            cnt = cnt + 1
        if not data:
            return ['noExe']
        cnt1 = 0
        data_set = None
        cnt1max = 15 + min(used_cpus, (parSetNr - 1)) * 15
        while cnt1 < cnt1max:
            try:
                data_set = data['parSetNr' + str(int(parSetNr - 1))]
                break
            except:
                if loaded:
                    print('Waiting for parSetNr ', int(parSetNr - 1))
                cnt1 = cnt1 + 1
                if cnt1 < cnt1max:
                    time.sleep(10)
                    try:
                        data = readOpenCVYaml(ov_file)
                    except:
                        raise BaseException
                    continue
                elif loaded:
                    time.sleep(10)
                    break
                # print('Exception during reading yaml entry.')
                # e = sys.exc_info()
                # print(str(e))
                # sys.stdout.flush()
                # raise BaseException
                return ['noExe']
        # time.sleep(np.random.uniform(0.5, 20.0))
        # cnt = 0
        # while not data_set and cnt < cnt1max:
        #     time.sleep(10)
        #     data = readOpenCVYaml(ov_file)
        #     data_set = data['parSetNr' + str(int(parSetNr - 1))]
        #     cnt = cnt + 1
        # if not data_set and not loaded:
        #     return ['noExe']
        # elif loaded:
        #     #Wait for a few more seconds and start the calculation anyway
        #     time.sleep(10)
    elif os.path.exists(ov_file):
        raise FileExistsError

    #Generate sequence
    cf_name = os.path.basename(cmd_l[8])
    fnObj = re.match('(.+)\..+', cf_name, re.I)
    if fnObj:
        base = fnObj.group(1)
    else:
        return ['badName']
    err_out = 'stderr_' + base + '.txt'
    mess_out = 'stdout_' + base + '.txt'
    fname_cerr = os.path.join(message_path, err_out)
    fname_mess = os.path.join(message_path, mess_out)
    cerrf = open(fname_cerr, 'w')
    messf = open(fname_mess, 'w')
    cnt3 = 3
    result = None
    while cnt3 > 0:
        try:
            sp.run(cmd_l, stdout=messf, stderr=cerrf, check=True, timeout=7200)
            result = [cmd_l, parSetNr]
            cnt3 = 0
        except sp.CalledProcessError as e:
            cnt3 = cnt3 - 1
            print('Failed to generate scene and/or matches!')
            if (cnt3 > 0) and (abs(e.returncode) == 2):
                print('Trying again!')
                sys.stdout.flush()
                cerrf.close()
                messf.close()
                fname_cerr_err = os.path.join(message_path, 'err_' + str(cnt3) + '_' + err_out)
                fname_mess_err = os.path.join(message_path, 'err_' + str(cnt3) + '_' + mess_out)
                os.rename(fname_cerr, fname_cerr_err)
                os.rename(fname_mess, fname_mess_err)
                cerrf = open(fname_cerr, 'w')
                messf = open(fname_mess, 'w')
                continue
            sys.stdout.flush()
            err_filen = 'errorInfo_' + base + '.txt'
            fname_err = os.path.join(message_path, err_filen)
            if e.cmd or e.stderr or e.stdout:
                with open(fname_err, 'w') as fo1:
                    if e.cmd:
                        for line in e.cmd:
                            fo1.write(line + ' ')
                        fo1.write('\n\n')
                    if e.stderr:
                        for line in e.stderr:
                            fo1.write(line + ' ')
                        fo1.write('\n\n')
                    if e.stdout:
                        for line in e.stdout:
                            fo1.write(line + ' ')
            cerrf.close()
            messf.close()
            if abs(e.returncode) == 3:
                raise ChildProcessError
            raise InterruptedError
        except sp.TimeoutExpired as e:
            print('Timeout expired for generating a scene and/or matches.')
            sys.stdout.flush()
            err_filen = 'errorInfoTimeOut_' + base + '.txt'
            fname_err = os.path.join(message_path, err_filen)
            if e.cmd or e.stderr or e.stdout:
                with open(fname_err, 'w') as fo1:
                    if e.cmd:
                        for line in e.cmd:
                            fo1.write(line + ' ')
                        fo1.write('\n\n')
                    if e.stderr:
                        for line in e.stderr:
                            fo1.write(line + ' ')
                        fo1.write('\n\n')
                    if e.stdout:
                        for line in e.stdout:
                            fo1.write(line + ' ')
            cerrf.close()
            messf.close()
            raise TimeoutError
    cerrf.close()
    messf.close()

    if os.stat(fname_cerr).st_size < 4:
        os.remove(fname_cerr)
    if os.stat(fname_mess).st_size < 4:
        os.remove(fname_mess)

    return result


def readOpenCVYaml(file):
    with open(file, 'r') as fi:
        data = fi.readlines()
    data = [line for line in data if line[0] is not '%']
    try:
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


def main():
    parser = argparse.ArgumentParser(description='Generate multiple scenes and matches from configuration files')
    parser.add_argument('--path', type=str, required=False,
                        help='Directory holding file \'generated_dirs_config.txt\'')
    parser.add_argument('--nrCPUs', type=int, required=False, default=4,
                        help='Number of CPU cores for parallel processing. If a negative value is provided, '
                             'the program tries to find the number of available CPUs on the system - if it fails, '
                             'the absolute value of nrCPUs is used. Default: 4')
    parser.add_argument('--executable', type=str, required=False,
                        help='Executable of the application generating the sequences')
    parser.add_argument('--message_path', type=str, required=True,
                        help='Storing path for text files containing error and normal mesages during the '
                             'generation process of scenes and matches')
    parser.add_argument('--retry_dirs_file', type=str, required=False,
                        help='File name including path which holds directory names for which the last scene creation '
                             'process failed and, thus, should be tried again.')
    parser.add_argument('--retry_cmds_file', type=str, required=False,
                        help='File name including path which holds command lines for which the last scene and/or '
                             'matches creation process failed and, thus, should be tried again.')
    args = parser.parse_args()
    if args.path:
        if not args.executable:
            raise ValueError('Missing argument executable')
        if not os.path.exists(args.path):
            raise ValueError('Directory ' + args.path + ' does not exist')
    if args.retry_dirs_file and not args.executable:
        raise ValueError('Missing argument executable')
    if args.executable:
        if not os.path.isfile(args.executable):
            raise ValueError('Executable ' + args.executable + ' for generating scenes does not exist')
        elif not os.access(args.executable, os.X_OK):
            raise ValueError('Unable to execute ' + args.executable)
    if not os.path.exists(args.message_path):
        raise ValueError("Path for storing stdout and stderr does not exist")
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

    if args.path:
        ret = genScenes(args.path, args.executable, cpu_use, args.message_path)
    elif args.retry_dirs_file:
        ret = retry_dirs_from_file(args.retry_dirs_file, args.executable, cpu_use, args.message_path)
    elif not args.retry_cmds_file:
        parser.print_help()
        raise ValueError('Too less arguments.')
    else:
        ret = retry_cmds_from_file(args.retry_cmds_file, cpu_use, args.message_path)
    sys.exit(ret)


if __name__ == "__main__":
    main()


