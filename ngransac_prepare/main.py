"""
Main script file for executing the whole test procedure for testing the autocalibration SW
"""
import sys, re, argparse, os, subprocess as sp, warnings, numpy as np, logging
import pandas as pd
import shutil, zipfile, math


def get_config_file_parameters():
    parameters = {'img_overlap_range': [0.3, 0.7, 0.2],
                  'kp_min_dist_range': [1, 5, 4],
                  'both_kp_err_types': True,
                  'kpAccRange': [0.5, 3.5, 1.0],
                  'imgIntNoiseMeanRange': [10, 30, 10],
                  'imgIntNoiseStdRange': [15, 20, 5]}
    return parameters


def start_testing(path, path_confs_out, skip_gen_sc_conf, skip_crt_sc,
                  img_path, store_path_sequ, load_path, cpu_use,
                  exec_sequ, message_path, log_new_folders, path_converted_out):
    # Generate configuration files
    if not skip_gen_sc_conf:
        ret = gen_config_files(path, path_confs_out, img_path, store_path_sequ, load_path, log_new_folders)
        if ret:
            return ret
        print('Finished generating sequence configuration files')
    # Generate scenes and matches
    if not skip_crt_sc:
        if path_confs_out:
            pco = path_confs_out
        else:
            pco = path
        ret = gen_scenes(pco, exec_sequ, message_path, cpu_use)
        log_new_folders.append(store_path_sequ)
        if ret:
            return ret
        print('Finished generating scenes and matches')

    if path_confs_out:
        pco = path_confs_out
    else:
        pco = path
    ret = convertSequences(pco, path_converted_out, cpu_use, log_new_folders)
    if ret == 0:
        print('Testing finished without errors')
    return ret


def gen_scenes(gen_dirs_config_f, executable, message_path, cpu_use):
    pyfilepath = os.path.dirname(os.path.realpath(__file__))
    pyfilename = os.path.join(pyfilepath, 'create_scenes.py')
    cmdline = ['python', pyfilename, '--path', gen_dirs_config_f, '--nrCPUs', str(cpu_use),
               '--executable', executable, '--message_path', message_path]
    tout = int(3024000 / cpu_use)
    try:
        ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                     check=True, timeout=tout).returncode
    except sp.TimeoutExpired:
        logging.error('Timeout expired for generating scenes and matches using directory ' +
                      gen_dirs_config_f, exc_info=True)
        ret = 98
    except Exception:
        logging.error('Failed to generate scenes and matches using directory ' +
                      gen_dirs_config_f, exc_info=True)
        ret = 99
    if ret:
        if ret < 98:
            logging.error('Failed to generate scenes and matches using directory ' +
                          gen_dirs_config_f)
    return ret


def gen_config_files(path_init_confs, path_confs_out, img_path, store_path_sequ, load_path, log_new_folders):
    if not path_init_confs:
        raise ValueError('Path containing initial configuration files must be provided.')
    if not img_path:
        raise ValueError('Path containing images for creating matches must be provided.')
    if not store_path_sequ:
        raise ValueError('Path for storing scenes and matches must be provided.')
    pars = get_config_file_parameters()
    pyfilepath = os.path.dirname(os.path.realpath(__file__))
    pyfilename = os.path.join(pyfilepath, 'gen_mult_scene_configs.py')
    ret = 3
    cmdline = ['python', pyfilename, '--path', path_init_confs,
               '--img_path', img_path, '--store_path', store_path_sequ]
    if path_confs_out:
        cmdline += ['--path_confs_out', path_confs_out]
        log_new_folders.append(path_confs_out)
    else:
        log_new_folders.append(path_init_confs)
    if load_path:
        cmdline += ['--load_path', load_path]
    for key, val in pars.items():
        if not isinstance(val, bool) or val:
            cmdline.append('--' + key)
        if isinstance(val, list):
            cmdline += ['%.2f' % a for a in val]
        elif not isinstance(val, bool):
            cmdline.append(str(val))
    try:
        ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                     check=True, timeout=120).returncode
    except sp.TimeoutExpired:
        logging.error('Timeout expired for generating sequence configuration files in directory ' +
                      path_init_confs, exc_info=True)
        ret = 98
    except Exception:
        logging.error('Failed to generate sequence configuration files in directory ' +
                      path_init_confs, exc_info=True)
        ret = 99
    if ret:
        if ret < 98:
            logging.error('Failed to generate sequence configuration files in directory ' +
                          path_init_confs)
        return ret
    return ret


