import util, sys, math
import subprocess as sp, os
import multiprocessing
import multiprocessing.pool


# To allow multiprocessing (children) during multiprocessing
class NoDaemonProcess(multiprocessing.Process):
    # make 'daemon' attribute always return False
    def _get_daemon(self):
        return False

    def _set_daemon(self, value):
        pass
    daemon = property(_get_daemon, _set_daemon)


# To allow multiprocessing (children) during multiprocessing
# We sub-class multiprocessing.pool.Pool instead of multiprocessing.Pool
# because the latter is only a wrapper function, not a proper class.
class MyPool(multiprocessing.pool.Pool):
    Process = NoDaemonProcess


def perform_training(pyfilepath, model, variant_train, learningrateInit, epochsInit, fmat, orb, rootsift,
                     ratio, session, path, hyps_e2e, learningrate_e2e, epochs_e2e, samplecount, loss, refine_e2e,
                     variantTest, hypsTest, evalbinsize, refineTest, nfeatures, threshold, resblocks, batchsize,
                     nosideinfo, nrCPUs):
    cmdline1 = ['--path', path, '--nfeatures', str(nfeatures), '--ratio', str(ratio), '--nrCPUs', str(nrCPUs),
                '--threshold', str(threshold), '--resblocks', str(resblocks), '--batchsize', str(batchsize)]
    if fmat:
        cmdline1.append('--fmat')
    if rootsift:
        cmdline1.append('--rootsift')
    if orb:
        cmdline1.append('--orb')
    if nosideinfo:
        cmdline1.append('--nosideinfo')
    if session:
        cmdline1 += ['--session', session]

    if not model:
        # Initialize weights
        pyfilename = os.path.join(pyfilepath, 'ngransac_train_init_virtSequ.py')
        cmdline = ['python', pyfilename, '--variant', variant_train,
                   '--learningrate', str(learningrateInit), '--epochs', str(epochsInit)]
        if cmdline1:
            cmdline += cmdline1

        try:
            ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                         check=True, timeout=345600).returncode
        except sp.TimeoutExpired:
            print('Timeout expired for initialzing net weights.', sys.stderr)
            ret = 98
        except Exception:
            print('Failed to initialze net weights.', sys.stderr)
            ret = 99

        if ret != 0 and ret != 98:
            return ret

        session_string = util.create_session_string('init', fmat, orb, rootsift, ratio, session)
        out_folder = os.path.join(path, 'training_results')
        net_file = os.path.join(out_folder, 'weights_%s.net' % (session_string))
    else:
        net_file = model

    # Perform e2e training
    pyfilename = os.path.join(pyfilepath, 'ngransac_train_e2e_virtSequ.py')
    cmdline = ['python', pyfilename, '--path', path, '--variant', variant_train, '--hyps', hyps_e2e,
               '--learningrate', str(learningrate_e2e), '--epochs', str(epochs_e2e), '--model', net_file,
               '--samplecount', str(samplecount), '--loss', loss]
    if refine_e2e:
        cmdline.append('--refine')
    if cmdline1:
        cmdline += cmdline1
    try:
        ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                     check=True, timeout=432000).returncode
    except sp.TimeoutExpired:
        print('Timeout expired for training network.', sys.stderr)
        ret = 98
    except Exception:
        print('Failed to train network.', sys.stderr)
        ret = 99

    if ret != 0 and ret != 98:
        return ret

    # Perform testing
    pyfilename = os.path.join(pyfilepath, 'ngransac_test_virtSequ.py')
    cmdline = ['python', pyfilename, '--path', path, '--variant', variantTest, '--hyps', hypsTest,
               '--model', net_file, '--evalbinsize', str(evalbinsize)]
    if refineTest:
        cmdline.append('--refine')
    if cmdline1:
        cmdline += cmdline1
    try:
        ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                     check=True, timeout=259200).returncode
    except sp.TimeoutExpired:
        print('Timeout expired for testing network.', sys.stderr)
        ret = 98
    except Exception:
        print('Failed to test network.', sys.stderr)
        ret = 99

    return ret


