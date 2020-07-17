"""
Released under the MIT License - https://opensource.org/licenses/MIT

Copyright (c) 2020 Josef Maier

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Josef Maier (josefjohann-dot-maier-at-gmail-dot-at)
"""

import numpy as np
from ComputeFramework import *
from ng_ransac_interface import start_ngransac
import sys


class Compute(ComputeInstance):
    def compute(self, o):
        try:
            if o.K1 or o.K2:
                K1 = np.reshape(np.array(o.K1), (3, 3), order='C')
                K2 = np.reshape(np.array(o.K2), (3, 3), order='C')
                result = start_ngransac(o.pts1, o.pts2, o.ratios, o.model_file_name, o.threshold, o.gpu_nr, K1, K2)
            else:
                result = start_ngransac(o.pts1, o.pts2, o.ratios, o.model_file_name, o.threshold, o.gpu_nr)
        except:
            e = sys.exc_info()
            print(str(e))
            sys.stdout.flush()
        model = np.reshape(result['model'], -1, order='C').tolist()
        return [model, result['inlier_mask'], int(result['nr_inliers']), int(result['used_gpu'])]