def convertSequences(gen_dirs_config_f, path_out, cpu_use, log_new_folders):
    conf = os.path.join(gen_dirs_config_f, 'generated_dirs_config.txt')
    if not os.path.exists(conf):
        print('File ' + conf + 'does not exist', sys.stderr)
        return 98
    dirsc = []
    with open(conf, 'r') as fi:
        # Load directory holding configuration files
        dircf = fi.readline()
        while dircf:
            dircf = dircf.rstrip()
            if dircf:
                ovcf = os.path.join(dircf, 'config_files.csv')
                if not os.path.exists(ovcf):
                    print('Missing file ' + ovcf, sys.stderr)
                    continue
                cf = pd.read_csv(ovcf, delimiter=';')
                if cf.empty:
                    print("File " + ovcf + " is empty.", sys.stderr)
                    continue
                dirsc.append(cf.loc[:, 'store_path'].iloc[0])
            dircf = fi.readline()
    pyfilepath = os.path.dirname(os.path.realpath(__file__))
    pyfilename = os.path.join(pyfilepath, 'readSequMatches.py')
    ret = 0
    log_new_folders.append(path_out)
    for p in dirsc:
        cmdline = ['python', pyfilename, '--path', p, '--nrCPUs', str(cpu_use),
                   '--path_out', path_out]
        try:
            ret += sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                          check=True, timeout=172800).returncode
        except sp.TimeoutExpired:
            logging.error('Timeout expired for generating scenes and matches using directory ' + p, exc_info=True)
            ret = 98
        except Exception:
            logging.error('Failed to generate scenes and matches using directory ' + p, exc_info=True)
            ret = 99
    if ret:
        if ret < 98:
            logging.error('Failed to generate scenes and matches using directory ' + p)
    return ret


def enable_logging(path):
    base = 'error_log_main_level_%03d'
    excmess = os.path.join(path, (base % 1) + '.txt')
    cnt = 2
    while os.path.exists(excmess):
        excmess = os.path.join(path, (base % cnt) + '.txt')
        cnt += 1
    logging.basicConfig(filename=excmess, level=logging.DEBUG)


def retry_scenes_gen_dir(filename, exec_sequ, message_path, cpu_use):
    pyfilepath = os.path.dirname(os.path.realpath(__file__))
    pyfilename = os.path.join(pyfilepath, 'create_scenes.py')
    cmdline = ['python', pyfilename, '--retry_dirs_file', filename,
               '--nrCPUs', str(cpu_use), '--executable', exec_sequ, '--message_path', message_path]
    try:
        ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                     check=True, timeout=18000).returncode
    except sp.TimeoutExpired:
        logging.error('Timeout expired for generating sequences.', exc_info=True)
        ret = 98
    except Exception:
        logging.error('Failed to generate scene and/or matches during retry', exc_info=True)
        ret = 99
    if ret:
        if ret < 98:
            logging.error('Retrying generation of sequences based on all configuration files in directories failed.')
    return ret


def retry_scenes_gen_cmds(filename, message_path, cpu_use):
    pyfilepath = os.path.dirname(os.path.realpath(__file__))
    pyfilename = os.path.join(pyfilepath, 'create_scenes.py')
    cmdline = ['python', pyfilename, '--retry_cmds_file', filename,
               '--nrCPUs', str(cpu_use), '--message_path', message_path]
    try:
        ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                     check=True, timeout=18000).returncode
    except sp.TimeoutExpired:
        logging.error('Timeout expired for generating sequences.', exc_info=True)
        ret = 98
    except Exception:
        logging.error('Failed to generate scene and/or matches during retry', exc_info=True)
        ret = 99
    if ret:
        if ret < 98:
            logging.error('Retrying generation of sequences based on single command lines failed.')
    return ret


def compress_res_folder(zip_res_folder, res_path):
    if not zip_res_folder:
        return
    if res_path[-1] == '/':
        res_path = res_path[:-1]
    res_path_main = os.path.dirname(res_path)
    (parent, tail) = os.path.split(res_path_main)
    save_dir = os.path.join(parent, 'res_save_compressed')
    if not os.path.exists(save_dir):
        os.mkdir(save_dir)
    cnt = 0
    base_name = tail + '_%03d' % cnt
    f_name = os.path.join(save_dir, base_name)
    f_name1 = f_name + '.zip'
    while os.path.exists(f_name1):
        cnt += 1
        base_name = tail + '_%03d' % cnt
        f_name = os.path.join(save_dir, base_name)
        f_name1 = f_name + '.zip'
    shutil.make_archive(f_name, 'zip', parent, tail)