def main():
    # parse command line arguments
    parser = util.create_parser(description = "Train a neural guidance network using correspondence "
                      "distance to a ground truth model to calculate target probabilities.")

    parser.add_argument('--path', '-p', required=True, default='default', help='Path to folders train and validate')

    parser.add_argument('--variant_train', '-ve', default='train',	choices=['train', 'validate'],
                        help='subfolder of the dataset to use for training')

    parser.add_argument('--learningrateInit', '-lri', type=float, default=0.001,
                        help='learning rate for initializing model')

    parser.add_argument('--epochsInit', '-ei', type=int, default=1000, help='number of epochs for initializing')

    parser.add_argument('--model', '-m', default='',
                        help='load a model to contuinue training or leave empty to create a new model')

    parser.add_argument('--hyps_e2e', '-hypse', type=int, default=16,
                        help='number of hypotheses, i.e. number of RANSAC iterations')

    parser.add_argument('--samplecount', '-ss', type=int, default=4,
                        help='number of samples when approximating the expectation')

    parser.add_argument('--learningrate_e2e', '-lre', type=float, default=0.00001, help='learning rate for e2e training')

    parser.add_argument('--loss', '-l', choices=['pose', 'inliers', 'f1', 'epi'], default='pose',
                        help='Loss to use as a reward signal; "pose" means max of translational and rotational angle error, '
                             '"inliers" maximizes the inlier count (self-supervised training), '
                             '"f1" is the alignment of estimated inliers and ground truth inliers '
                             '(only for fundamental matrixes, i.e. -fmat), '
                             '"epi" is the mean epipolar error of inliers to ground truth epi lines '
                             '(only for fundamental matrixes, i.e. -fmat)')

    parser.add_argument('--epochs_e2e', '-ee', type=int, default=100, help='number of epochs for e2e training')

    parser.add_argument('--refine_e2e', '-refe',
                        help='refine using the 8point algorithm on all inliers during training, '
                             'only used for fundamental matrix estimation (-fmat)')

    parser.add_argument('--variantTest', '-vt', default='validate', choices=['train', 'validate'],
                        help='subfolder of the dataset to use for testing')

    parser.add_argument('--hypsTest', '-hypst', type=int, default=1000,
                        help='number of hypotheses for testing, i.e. number of RANSAC iterations')

    parser.add_argument('--evalbinsize', '-eb', type=float, default=5,
                        help='bin size when calculating the AUC evaluation score, 5 was used by Yi et al., a'
                             'nd therefore also in the NG-RANSAC paper for reasons of comparability; for accurate AUC '
                             'values, set to e.g. 0.1')

    parser.add_argument('--refineTest', '-reft',
                        help='refine using the 8point algorithm on all inliers for testing, '
                             'only used for fundamental matrix estimation (-fmat)')

    parser.add_argument('--nfeaturesMult', '-nfm', type='+', default=None, required=False,
            help='Multiple values for option nfeaturesMult: fixes number of features by clamping/replicating, '
                 'set to -1 for dynamic feature count but then batchsize (-bs) has to be set to 1')

    parser.add_argument('--noAndSideinfo', '-noas', type='?', const=True, default=False, required=False,
            help='Both options (with and without sideinfo) are used: Do and do not provide side information '
                 '(matching ratios) to the network. The network should be trained and tested consistently.')

    parser.add_argument('--multsessions', '-sids', type='+', default=None, required=False,
            help='custom session name appended to output files, useful to separate different runs of a script')

    parser.add_argument('--nrCPUs', type=int, required=False, default=-6,
                        help='Number of CPU cores for parallel processing. If a negative value is provided, '
                             'the program tries to find the number of available CPUs on the system - if it fails, '
                             'the absolute value of nrCPUs is used. Default: -6')

    opt = parser.parse_args()

    if opt.nrCPUs > 72 or opt.nrCPUs == 0:
        raise ValueError("Unable to use " + str(opt.nrCPUs) + " CPU cores.")
    av_cpus = os.cpu_count()
    if av_cpus:
        if opt.nrCPUs < 0:
            cpu_use = av_cpus
        elif opt.nrCPUs > av_cpus:
            print('Demanded ' + str(opt.nrCPUs) + ' but only ' + str(av_cpus) + ' CPUs are available. Using '
                  + str(av_cpus) + ' CPUs.')
            cpu_use = av_cpus
        else:
            cpu_use = opt.nrCPUs
    elif opt.nrCPUs < 0:
        print('Unable to determine # of CPUs. Using ' + str(abs(opt.nrCPUs)) + ' CPUs.')
        cpu_use = abs(opt.nrCPUs)
    else:
        cpu_use = opt.nrCPUs

    pyfilepath = os.path.dirname(os.path.realpath(__file__))
    if opt.path == 'default':
        parent = os.path.dirname(pyfilepath)
        res_folder = os.path.join(parent, 'results_train')
        if os.path.exists(res_folder):
            res_folder = os.path.join(res_folder, 'conversion_results')
            if os.path.exists(res_folder):
                opt.path = res_folder
            else:
                print('Provide path', sys.stderr)
                sys.exit(1)
            opt.path = res_folder
        else:
            print('Provide path', sys.stderr)
            sys.exit(1)

    if not opt.nfeaturesMult and not opt.noAndSideinfo:
        ret = perform_training(pyfilepath, opt.model, opt.variant_train, opt.learningrateInit, opt.epochsInit,
                               opt.fmat, opt.orb, opt.rootsift, opt.ratio, opt.session, opt.path, opt.hyps_e2e,
                               opt.learningrate_e2e, opt.epochs_e2e, opt.samplecount, opt.loss, opt.refine_e2e,
                               opt.variantTest, opt.hypsTest, opt.evalbinsize, opt.refineTest, opt.nfeatures,
                               opt.threshold, opt.resblocks, opt.batchsize, opt.nosideinfo, opt.nrCPUs)
    else:
        if not opt.nfeaturesMult:
            opt.nfeaturesMult = [opt.nfeatures]
        nr_training = len(opt.nfeaturesMult) * (2 if opt.noAndSideinfo else 1)
        if opt.multsessions and len(opt.multsessions) != nr_training:
            raise ValueError('Number of session strings wrong')
        elif not opt.multsessions:
            opt.multsessions = []
            for features in opt.nfeaturesMult:
                opt.multsessions.append('nfeat' + str(features))
            if opt.noAndSideinfo:
                tmp = []
                for a in opt.multsessions:
                    tmp.append(a + '_withSideInfo')
                    tmp.append(a + '_noSideInfo')
                opt.multsessions = tmp
        cpu_part = int(math.ceil(cpu_use / nr_training))
        cmds = []
        cnt = 0
        for features in opt.nfeaturesMult:
            batchsize = 1 if features == -1 else opt.batchsize
            if not opt.noAndSideinfo:
                cmds.append((pyfilepath, opt.model, opt.variant_train, opt.learningrateInit, opt.epochsInit,
                               opt.fmat, opt.orb, opt.rootsift, opt.ratio, opt.multsessions[cnt], opt.path, opt.hyps_e2e,
                               opt.learningrate_e2e, opt.epochs_e2e, opt.samplecount, opt.loss, opt.refine_e2e,
                               opt.variantTest, opt.hypsTest, opt.evalbinsize, opt.refineTest, features,
                               opt.threshold, opt.resblocks, batchsize, opt.nosideinfo, cpu_part))
            else:
                cmds.append((pyfilepath, opt.model, opt.variant_train, opt.learningrateInit, opt.epochsInit,
                             opt.fmat, opt.orb, opt.rootsift, opt.ratio, opt.multsessions[cnt], opt.path, opt.hyps_e2e,
                             opt.learningrate_e2e, opt.epochs_e2e, opt.samplecount, opt.loss, opt.refine_e2e,
                             opt.variantTest, opt.hypsTest, opt.evalbinsize, opt.refineTest, features,
                             opt.threshold, opt.resblocks, batchsize, False, cpu_part))
                cnt += 1
                cmds.append((pyfilepath, opt.model, opt.variant_train, opt.learningrateInit, opt.epochsInit,
                             opt.fmat, opt.orb, opt.rootsift, opt.ratio, opt.multsessions[cnt], opt.path, opt.hyps_e2e,
                             opt.learningrate_e2e, opt.epochs_e2e, opt.samplecount, opt.loss, opt.refine_e2e,
                             opt.variantTest, opt.hypsTest, opt.evalbinsize, opt.refineTest, features,
                             opt.threshold, opt.resblocks, batchsize, True, cpu_part))
        res = 0
        cnt_dot = 0
        with MyPool(processes=cpu_use) as pool:
            results = [pool.apply_async(perform_training, t) for t in cmds]

            for r in results:
                time_out_cnt = 0
                while 1:
                    sys.stdout.flush()
                    try:
                        res += r.get(2.0)
                        break
                    except multiprocessing.TimeoutError:
                        if cnt_dot >= 90:
                            print()
                            cnt_dot = 0
                        sys.stdout.write('.')
                        cnt_dot = cnt_dot + 1
                        time_out_cnt += 1
                        if time_out_cnt > 518400:
                            res = 4
                            print('Timeout for executing python process for starting autocalib reached.')
                            pool.terminate()
                            break
                    except ChildProcessError:
                        res = 1
                        break
                    except TimeoutError:
                        res = 2
                        break
                    except:
                        res = 3
                        break

    if res != 0:
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()