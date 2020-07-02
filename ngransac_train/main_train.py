import util, sys
import subprocess as sp, os

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

opt = parser.parse_args()

pyfilepath = os.path.dirname(os.path.realpath(__file__))
if opt.path == 'default':
    parent = os.path.dirname(pyfilepath)
    res_folder = os.path.join(parent, 'results_train')
    if os.path.exists(res_folder):
        opt.path = res_folder
    else:
        print('Provide path', sys.stderr)
        sys.exit(1)
cmdline1 = ['--path', opt.path, '--nfeatures', str(opt.nfeatures), '--ratio', str(opt.ratio),
            '--threshold', str(opt.threshold), '--resblocks', str(opt.resblocks), '--batchsize', str(opt.batchsize)]
if opt.fmat:
    cmdline1.append('--fmat')
if opt.rootsift:
    cmdline1.append('--rootsift')
if opt.orb:
    cmdline1.append('--orb')
if opt.nosideinfo:
    cmdline1.append('--nosideinfo')
if opt.session:
    cmdline1 += ['--session', opt.session]

if not opt.model:
    #Initialize weights
    pyfilename = os.path.join(pyfilepath, 'ngransac_train_init_virtSequ.py')
    cmdline = ['python', pyfilename, '--variant', opt.variant_train,
               '--learningrate', str(opt.learningrateInit), '--epochs', str(opt.epochsInit)]
    if cmdline1:
        cmdline += cmdline1

    try:
        ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                     check=True, timeout=172800).returncode
    except sp.TimeoutExpired:
        print('Timeout expired for initialzing net weights.', sys.stderr)
        ret = 98
    except Exception:
        print('Failed to initialze net weights.', sys.stderr)
        ret = 99

    if ret != 0:
        sys.exit(1)

    session_string = util.create_session_string('init', opt.fmat, opt.orb, opt.rootsift, opt.ratio, opt.session)
    out_folder = os.path.join(opt.path, 'training_results')
    net_file = os.path.join(out_folder, 'weights_%s.net' % (session_string))
else:
    net_file = opt.model

#Perform e2e training
pyfilename = os.path.join(pyfilepath, 'ngransac_train_e2e_virtSequ.py')
cmdline = ['python', pyfilename, '--path', opt.path, '--variant', opt.variant_train, '--hyps', opt.hyps_e2e,
           '--learningrate', str(opt.learningrate_e2e), '--epochs', str(opt.epochs_e2e), '--model', net_file,
           '--samplecount', str(opt.samplecount), '--loss', opt.loss]
if opt.refine_e2e:
    cmdline.append('--refine')
if cmdline1:
    cmdline += cmdline1
try:
    ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                 check=True, timeout=172800).returncode
except sp.TimeoutExpired:
    print('Timeout expired for training network.', sys.stderr)
    ret = 98
except Exception:
    print('Failed to train network.', sys.stderr)
    ret = 99

if ret != 0:
    sys.exit(1)

#Perform testing
pyfilename = os.path.join(pyfilepath, 'ngransac_test_virtSequ.py')
cmdline = ['python', pyfilename, '--path', opt.path, '--variant', opt.variantTest, '--hyps', opt.hypsTest,
           '--model', net_file, '--evalbinsize', str(opt.evalbinsize)]
if opt.refineTest:
    cmdline.append('--refine')
if cmdline1:
    cmdline += cmdline1
try:
    ret = sp.run(cmdline, shell=False, stdout=sys.stdout, stderr=sys.stderr,
                 check=True, timeout=172800).returncode
except sp.TimeoutExpired:
    print('Timeout expired for testing network.', sys.stderr)
    ret = 98
except Exception:
    print('Failed to test network.', sys.stderr)
    ret = 99

if ret != 0:
    sys.exit(1)

sys.exit(0)