def compress_mess_folder(zip_mess_folder, mess_path, ret=1):
    if not zip_mess_folder or not ret:
        return
    if mess_path[-1] == '/':
        mess_path = mess_path[:-1]
    (res_path_main, tail) = os.path.split(mess_path)
    parent = os.path.dirname(res_path_main)
    save_dir = os.path.join(parent, 'res_save_compressed')
    if not os.path.exists(save_dir):
        os.mkdir(save_dir)
    cnt = 0
    base_name = 'messages_%03d' % cnt
    f_name = os.path.join(save_dir, base_name)
    f_name1 = f_name + '.zip'
    while os.path.exists(f_name1):
        cnt += 1
        base_name = 'messages_%03d' % cnt
        f_name = os.path.join(save_dir, base_name)
        f_name1 = f_name + '.zip'
    shutil.make_archive(f_name, 'zip', res_path_main, tail)


def IsPathValid(parentDir, path, rootDir, ignoreDir, ignoreExt, useOnlyDir):
    splitted = None
    main_dir = os.path.join(parentDir, path)
    is_file = os.path.isfile(main_dir)
    if is_file:
        if ignoreExt:
            _, ext = os.path.splitext(path)
            if ext in ignoreExt:
                return False
        if not parentDir:
            parentDir = main_dir
        main_dir = os.path.abspath(os.path.dirname(main_dir))
        splitted = re.split(r'\\|/', main_dir)
    else:
        main_dir = os.path.join(parentDir, path)
        splitted = re.split(r'\\|/', path)

    if useOnlyDir:
        take_dir = []
        sim_dirs = []
        for uod in useOnlyDir:
            try:
                if os.path.samefile(uod, main_dir):
                    return True
                cdp = common_dir_prefix([main_dir, uod])
                if os.path.samefile(cdp, parentDir):
                    if os.path.samefile(uod, main_dir):
                        take_dir.append(True)
                        sim_dirs.append(uod)
                    elif main_dir.startswith(os.path.abspath(uod)+os.sep):
                        return True
                    else:
                        take_dir.append(False)  # Is only equal to main path
                elif cdp.startswith(os.path.abspath(parentDir)+os.sep):
                    take_dir.append(True)#Is subdir
                    sim_dirs.append(uod)
                elif main_dir.startswith(os.path.abspath(uod) + os.sep):
                    return True
                else:
                    take_dir.append(False)#Is not even equal to main path
            except:
                pass
        if not any(take_dir):
            return False

        if is_file:
            for i in sim_dirs:
                try:
                    if os.path.samefile(main_dir, i) or main_dir.startswith(os.path.abspath(i) + os.sep):
                        return True
                except:
                    pass
        else:
            for i in sim_dirs:
                if main_dir.startswith(os.path.abspath(i) + os.sep):
                    return True
                rel_ps = re.split(r'\\|/', os.path.relpath(i, parentDir))
                if len(rel_ps) == 1:
                    try:
                        if not os.path.samefile(i, main_dir):
                            return False
                    except:
                        return False

    if not is_file and not ignoreDir:
        return True

    if ignoreDir:
        for s in splitted:
            if s in ignoreDir:  # You can also use set.intersection or [x for],
                return False

    return True


def common_dir_prefix(paths):
    c_pre = os.path.commonprefix(paths)
    if not os.path.exists(c_pre):
        c_pre = os.path.dirname(c_pre)
    return c_pre


def zipDirHelper(path, rootDir, zf, ignoreDir=None, ignoreExt=None, useOnlyDir=None):
    # zf is zipfile handle
    if os.path.isfile(path):
        if IsPathValid('', path, rootDir, ignoreDir, ignoreExt, useOnlyDir):
            relative = os.path.relpath(path, rootDir)
            zf.write(path, relative)
        return

    ls = os.listdir(path)
    for subFileOrDir in ls:
        if not IsPathValid(path, subFileOrDir, rootDir, ignoreDir, ignoreExt, useOnlyDir):
            continue

        joinedPath = os.path.join(path, subFileOrDir)
        zipDirHelper(joinedPath, rootDir, zf, ignoreDir, ignoreExt, useOnlyDir)


