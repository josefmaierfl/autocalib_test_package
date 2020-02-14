from ComputeFramework import *
from ng_ransac_interface import start_ngransac
import numpy as np

class Compute(ComputeInstance):
    def __init__(self, server):
        ComputeInstance.__init__(self, server)

    def compute(self, o):
        if o.K1 or o.K2:
            K1 = np.reshape(np.array(o.K1), (3, 3), order='C')
            K2 = np.reshape(np.array(o.K2), (3, 3), order='C')
            result = start_ngransac(o.pts1, o.pts2, o.model_file, o.threshold, K1, K2)
        else:
            result = start_ngransac(o.pts1, o.pts2, o.model_file, o.threshold)
        model = np.reshape(result['model'], -1, order='C').tolist()
        self.transferModel(model, result['inlier_mask'], result['nr_inliers'])