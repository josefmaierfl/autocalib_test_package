from ComputeFramework import *
from ng_ransac_interface import start_ngransac
import numpy as np
import sys


class Compute(ComputeInstance):
    def compute(self, o):
        try:
            if o.K1 or o.K2:
                K1 = np.reshape(np.array(o.K1), (3, 3), order='C')
                K2 = np.reshape(np.array(o.K2), (3, 3), order='C')
                result = start_ngransac(o.pts1, o.pts2, o.model_file_name, o.threshold, K1, K2)
            else:
                result = start_ngransac(o.pts1, o.pts2, o.model_file_name, o.threshold)
        except:
            e = sys.exc_info()
            print(str(e))
            sys.stdout.flush()
        model = np.reshape(result['model'], -1, order='C').tolist()
        return [model, result['inlier_mask'], int(result['nr_inliers'])]