def ZipDir(path, zf, ignoreDir=None, ignoreExt=None, useOnlyDir=None):
    rootDir = path if os.path.isdir(path) else os.path.dirname(path)
    if ignoreExt:
        for idx, i in enumerate(ignoreExt):
            if i[0] != '.':
                ignoreExt[idx] = '.' + i
    zipDirHelper(path, rootDir, zf, ignoreDir, ignoreExt, useOnlyDir)
    # pass


def compress_new_dirs(zip_new_folders, res_path, log_new_folders, message_path, ignoreDirs=None, ignoreExts=None):
    if not zip_new_folders:
        return
    log_new_folders.append(message_path)
    if res_path[-1] == '/':
        res_path = res_path[:-1]
    res_path_main = os.path.dirname(res_path)
    (parent, tail) = os.path.split(res_path_main)
    save_dir = os.path.join(parent, 'res_save_compressed')
    if not os.path.exists(save_dir):
        os.mkdir(save_dir)
    cnt = 0
    base_name = tail + '_new_folders_%03d' % cnt
    f_name = os.path.join(save_dir, base_name)
    f_name1 = f_name + '.zip'
    while os.path.exists(f_name1):
        cnt += 1
        base_name = tail + '_new_folders_%03d' % cnt
        f_name = os.path.join(save_dir, base_name)
        f_name1 = f_name + '.zip'
    if log_new_folders:
        log_new_folders = list(dict.fromkeys(log_new_folders))
    theZipFile = zipfile.ZipFile(f_name1, 'w')
    # ZipDir(res_path_main, theZipFile, ignoreDir=[".svn"], ignoreExt=[".zip"], useOnlyDir=log_new_folders)
    ZipDir(res_path_main, theZipFile, ignoreDirs, ignoreExts, log_new_folders)
    theZipFile.close()


def log_autoc_folders(test_name, test_nr, store_path_cal, log_new_folders, is_eval_only=False):
    path_to_tests = os.path.join(store_path_cal, test_name)
    if test_nr:
        path_to_tests = os.path.join(path_to_tests, str(test_nr))
    if is_eval_only:
        path_to_tests = os.path.join(path_to_tests, 'evals')
    if os.path.exists(path_to_tests):
        log_new_folders.append(path_to_tests)


