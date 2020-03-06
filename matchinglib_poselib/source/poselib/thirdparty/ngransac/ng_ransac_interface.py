import numpy as np
import sys
sys.path.append('/usr/local/include/opencv4')
sys.path.append('/usr/local/lib')
import cv2
import math
import argparse
import os
import random

import torch
import torch.optim as optim
import ngransac

from network import CNNet
from dataset import SparseDataset
import util
from copy import deepcopy
import eval_mutex as em
import time
from pynvml import *


def start_ngransac(pts1, pts2, model_file, threshold=0.001, K1=None, K2=None):
    # pts1 & pts2 list of tuples with matching image locations
    em.init_lock('gpu_mem_acqu')
    pyfilepath = os.path.dirname(os.path.realpath(__file__))
    os.chdir(pyfilepath)
    if len(model_file) == 0:
        fmat = False
        orb = True
        rootsift = False
        ratio = 1.0
        session = ''
        model_file = util.create_session_string('e2e', fmat, orb, rootsift, ratio, session)
        model_file = 'models/weights_' + model_file + '.net'
        # print("No model file specified. Inferring pre-trained model from given parameters:", model_file)
    resblocks = 12 # number of res blocks of the network
    hyps = 1000 # number of hypotheses, i.e. number of RANSAC iterations

    necc_mem = 850
    nrGPUs = torch.cuda.device_count()
    useGPU = 0
    try:
        em.acquire_lock(40)
        nfound = True

        if nrGPUs > 1:
            nvmlInit()
            wcnt = 0
            while nfound and wcnt < 40:
                for i in range(0, nrGPUs):
                    h = nvmlDeviceGetHandleByIndex(i)
                    info = nvmlDeviceGetMemoryInfo(h)
                    free = info.free / 1048576
                    if free >= necc_mem:
                        useGPU = i
                        nfound = False
                        break
                if nfound:
                    time.sleep(1)
                    if wcnt % 10 == 0:
                        print('Already waiting for ', wcnt, 'seconds for free GPU memory.')
                    wcnt += 1

        with torch.cuda.device(useGPU):
            model = CNNet(resblocks)
            model.load_state_dict(torch.load(model_file))
            model = model.cuda()
            model.eval()
            em.release_lock()
            # print("Successfully loaded model.")

            ratios = np.array([[0] * len(pts1)])
            ratios = np.expand_dims(ratios, 2)
            pts1 = np.array([deepcopy(pts1)])
            pts2 = np.array([deepcopy(pts2)])

            # normalize key point coordinates when fitting the essential matrix
            if K1 or K2:
                pts1 = cv2.undistortPoints(pts1, K1, None)
                pts2 = cv2.undistortPoints(pts2, K2, None)

            # create data tensor of feature coordinates and matching ratios
            correspondences = np.concatenate((pts1, pts2, ratios), axis=2)
            correspondences = np.transpose(correspondences)
            correspondences = torch.from_numpy(correspondences).float()
            # print('Converted to torch')

            # predict neural guidance, i.e. RANSAC sampling probabilities
            log_probs = model(correspondences.unsqueeze(0).cuda())[
                0]  # zero-indexing creates and removes a dummy batch dimension
            probs = torch.exp(log_probs).cpu()

            out_model = torch.zeros((3, 3)).float()  # estimated model
            out_inliers = torch.zeros(log_probs.size())  # inlier mask of estimated model
            out_gradients = torch.zeros(log_probs.size())  # gradient tensor (only used during training)
            rand_seed = 0  # random seed to by used in C++

            # === CASE ESSENTIAL MATRIX =========================================
            try:
                incount = ngransac.find_essential_mat(correspondences, probs, rand_seed, hyps, threshold, out_model,
                                                      out_inliers, out_gradients)
            except:
                e = sys.exc_info()
                print(str(e))
                sys.stdout.flush()
                raise
            model = out_model.numpy()
    except:
        em.release_lock()
        e = sys.exc_info()
        print(str(e))
        sys.stdout.flush()
        raise
    out_inliers = out_inliers.byte().numpy().ravel().tolist()
    output = {'model': model, 'inlier_mask': out_inliers, 'nr_inliers': incount}
    return output
