import numpy as np
import os, sys
from shutil import copy

from dataset_notorch_virtSequ import SparseDataset
import util_virtSequ

parser = util_virtSequ.create_parser(
	description = "Copy correspondences with a higher inlier ratio than 0.49 to a different folder.")

parser.add_argument('--path', '-p',
	required=True,
	help='Path to folders train and validate')

parser.add_argument('--path_out', '-po',
	required=True,
	help='Output path')

parser.add_argument('--variant', '-v', default='train', choices=['train', 'validate'],
	help='subfolder of the dataset to use')

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

for filename, pts1, pts2, gt_F, gt_R, gt_t, K1, K2, gt_res, gt_inliers, inl_cnt, inlrat in testset:
	if inlrat > 0.49:
		copy(filename, out_folder)
