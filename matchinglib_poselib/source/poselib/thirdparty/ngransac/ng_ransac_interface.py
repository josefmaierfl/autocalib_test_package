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
from pathlib import Path
from pynvml import *


def start_ngransac(pts1, pts2, model_file, threshold=0.001, gpu_nr=-1, K1=None, K2=None):
    # pts1 & pts2 list of tuples with matching image locations
    gpu_mutex = em.Locking('gpu_mem_acqu')
    file_mutex = em.Locking('file_write')
    file2_mutex = em.Locking('file_mem_write')
    file_procs = 'nr_gpu_processes.txt'
    file_mem = 'max_cached_mem.txt'
    home_dir = str(Path.home())
    pfile = os.path.join(home_dir, file_procs)
    mfile = os.path.join(home_dir, file_mem)
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
    rec_mem = read_max_used_mem(file2_mutex, mfile)
    necc_mem = 2 * rec_mem
    mem_per_task = 1.1 * rec_mem
    nrGPUs = torch.cuda.device_count()
    useGPU = 0
    try:
        if gpu_nr < 0:
            gpu_mutex.acquire_lock(62)
            nfound = True

            if nrGPUs > 1:
                nvmlInit()
                wcnt = 0
                while nfound and wcnt < 60:
                    if os.path.exists(pfile):
                        file_mutex.acquire_lock()
                        with open(pfile, 'r') as fi:
                            li = fi.readline()
                        file_mutex.release_lock()
                        procs = list(map(int, li.split(',')))
                    else:
                        procs = [0] * nrGPUs
                    for i in range(0, nrGPUs):
                        h = nvmlDeviceGetHandleByIndex(i)
                        info = nvmlDeviceGetMemoryInfo(h)
                        free = info.free / 1048576
                        total = info.total / 1048576
                        free2 = total - procs[i] * mem_per_task
                        if free >= necc_mem and free2 >= necc_mem:
                            useGPU = i
                            nfound = False
                            procs[i] += 1
                            file_mutex.acquire_lock()
                            with open(pfile, 'w') as fo:
                                fo.write(','.join(map(str, procs)))
                            file_mutex.release_lock()
                            break
                    if nfound:
                        time.sleep(1)
                        if wcnt % 10 == 0 and wcnt > 0:
                            print('Already waiting for ', wcnt, 'seconds for free GPU memory.')
                        wcnt += 1
                if wcnt >= 40:
                    gpu_mutex.release_lock()
                    raise TimeoutError('Waited too long for free memory')
        else:
            useGPU = gpu_nr

        with torch.cuda.device(useGPU):
            model = CNNet(resblocks)
            model.load_state_dict(torch.load(model_file))
            model = model.cuda()
            model.eval()
            if gpu_nr < 0:
                gpu_mutex.release_lock()
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
            model_npy = out_model.numpy()
            update_max_used_mem(file2_mutex, mfile, nrGPUs)
            try:
                del model
                del log_probs
                del out_gradients
                del correspondences
                del probs
            except:
                e = sys.exc_info()
                print('Deleting objects failed')
                print(str(e))
                sys.stdout.flush()
                pass
            torch.cuda.empty_cache()
    except:
        if gpu_nr < 0:
            gpu_mutex.release_lock()
        e = sys.exc_info()
        print(str(e))
        sys.stdout.flush()
        rem_proc_num(file_mutex, pfile, useGPU)
        raise
    out_inliers = out_inliers.byte().numpy().ravel().tolist()
    output = {'model': model_npy, 'inlier_mask': out_inliers, 'nr_inliers': incount, 'used_gpu': useGPU}
    rem_proc_num(file_mutex, pfile, useGPU)
    return output


def rem_proc_num(file_mutex, file, gpu_nr):
    file_mutex.acquire_lock()
    if os.path.exists(file):
        with open(file, 'r') as fi:
            li = fi.readline()
        procs = list(map(int, li.split(',')))
        procs[gpu_nr] -= 1
        with open(file, 'w') as fo:
            fo.write(','.join(map(str, procs)))
    file_mutex.release_lock()


def update_max_used_mem(file_mutex, file, nrGPUs):
    max_mem = 0
    for i in range(0, nrGPUs):
        max_mem += int(torch.cuda.max_memory_reserved(i) / 1048576)
    file_mutex.acquire_lock()
    if os.path.exists(file):
        with open(file, 'r') as fi:
            li = fi.readline()
        mem_stored = int(li)
        if max_mem > mem_stored:
            with open(file, 'w') as fo:
                fo.write(str(max_mem))
    else:
        with open(file, 'w') as fo:
            fo.write(str(max_mem))
    file_mutex.release_lock()


def read_max_used_mem(file_mutex, file):
    mem_per_task = 850
    if os.path.exists(file):
        file_mutex.acquire_lock()
        with open(file, 'r') as fi:
            li = fi.readline()
        file_mutex.release_lock()
        mem_per_task = int(li)
    return mem_per_task