def main():
    parser = argparse.ArgumentParser(description='Main script file for executing the whole test procedure for '
                                                 'testing the autocalibration SW')
    parser.add_argument('--path', type=str, required=False,
                        help='Directory holding directories with template configuration files')
    parser.add_argument('--path_confs_out', type=str, required=False,
                        help='Optional directory for writing configuration files. If not available, '
                             'the directory is derived from argument \'path\' if '
                             'option \'complete_res_path\' is not provided.')
    parser.add_argument('--skip_gen_sc_conf', type=str, nargs='+', required=False,
                        help='List of test names for which the generation of configuration files out of '
                             'initial configuration files should be skipped as they are already available. '
                             'Possible tests: usac-testing, correspondence_pool, robustness, usac_vs_autocalib; '
                             'Format: test1 test2 ...')
    parser.add_argument('--skip_crt_sc', type=str, nargs='+', required=False,
                        help='List of test names for which the creation of scenes '
                             'should be skipped as they are already available. '
                             'Possible tests: usac-testing, correspondence_pool, robustness, usac_vs_autocalib; '
                             'Format: test1 test2 ...')
    parser.add_argument('--crt_sc_dirs_file', type=str, required=False,
                        help='Optional (only used when scene creation process failed for entire directory/ies): '
                             'File holding directory names which include configuration files to generate '
                             'multiple scenes.')
    parser.add_argument('--crt_sc_cmds_file', type=str, required=False,
                        help='Optional (only used when scene creation process failed for a few scenes): '
                             'File holding command lines to generate scenes.')
    parser.add_argument('--img_path', type=str, required=False,
                        help='Path to images')
    parser.add_argument('--store_path_sequ', type=str, required=False,
                        help='Storing path for generated scenes and matches')
    parser.add_argument('--load_path', type=str, required=False,
                        help='Optional loading path for generated scenes and matches. '
                             'If not provided, store_path is used.')
    parser.add_argument('--nrCPUs', type=int, required=False, default=-16,
                        help='Number of CPU cores for parallel processing. If a negative value is provided, '
                             'the program tries to find the number of available CPUs on the system - if it fails, '
                             'the absolute value of nrCPUs is used. Default: -16')
    parser.add_argument('--exec_sequ', type=str, required=False,
                        help='Executable of the application generating the sequences')
    parser.add_argument('--message_path', type=str, required=False,
                        help='Storing path for text files containing error and normal messages')
    parser.add_argument('--store_path_converted', '-pc', type=str, required=False,
                        help='Main output path for converted data.')
    parser.add_argument('--complete_res_path', type=str, required=False, default='default',
                        help='If provided or a folder \'results_train\' exists in the parent directory (../), '
                             'the full path structure for storing data is generated at the given '
                             'location except for parameters that were explicitely provided. Moreover, the input path '
                             'for initial configuration files is expected to be in \'config_files\' within the '
                             'directory holding this python file except option \'path\' is provided. '
                             'The input images should be located in a folder called \'images\' one folder level up '
                             'compared to the directory holding this python file (../images/). The latter also holds '
                             'for the executable generating sequences which should be located in '
                             '\'../generateVirtualSequence/build/\' and should be named '
                             '\'virtualSequenceLib-CMD-interface\'.')
    parser.add_argument('--zip_res_folder', type=bool, nargs='?', required=False, default=False, const=True,
                        help='If provided, the whole content within the main results folder is compressed and stored '
                             'in the parent directory of the results folder. If a zipped file exists, a new filename '
                             'is created. Thus, it could be used for versioning.')
    parser.add_argument('--zip_message_folder', type=bool, nargs='?', required=False, default=False, const=True,
                        help='If provided, the whole content within the messages folder is compressed and stored '
                             'in the parent directory of the results folder if an error occured. If a zipped file '
                             'exists, a new filename is created.')
    parser.add_argument('--zip_new_folders', type=str, nargs='?', required=False, default=None, const='noSpecific',
                        help='If provided, all folders where new files were created within the main results folder '
                             'are compressed and stored in the parent directory of the results folder. If a zipped '
                             'file exists, a new filename is created. Furthermore, a main test name can be provided'
                             'to take only folders created within this main test (only used when creating sequences).')
    args = parser.parse_args()
    if args.path and not os.path.exists(args.path):
        raise ValueError('Directory ' + args.path + ' holding directories with template scene '
                                                    'configuration files does not exist')
    if args.path_confs_out and not os.path.exists(args.path_confs_out):
        raise ValueError('Directory ' + args.path_confs_out + ' for storing config files does not exist')
    if args.crt_sc_dirs_file and not os.path.exists(args.crt_sc_dirs_file):
        raise ValueError('File ' + args.crt_sc_dirs_file + ' does not exist.')
    elif args.crt_sc_dirs_file and not args.exec_sequ:
        raise ValueError('Executable for generating scenes must be provided')
    if args.crt_sc_cmds_file and not os.path.exists(args.crt_sc_cmds_file):
        raise ValueError('File ' + args.crt_sc_cmds_file + ' does not exist.')
    if args.img_path and not os.path.exists(args.img_path):
        raise ValueError("Image path does not exist")
    if args.store_path_sequ and not os.path.exists(args.store_path_sequ):
        raise ValueError("Path for storing sequences does not exist")
    if args.store_path_sequ and len(os.listdir(args.store_path_sequ)) != 0 and \
            not (args.crt_sc_dirs_file or args.crt_sc_cmds_file):
        raise ValueError("Path for storing sequences is not empty")
    if args.load_path:
        if not os.path.exists(args.load_path):
            raise ValueError("Path for loading sequences does not exist")
    if args.complete_res_path and args.complete_res_path == 'default':
        pyfilepath = os.path.dirname(os.path.realpath(__file__))
        parent = os.path.dirname(pyfilepath)
        res_folder = os.path.join(parent, 'results_train')
        if os.path.exists(res_folder):
            args.complete_res_path = res_folder
        else:
            args.complete_res_path = None
    if not args.complete_res_path and (not args.message_path or not os.path.exists(args.message_path) or
                                       not os.path.exists(args.store_path_converted) or
                                       not os.path.exists(args.store_path_sequ) or
                                       not os.path.exists(args.img_path)):
        raise ValueError("Missing paths")
    if args.exec_sequ:
        if not os.path.isfile(args.exec_sequ):
            raise ValueError('Executable ' + args.exec_sequ + ' for generating scenes does not exist')
        elif not os.access(args.exec_sequ, os.X_OK):
            raise ValueError('Unable to execute ' + args.exec_sequ)
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
    print('Using ', cpu_use, ' CPUs for testing')
    sys.stdout.flush()
    if args.complete_res_path:
        if not os.path.exists(args.complete_res_path):
            raise ValueError('Folder ' + args.complete_res_path + ' does not exist')
        pyfilepath = os.path.dirname(os.path.realpath(__file__))
        parent = os.path.dirname(pyfilepath)
        if not args.path:
            args.path = os.path.join(pyfilepath, 'config_files')
            if not os.path.exists(args.path):
                raise ValueError('Missing initial configuration main folder within python files folder')
        if not args.path_confs_out:
            args.path_confs_out = os.path.join(args.complete_res_path, 'conf_files_generated')
            try:
                os.mkdir(args.path_confs_out)
            except FileExistsError:
                pass
        if not args.img_path:
            args.img_path = os.path.join(parent, 'images')
            if not os.path.exists(args.img_path):
                raise ValueError('Missing image folder at: ' + args.img_path)
        if not args.store_path_sequ:
            args.store_path_sequ = os.path.join(args.complete_res_path, 'sequences_generated')
            try:
                os.mkdir(args.store_path_sequ)
            except FileExistsError:
                pass
        if not args.exec_sequ:
            args.exec_sequ = os.path.join(parent, 'generateVirtualSequence/build/virtualSequenceLib-CMD-interface')
            if not os.path.isfile(args.exec_sequ):
                raise ValueError('Executable ' + args.exec_sequ + ' for generating scenes does not exist')
            elif not os.access(args.exec_sequ, os.X_OK):
                raise ValueError('Unable to execute ' + args.exec_sequ)
        if not args.message_path:
            args.message_path = os.path.join(args.complete_res_path, 'messages')
            try:
                os.mkdir(args.message_path)
            except FileExistsError:
                pass
        if not args.store_path_converted:
            args.store_path_converted = os.path.join(args.complete_res_path, 'conversion_results')
            try:
                os.mkdir(args.store_path_converted)
            except FileExistsError:
                pass
    elif not args.store_path_converted:
        raise ValueError('Path for storing results missing')

    log_new_folders = []
    enable_logging(args.message_path)

    if args.crt_sc_dirs_file and args.crt_sc_cmds_file:
        ret = retry_scenes_gen_dir(args.crt_sc_dirs_file, args.exec_sequ, args.message_path, cpu_use)
        ret += retry_scenes_gen_cmds(args.crt_sc_cmds_file, args.message_path, cpu_use)
        compress_res_folder(args.zip_res_folder, args.store_path_sequ)
        compress_mess_folder(args.zip_message_folder, args.message_path, ret)
        log_new_folders.append(args.store_path_sequ)
        compress_new_dirs(args.zip_new_folders, args.store_path_sequ, log_new_folders, args.message_path)
        sys.exit(ret)
    if args.crt_sc_dirs_file:
        ret = retry_scenes_gen_dir(args.crt_sc_dirs_file, args.exec_sequ, args.message_path, cpu_use)
        compress_res_folder(args.zip_res_folder, args.store_path_sequ)
        compress_mess_folder(args.zip_message_folder, args.message_path, ret)
        log_new_folders.append(args.store_path_sequ)
        compress_new_dirs(args.zip_new_folders, args.store_path_sequ, log_new_folders, args.message_path)
        sys.exit(ret)
    if args.crt_sc_cmds_file:
        ret = retry_scenes_gen_cmds(args.crt_sc_cmds_file, args.message_path, cpu_use)
        compress_res_folder(args.zip_res_folder, args.store_path_sequ)
        compress_mess_folder(args.zip_message_folder, args.message_path, ret)
        log_new_folders.append(args.store_path_sequ)
        compress_new_dirs(args.zip_new_folders, args.store_path_sequ, log_new_folders, args.message_path)
        sys.exit(ret)

    try:
        ret = start_testing(args.path, args.path_confs_out, args.skip_gen_sc_conf, args.skip_crt_sc,
                            args.img_path, args.store_path_sequ, args.load_path, cpu_use,
                            args.exec_sequ, args.message_path, log_new_folders, args.store_path_converted)
    except Exception:
        logging.error('Error in main file', exc_info=True)
        ret = 99
    compress_res_folder(args.zip_res_folder, args.store_path_sequ)
    compress_mess_folder(args.zip_message_folder, args.message_path, ret)
    compress_new_dirs(args.zip_new_folders, args.store_path_sequ, log_new_folders, args.message_path)
    logging.shutdown()
    sys.exit(ret)


if __name__ == "__main__":
    main()