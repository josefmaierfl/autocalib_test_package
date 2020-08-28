import numpy as np
import os


class SparseDataset:
    """Sparse correspondences dataset."""

    def __init__(self, folders, threshold):
        # collect precalculated correspondences of all provided datasets
        self.files = []
        for folder in folders:
            self.files += [folder + f for f in os.listdir(folder)]

        self.threshold = threshold

    def __len__(self):
        return len(self.files)

    def __getitem__(self, idx):
        # load precalculated correspondences
        data = np.load(self.files[idx], allow_pickle=True)

        # correspondence coordinates
        pts1, pts2 = data[0], data[1]

        # image calibration parameters
        K1, K2 = data[5], data[6]
        # ground truth pose
        gt_R, gt_t = data[7], data[8]

        # fundamental matrix from essential matrix
        gt_F = getFundamentalMat(gt_R, gt_t, K1, K2)

        num_pts = pts1.shape[1]
        # 2D coordinates to 3D homogeneous coordinates
        hom_pts1 = np.transpose(np.concatenate((pts1[0, :, :], np.ones((num_pts, 1))), axis=1))
        hom_pts2 = np.transpose(np.concatenate((pts2[0, :, :], np.ones((num_pts, 1))), axis=1))

        def epipolar_error(hom_pts1, hom_pts2, F):
            """Compute the symmetric epipolar error."""
            res = 1 / np.linalg.norm(F.T.dot(hom_pts2)[0:2], axis=0)
            res += 1 / np.linalg.norm(F.dot(hom_pts1)[0:2], axis=0)
            res *= abs(np.sum(hom_pts2 * np.matmul(F, hom_pts1), axis=0))
            return res

        # determine inliers based on the epipolar error
        gt_res = epipolar_error(hom_pts1, hom_pts2, gt_F)

        gt_inliers = (gt_res < self.threshold)
        inl_cnt = float(gt_inliers.sum())
        inlrat = inl_cnt / float(num_pts)

        return self.files[idx], pts1, pts2, gt_F, gt_R, gt_t, K1, K2, gt_res, gt_inliers, inl_cnt, inlrat


def getEssentialMat(R, t):
    gt_E = np.zeros((3, 3))
    gt_E[0, 1] = -float(t[2, 0])
    gt_E[0, 2] = float(t[1, 0])
    gt_E[1, 0] = float(t[2, 0])
    gt_E[1, 2] = -float(t[0, 0])
    gt_E[2, 0] = -float(t[1, 0])
    gt_E[2, 1] = float(t[0, 0])

    gt_E = gt_E @ R
    return gt_E


def getFundamentalMat(R, t, K1, K2):
    gt_E = getEssentialMat(R, t)
    gt_F = np.linalg.inv(K2).transpose() @ gt_E @ np.linalg.inv(K1)
    return gt_F
