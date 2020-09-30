import numpy as np
import os, sys
from shutil import copy
import statistics

from dataset_notorch_virtSequ import SparseDataset
import util_virtSequ

parser = util_virtSequ.create_parser(
	description = "Copy correspondences with a higher inlier ratio than minInlRat to a different folder.")

parser.add_argument('--path', '-p',
	required=True,
	help='Path to folders train and validate')

parser.add_argument('--path_out', '-po',
	required=True,
	help='Output path')

parser.add_argument('--variant', '-v', default='train', choices=['train', 'validate'],
	help='subfolder of the dataset to use')

parser.add_argument('--minInlRat', '-irt', type=float, default=0.29,
                        help='Threshold on the inlier ratio to take only ratios above given ratio')

opt = parser.parse_args()

print("")

# construct folder that should contain pre-calculated correspondences
data_folder = os.path.join(opt.path, opt.variant)
if not os.path.exists(data_folder):
	raise ValueError('Path ' + data_folder + ' does not exist')
data_folder = [data_folder + '/']

if not os.path.exists(opt.path_out):
	raise ValueError('Path ' + opt.path_out + ' does not exist')
out_folder = os.path.join(opt.path_out, opt.variant)
if not os.path.exists(out_folder):
	os.mkdir(out_folder)
	
print('Starting ...\n')


testset = SparseDataset(data_folder, opt.threshold)

high = []
low = []
errm = []
for filename, pts1, pts2, gt_F, gt_R, gt_t, K1, K2, gt_res, gt_inliers, inl_cnt, inlrat in testset:
	if inlrat > opt.minInlRat:
		copy(filename, out_folder)
		high.append(inlrat)
		err = np.array(gt_res)
		err = err[gt_inliers]
		errm.append(np.mean(err))
	else:
		low.append(inlrat)

if len(high) > 0:
	mean_h = round(statistics.mean(high), 3)
	sd_h = round(statistics.stdev(high), 4)
	mi_h = min(high)
	ma_h = round(max(high), 3)
	q_h = [round(q, 3) for q in np.quantile(high, [0.25, 0.5, 0.75])]
	err_mean_h = round(statistics.mean(errm), 4)
	err_sd_h = round(statistics.stdev(errm), 4)
	err_mi_h = min(errm)
	err_ma_h = round(max(errm), 3)
	err_q_h = [round(q, 5) for q in np.quantile(errm, [0.25, 0.5, 0.75])]
else:
	mean_h = sd_h = mi_h = ma_h = err_mean_h = err_sd_h = err_mi_h = err_ma_h = 0
	q_h = err_q_h = [0] * 3

if len(low) > 0:
	mean_l = round(statistics.mean(low), 4)
	sd_l = round(statistics.stdev(low), 4)
	mi_l = min(low)
	ma_l = round(max(low), 3)
	q_l = [round(q, 3) for q in np.quantile(low, [0.25, 0.5, 0.75])]
else:
	mean_l = sd_l = mi_l = ma_l = 0
	q_l = [0] * 3

print('Accepted data:\n')
print('Inlier ratio statistic:')
print('Min:', mi_h, 'Max:', ma_h, 'Mean:', mean_h, 'Median:', q_h[1])
print('StDev:', sd_h, 'q25:', q_h[0], 'q75:', q_h[2])
print('\nReprojection error statistic:')
print('Min:', err_mi_h, 'Max:', err_ma_h, 'Mean:', err_mean_h, 'Median:', err_q_h[1])
print('StDev:', err_sd_h, 'q25:', err_q_h[0], 'q75:', err_q_h[2])
print('\n\nDismissed data:')
print('Inlier ratio statistic:')
print('Min:', mi_l, 'Max:', ma_l, 'Mean:', mean_l, 'Median:', q_l[1])
print('StDev:', sd_l, 'q25:', q_l[0], 'q75:', q_l[2])
