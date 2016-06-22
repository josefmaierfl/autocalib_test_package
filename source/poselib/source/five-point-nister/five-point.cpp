/*  Copyright (c) 2013, Bo Li, prclibo@gmail.com
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the copyright holder nor the
          names of its contributors may be used to endorse or promote products
          derived from this software without specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/****************************************************************************
 *							IMPORTANT NOTICE								*
 ****************************************************************************
 *																			*
 * Most of this code was completele rewritten by Josef Maier				*
 * (josef.maier.fl@ait.ac.at) as the original code was not working			*
 * correctly. Moreover, additional functionalities (like ARRSAC or			*
 * refinement procedures) were added. Most interfaces also differ			*
 * completely to the original ones.											*
 *																			*
 * For further information please contact Josef Maier, AIT Austrian			*
 * Institute of Technology.													*
 *																			*
 ****************************************************************************/


#include "five-point-nister\precomp.hpp"
#include "five-point-nister\_modelest.h"
#include "five-point-nister\five-point.hpp"
#include <iostream>
#include <complex>

using namespace cv; 
using namespace std; 

class CvEMEstimator : public CvModelEstimator3
{
public:
    CvEMEstimator(); 
    virtual int runKernel( const CvMat* m1, const CvMat* m2, CvMat* model ); 
    virtual int run5Point( const CvMat* _q1, const CvMat* _q2, CvMat* _ematrix ); 
//protected: 
	bool reliable( const CvMat* m1, const CvMat* m2, const CvMat* model ); 
    virtual void getCoeffMat( double *eet, double* a ); 
    void computeReprojError3( const CvMat* m1, const CvMat* m2,
                                     const CvMat* model, CvMat* error );
	bool ValidModel(const CvMat* m1, const CvMat* m2, const CvMat* model);
}; 



// Input should be a vector of n 2D points or a Nx2 matrix
bool findEssentialMat(OutputArray Essential, InputArray _points1, InputArray _points2, /*double focal, Point2d pp, */
					int method, double prob, double threshold, OutputArray _mask, bool lesqu,
					void (*refineEssential)(cv::InputArray points1, cv::InputArray points2, cv::InputArray E_init, 
											cv::Mat & E_refined, double th, unsigned int iters, bool makeClosestE, 
											double *sumSqrErr_init, double *sumSqrErr, 
											cv::OutputArray errors, cv::InputOutputArray mask, int model)) 
{
	Mat points1, points2; 
	_points1.getMat().copyTo(points1); 
	_points2.getMat().copyTo(points2); 

	int npoints = points1.checkVector(2);
    CV_Assert( npoints >= 5 && points2.checkVector(2) == npoints &&
				              points1.type() == points2.type());

	/*if (points1.channels() > 1)
	{
		points1 = points1.reshape(1, npoints); 
		points2 = points2.reshape(1, npoints); 
	}*/
	points1.convertTo(points1, CV_64F); 
	points2.convertTo(points2, CV_64F); 

	/*points1.col(0) = (points1.col(0) - pp.x) / focal; 
	points2.col(0) = (points2.col(0) - pp.x) / focal; 
	points1.col(1) = (points1.col(1) - pp.y) / focal; 
	points2.col(1) = (points2.col(1) - pp.y) / focal; */
	
	// Reshape data to fit opencv ransac function
	points1 = points1.reshape(2, 1); 
	points2 = points2.reshape(2, 1); 

	Mat E(3, 3, CV_64F); 
	CvEMEstimator estimator; 

	CvMat p1 = points1; 
	CvMat p2 = points2; 
	CvMat _E = E;  
	CvMat* tempMask = cvCreateMat(1, npoints, CV_8U); 
	
	assert(npoints >= 5); 
	//threshold /= focal; 
    int count = 1; 
    if (npoints == 5)
    {
        E.create(3 * 10, 3, CV_64F); 
        _E = E; 
        count = estimator.runKernel(&p1, &p2, &_E); 
        E = E.rowRange(0, 3 * count) * 1.0; 
        //Mat(tempMask).setTo(true); 
		Mat tempMask1 = cvarrToMat(tempMask);
		tempMask1.setTo(true);
    }
    else if (method == CV_RANSAC)
	{
		if(!estimator.runRANSAC(&p1, &p2, &_E, tempMask, threshold, prob, 1000,lesqu))
			return false;
	}
	else if (method == ARRSAC)
	{
		if(!estimator.runARRSAC(&p1, &p2, &_E, tempMask, threshold, lesqu, refineEssential))
			return false;
	}
	else
	{
		if(!estimator.runLMeDS(&p1, &p2, &_E, tempMask, prob))
			return false;
	}
    if (_mask.needed())
    {
    	_mask.create(1, npoints, CV_8U, -1, true); 
    	Mat mask = _mask.getMat(); 
		//Mat(tempMask).copyTo(mask); 
		Mat tempMask1 = cvarrToMat(tempMask);
		tempMask1.copyTo(mask);
    }

	if(Essential.needed())
	{
		if(Essential.empty())
			Essential.create(3, 3, CV_64F); 
		E.copyTo(Essential.getMat());
	}
	else
		return false;

	return true; 

}

int recoverPose( const Mat & E, InputArray _points1, InputArray _points2, Mat & _R, Mat & _t, Mat & Q3D,
					/*double focal, Point2d pp, */
					InputOutputArray _mask, const double dist, cv::InputArray t_only) 
{
	Mat points1, points2; 
	_points1.getMat().copyTo(points1); 
	_points2.getMat().copyTo(points2); 
	int npoints = points1.checkVector(2);
    CV_Assert( npoints >= 0 && points2.checkVector(2) == npoints &&
				              points1.type() == points2.type());

	/*if (points1.channels() > 1)
	{
		points1 = points1.reshape(1, npoints); 
		points2 = points2.reshape(1, npoints); 
	}
	points1.convertTo(points1, CV_64F); 
	points2.convertTo(points2, CV_64F); */

	/*points1.col(0) = (points1.col(0) - pp.x) / focal; 
	points2.col(0) = (points2.col(0) - pp.x) / focal; 
	points1.col(1) = (points1.col(1) - pp.y) / focal; 
	points2.col(1) = (points2.col(1) - pp.y) / focal; */

	points1 = points1.t(); 
	points2 = points2.t(); 
	
	Mat R1, R2, t; 
	if(t_only.empty())
		decomposeEssentialMat(E, R1, R2, t); 
	else
	{
		R1 = cv::Mat::eye(3,3,t_only.type());
		t = t_only.getMat();
	}
	Mat P0 = Mat::eye(3, 4, R1.type()); 
	Mat P1(3, 4, R1.type()), P2(3, 4, R1.type()), P3(3, 4, R1.type()), P4(3, 4, R1.type()); 
	P1(Range::all(), Range(0, 3)) = R1 * 1.0; P1.col(3) = t * 1.0; 
	P3(Range::all(), Range(0, 3)) = R1 * 1.0; P3.col(3) = -t * 1.0;
	if(t_only.empty())
	{
		P2(Range::all(), Range(0, 3)) = R2 * 1.0; P2.col(3) = t * 1.0;  
		P4(Range::all(), Range(0, 3)) = R2 * 1.0; P4.col(3) = -t * 1.0; 
	}

	// Do the cheirality check. 
	// Notice here a threshold dist is used to filter
	// out far away points (i.e. infinite points) since 
	// there depth may vary between postive and negtive. 
	//const double dist = 50.0; 
	Mat Q1,q1; 
	triangulatePoints(P0, P1, points1, points2, Q1); 
	Mat mask1 = Q1.row(2).mul(Q1.row(3)) > 0;
	q1 = P1 * Q1;
	mask1 = (q1.row(2).mul(Q1.row(3)) > 0) & mask1;
	Q1.row(0) /= Q1.row(3); 
	Q1.row(1) /= Q1.row(3); 
	Q1.row(2) /= Q1.row(3); 
	//Q.row(3) = Mat::ones(1,Q.size().width,Q.type()); 
	mask1 = (Q1.row(2) < dist) & mask1; 

	/*q1.row(0) /= q1.row(3); 
	q1.row(1) /= q1.row(3); 
	q1.row(2) = Mat::ones(1,Q.size().width,Q.type());
	mask1 = (Q.row(2) > 0) & mask1; 
	mask1 = (Q.row(2) < dist) & mask1;*/

	/*mask1 = (Q.row(2) > 0) & mask1; 
	mask1 = (Q.row(2) < dist) & mask1; */

	Mat Q2;
	Mat mask2;
	if(t_only.empty())
	{
		triangulatePoints(P0, P2, points1, points2, Q2); 
		mask2 = Q2.row(2).mul(Q2.row(3)) > 0; 
		q1 = P2 * Q2;
		mask2 = (q1.row(2).mul(Q2.row(3)) > 0) & mask2;
		Q2.row(0) /= Q2.row(3); 
		Q2.row(1) /= Q2.row(3); 
		Q2.row(2) /= Q2.row(3); 
		//Q.row(3) = Mat::ones(1,Q.size().width,Q.type()); 
		mask2 = (Q2.row(2) < dist) & mask2; 
		/*Q = P2 * Q; 
		mask2 = (Q.row(2) > 0) & mask2; 
		mask2 = (Q.row(2) < dist) & mask2; */
	}

	Mat Q3;
	triangulatePoints(P0, P3, points1, points2, Q3); 
	Mat mask3 = Q3.row(2).mul(Q3.row(3)) > 0; 
	q1 = P3 * Q3;
	mask3 = (q1.row(2).mul(Q3.row(3)) > 0) & mask3;
	Q3.row(0) /= Q3.row(3); 
	Q3.row(1) /= Q3.row(3); 
	Q3.row(2) /= Q3.row(3); 
	//Q.row(3) = Mat::ones(1,Q.size().width,Q.type());
	mask3 = (Q3.row(2) < dist) & mask3; 
	/*Q = P3 * Q; 
	mask3 = (Q.row(2) > 0) & mask3; 
	mask3 = (Q.row(2) < dist) & mask3; */

	Mat Q4;
	Mat mask4;
	if(t_only.empty())
	{
		triangulatePoints(P0, P4, points1, points2, Q4); 
		mask4 = Q4.row(2).mul(Q4.row(3)) > 0; 
		q1 = P4 * Q4;
		mask4 = (q1.row(2).mul(Q4.row(3)) > 0) & mask4;
		Q4.row(0) /= Q4.row(3); 
		Q4.row(1) /= Q4.row(3); 
		Q4.row(2) /= Q4.row(3); 
		//Q.row(3) = Mat::ones(1,Q.size().width,Q.type());
		mask4 = (Q4.row(2) < dist) & mask4; 
		/*Q = P4 * Q; 
		mask4 = (Q.row(2) > 0) & mask4; 
		mask4 = (Q.row(2) < dist) & mask4; */
	}

	// If _mask is given, then use it to filter outliers. 
	if (_mask.needed())
	{
		//_mask.create(1, npoints, CV_8U, -1, true); 
		Mat mask = _mask.getMat();
		if(mask.empty())
		{
			_mask.create(1, npoints, CV_8U, -1, true);
			mask = _mask.getMat();
			mask = Mat::ones(1, npoints, CV_8U);
		}
		bitwise_and(mask, mask1, mask1); 
		bitwise_and(mask, mask3, mask3); 
		if(t_only.empty())
		{
			bitwise_and(mask, mask2, mask2); 
			bitwise_and(mask, mask4, mask4); 
		}
	}

	int good1 = countNonZero(mask1); 
	int good3 = countNonZero(mask3); 
	int good2 = 0;
	int good4 = 0;
	if(t_only.empty())
	{
		good2 = countNonZero(mask2); 
		good4 = countNonZero(mask4); 
	}
	if (good1 >= good2 && good1 >= good3 && good1 >= good4)
	{
		_R = R1; _t = t; 
		if (_mask.needed()) mask1.copyTo(_mask.getMat()); 
		Q3D = Q1.rowRange(0,3).t();
		return good1; 
	}
	else if (good2 && good2 >= good1 && good2 >= good3 && good2 >= good4)
	{
		_R = R2; _t = t; 
		if (_mask.needed()) mask2.copyTo(_mask.getMat()); 
		Q3D = Q2.rowRange(0,3).t();
		return good2; 
	}
	else if (good3 >= good1 && good3 >= good2 && good3 >= good4)
	{
		_R = R1; _t = -t; 
		if (_mask.needed()) mask3.copyTo(_mask.getMat()); 
		Q3D = Q3.rowRange(0,3).t();
		return good3; 
	}
	else  
	{
		if(good4)
		{
			_R = R2; _t = -t; 
			if (_mask.needed()) mask4.copyTo(_mask.getMat()); 
			Q3D = Q4.rowRange(0,3).t();
		}
		else
		{
			_R = cv::Mat::eye(3,3,t.type());
			_t = cv::Mat::zeros(3,1,t.type());
			if (_mask.needed()) _mask.getMat() = cv::Mat::zeros(1,Q1.cols,mask1.type());
			Q3D = cv::Mat::zeros(Q1.cols,3,Q1.type());
		}
		return good4; 
	}

}



void decomposeEssentialMat( const Mat & E, Mat & R1, Mat & R2, Mat & t ) 
{
	assert(E.cols == 3 && E.rows == 3); 
	Mat D, U, Vt; 
	SVD::compute(E, D, U, Vt); 
	if (determinant(U) < 0) U = -U; 
	if (determinant(Vt) < 0) Vt = -Vt; 
	Mat W = (Mat_<double>(3, 3) << 0, 1, 0, -1, 0, 0, 0, 0, 1); 
	W.convertTo(W, E.type()); 
	R1 = U * W * Vt; 
	R2 = U * W.t() * Vt; 
	t = U.col(2) * 1.0; 
}


CvEMEstimator::CvEMEstimator()
: CvModelEstimator3( 5, cvSize(3,3),  10 )
{
}

int CvEMEstimator::runKernel( const CvMat* m1, const CvMat* m2, CvMat* model )
{
    return run5Point(m1, m2, model); 
}

// Notice to keep compatibility with opencv ransac, q1 and q2 have
// to be of 1 row x n col x 2 channel. 
int CvEMEstimator::run5Point( const CvMat* q1, const CvMat* q2, CvMat* ematrix )
{
	/*Mat Q1 = Mat(q1).reshape(1, q1->cols); 
	Mat Q2 = Mat(q2).reshape(1, q2->cols);*/ 
	Mat Q1 = cvarrToMat(q1).reshape(1, q1->cols);;
	Mat Q2 = cvarrToMat(q2).reshape(1, q2->cols); ;

	int n = Q1.rows; 
	Mat Q(n, 9, CV_64F); 
	Q.col(0) = Q1.col(0).mul( Q2.col(0) ); 
	Q.col(1) = Q1.col(1).mul( Q2.col(0) ); 
	Q.col(2) = Q2.col(0) * 1.0; 
	Q.col(3) = Q1.col(0).mul( Q2.col(1) ); 
	Q.col(4) = Q1.col(1).mul( Q2.col(1) ); 
	Q.col(5) = Q2.col(1) * 1.0; 
	Q.col(6) = Q1.col(0) * 1.0; 
	Q.col(7) = Q1.col(1) * 1.0; 
	Q.col(8) = 1.0; 

    Mat U, W, Vt; 
    SVD::compute(Q, W, U, Vt, SVD::MODIFY_A | SVD::FULL_UV); 
    
    Mat EE = Mat(Vt.t()).colRange(5, 9) * 1.0; 
    Mat A(10, 20, CV_64F); 
    EE = EE.t(); 
	getCoeffMat((double*)EE.data, (double*)A.data); 
    EE = EE.t(); 

	A = A.colRange(0, 10).inv() * A.colRange(10, 20); 
    
    double b[3 * 13]; 
    Mat B(3, 13, CV_64F, b); 
    for (int i = 0; i < 3; i++)
    {
        Mat arow1 = A.row(i * 2 + 4) * 1.0; 
        Mat arow2 = A.row(i * 2 + 5) * 1.0; 
        Mat row1(1, 13, CV_64F, Scalar(0.0)); 
        Mat row2(1, 13, CV_64F, Scalar(0.0)); 

        row1.colRange(1, 4) = arow1.colRange(0, 3) * 1.0; 
        row1.colRange(5, 8) = arow1.colRange(3, 6) * 1.0; 
        row1.colRange(9, 13) = arow1.colRange(6, 10) * 1.0; 

        row2.colRange(0, 3) = arow2.colRange(0, 3) * 1.0; 
        row2.colRange(4, 7) = arow2.colRange(3, 6) * 1.0; 
        row2.colRange(8, 12) = arow2.colRange(6, 10) * 1.0; 
        
        B.row(i) = row1 - row2; 
    }

    double c[11]; 
    Mat coeffs(1, 11, CV_64F, c); 
    c[10] = (b[0]*b[17]*b[34]+b[26]*b[4]*b[21]-b[26]*b[17]*b[8]-b[13]*b[4]*b[34]-b[0]*b[21]*b[30]+b[13]*b[30]*b[8]); 
    c[9] = (b[26]*b[4]*b[22]+b[14]*b[30]*b[8]+b[13]*b[31]*b[8]+b[1]*b[17]*b[34]-b[13]*b[5]*b[34]+b[26]*b[5]*b[21]-b[0]*b[21]*b[31]-b[26]*b[17]*b[9]-b[1]*b[21]*b[30]+b[27]*b[4]*b[21]+b[0]*b[17]*b[35]-b[0]*b[22]*b[30]+b[13]*b[30]*b[9]+b[0]*b[18]*b[34]-b[27]*b[17]*b[8]-b[14]*b[4]*b[34]-b[13]*b[4]*b[35]-b[26]*b[18]*b[8]); 
    c[8] = (b[14]*b[30]*b[9]+b[14]*b[31]*b[8]+b[13]*b[31]*b[9]-b[13]*b[4]*b[36]-b[13]*b[5]*b[35]+b[15]*b[30]*b[8]-b[13]*b[6]*b[34]+b[13]*b[30]*b[10]+b[13]*b[32]*b[8]-b[14]*b[4]*b[35]-b[14]*b[5]*b[34]+b[26]*b[4]*b[23]+b[26]*b[5]*b[22]+b[26]*b[6]*b[21]-b[26]*b[17]*b[10]-b[15]*b[4]*b[34]-b[26]*b[18]*b[9]-b[26]*b[19]*b[8]+b[27]*b[4]*b[22]+b[27]*b[5]*b[21]-b[27]*b[17]*b[9]-b[27]*b[18]*b[8]-b[1]*b[21]*b[31]-b[0]*b[23]*b[30]-b[0]*b[21]*b[32]+b[28]*b[4]*b[21]-b[28]*b[17]*b[8]+b[2]*b[17]*b[34]+b[0]*b[18]*b[35]-b[0]*b[22]*b[31]+b[0]*b[17]*b[36]+b[0]*b[19]*b[34]-b[1]*b[22]*b[30]+b[1]*b[18]*b[34]+b[1]*b[17]*b[35]-b[2]*b[21]*b[30]); 
    c[7] = (b[14]*b[30]*b[10]+b[14]*b[32]*b[8]-b[3]*b[21]*b[30]+b[3]*b[17]*b[34]+b[13]*b[32]*b[9]+b[13]*b[33]*b[8]-b[13]*b[4]*b[37]-b[13]*b[5]*b[36]+b[15]*b[30]*b[9]+b[15]*b[31]*b[8]-b[16]*b[4]*b[34]-b[13]*b[6]*b[35]-b[13]*b[7]*b[34]+b[13]*b[30]*b[11]+b[13]*b[31]*b[10]+b[14]*b[31]*b[9]-b[14]*b[4]*b[36]-b[14]*b[5]*b[35]-b[14]*b[6]*b[34]+b[16]*b[30]*b[8]-b[26]*b[20]*b[8]+b[26]*b[4]*b[24]+b[26]*b[5]*b[23]+b[26]*b[6]*b[22]+b[26]*b[7]*b[21]-b[26]*b[17]*b[11]-b[15]*b[4]*b[35]-b[15]*b[5]*b[34]-b[26]*b[18]*b[10]-b[26]*b[19]*b[9]+b[27]*b[4]*b[23]+b[27]*b[5]*b[22]+b[27]*b[6]*b[21]-b[27]*b[17]*b[10]-b[27]*b[18]*b[9]-b[27]*b[19]*b[8]+b[0]*b[17]*b[37]-b[0]*b[23]*b[31]-b[0]*b[24]*b[30]-b[0]*b[21]*b[33]-b[29]*b[17]*b[8]+b[28]*b[4]*b[22]+b[28]*b[5]*b[21]-b[28]*b[17]*b[9]-b[28]*b[18]*b[8]+b[29]*b[4]*b[21]+b[1]*b[19]*b[34]-b[2]*b[21]*b[31]+b[0]*b[20]*b[34]+b[0]*b[19]*b[35]+b[0]*b[18]*b[36]-b[0]*b[22]*b[32]-b[1]*b[23]*b[30]-b[1]*b[21]*b[32]+b[1]*b[18]*b[35]-b[1]*b[22]*b[31]-b[2]*b[22]*b[30]+b[2]*b[17]*b[35]+b[1]*b[17]*b[36]+b[2]*b[18]*b[34]); 
    c[6] = (-b[14]*b[6]*b[35]-b[14]*b[7]*b[34]-b[3]*b[22]*b[30]-b[3]*b[21]*b[31]+b[3]*b[17]*b[35]+b[3]*b[18]*b[34]+b[13]*b[32]*b[10]+b[13]*b[33]*b[9]-b[13]*b[4]*b[38]-b[13]*b[5]*b[37]-b[15]*b[6]*b[34]+b[15]*b[30]*b[10]+b[15]*b[32]*b[8]-b[16]*b[4]*b[35]-b[13]*b[6]*b[36]-b[13]*b[7]*b[35]+b[13]*b[31]*b[11]+b[13]*b[30]*b[12]+b[14]*b[32]*b[9]+b[14]*b[33]*b[8]-b[14]*b[4]*b[37]-b[14]*b[5]*b[36]+b[16]*b[30]*b[9]+b[16]*b[31]*b[8]-b[26]*b[20]*b[9]+b[26]*b[4]*b[25]+b[26]*b[5]*b[24]+b[26]*b[6]*b[23]+b[26]*b[7]*b[22]-b[26]*b[17]*b[12]+b[14]*b[30]*b[11]+b[14]*b[31]*b[10]+b[15]*b[31]*b[9]-b[15]*b[4]*b[36]-b[15]*b[5]*b[35]-b[26]*b[18]*b[11]-b[26]*b[19]*b[10]-b[27]*b[20]*b[8]+b[27]*b[4]*b[24]+b[27]*b[5]*b[23]+b[27]*b[6]*b[22]+b[27]*b[7]*b[21]-b[27]*b[17]*b[11]-b[27]*b[18]*b[10]-b[27]*b[19]*b[9]-b[16]*b[5]*b[34]-b[29]*b[17]*b[9]-b[29]*b[18]*b[8]+b[28]*b[4]*b[23]+b[28]*b[5]*b[22]+b[28]*b[6]*b[21]-b[28]*b[17]*b[10]-b[28]*b[18]*b[9]-b[28]*b[19]*b[8]+b[29]*b[4]*b[22]+b[29]*b[5]*b[21]-b[2]*b[23]*b[30]+b[2]*b[18]*b[35]-b[1]*b[22]*b[32]-b[2]*b[21]*b[32]+b[2]*b[19]*b[34]+b[0]*b[19]*b[36]-b[0]*b[22]*b[33]+b[0]*b[20]*b[35]-b[0]*b[23]*b[32]-b[0]*b[25]*b[30]+b[0]*b[17]*b[38]+b[0]*b[18]*b[37]-b[0]*b[24]*b[31]+b[1]*b[17]*b[37]-b[1]*b[23]*b[31]-b[1]*b[24]*b[30]-b[1]*b[21]*b[33]+b[1]*b[20]*b[34]+b[1]*b[19]*b[35]+b[1]*b[18]*b[36]+b[2]*b[17]*b[36]-b[2]*b[22]*b[31]); 
    c[5] = (-b[14]*b[6]*b[36]-b[14]*b[7]*b[35]+b[14]*b[31]*b[11]-b[3]*b[23]*b[30]-b[3]*b[21]*b[32]+b[3]*b[18]*b[35]-b[3]*b[22]*b[31]+b[3]*b[17]*b[36]+b[3]*b[19]*b[34]+b[13]*b[32]*b[11]+b[13]*b[33]*b[10]-b[13]*b[5]*b[38]-b[15]*b[6]*b[35]-b[15]*b[7]*b[34]+b[15]*b[30]*b[11]+b[15]*b[31]*b[10]+b[16]*b[31]*b[9]-b[13]*b[6]*b[37]-b[13]*b[7]*b[36]+b[13]*b[31]*b[12]+b[14]*b[32]*b[10]+b[14]*b[33]*b[9]-b[14]*b[4]*b[38]-b[14]*b[5]*b[37]-b[16]*b[6]*b[34]+b[16]*b[30]*b[10]+b[16]*b[32]*b[8]-b[26]*b[20]*b[10]+b[26]*b[5]*b[25]+b[26]*b[6]*b[24]+b[26]*b[7]*b[23]+b[14]*b[30]*b[12]+b[15]*b[32]*b[9]+b[15]*b[33]*b[8]-b[15]*b[4]*b[37]-b[15]*b[5]*b[36]+b[29]*b[5]*b[22]+b[29]*b[6]*b[21]-b[26]*b[18]*b[12]-b[26]*b[19]*b[11]-b[27]*b[20]*b[9]+b[27]*b[4]*b[25]+b[27]*b[5]*b[24]+b[27]*b[6]*b[23]+b[27]*b[7]*b[22]-b[27]*b[17]*b[12]-b[27]*b[18]*b[11]-b[27]*b[19]*b[10]-b[28]*b[20]*b[8]-b[16]*b[4]*b[36]-b[16]*b[5]*b[35]-b[29]*b[17]*b[10]-b[29]*b[18]*b[9]-b[29]*b[19]*b[8]+b[28]*b[4]*b[24]+b[28]*b[5]*b[23]+b[28]*b[6]*b[22]+b[28]*b[7]*b[21]-b[28]*b[17]*b[11]-b[28]*b[18]*b[10]-b[28]*b[19]*b[9]+b[29]*b[4]*b[23]-b[2]*b[22]*b[32]-b[2]*b[21]*b[33]-b[1]*b[24]*b[31]+b[0]*b[18]*b[38]-b[0]*b[24]*b[32]+b[0]*b[19]*b[37]+b[0]*b[20]*b[36]-b[0]*b[25]*b[31]-b[0]*b[23]*b[33]+b[1]*b[19]*b[36]-b[1]*b[22]*b[33]+b[1]*b[20]*b[35]+b[2]*b[19]*b[35]-b[2]*b[24]*b[30]-b[2]*b[23]*b[31]+b[2]*b[20]*b[34]+b[2]*b[17]*b[37]-b[1]*b[25]*b[30]+b[1]*b[18]*b[37]+b[1]*b[17]*b[38]-b[1]*b[23]*b[32]+b[2]*b[18]*b[36]); 
    c[4] = (-b[14]*b[6]*b[37]-b[14]*b[7]*b[36]+b[14]*b[31]*b[12]+b[3]*b[17]*b[37]-b[3]*b[23]*b[31]-b[3]*b[24]*b[30]-b[3]*b[21]*b[33]+b[3]*b[20]*b[34]+b[3]*b[19]*b[35]+b[3]*b[18]*b[36]-b[3]*b[22]*b[32]+b[13]*b[32]*b[12]+b[13]*b[33]*b[11]-b[15]*b[6]*b[36]-b[15]*b[7]*b[35]+b[15]*b[31]*b[11]+b[15]*b[30]*b[12]+b[16]*b[32]*b[9]+b[16]*b[33]*b[8]-b[13]*b[6]*b[38]-b[13]*b[7]*b[37]+b[14]*b[32]*b[11]+b[14]*b[33]*b[10]-b[14]*b[5]*b[38]-b[16]*b[6]*b[35]-b[16]*b[7]*b[34]+b[16]*b[30]*b[11]+b[16]*b[31]*b[10]-b[26]*b[19]*b[12]-b[26]*b[20]*b[11]+b[26]*b[6]*b[25]+b[26]*b[7]*b[24]+b[15]*b[32]*b[10]+b[15]*b[33]*b[9]-b[15]*b[4]*b[38]-b[15]*b[5]*b[37]+b[29]*b[5]*b[23]+b[29]*b[6]*b[22]+b[29]*b[7]*b[21]-b[27]*b[20]*b[10]+b[27]*b[5]*b[25]+b[27]*b[6]*b[24]+b[27]*b[7]*b[23]-b[27]*b[18]*b[12]-b[27]*b[19]*b[11]-b[28]*b[20]*b[9]-b[16]*b[4]*b[37]-b[16]*b[5]*b[36]+b[0]*b[19]*b[38]-b[0]*b[24]*b[33]+b[0]*b[20]*b[37]-b[29]*b[17]*b[11]-b[29]*b[18]*b[10]-b[29]*b[19]*b[9]+b[28]*b[4]*b[25]+b[28]*b[5]*b[24]+b[28]*b[6]*b[23]+b[28]*b[7]*b[22]-b[28]*b[17]*b[12]-b[28]*b[18]*b[11]-b[28]*b[19]*b[10]-b[29]*b[20]*b[8]+b[29]*b[4]*b[24]+b[2]*b[18]*b[37]-b[0]*b[25]*b[32]+b[1]*b[18]*b[38]-b[1]*b[24]*b[32]+b[1]*b[19]*b[37]+b[1]*b[20]*b[36]-b[1]*b[25]*b[31]+b[2]*b[17]*b[38]+b[2]*b[19]*b[36]-b[2]*b[24]*b[31]-b[2]*b[22]*b[33]-b[2]*b[23]*b[32]+b[2]*b[20]*b[35]-b[1]*b[23]*b[33]-b[2]*b[25]*b[30]); 
    c[3] = (-b[14]*b[6]*b[38]-b[14]*b[7]*b[37]+b[3]*b[19]*b[36]-b[3]*b[22]*b[33]+b[3]*b[20]*b[35]-b[3]*b[23]*b[32]-b[3]*b[25]*b[30]+b[3]*b[17]*b[38]+b[3]*b[18]*b[37]-b[3]*b[24]*b[31]-b[15]*b[6]*b[37]-b[15]*b[7]*b[36]+b[15]*b[31]*b[12]+b[16]*b[32]*b[10]+b[16]*b[33]*b[9]+b[13]*b[33]*b[12]-b[13]*b[7]*b[38]+b[14]*b[32]*b[12]+b[14]*b[33]*b[11]-b[16]*b[6]*b[36]-b[16]*b[7]*b[35]+b[16]*b[31]*b[11]+b[16]*b[30]*b[12]+b[15]*b[32]*b[11]+b[15]*b[33]*b[10]-b[15]*b[5]*b[38]+b[29]*b[5]*b[24]+b[29]*b[6]*b[23]-b[26]*b[20]*b[12]+b[26]*b[7]*b[25]-b[27]*b[19]*b[12]-b[27]*b[20]*b[11]+b[27]*b[6]*b[25]+b[27]*b[7]*b[24]-b[28]*b[20]*b[10]-b[16]*b[4]*b[38]-b[16]*b[5]*b[37]+b[29]*b[7]*b[22]-b[29]*b[17]*b[12]-b[29]*b[18]*b[11]-b[29]*b[19]*b[10]+b[28]*b[5]*b[25]+b[28]*b[6]*b[24]+b[28]*b[7]*b[23]-b[28]*b[18]*b[12]-b[28]*b[19]*b[11]-b[29]*b[20]*b[9]+b[29]*b[4]*b[25]-b[2]*b[24]*b[32]+b[0]*b[20]*b[38]-b[0]*b[25]*b[33]+b[1]*b[19]*b[38]-b[1]*b[24]*b[33]+b[1]*b[20]*b[37]-b[2]*b[25]*b[31]+b[2]*b[20]*b[36]-b[1]*b[25]*b[32]+b[2]*b[19]*b[37]+b[2]*b[18]*b[38]-b[2]*b[23]*b[33]); 
    c[2] = (b[3]*b[18]*b[38]-b[3]*b[24]*b[32]+b[3]*b[19]*b[37]+b[3]*b[20]*b[36]-b[3]*b[25]*b[31]-b[3]*b[23]*b[33]-b[15]*b[6]*b[38]-b[15]*b[7]*b[37]+b[16]*b[32]*b[11]+b[16]*b[33]*b[10]-b[16]*b[5]*b[38]-b[16]*b[6]*b[37]-b[16]*b[7]*b[36]+b[16]*b[31]*b[12]+b[14]*b[33]*b[12]-b[14]*b[7]*b[38]+b[15]*b[32]*b[12]+b[15]*b[33]*b[11]+b[29]*b[5]*b[25]+b[29]*b[6]*b[24]-b[27]*b[20]*b[12]+b[27]*b[7]*b[25]-b[28]*b[19]*b[12]-b[28]*b[20]*b[11]+b[29]*b[7]*b[23]-b[29]*b[18]*b[12]-b[29]*b[19]*b[11]+b[28]*b[6]*b[25]+b[28]*b[7]*b[24]-b[29]*b[20]*b[10]+b[2]*b[19]*b[38]-b[1]*b[25]*b[33]+b[2]*b[20]*b[37]-b[2]*b[24]*b[33]-b[2]*b[25]*b[32]+b[1]*b[20]*b[38]); 
    c[1] = (b[29]*b[7]*b[24]-b[29]*b[20]*b[11]+b[2]*b[20]*b[38]-b[2]*b[25]*b[33]-b[28]*b[20]*b[12]+b[28]*b[7]*b[25]-b[29]*b[19]*b[12]-b[3]*b[24]*b[33]+b[15]*b[33]*b[12]+b[3]*b[19]*b[38]-b[16]*b[6]*b[38]+b[3]*b[20]*b[37]+b[16]*b[32]*b[12]+b[29]*b[6]*b[25]-b[16]*b[7]*b[37]-b[3]*b[25]*b[32]-b[15]*b[7]*b[38]+b[16]*b[33]*b[11]); 
    c[0] = -b[29]*b[20]*b[12]+b[29]*b[7]*b[25]+b[16]*b[33]*b[12]-b[16]*b[7]*b[38]+b[3]*b[20]*b[38]-b[3]*b[25]*b[33]; 
    
    std::vector<std::complex<double> > roots; 
    solvePoly(coeffs, roots); 

    std::vector<double> xs, ys, zs; 
    int count = 0; 
    double * e = ematrix->data.db; 
    for (unsigned int i = 0; i < roots.size(); i++)
    {
        if (fabs(roots[i].imag()) > 1e-10) continue; 
        double z1 = roots[i].real(); 
        double z2 = z1 * z1; 
        double z3 = z2 * z1; 
        double z4 = z3 * z1; 
        
        double bz[3][3]; 
        for (int j = 0; j < 3; j++)
        {
            const double * br = b + j * 13; 
            bz[j][0] = br[0] * z3 + br[1] * z2 + br[2] * z1 + br[3]; 
            bz[j][1] = br[4] * z3 + br[5] * z2 + br[6] * z1 + br[7]; 
            bz[j][2] = br[8] * z4 + br[9] * z3 + br[10] * z2 + br[11] * z1 + br[12]; 
        }

        Mat Bz(3, 3, CV_64F, bz); 
        cv::Mat xy1; 
        SVD::solveZ(Bz, xy1); 

        if (fabs(xy1.at<double>(2)) < 1e-10) continue; 
        xs.push_back(xy1.at<double>(0) / xy1.at<double>(2)); 
        ys.push_back(xy1.at<double>(1) / xy1.at<double>(2)); 
        zs.push_back(z1); 

        cv::Mat Evec = EE.col(0) * xs.back() + EE.col(1) * ys.back() + EE.col(2) * zs.back() + EE.col(3); 
        Evec /= norm(Evec); 
        
        memcpy(e + count * 9, Evec.data, 9 * sizeof(double)); 
        count++; 
    }
    
    return count; 

}

// Same as the runKernel (run5Point), m1 and m2 should be
// 1 row x n col x 2 channels. 
// And also, error has to be of CV_32FC1. 
void CvEMEstimator::computeReprojError3( const CvMat* m1, const CvMat* m2,
                                     const CvMat* model, CvMat* error )
{
    //Mat X1(m1), X2(m2); 
	Mat X1 = cvarrToMat(m1);
	Mat X2 = cvarrToMat(m2);
    int n = X1.cols; 
    X1 = X1.reshape(1, n); 
    X2 = X2.reshape(1, n); 

    X1.convertTo(X1, CV_64F); 
    X2.convertTo(X2, CV_64F); 

    //Mat E(model);
	Mat E = cvarrToMat(model);
	Mat Et = E.t();
    for (int i = 0; i < n; i++)
    {
        Mat x1 = (Mat_<double>(3, 1) << X1.at<double>(i, 0), X1.at<double>(i, 1), 1.0); 
        Mat x2 = (Mat_<double>(3, 1) << X2.at<double>(i, 0), X2.at<double>(i, 1), 1.0); 
        double x2tEx1 = x2.dot(E * x1); 
        Mat Ex1 = E * x1; 
        Mat Etx2 = Et * x2; 
        double a = Ex1.at<double>(0) * Ex1.at<double>(0); 
        double b = Ex1.at<double>(1) * Ex1.at<double>(1); 
        double c = Etx2.at<double>(0) * Etx2.at<double>(0); 
        double d = Etx2.at<double>(1) * Etx2.at<double>(1); 

		error->data.fl[i] = (float)(x2tEx1 * x2tEx1 / (a + b + c + d)); 
    }

/*	Eigen::MatrixXd X1t, X2t; 
	cv2eigen(Mat(m1).reshape(1, m1->cols), X1t); 
	cv2eigen(Mat(m2).reshape(1, m2->cols), X2t); 
	Eigen::MatrixXd X1(3, X1t.rows()); 
	Eigen::MatrixXd X2(3, X2t.rows()); 
	X1.topRows(2) = X1t.transpose(); 
	X2.topRows(2) = X2t.transpose(); 
	X1.row(2).setOnes(); 
	X2.row(2).setOnes(); 

	Eigen::MatrixXd E; 
	cv2eigen(Mat(model), E); 
	
	// Compute Simpson's error
	Eigen::MatrixXd Ex1, x2tEx1, Etx2, SimpsonError; 
	Ex1 = E * X1; 
	x2tEx1 = (X2.array() * Ex1.array()).matrix().colwise().sum(); 
	Etx2 = E.transpose() * X2; 
	SimpsonError = x2tEx1.array().square() / (Ex1.row(0).array().square() + Ex1.row(1).array().square() + Etx2.row(0).array().square() + Etx2.row(1).array().square()); 
	
	assert( CV_IS_MAT_CONT(error->type) ); 
	Mat isInliers, R, t; 	
	for (int i = 0; i < SimpsonError.cols(); i++) 
	{
		error->data.fl[i] = SimpsonError(0, i); 
	}
*/
}

bool CvEMEstimator::ValidModel(const CvMat* m1, const CvMat* m2, const CvMat* model)
{
	Mat /*p1(m1), p2(m2), Ecv(model),*/ _p1, _p2;
	Mat p1 = cvarrToMat(m1);
	Mat p2 = cvarrToMat(m2);
	Mat Ecv = cvarrToMat(model);
	Eigen::Matrix3d E, V;
	Eigen::Vector3d e2, x1, x2;
	

	int n = p1.cols;
	int failCnt = 0;
	bool emult = false;
    p1 = p1.reshape(1, n); 
    p2 = p2.reshape(1, n);
	//_p1 = p1.clone();
	//_p2 = p2.clone();
	//_p1 = _p1.t();
	//_p2 = _p2.t();
	//_p1 = _p1.reshape(1); 
    //_p2 = _p2.reshape(1);

	cv2eigen(Ecv, E);
	Eigen::JacobiSVD<Eigen::Matrix3d > svdE;
	
	tryocagain:
	svdE.compute(E.transpose(), Eigen::ComputeFullV); //transpose of E added

	if(svdE.singularValues()(0) / svdE.singularValues()(1) > 1.2)
		return false;
	if(!isZero(0.01*svdE.singularValues()(2)/svdE.singularValues()(1)))
		return false;

	V = svdE.matrixV();
	e2 = V.col(2);

	for(int i = 0; i < n; i++)
	{
		Eigen::Vector3d e2_line1, e2_line2;
		x1 << p1.at<double>(i,0),
			  p1.at<double>(i,1),
			  1.0;
		x2 << p2.at<double>(i,0),
			  p2.at<double>(i,1),
			  1.0;
		e2_line1 = e2.cross(x2);
		e2_line2 = E * x1;
		for(int j = 0; j < 3; j++)
		{
			if(isZero(0.1*e2_line1(j)) || isZero(0.1*e2_line2(j)))
				continue;
			if(e2_line1(j)*e2_line2(j) < 0)
			{
				if(emult == false)
				{
					emult = true;
					failCnt = 0;
					E = E * -1.0;
					goto tryocagain;
				}
				failCnt++;
				break;
			}
		}
	}
	if((float)failCnt/(float)n >= 0.4)
		return false;

	return true;
}

void CvEMEstimator::getCoeffMat(double *e, double *A)
{
    double ep2[36], ep3[36]; 
    for (int i = 0; i < 36; i++)
    {
        ep2[i] = e[i] * e[i]; 
        ep3[i] = ep2[i] * e[i]; 
    }

    A[0]=e[33]*e[28]*e[32]-e[33]*e[31]*e[29]+e[30]*e[34]*e[29]-e[30]*e[28]*e[35]-e[27]*e[32]*e[34]+e[27]*e[31]*e[35]; 
    A[146]=.5000000000*e[6]*ep2[8]-.5000000000*e[6]*ep2[5]+.5000000000*ep3[6]+.5000000000*e[6]*ep2[7]-.5000000000*e[6]*ep2[4]+e[0]*e[2]*e[8]+e[3]*e[4]*e[7]+e[3]*e[5]*e[8]+e[0]*e[1]*e[7]-.5000000000*e[6]*ep2[1]-.5000000000*e[6]*ep2[2]+.5000000000*ep2[0]*e[6]+.5000000000*ep2[3]*e[6]; 
    A[1]=e[30]*e[34]*e[2]+e[33]*e[1]*e[32]-e[3]*e[28]*e[35]+e[0]*e[31]*e[35]+e[3]*e[34]*e[29]-e[30]*e[1]*e[35]+e[27]*e[31]*e[8]-e[27]*e[32]*e[7]-e[30]*e[28]*e[8]-e[33]*e[31]*e[2]-e[0]*e[32]*e[34]+e[6]*e[28]*e[32]-e[33]*e[4]*e[29]+e[33]*e[28]*e[5]+e[30]*e[7]*e[29]+e[27]*e[4]*e[35]-e[27]*e[5]*e[34]-e[6]*e[31]*e[29]; 
    A[147]=e[9]*e[27]*e[15]+e[9]*e[29]*e[17]+e[9]*e[11]*e[35]+e[9]*e[28]*e[16]+e[9]*e[10]*e[34]+e[27]*e[11]*e[17]+e[27]*e[10]*e[16]+e[12]*e[30]*e[15]+e[12]*e[32]*e[17]+e[12]*e[14]*e[35]+e[12]*e[31]*e[16]+e[12]*e[13]*e[34]+e[30]*e[14]*e[17]+e[30]*e[13]*e[16]+e[15]*e[35]*e[17]+e[15]*e[34]*e[16]-1.*e[15]*e[28]*e[10]-1.*e[15]*e[31]*e[13]-1.*e[15]*e[32]*e[14]-1.*e[15]*e[29]*e[11]+.5000000000*ep2[9]*e[33]+.5000000000*e[33]*ep2[16]-.5000000000*e[33]*ep2[11]+.5000000000*e[33]*ep2[12]+1.500000000*e[33]*ep2[15]+.5000000000*e[33]*ep2[17]-.5000000000*e[33]*ep2[10]-.5000000000*e[33]*ep2[14]-.5000000000*e[33]*ep2[13]; 
    A[2]=-e[33]*e[22]*e[29]-e[33]*e[31]*e[20]-e[27]*e[32]*e[25]+e[27]*e[22]*e[35]-e[27]*e[23]*e[34]+e[27]*e[31]*e[26]+e[33]*e[28]*e[23]-e[21]*e[28]*e[35]+e[30]*e[25]*e[29]+e[24]*e[28]*e[32]-e[24]*e[31]*e[29]+e[18]*e[31]*e[35]-e[30]*e[28]*e[26]-e[30]*e[19]*e[35]+e[21]*e[34]*e[29]+e[33]*e[19]*e[32]-e[18]*e[32]*e[34]+e[30]*e[34]*e[20]; 
    A[144]=e[18]*e[2]*e[17]+e[3]*e[21]*e[15]+e[3]*e[12]*e[24]+e[3]*e[23]*e[17]+e[3]*e[14]*e[26]+e[3]*e[22]*e[16]+e[3]*e[13]*e[25]+3.*e[6]*e[24]*e[15]+e[6]*e[26]*e[17]+e[6]*e[25]*e[16]+e[0]*e[20]*e[17]+e[0]*e[11]*e[26]+e[0]*e[19]*e[16]+e[0]*e[10]*e[25]+e[15]*e[26]*e[8]-1.*e[15]*e[20]*e[2]-1.*e[15]*e[19]*e[1]-1.*e[15]*e[22]*e[4]+e[15]*e[25]*e[7]-1.*e[15]*e[23]*e[5]+e[12]*e[21]*e[6]+e[12]*e[22]*e[7]+e[12]*e[4]*e[25]+e[12]*e[23]*e[8]+e[12]*e[5]*e[26]-1.*e[24]*e[11]*e[2]-1.*e[24]*e[10]*e[1]-1.*e[24]*e[13]*e[4]+e[24]*e[16]*e[7]-1.*e[24]*e[14]*e[5]+e[24]*e[17]*e[8]+e[21]*e[13]*e[7]+e[21]*e[4]*e[16]+e[21]*e[14]*e[8]+e[21]*e[5]*e[17]-1.*e[6]*e[23]*e[14]-1.*e[6]*e[20]*e[11]-1.*e[6]*e[19]*e[10]-1.*e[6]*e[22]*e[13]+e[9]*e[18]*e[6]+e[9]*e[0]*e[24]+e[9]*e[19]*e[7]+e[9]*e[1]*e[25]+e[9]*e[20]*e[8]+e[9]*e[2]*e[26]+e[18]*e[0]*e[15]+e[18]*e[10]*e[7]+e[18]*e[1]*e[16]+e[18]*e[11]*e[8]; 
    A[3]=e[33]*e[10]*e[32]+e[33]*e[28]*e[14]-e[33]*e[13]*e[29]-e[33]*e[31]*e[11]+e[9]*e[31]*e[35]-e[9]*e[32]*e[34]+e[27]*e[13]*e[35]-e[27]*e[32]*e[16]+e[27]*e[31]*e[17]-e[27]*e[14]*e[34]+e[12]*e[34]*e[29]-e[12]*e[28]*e[35]+e[30]*e[34]*e[11]+e[30]*e[16]*e[29]-e[30]*e[10]*e[35]-e[30]*e[28]*e[17]+e[15]*e[28]*e[32]-e[15]*e[31]*e[29]; 
    A[145]=e[0]*e[27]*e[6]+e[0]*e[28]*e[7]+e[0]*e[1]*e[34]+e[0]*e[29]*e[8]+e[0]*e[2]*e[35]+e[6]*e[34]*e[7]-1.*e[6]*e[32]*e[5]+e[6]*e[30]*e[3]+e[6]*e[35]*e[8]-1.*e[6]*e[29]*e[2]-1.*e[6]*e[28]*e[1]-1.*e[6]*e[31]*e[4]+e[27]*e[1]*e[7]+e[27]*e[2]*e[8]+e[3]*e[31]*e[7]+e[3]*e[4]*e[34]+e[3]*e[32]*e[8]+e[3]*e[5]*e[35]+e[30]*e[4]*e[7]+e[30]*e[5]*e[8]+.5000000000*ep2[0]*e[33]+1.500000000*e[33]*ep2[6]-.5000000000*e[33]*ep2[4]-.5000000000*e[33]*ep2[5]-.5000000000*e[33]*ep2[1]+.5000000000*e[33]*ep2[7]+.5000000000*e[33]*ep2[3]-.5000000000*e[33]*ep2[2]+.5000000000*e[33]*ep2[8]; 
    A[4]=-e[0]*e[23]*e[16]+e[9]*e[4]*e[26]+e[9]*e[22]*e[8]-e[9]*e[5]*e[25]-e[9]*e[23]*e[7]+e[18]*e[4]*e[17]+e[18]*e[13]*e[8]-e[18]*e[5]*e[16]-e[18]*e[14]*e[7]+e[3]*e[16]*e[20]+e[3]*e[25]*e[11]-e[3]*e[10]*e[26]-e[3]*e[19]*e[17]+e[12]*e[7]*e[20]+e[12]*e[25]*e[2]-e[12]*e[1]*e[26]-e[12]*e[19]*e[8]+e[21]*e[7]*e[11]+e[21]*e[16]*e[2]-e[21]*e[1]*e[17]-e[21]*e[10]*e[8]+e[6]*e[10]*e[23]+e[6]*e[19]*e[14]-e[6]*e[13]*e[20]-e[6]*e[22]*e[11]+e[15]*e[1]*e[23]+e[15]*e[19]*e[5]-e[15]*e[4]*e[20]-e[15]*e[22]*e[2]+e[24]*e[1]*e[14]+e[24]*e[10]*e[5]-e[24]*e[4]*e[11]-e[24]*e[13]*e[2]+e[0]*e[13]*e[26]+e[0]*e[22]*e[17]-e[0]*e[14]*e[25]; 
    A[150]=e[18]*e[19]*e[25]+.5000000000*ep3[24]-.5000000000*e[24]*ep2[23]+e[18]*e[20]*e[26]+e[21]*e[22]*e[25]+e[21]*e[23]*e[26]-.5000000000*e[24]*ep2[19]+.5000000000*ep2[21]*e[24]+.5000000000*e[24]*ep2[26]-.5000000000*e[24]*ep2[20]+.5000000000*ep2[18]*e[24]-.5000000000*e[24]*ep2[22]+.5000000000*e[24]*ep2[25]; 
    A[5]=-e[3]*e[1]*e[35]-e[0]*e[32]*e[7]+e[27]*e[4]*e[8]+e[33]*e[1]*e[5]-e[33]*e[4]*e[2]+e[0]*e[4]*e[35]+e[3]*e[34]*e[2]-e[30]*e[1]*e[8]+e[30]*e[7]*e[2]-e[6]*e[4]*e[29]+e[3]*e[7]*e[29]+e[6]*e[1]*e[32]-e[0]*e[5]*e[34]-e[3]*e[28]*e[8]+e[0]*e[31]*e[8]+e[6]*e[28]*e[5]-e[6]*e[31]*e[2]-e[27]*e[5]*e[7]; 
    A[151]=e[33]*e[16]*e[7]-1.*e[33]*e[14]*e[5]+e[33]*e[17]*e[8]+e[30]*e[13]*e[7]+e[30]*e[4]*e[16]+e[30]*e[14]*e[8]+e[30]*e[5]*e[17]+e[6]*e[27]*e[9]-1.*e[6]*e[28]*e[10]-1.*e[6]*e[31]*e[13]-1.*e[6]*e[32]*e[14]-1.*e[6]*e[29]*e[11]+e[9]*e[28]*e[7]+e[9]*e[1]*e[34]+e[9]*e[29]*e[8]+e[9]*e[2]*e[35]+e[27]*e[10]*e[7]+e[27]*e[1]*e[16]+e[27]*e[11]*e[8]+e[27]*e[2]*e[17]+e[3]*e[30]*e[15]+e[3]*e[12]*e[33]+e[3]*e[32]*e[17]+e[3]*e[14]*e[35]+e[3]*e[31]*e[16]+e[3]*e[13]*e[34]+3.*e[6]*e[33]*e[15]+e[6]*e[35]*e[17]+e[6]*e[34]*e[16]+e[0]*e[27]*e[15]+e[0]*e[9]*e[33]+e[0]*e[29]*e[17]+e[0]*e[11]*e[35]+e[0]*e[28]*e[16]+e[0]*e[10]*e[34]+e[15]*e[34]*e[7]-1.*e[15]*e[32]*e[5]+e[15]*e[35]*e[8]-1.*e[15]*e[29]*e[2]-1.*e[15]*e[28]*e[1]-1.*e[15]*e[31]*e[4]+e[12]*e[30]*e[6]+e[12]*e[31]*e[7]+e[12]*e[4]*e[34]+e[12]*e[32]*e[8]+e[12]*e[5]*e[35]-1.*e[33]*e[11]*e[2]-1.*e[33]*e[10]*e[1]-1.*e[33]*e[13]*e[4]; 
    A[6]=e[6]*e[1]*e[5]-e[6]*e[4]*e[2]+e[3]*e[7]*e[2]+e[0]*e[4]*e[8]-e[0]*e[5]*e[7]-e[3]*e[1]*e[8]; 
    A[148]=.5000000000*ep3[15]+e[9]*e[10]*e[16]-.5000000000*e[15]*ep2[11]+e[9]*e[11]*e[17]+.5000000000*ep2[12]*e[15]+.5000000000*e[15]*ep2[16]+.5000000000*e[15]*ep2[17]-.5000000000*e[15]*ep2[13]+.5000000000*ep2[9]*e[15]+e[12]*e[14]*e[17]-.5000000000*e[15]*ep2[10]-.5000000000*e[15]*ep2[14]+e[12]*e[13]*e[16]; 
    A[7]=e[15]*e[28]*e[14]-e[15]*e[13]*e[29]-e[15]*e[31]*e[11]+e[33]*e[10]*e[14]-e[33]*e[13]*e[11]+e[9]*e[13]*e[35]-e[9]*e[32]*e[16]+e[9]*e[31]*e[17]-e[9]*e[14]*e[34]+e[27]*e[13]*e[17]-e[27]*e[14]*e[16]+e[12]*e[34]*e[11]+e[12]*e[16]*e[29]-e[12]*e[10]*e[35]-e[12]*e[28]*e[17]+e[30]*e[16]*e[11]-e[30]*e[10]*e[17]+e[15]*e[10]*e[32]; 
    A[149]=e[18]*e[27]*e[24]+e[18]*e[28]*e[25]+e[18]*e[19]*e[34]+e[18]*e[29]*e[26]+e[18]*e[20]*e[35]+e[27]*e[19]*e[25]+e[27]*e[20]*e[26]+e[21]*e[30]*e[24]+e[21]*e[31]*e[25]+e[21]*e[22]*e[34]+e[21]*e[32]*e[26]+e[21]*e[23]*e[35]+e[30]*e[22]*e[25]+e[30]*e[23]*e[26]+e[24]*e[34]*e[25]+e[24]*e[35]*e[26]-1.*e[24]*e[29]*e[20]-1.*e[24]*e[31]*e[22]-1.*e[24]*e[32]*e[23]-1.*e[24]*e[28]*e[19]+1.500000000*e[33]*ep2[24]+.5000000000*e[33]*ep2[25]+.5000000000*e[33]*ep2[26]-.5000000000*e[33]*ep2[23]-.5000000000*e[33]*ep2[19]-.5000000000*e[33]*ep2[20]-.5000000000*e[33]*ep2[22]+.5000000000*ep2[18]*e[33]+.5000000000*ep2[21]*e[33]; 
    A[9]=e[21]*e[25]*e[29]-e[27]*e[23]*e[25]+e[24]*e[19]*e[32]-e[21]*e[28]*e[26]-e[21]*e[19]*e[35]+e[18]*e[31]*e[26]-e[30]*e[19]*e[26]-e[24]*e[31]*e[20]+e[24]*e[28]*e[23]+e[27]*e[22]*e[26]+e[30]*e[25]*e[20]-e[33]*e[22]*e[20]+e[33]*e[19]*e[23]+e[21]*e[34]*e[20]-e[18]*e[23]*e[34]-e[24]*e[22]*e[29]-e[18]*e[32]*e[25]+e[18]*e[22]*e[35]; 
    A[155]=e[12]*e[14]*e[8]+e[12]*e[5]*e[17]+e[15]*e[16]*e[7]+e[15]*e[17]*e[8]+e[0]*e[11]*e[17]+e[0]*e[9]*e[15]+e[0]*e[10]*e[16]+e[3]*e[14]*e[17]+e[3]*e[13]*e[16]+e[9]*e[10]*e[7]+e[9]*e[1]*e[16]+e[9]*e[11]*e[8]+e[9]*e[2]*e[17]-1.*e[15]*e[11]*e[2]-1.*e[15]*e[10]*e[1]-1.*e[15]*e[13]*e[4]-1.*e[15]*e[14]*e[5]+e[12]*e[3]*e[15]+e[12]*e[13]*e[7]+e[12]*e[4]*e[16]+.5000000000*ep2[12]*e[6]+1.500000000*ep2[15]*e[6]+.5000000000*e[6]*ep2[17]+.5000000000*e[6]*ep2[16]+.5000000000*e[6]*ep2[9]-.5000000000*e[6]*ep2[11]-.5000000000*e[6]*ep2[10]-.5000000000*e[6]*ep2[14]-.5000000000*e[6]*ep2[13]; 
    A[8]=-e[9]*e[14]*e[16]-e[12]*e[10]*e[17]+e[9]*e[13]*e[17]-e[15]*e[13]*e[11]+e[15]*e[10]*e[14]+e[12]*e[16]*e[11]; 
    A[154]=e[21]*e[14]*e[17]+e[21]*e[13]*e[16]+e[15]*e[26]*e[17]+e[15]*e[25]*e[16]-1.*e[15]*e[23]*e[14]-1.*e[15]*e[20]*e[11]-1.*e[15]*e[19]*e[10]-1.*e[15]*e[22]*e[13]+e[9]*e[20]*e[17]+e[9]*e[11]*e[26]+e[9]*e[19]*e[16]+e[9]*e[10]*e[25]+.5000000000*ep2[12]*e[24]+1.500000000*e[24]*ep2[15]+.5000000000*e[24]*ep2[17]+.5000000000*e[24]*ep2[16]+.5000000000*ep2[9]*e[24]-.5000000000*e[24]*ep2[11]-.5000000000*e[24]*ep2[10]-.5000000000*e[24]*ep2[14]-.5000000000*e[24]*ep2[13]+e[18]*e[11]*e[17]+e[18]*e[9]*e[15]+e[18]*e[10]*e[16]+e[12]*e[21]*e[15]+e[12]*e[23]*e[17]+e[12]*e[14]*e[26]+e[12]*e[22]*e[16]+e[12]*e[13]*e[25]; 
    A[11]=-e[9]*e[5]*e[34]+e[9]*e[31]*e[8]-e[9]*e[32]*e[7]+e[27]*e[4]*e[17]+e[27]*e[13]*e[8]-e[27]*e[5]*e[16]-e[27]*e[14]*e[7]+e[0]*e[13]*e[35]-e[0]*e[32]*e[16]+e[0]*e[31]*e[17]-e[0]*e[14]*e[34]+e[9]*e[4]*e[35]+e[6]*e[10]*e[32]+e[6]*e[28]*e[14]-e[6]*e[13]*e[29]-e[6]*e[31]*e[11]+e[15]*e[1]*e[32]+e[3]*e[34]*e[11]+e[3]*e[16]*e[29]-e[3]*e[10]*e[35]-e[3]*e[28]*e[17]-e[12]*e[1]*e[35]+e[12]*e[7]*e[29]+e[12]*e[34]*e[2]-e[12]*e[28]*e[8]+e[15]*e[28]*e[5]-e[15]*e[4]*e[29]-e[15]*e[31]*e[2]+e[33]*e[1]*e[14]+e[33]*e[10]*e[5]-e[33]*e[4]*e[11]-e[33]*e[13]*e[2]+e[30]*e[7]*e[11]+e[30]*e[16]*e[2]-e[30]*e[1]*e[17]-e[30]*e[10]*e[8]; 
    A[153]=e[21]*e[31]*e[7]+e[21]*e[4]*e[34]+e[21]*e[32]*e[8]+e[21]*e[5]*e[35]+e[30]*e[22]*e[7]+e[30]*e[4]*e[25]+e[30]*e[23]*e[8]+e[30]*e[5]*e[26]+3.*e[24]*e[33]*e[6]+e[24]*e[34]*e[7]+e[24]*e[35]*e[8]+e[33]*e[25]*e[7]+e[33]*e[26]*e[8]+e[0]*e[27]*e[24]+e[0]*e[18]*e[33]+e[0]*e[28]*e[25]+e[0]*e[19]*e[34]+e[0]*e[29]*e[26]+e[0]*e[20]*e[35]+e[18]*e[27]*e[6]+e[18]*e[28]*e[7]+e[18]*e[1]*e[34]+e[18]*e[29]*e[8]+e[18]*e[2]*e[35]+e[27]*e[19]*e[7]+e[27]*e[1]*e[25]+e[27]*e[20]*e[8]+e[27]*e[2]*e[26]+e[3]*e[30]*e[24]+e[3]*e[21]*e[33]+e[3]*e[31]*e[25]+e[3]*e[22]*e[34]+e[3]*e[32]*e[26]+e[3]*e[23]*e[35]+e[6]*e[30]*e[21]-1.*e[6]*e[29]*e[20]+e[6]*e[35]*e[26]-1.*e[6]*e[31]*e[22]-1.*e[6]*e[32]*e[23]-1.*e[6]*e[28]*e[19]+e[6]*e[34]*e[25]-1.*e[24]*e[32]*e[5]-1.*e[24]*e[29]*e[2]-1.*e[24]*e[28]*e[1]-1.*e[24]*e[31]*e[4]-1.*e[33]*e[20]*e[2]-1.*e[33]*e[19]*e[1]-1.*e[33]*e[22]*e[4]-1.*e[33]*e[23]*e[5]; 
    A[10]=e[21]*e[25]*e[20]-e[21]*e[19]*e[26]+e[18]*e[22]*e[26]-e[18]*e[23]*e[25]-e[24]*e[22]*e[20]+e[24]*e[19]*e[23]; 
    A[152]=e[3]*e[4]*e[25]+e[3]*e[23]*e[8]+e[3]*e[5]*e[26]+e[21]*e[4]*e[7]+e[21]*e[5]*e[8]+e[6]*e[25]*e[7]+e[6]*e[26]*e[8]+e[0]*e[19]*e[7]+e[0]*e[1]*e[25]+e[0]*e[20]*e[8]+e[0]*e[2]*e[26]-1.*e[6]*e[20]*e[2]-1.*e[6]*e[19]*e[1]-1.*e[6]*e[22]*e[4]-1.*e[6]*e[23]*e[5]+e[18]*e[1]*e[7]+e[18]*e[0]*e[6]+e[18]*e[2]*e[8]+e[3]*e[21]*e[6]+e[3]*e[22]*e[7]-.5000000000*e[24]*ep2[4]+.5000000000*e[24]*ep2[0]+1.500000000*e[24]*ep2[6]-.5000000000*e[24]*ep2[5]-.5000000000*e[24]*ep2[1]+.5000000000*e[24]*ep2[7]+.5000000000*e[24]*ep2[3]-.5000000000*e[24]*ep2[2]+.5000000000*e[24]*ep2[8]; 
    A[13]=e[6]*e[28]*e[23]-e[6]*e[22]*e[29]-e[6]*e[31]*e[20]-e[3]*e[19]*e[35]+e[3]*e[34]*e[20]+e[3]*e[25]*e[29]-e[21]*e[1]*e[35]+e[21]*e[7]*e[29]+e[21]*e[34]*e[2]+e[24]*e[1]*e[32]+e[24]*e[28]*e[5]-e[24]*e[4]*e[29]-e[24]*e[31]*e[2]+e[33]*e[1]*e[23]+e[33]*e[19]*e[5]-e[33]*e[4]*e[20]-e[33]*e[22]*e[2]-e[21]*e[28]*e[8]+e[30]*e[7]*e[20]+e[30]*e[25]*e[2]-e[30]*e[1]*e[26]+e[18]*e[4]*e[35]-e[18]*e[5]*e[34]+e[18]*e[31]*e[8]-e[18]*e[32]*e[7]+e[27]*e[4]*e[26]+e[27]*e[22]*e[8]-e[27]*e[5]*e[25]-e[27]*e[23]*e[7]-e[3]*e[28]*e[26]-e[0]*e[32]*e[25]+e[0]*e[22]*e[35]-e[0]*e[23]*e[34]+e[0]*e[31]*e[26]-e[30]*e[19]*e[8]+e[6]*e[19]*e[32]; 
    A[159]=.5000000000*ep2[18]*e[6]+.5000000000*ep2[21]*e[6]+1.500000000*ep2[24]*e[6]+.5000000000*e[6]*ep2[26]-.5000000000*e[6]*ep2[23]-.5000000000*e[6]*ep2[19]-.5000000000*e[6]*ep2[20]-.5000000000*e[6]*ep2[22]+.5000000000*e[6]*ep2[25]+e[21]*e[3]*e[24]+e[18]*e[20]*e[8]+e[21]*e[4]*e[25]+e[18]*e[19]*e[7]+e[18]*e[1]*e[25]+e[21]*e[22]*e[7]+e[21]*e[23]*e[8]+e[18]*e[0]*e[24]+e[18]*e[2]*e[26]+e[21]*e[5]*e[26]+e[24]*e[26]*e[8]-1.*e[24]*e[20]*e[2]-1.*e[24]*e[19]*e[1]-1.*e[24]*e[22]*e[4]+e[24]*e[25]*e[7]-1.*e[24]*e[23]*e[5]+e[0]*e[19]*e[25]+e[0]*e[20]*e[26]+e[3]*e[22]*e[25]+e[3]*e[23]*e[26]; 
    A[12]=e[18]*e[4]*e[8]+e[3]*e[7]*e[20]+e[3]*e[25]*e[2]-e[3]*e[1]*e[26]-e[18]*e[5]*e[7]+e[6]*e[1]*e[23]+e[6]*e[19]*e[5]-e[6]*e[4]*e[20]-e[6]*e[22]*e[2]+e[21]*e[7]*e[2]-e[21]*e[1]*e[8]+e[24]*e[1]*e[5]-e[24]*e[4]*e[2]-e[3]*e[19]*e[8]+e[0]*e[4]*e[26]+e[0]*e[22]*e[8]-e[0]*e[5]*e[25]-e[0]*e[23]*e[7]; 
    A[158]=e[9]*e[1]*e[7]+e[9]*e[0]*e[6]+e[9]*e[2]*e[8]+e[3]*e[12]*e[6]+e[3]*e[13]*e[7]+e[3]*e[4]*e[16]+e[3]*e[14]*e[8]+e[3]*e[5]*e[17]+e[12]*e[4]*e[7]+e[12]*e[5]*e[8]+e[6]*e[16]*e[7]+e[6]*e[17]*e[8]-1.*e[6]*e[11]*e[2]-1.*e[6]*e[10]*e[1]-1.*e[6]*e[13]*e[4]-1.*e[6]*e[14]*e[5]+e[0]*e[10]*e[7]+e[0]*e[1]*e[16]+e[0]*e[11]*e[8]+e[0]*e[2]*e[17]+.5000000000*ep2[3]*e[15]+1.500000000*e[15]*ep2[6]+.5000000000*e[15]*ep2[7]+.5000000000*e[15]*ep2[8]+.5000000000*ep2[0]*e[15]-.5000000000*e[15]*ep2[4]-.5000000000*e[15]*ep2[5]-.5000000000*e[15]*ep2[1]-.5000000000*e[15]*ep2[2]; 
    A[15]=-e[15]*e[13]*e[2]-e[6]*e[13]*e[11]-e[15]*e[4]*e[11]+e[12]*e[16]*e[2]-e[3]*e[10]*e[17]+e[3]*e[16]*e[11]+e[0]*e[13]*e[17]-e[0]*e[14]*e[16]+e[15]*e[1]*e[14]-e[12]*e[10]*e[8]+e[9]*e[4]*e[17]+e[9]*e[13]*e[8]-e[9]*e[5]*e[16]-e[9]*e[14]*e[7]+e[15]*e[10]*e[5]+e[12]*e[7]*e[11]+e[6]*e[10]*e[14]-e[12]*e[1]*e[17]; 
    A[157]=e[12]*e[30]*e[24]+e[12]*e[21]*e[33]+e[12]*e[31]*e[25]+e[12]*e[22]*e[34]+e[12]*e[32]*e[26]+e[12]*e[23]*e[35]+e[9]*e[27]*e[24]+e[9]*e[18]*e[33]+e[9]*e[28]*e[25]+e[9]*e[19]*e[34]+e[9]*e[29]*e[26]+e[9]*e[20]*e[35]+e[21]*e[30]*e[15]+e[21]*e[32]*e[17]+e[21]*e[14]*e[35]+e[21]*e[31]*e[16]+e[21]*e[13]*e[34]+e[30]*e[23]*e[17]+e[30]*e[14]*e[26]+e[30]*e[22]*e[16]+e[30]*e[13]*e[25]+e[15]*e[27]*e[18]+3.*e[15]*e[33]*e[24]-1.*e[15]*e[29]*e[20]+e[15]*e[35]*e[26]-1.*e[15]*e[31]*e[22]-1.*e[15]*e[32]*e[23]-1.*e[15]*e[28]*e[19]+e[15]*e[34]*e[25]+e[18]*e[29]*e[17]+e[18]*e[11]*e[35]+e[18]*e[28]*e[16]+e[18]*e[10]*e[34]+e[27]*e[20]*e[17]+e[27]*e[11]*e[26]+e[27]*e[19]*e[16]+e[27]*e[10]*e[25]-1.*e[24]*e[28]*e[10]-1.*e[24]*e[31]*e[13]-1.*e[24]*e[32]*e[14]+e[24]*e[34]*e[16]+e[24]*e[35]*e[17]-1.*e[24]*e[29]*e[11]-1.*e[33]*e[23]*e[14]+e[33]*e[25]*e[16]+e[33]*e[26]*e[17]-1.*e[33]*e[20]*e[11]-1.*e[33]*e[19]*e[10]-1.*e[33]*e[22]*e[13]; 
    A[14]=e[18]*e[13]*e[17]+e[9]*e[13]*e[26]+e[9]*e[22]*e[17]-e[9]*e[14]*e[25]-e[18]*e[14]*e[16]-e[15]*e[13]*e[20]-e[15]*e[22]*e[11]+e[12]*e[16]*e[20]+e[12]*e[25]*e[11]-e[12]*e[10]*e[26]-e[12]*e[19]*e[17]+e[21]*e[16]*e[11]-e[21]*e[10]*e[17]-e[9]*e[23]*e[16]+e[24]*e[10]*e[14]-e[24]*e[13]*e[11]+e[15]*e[10]*e[23]+e[15]*e[19]*e[14]; 
    A[156]=e[21]*e[12]*e[24]+e[21]*e[23]*e[17]+e[21]*e[14]*e[26]+e[21]*e[22]*e[16]+e[21]*e[13]*e[25]+e[24]*e[26]*e[17]+e[24]*e[25]*e[16]+e[9]*e[19]*e[25]+e[9]*e[18]*e[24]+e[9]*e[20]*e[26]+e[12]*e[22]*e[25]+e[12]*e[23]*e[26]+e[18]*e[20]*e[17]+e[18]*e[11]*e[26]+e[18]*e[19]*e[16]+e[18]*e[10]*e[25]-1.*e[24]*e[23]*e[14]-1.*e[24]*e[20]*e[11]-1.*e[24]*e[19]*e[10]-1.*e[24]*e[22]*e[13]+.5000000000*ep2[21]*e[15]+1.500000000*ep2[24]*e[15]+.5000000000*e[15]*ep2[25]+.5000000000*e[15]*ep2[26]+.5000000000*e[15]*ep2[18]-.5000000000*e[15]*ep2[23]-.5000000000*e[15]*ep2[19]-.5000000000*e[15]*ep2[20]-.5000000000*e[15]*ep2[22]; 
    A[18]=e[6]*e[1]*e[14]+e[15]*e[1]*e[5]-e[0]*e[5]*e[16]-e[0]*e[14]*e[7]+e[0]*e[13]*e[8]-e[15]*e[4]*e[2]+e[12]*e[7]*e[2]+e[6]*e[10]*e[5]+e[3]*e[7]*e[11]-e[6]*e[4]*e[11]+e[3]*e[16]*e[2]-e[6]*e[13]*e[2]-e[3]*e[1]*e[17]-e[9]*e[5]*e[7]-e[3]*e[10]*e[8]-e[12]*e[1]*e[8]+e[0]*e[4]*e[17]+e[9]*e[4]*e[8]; 
    A[128]=-.5000000000*e[14]*ep2[16]-.5000000000*e[14]*ep2[10]-.5000000000*e[14]*ep2[9]+e[11]*e[9]*e[12]+.5000000000*ep3[14]+e[17]*e[13]*e[16]+.5000000000*e[14]*ep2[12]+e[11]*e[10]*e[13]-.5000000000*e[14]*ep2[15]+.5000000000*e[14]*ep2[17]+e[17]*e[12]*e[15]+.5000000000*ep2[11]*e[14]+.5000000000*e[14]*ep2[13]; 
    A[19]=-e[21]*e[19]*e[8]+e[18]*e[4]*e[26]-e[18]*e[5]*e[25]-e[18]*e[23]*e[7]+e[21]*e[25]*e[2]-e[21]*e[1]*e[26]+e[6]*e[19]*e[23]+e[18]*e[22]*e[8]-e[0]*e[23]*e[25]-e[6]*e[22]*e[20]+e[24]*e[1]*e[23]+e[24]*e[19]*e[5]-e[24]*e[4]*e[20]-e[24]*e[22]*e[2]+e[3]*e[25]*e[20]-e[3]*e[19]*e[26]+e[0]*e[22]*e[26]+e[21]*e[7]*e[20]; 
    A[129]=.5000000000*ep2[20]*e[32]+1.500000000*e[32]*ep2[23]+.5000000000*e[32]*ep2[22]+.5000000000*e[32]*ep2[21]+.5000000000*e[32]*ep2[26]-.5000000000*e[32]*ep2[18]-.5000000000*e[32]*ep2[19]-.5000000000*e[32]*ep2[24]-.5000000000*e[32]*ep2[25]+e[20]*e[27]*e[21]+e[20]*e[18]*e[30]+e[20]*e[28]*e[22]+e[20]*e[19]*e[31]+e[20]*e[29]*e[23]+e[29]*e[19]*e[22]+e[29]*e[18]*e[21]+e[23]*e[30]*e[21]+e[23]*e[31]*e[22]+e[26]*e[30]*e[24]+e[26]*e[21]*e[33]+e[26]*e[31]*e[25]+e[26]*e[22]*e[34]+e[26]*e[23]*e[35]+e[35]*e[22]*e[25]+e[35]*e[21]*e[24]-1.*e[23]*e[27]*e[18]-1.*e[23]*e[33]*e[24]-1.*e[23]*e[28]*e[19]-1.*e[23]*e[34]*e[25]; 
    A[16]=-e[9]*e[23]*e[25]-e[21]*e[10]*e[26]-e[21]*e[19]*e[17]-e[18]*e[23]*e[16]+e[18]*e[13]*e[26]+e[12]*e[25]*e[20]-e[12]*e[19]*e[26]-e[15]*e[22]*e[20]+e[21]*e[16]*e[20]+e[21]*e[25]*e[11]+e[24]*e[10]*e[23]+e[24]*e[19]*e[14]-e[24]*e[13]*e[20]-e[24]*e[22]*e[11]+e[18]*e[22]*e[17]-e[18]*e[14]*e[25]+e[9]*e[22]*e[26]+e[15]*e[19]*e[23]; 
    A[130]=.5000000000*e[23]*ep2[21]+e[20]*e[19]*e[22]+e[20]*e[18]*e[21]+.5000000000*ep3[23]+e[26]*e[22]*e[25]+.5000000000*e[23]*ep2[26]-.5000000000*e[23]*ep2[18]+.5000000000*e[23]*ep2[22]-.5000000000*e[23]*ep2[19]+e[26]*e[21]*e[24]+.5000000000*ep2[20]*e[23]-.5000000000*e[23]*ep2[24]-.5000000000*e[23]*ep2[25]; 
    A[17]=e[18]*e[13]*e[35]-e[18]*e[32]*e[16]+e[18]*e[31]*e[17]-e[18]*e[14]*e[34]+e[27]*e[13]*e[26]+e[27]*e[22]*e[17]-e[27]*e[14]*e[25]-e[27]*e[23]*e[16]-e[9]*e[32]*e[25]+e[9]*e[22]*e[35]-e[9]*e[23]*e[34]+e[9]*e[31]*e[26]+e[15]*e[19]*e[32]+e[15]*e[28]*e[23]-e[15]*e[22]*e[29]-e[15]*e[31]*e[20]+e[24]*e[10]*e[32]+e[24]*e[28]*e[14]-e[24]*e[13]*e[29]-e[24]*e[31]*e[11]+e[33]*e[10]*e[23]+e[33]*e[19]*e[14]-e[33]*e[13]*e[20]-e[33]*e[22]*e[11]+e[21]*e[16]*e[29]-e[21]*e[10]*e[35]-e[21]*e[28]*e[17]+e[30]*e[16]*e[20]+e[30]*e[25]*e[11]-e[30]*e[10]*e[26]-e[30]*e[19]*e[17]-e[12]*e[28]*e[26]-e[12]*e[19]*e[35]+e[12]*e[34]*e[20]+e[12]*e[25]*e[29]+e[21]*e[34]*e[11]; 
    A[131]=-1.*e[32]*e[10]*e[1]+e[32]*e[13]*e[4]-1.*e[32]*e[16]*e[7]-1.*e[32]*e[15]*e[6]-1.*e[32]*e[9]*e[0]+e[32]*e[12]*e[3]+e[17]*e[30]*e[6]+e[17]*e[3]*e[33]+e[17]*e[31]*e[7]+e[17]*e[4]*e[34]+e[17]*e[5]*e[35]-1.*e[5]*e[27]*e[9]-1.*e[5]*e[28]*e[10]-1.*e[5]*e[33]*e[15]-1.*e[5]*e[34]*e[16]+e[5]*e[29]*e[11]+e[35]*e[12]*e[6]+e[35]*e[3]*e[15]+e[35]*e[13]*e[7]+e[35]*e[4]*e[16]+e[11]*e[27]*e[3]+e[11]*e[0]*e[30]+e[11]*e[28]*e[4]+e[11]*e[1]*e[31]+e[29]*e[9]*e[3]+e[29]*e[0]*e[12]+e[29]*e[10]*e[4]+e[29]*e[1]*e[13]+e[5]*e[30]*e[12]+3.*e[5]*e[32]*e[14]+e[5]*e[31]*e[13]+e[8]*e[30]*e[15]+e[8]*e[12]*e[33]+e[8]*e[32]*e[17]+e[8]*e[14]*e[35]+e[8]*e[31]*e[16]+e[8]*e[13]*e[34]+e[2]*e[27]*e[12]+e[2]*e[9]*e[30]+e[2]*e[29]*e[14]+e[2]*e[11]*e[32]+e[2]*e[28]*e[13]+e[2]*e[10]*e[31]-1.*e[14]*e[27]*e[0]-1.*e[14]*e[34]*e[7]-1.*e[14]*e[33]*e[6]+e[14]*e[30]*e[3]-1.*e[14]*e[28]*e[1]+e[14]*e[31]*e[4]; 
    A[22]=.5000000000*e[18]*ep2[29]+.5000000000*e[18]*ep2[28]+.5000000000*e[18]*ep2[30]+.5000000000*e[18]*ep2[33]-.5000000000*e[18]*ep2[32]-.5000000000*e[18]*ep2[31]-.5000000000*e[18]*ep2[34]-.5000000000*e[18]*ep2[35]+1.500000000*e[18]*ep2[27]+e[27]*e[28]*e[19]+e[27]*e[29]*e[20]+e[21]*e[27]*e[30]+e[21]*e[29]*e[32]+e[21]*e[28]*e[31]+e[30]*e[28]*e[22]+e[30]*e[19]*e[31]+e[30]*e[29]*e[23]+e[30]*e[20]*e[32]+e[24]*e[27]*e[33]+e[24]*e[29]*e[35]+e[24]*e[28]*e[34]+e[33]*e[28]*e[25]+e[33]*e[19]*e[34]+e[33]*e[29]*e[26]+e[33]*e[20]*e[35]-1.*e[27]*e[35]*e[26]-1.*e[27]*e[31]*e[22]-1.*e[27]*e[32]*e[23]-1.*e[27]*e[34]*e[25]; 
    A[132]=e[20]*e[1]*e[4]+e[20]*e[0]*e[3]+e[20]*e[2]*e[5]+e[5]*e[21]*e[3]+e[5]*e[22]*e[4]+e[8]*e[21]*e[6]+e[8]*e[3]*e[24]+e[8]*e[22]*e[7]+e[8]*e[4]*e[25]+e[8]*e[5]*e[26]+e[26]*e[4]*e[7]+e[26]*e[3]*e[6]+e[2]*e[18]*e[3]+e[2]*e[0]*e[21]+e[2]*e[19]*e[4]+e[2]*e[1]*e[22]-1.*e[5]*e[19]*e[1]-1.*e[5]*e[18]*e[0]-1.*e[5]*e[25]*e[7]-1.*e[5]*e[24]*e[6]+.5000000000*e[23]*ep2[4]-.5000000000*e[23]*ep2[0]-.5000000000*e[23]*ep2[6]+1.500000000*e[23]*ep2[5]-.5000000000*e[23]*ep2[1]-.5000000000*e[23]*ep2[7]+.5000000000*e[23]*ep2[3]+.5000000000*e[23]*ep2[2]+.5000000000*e[23]*ep2[8]; 
    A[23]=1.500000000*e[9]*ep2[27]+.5000000000*e[9]*ep2[29]+.5000000000*e[9]*ep2[28]-.5000000000*e[9]*ep2[32]-.5000000000*e[9]*ep2[31]+.5000000000*e[9]*ep2[33]+.5000000000*e[9]*ep2[30]-.5000000000*e[9]*ep2[34]-.5000000000*e[9]*ep2[35]+e[33]*e[27]*e[15]+e[33]*e[29]*e[17]+e[33]*e[11]*e[35]+e[33]*e[28]*e[16]+e[33]*e[10]*e[34]+e[27]*e[29]*e[11]+e[27]*e[28]*e[10]+e[27]*e[30]*e[12]-1.*e[27]*e[31]*e[13]-1.*e[27]*e[32]*e[14]-1.*e[27]*e[34]*e[16]-1.*e[27]*e[35]*e[17]+e[30]*e[29]*e[14]+e[30]*e[11]*e[32]+e[30]*e[28]*e[13]+e[30]*e[10]*e[31]+e[12]*e[29]*e[32]+e[12]*e[28]*e[31]+e[15]*e[29]*e[35]+e[15]*e[28]*e[34]; 
    A[133]=-1.*e[32]*e[24]*e[6]+e[8]*e[30]*e[24]+e[8]*e[21]*e[33]+e[8]*e[31]*e[25]+e[8]*e[22]*e[34]+e[26]*e[30]*e[6]+e[26]*e[3]*e[33]+e[26]*e[31]*e[7]+e[26]*e[4]*e[34]+e[26]*e[32]*e[8]+e[26]*e[5]*e[35]+e[35]*e[21]*e[6]+e[35]*e[3]*e[24]+e[35]*e[22]*e[7]+e[35]*e[4]*e[25]+e[35]*e[23]*e[8]+e[2]*e[27]*e[21]+e[2]*e[18]*e[30]+e[2]*e[28]*e[22]+e[2]*e[19]*e[31]+e[2]*e[29]*e[23]+e[2]*e[20]*e[32]+e[20]*e[27]*e[3]+e[20]*e[0]*e[30]+e[20]*e[28]*e[4]+e[20]*e[1]*e[31]+e[20]*e[29]*e[5]+e[29]*e[18]*e[3]+e[29]*e[0]*e[21]+e[29]*e[19]*e[4]+e[29]*e[1]*e[22]+e[5]*e[30]*e[21]+e[5]*e[31]*e[22]+3.*e[5]*e[32]*e[23]-1.*e[5]*e[27]*e[18]-1.*e[5]*e[33]*e[24]-1.*e[5]*e[28]*e[19]-1.*e[5]*e[34]*e[25]-1.*e[23]*e[27]*e[0]-1.*e[23]*e[34]*e[7]-1.*e[23]*e[33]*e[6]+e[23]*e[30]*e[3]-1.*e[23]*e[28]*e[1]+e[23]*e[31]*e[4]+e[32]*e[21]*e[3]-1.*e[32]*e[19]*e[1]+e[32]*e[22]*e[4]-1.*e[32]*e[18]*e[0]-1.*e[32]*e[25]*e[7]; 
    A[20]=.5000000000*e[27]*ep2[33]-.5000000000*e[27]*ep2[32]-.5000000000*e[27]*ep2[31]-.5000000000*e[27]*ep2[34]-.5000000000*e[27]*ep2[35]+e[33]*e[29]*e[35]+.5000000000*e[27]*ep2[29]+e[30]*e[29]*e[32]+e[30]*e[28]*e[31]+e[33]*e[28]*e[34]+.5000000000*e[27]*ep2[28]+.5000000000*e[27]*ep2[30]+.5000000000*ep3[27]; 
    A[134]=e[14]*e[21]*e[12]+e[14]*e[22]*e[13]+e[17]*e[21]*e[15]+e[17]*e[12]*e[24]+e[17]*e[14]*e[26]+e[17]*e[22]*e[16]+e[17]*e[13]*e[25]+e[26]*e[12]*e[15]+e[26]*e[13]*e[16]-1.*e[14]*e[24]*e[15]-1.*e[14]*e[25]*e[16]-1.*e[14]*e[18]*e[9]-1.*e[14]*e[19]*e[10]+e[11]*e[18]*e[12]+e[11]*e[9]*e[21]+e[11]*e[19]*e[13]+e[11]*e[10]*e[22]+e[20]*e[11]*e[14]+e[20]*e[9]*e[12]+e[20]*e[10]*e[13]+1.500000000*e[23]*ep2[14]+.5000000000*e[23]*ep2[12]+.5000000000*e[23]*ep2[13]+.5000000000*e[23]*ep2[17]+.5000000000*ep2[11]*e[23]-.5000000000*e[23]*ep2[16]-.5000000000*e[23]*ep2[9]-.5000000000*e[23]*ep2[15]-.5000000000*e[23]*ep2[10]; 
    A[21]=1.500000000*e[0]*ep2[27]+.5000000000*e[0]*ep2[29]+.5000000000*e[0]*ep2[28]+.5000000000*e[0]*ep2[30]-.5000000000*e[0]*ep2[32]-.5000000000*e[0]*ep2[31]+.5000000000*e[0]*ep2[33]-.5000000000*e[0]*ep2[34]-.5000000000*e[0]*ep2[35]-1.*e[27]*e[31]*e[4]+e[3]*e[27]*e[30]+e[3]*e[29]*e[32]+e[3]*e[28]*e[31]+e[30]*e[28]*e[4]+e[30]*e[1]*e[31]+e[30]*e[29]*e[5]+e[30]*e[2]*e[32]+e[6]*e[27]*e[33]+e[6]*e[29]*e[35]+e[6]*e[28]*e[34]+e[27]*e[28]*e[1]+e[27]*e[29]*e[2]+e[33]*e[28]*e[7]+e[33]*e[1]*e[34]+e[33]*e[29]*e[8]+e[33]*e[2]*e[35]-1.*e[27]*e[34]*e[7]-1.*e[27]*e[32]*e[5]-1.*e[27]*e[35]*e[8]; 
    A[135]=e[14]*e[12]*e[3]+e[14]*e[13]*e[4]+e[17]*e[12]*e[6]+e[17]*e[3]*e[15]+e[17]*e[13]*e[7]+e[17]*e[4]*e[16]+e[17]*e[14]*e[8]+e[8]*e[12]*e[15]+e[8]*e[13]*e[16]+e[2]*e[11]*e[14]+e[2]*e[9]*e[12]+e[2]*e[10]*e[13]+e[11]*e[9]*e[3]+e[11]*e[0]*e[12]+e[11]*e[10]*e[4]+e[11]*e[1]*e[13]-1.*e[14]*e[10]*e[1]-1.*e[14]*e[16]*e[7]-1.*e[14]*e[15]*e[6]-1.*e[14]*e[9]*e[0]-.5000000000*e[5]*ep2[16]-.5000000000*e[5]*ep2[9]+.5000000000*e[5]*ep2[11]+.5000000000*e[5]*ep2[12]-.5000000000*e[5]*ep2[15]-.5000000000*e[5]*ep2[10]+.5000000000*e[5]*ep2[13]+1.500000000*ep2[14]*e[5]+.5000000000*e[5]*ep2[17]; 
    A[27]=1.500000000*e[27]*ep2[9]-.5000000000*e[27]*ep2[16]+.5000000000*e[27]*ep2[11]+.5000000000*e[27]*ep2[12]+.5000000000*e[27]*ep2[15]-.5000000000*e[27]*ep2[17]+.5000000000*e[27]*ep2[10]-.5000000000*e[27]*ep2[14]-.5000000000*e[27]*ep2[13]+e[12]*e[10]*e[31]+e[30]*e[11]*e[14]+e[30]*e[10]*e[13]+e[15]*e[9]*e[33]+e[15]*e[29]*e[17]+e[15]*e[11]*e[35]+e[15]*e[28]*e[16]+e[15]*e[10]*e[34]+e[33]*e[11]*e[17]+e[33]*e[10]*e[16]-1.*e[9]*e[31]*e[13]-1.*e[9]*e[32]*e[14]-1.*e[9]*e[34]*e[16]-1.*e[9]*e[35]*e[17]+e[9]*e[29]*e[11]+e[9]*e[28]*e[10]+e[12]*e[9]*e[30]+e[12]*e[29]*e[14]+e[12]*e[11]*e[32]+e[12]*e[28]*e[13]; 
    A[137]=e[29]*e[18]*e[12]+e[29]*e[9]*e[21]+e[29]*e[19]*e[13]+e[29]*e[10]*e[22]+e[17]*e[30]*e[24]+e[17]*e[21]*e[33]+e[17]*e[31]*e[25]+e[17]*e[22]*e[34]+e[17]*e[32]*e[26]+e[17]*e[23]*e[35]-1.*e[23]*e[27]*e[9]-1.*e[23]*e[28]*e[10]-1.*e[23]*e[33]*e[15]-1.*e[23]*e[34]*e[16]-1.*e[32]*e[24]*e[15]-1.*e[32]*e[25]*e[16]-1.*e[32]*e[18]*e[9]-1.*e[32]*e[19]*e[10]+e[26]*e[30]*e[15]+e[26]*e[12]*e[33]+e[26]*e[31]*e[16]+e[26]*e[13]*e[34]+e[35]*e[21]*e[15]+e[35]*e[12]*e[24]+e[35]*e[22]*e[16]+e[35]*e[13]*e[25]+e[14]*e[30]*e[21]+e[14]*e[31]*e[22]+3.*e[14]*e[32]*e[23]+e[11]*e[27]*e[21]+e[11]*e[18]*e[30]+e[11]*e[28]*e[22]+e[11]*e[19]*e[31]+e[11]*e[29]*e[23]+e[11]*e[20]*e[32]+e[23]*e[30]*e[12]+e[23]*e[31]*e[13]+e[32]*e[21]*e[12]+e[32]*e[22]*e[13]-1.*e[14]*e[27]*e[18]-1.*e[14]*e[33]*e[24]+e[14]*e[29]*e[20]+e[14]*e[35]*e[26]-1.*e[14]*e[28]*e[19]-1.*e[14]*e[34]*e[25]+e[20]*e[27]*e[12]+e[20]*e[9]*e[30]+e[20]*e[28]*e[13]+e[20]*e[10]*e[31]; 
    A[26]=.5000000000*e[0]*ep2[1]+.5000000000*e[0]*ep2[2]+e[6]*e[2]*e[8]+e[6]*e[1]*e[7]+.5000000000*e[0]*ep2[3]+e[3]*e[1]*e[4]+.5000000000*e[0]*ep2[6]+e[3]*e[2]*e[5]-.5000000000*e[0]*ep2[5]-.5000000000*e[0]*ep2[8]+.5000000000*ep3[0]-.5000000000*e[0]*ep2[7]-.5000000000*e[0]*ep2[4]; 
    A[136]=1.500000000*ep2[23]*e[14]+.5000000000*e[14]*ep2[26]-.5000000000*e[14]*ep2[18]-.5000000000*e[14]*ep2[19]+.5000000000*e[14]*ep2[20]+.5000000000*e[14]*ep2[22]-.5000000000*e[14]*ep2[24]+.5000000000*e[14]*ep2[21]-.5000000000*e[14]*ep2[25]+e[23]*e[21]*e[12]+e[23]*e[22]*e[13]+e[26]*e[21]*e[15]+e[26]*e[12]*e[24]+e[26]*e[23]*e[17]+e[26]*e[22]*e[16]+e[26]*e[13]*e[25]+e[17]*e[22]*e[25]+e[17]*e[21]*e[24]+e[11]*e[19]*e[22]+e[11]*e[18]*e[21]+e[11]*e[20]*e[23]+e[20]*e[18]*e[12]+e[20]*e[9]*e[21]+e[20]*e[19]*e[13]+e[20]*e[10]*e[22]-1.*e[23]*e[24]*e[15]-1.*e[23]*e[25]*e[16]-1.*e[23]*e[18]*e[9]-1.*e[23]*e[19]*e[10]; 
    A[25]=1.500000000*e[27]*ep2[0]-.5000000000*e[27]*ep2[4]+.5000000000*e[27]*ep2[6]-.5000000000*e[27]*ep2[5]+.5000000000*e[27]*ep2[1]-.5000000000*e[27]*ep2[7]+.5000000000*e[27]*ep2[3]+.5000000000*e[27]*ep2[2]-.5000000000*e[27]*ep2[8]+e[0]*e[33]*e[6]+e[0]*e[30]*e[3]-1.*e[0]*e[35]*e[8]-1.*e[0]*e[31]*e[4]+e[3]*e[28]*e[4]+e[3]*e[1]*e[31]+e[3]*e[29]*e[5]+e[3]*e[2]*e[32]+e[30]*e[1]*e[4]+e[30]*e[2]*e[5]+e[6]*e[28]*e[7]+e[6]*e[1]*e[34]+e[6]*e[29]*e[8]+e[6]*e[2]*e[35]+e[33]*e[1]*e[7]+e[33]*e[2]*e[8]+e[0]*e[28]*e[1]+e[0]*e[29]*e[2]-1.*e[0]*e[34]*e[7]-1.*e[0]*e[32]*e[5]; 
    A[139]=e[8]*e[22]*e[25]+e[8]*e[21]*e[24]+e[20]*e[18]*e[3]+e[20]*e[0]*e[21]+e[20]*e[19]*e[4]+e[20]*e[1]*e[22]+e[20]*e[2]*e[23]+e[23]*e[21]*e[3]+e[23]*e[22]*e[4]+e[23]*e[26]*e[8]-1.*e[23]*e[19]*e[1]-1.*e[23]*e[18]*e[0]-1.*e[23]*e[25]*e[7]-1.*e[23]*e[24]*e[6]+e[2]*e[19]*e[22]+e[2]*e[18]*e[21]+e[26]*e[21]*e[6]+e[26]*e[3]*e[24]+e[26]*e[22]*e[7]+e[26]*e[4]*e[25]+.5000000000*ep2[20]*e[5]+1.500000000*ep2[23]*e[5]+.5000000000*e[5]*ep2[22]+.5000000000*e[5]*ep2[21]+.5000000000*e[5]*ep2[26]-.5000000000*e[5]*ep2[18]-.5000000000*e[5]*ep2[19]-.5000000000*e[5]*ep2[24]-.5000000000*e[5]*ep2[25]; 
    A[24]=e[24]*e[11]*e[8]+e[24]*e[2]*e[17]+3.*e[9]*e[18]*e[0]+e[9]*e[19]*e[1]+e[9]*e[20]*e[2]+e[18]*e[10]*e[1]+e[18]*e[11]*e[2]+e[3]*e[18]*e[12]+e[3]*e[9]*e[21]+e[3]*e[20]*e[14]+e[3]*e[11]*e[23]+e[3]*e[19]*e[13]+e[3]*e[10]*e[22]+e[6]*e[18]*e[15]+e[6]*e[9]*e[24]+e[6]*e[20]*e[17]+e[6]*e[11]*e[26]+e[6]*e[19]*e[16]+e[6]*e[10]*e[25]+e[0]*e[20]*e[11]+e[0]*e[19]*e[10]-1.*e[9]*e[26]*e[8]-1.*e[9]*e[22]*e[4]-1.*e[9]*e[25]*e[7]-1.*e[9]*e[23]*e[5]+e[12]*e[0]*e[21]+e[12]*e[19]*e[4]+e[12]*e[1]*e[22]+e[12]*e[20]*e[5]+e[12]*e[2]*e[23]-1.*e[18]*e[13]*e[4]-1.*e[18]*e[16]*e[7]-1.*e[18]*e[14]*e[5]-1.*e[18]*e[17]*e[8]+e[21]*e[10]*e[4]+e[21]*e[1]*e[13]+e[21]*e[11]*e[5]+e[21]*e[2]*e[14]+e[15]*e[0]*e[24]+e[15]*e[19]*e[7]+e[15]*e[1]*e[25]+e[15]*e[20]*e[8]+e[15]*e[2]*e[26]-1.*e[0]*e[23]*e[14]-1.*e[0]*e[25]*e[16]-1.*e[0]*e[26]*e[17]-1.*e[0]*e[22]*e[13]+e[24]*e[10]*e[7]+e[24]*e[1]*e[16]; 
    A[138]=e[11]*e[1]*e[4]+e[11]*e[0]*e[3]+e[11]*e[2]*e[5]+e[5]*e[12]*e[3]+e[5]*e[13]*e[4]+e[8]*e[12]*e[6]+e[8]*e[3]*e[15]+e[8]*e[13]*e[7]+e[8]*e[4]*e[16]+e[8]*e[5]*e[17]+e[17]*e[4]*e[7]+e[17]*e[3]*e[6]-1.*e[5]*e[10]*e[1]-1.*e[5]*e[16]*e[7]-1.*e[5]*e[15]*e[6]-1.*e[5]*e[9]*e[0]+e[2]*e[9]*e[3]+e[2]*e[0]*e[12]+e[2]*e[10]*e[4]+e[2]*e[1]*e[13]+.5000000000*ep2[2]*e[14]-.5000000000*e[14]*ep2[0]-.5000000000*e[14]*ep2[6]-.5000000000*e[14]*ep2[1]-.5000000000*e[14]*ep2[7]+1.500000000*e[14]*ep2[5]+.5000000000*e[14]*ep2[4]+.5000000000*e[14]*ep2[3]+.5000000000*e[14]*ep2[8]; 
    A[31]=e[3]*e[27]*e[12]+e[3]*e[9]*e[30]+e[3]*e[29]*e[14]+e[3]*e[11]*e[32]+e[3]*e[28]*e[13]+e[3]*e[10]*e[31]+e[6]*e[27]*e[15]+e[6]*e[9]*e[33]+e[6]*e[29]*e[17]+e[6]*e[11]*e[35]+e[6]*e[28]*e[16]+e[6]*e[10]*e[34]+3.*e[0]*e[27]*e[9]+e[0]*e[29]*e[11]+e[0]*e[28]*e[10]-1.*e[9]*e[34]*e[7]-1.*e[9]*e[32]*e[5]-1.*e[9]*e[35]*e[8]+e[9]*e[29]*e[2]+e[9]*e[28]*e[1]-1.*e[9]*e[31]*e[4]+e[12]*e[0]*e[30]+e[12]*e[28]*e[4]+e[12]*e[1]*e[31]+e[12]*e[29]*e[5]+e[12]*e[2]*e[32]+e[27]*e[11]*e[2]+e[27]*e[10]*e[1]-1.*e[27]*e[13]*e[4]-1.*e[27]*e[16]*e[7]-1.*e[27]*e[14]*e[5]-1.*e[27]*e[17]*e[8]+e[30]*e[10]*e[4]+e[30]*e[1]*e[13]+e[30]*e[11]*e[5]+e[30]*e[2]*e[14]+e[15]*e[0]*e[33]+e[15]*e[28]*e[7]+e[15]*e[1]*e[34]+e[15]*e[29]*e[8]+e[15]*e[2]*e[35]-1.*e[0]*e[31]*e[13]-1.*e[0]*e[32]*e[14]-1.*e[0]*e[34]*e[16]-1.*e[0]*e[35]*e[17]+e[33]*e[10]*e[7]+e[33]*e[1]*e[16]+e[33]*e[11]*e[8]+e[33]*e[2]*e[17]; 
    A[141]=.5000000000*ep2[30]*e[6]+.5000000000*e[6]*ep2[27]-.5000000000*e[6]*ep2[32]-.5000000000*e[6]*ep2[28]-.5000000000*e[6]*ep2[29]-.5000000000*e[6]*ep2[31]+1.500000000*e[6]*ep2[33]+.5000000000*e[6]*ep2[34]+.5000000000*e[6]*ep2[35]+e[0]*e[27]*e[33]+e[0]*e[29]*e[35]+e[0]*e[28]*e[34]+e[3]*e[30]*e[33]+e[3]*e[32]*e[35]+e[3]*e[31]*e[34]+e[30]*e[31]*e[7]+e[30]*e[4]*e[34]+e[30]*e[32]*e[8]+e[30]*e[5]*e[35]+e[27]*e[28]*e[7]+e[27]*e[1]*e[34]+e[27]*e[29]*e[8]+e[27]*e[2]*e[35]+e[33]*e[34]*e[7]+e[33]*e[35]*e[8]-1.*e[33]*e[32]*e[5]-1.*e[33]*e[29]*e[2]-1.*e[33]*e[28]*e[1]-1.*e[33]*e[31]*e[4]; 
    A[30]=e[24]*e[20]*e[26]+e[21]*e[19]*e[22]-.5000000000*e[18]*ep2[22]-.5000000000*e[18]*ep2[25]+.5000000000*ep3[18]+.5000000000*e[18]*ep2[21]+e[21]*e[20]*e[23]+.5000000000*e[18]*ep2[20]+.5000000000*e[18]*ep2[19]+.5000000000*e[18]*ep2[24]+e[24]*e[19]*e[25]-.5000000000*e[18]*ep2[23]-.5000000000*e[18]*ep2[26]; 
    A[140]=.5000000000*e[33]*ep2[35]+.5000000000*ep3[33]+.5000000000*ep2[27]*e[33]+.5000000000*ep2[30]*e[33]-.5000000000*e[33]*ep2[29]+.5000000000*e[33]*ep2[34]-.5000000000*e[33]*ep2[32]-.5000000000*e[33]*ep2[28]+e[30]*e[32]*e[35]-.5000000000*e[33]*ep2[31]+e[27]*e[29]*e[35]+e[27]*e[28]*e[34]+e[30]*e[31]*e[34]; 
    A[29]=1.500000000*e[27]*ep2[18]+.5000000000*e[27]*ep2[19]+.5000000000*e[27]*ep2[20]+.5000000000*e[27]*ep2[21]+.5000000000*e[27]*ep2[24]-.5000000000*e[27]*ep2[26]-.5000000000*e[27]*ep2[23]-.5000000000*e[27]*ep2[22]-.5000000000*e[27]*ep2[25]+e[33]*e[20]*e[26]-1.*e[18]*e[35]*e[26]-1.*e[18]*e[31]*e[22]-1.*e[18]*e[32]*e[23]-1.*e[18]*e[34]*e[25]+e[18]*e[28]*e[19]+e[18]*e[29]*e[20]+e[21]*e[18]*e[30]+e[21]*e[28]*e[22]+e[21]*e[19]*e[31]+e[21]*e[29]*e[23]+e[21]*e[20]*e[32]+e[30]*e[19]*e[22]+e[30]*e[20]*e[23]+e[24]*e[18]*e[33]+e[24]*e[28]*e[25]+e[24]*e[19]*e[34]+e[24]*e[29]*e[26]+e[24]*e[20]*e[35]+e[33]*e[19]*e[25]; 
    A[143]=e[9]*e[27]*e[33]+e[9]*e[29]*e[35]+e[9]*e[28]*e[34]+e[33]*e[35]*e[17]+e[33]*e[34]*e[16]+e[27]*e[29]*e[17]+e[27]*e[11]*e[35]+e[27]*e[28]*e[16]+e[27]*e[10]*e[34]+e[33]*e[30]*e[12]-1.*e[33]*e[28]*e[10]-1.*e[33]*e[31]*e[13]-1.*e[33]*e[32]*e[14]-1.*e[33]*e[29]*e[11]+e[30]*e[32]*e[17]+e[30]*e[14]*e[35]+e[30]*e[31]*e[16]+e[30]*e[13]*e[34]+e[12]*e[32]*e[35]+e[12]*e[31]*e[34]+.5000000000*e[15]*ep2[27]-.5000000000*e[15]*ep2[32]-.5000000000*e[15]*ep2[28]-.5000000000*e[15]*ep2[29]-.5000000000*e[15]*ep2[31]+1.500000000*e[15]*ep2[33]+.5000000000*e[15]*ep2[30]+.5000000000*e[15]*ep2[34]+.5000000000*e[15]*ep2[35]; 
    A[28]=.5000000000*e[9]*ep2[12]-.5000000000*e[9]*ep2[16]+.5000000000*e[9]*ep2[10]-.5000000000*e[9]*ep2[17]-.5000000000*e[9]*ep2[13]+e[15]*e[10]*e[16]+e[12]*e[11]*e[14]+.5000000000*e[9]*ep2[11]+.5000000000*e[9]*ep2[15]-.5000000000*e[9]*ep2[14]+e[15]*e[11]*e[17]+.5000000000*ep3[9]+e[12]*e[10]*e[13]; 
    A[142]=e[18]*e[27]*e[33]+e[18]*e[29]*e[35]+e[18]*e[28]*e[34]+e[27]*e[28]*e[25]+e[27]*e[19]*e[34]+e[27]*e[29]*e[26]+e[27]*e[20]*e[35]+e[21]*e[30]*e[33]+e[21]*e[32]*e[35]+e[21]*e[31]*e[34]+e[30]*e[31]*e[25]+e[30]*e[22]*e[34]+e[30]*e[32]*e[26]+e[30]*e[23]*e[35]+e[33]*e[34]*e[25]+e[33]*e[35]*e[26]-1.*e[33]*e[29]*e[20]-1.*e[33]*e[31]*e[22]-1.*e[33]*e[32]*e[23]-1.*e[33]*e[28]*e[19]+.5000000000*ep2[27]*e[24]+.5000000000*ep2[30]*e[24]+1.500000000*e[24]*ep2[33]+.5000000000*e[24]*ep2[35]+.5000000000*e[24]*ep2[34]-.5000000000*e[24]*ep2[32]-.5000000000*e[24]*ep2[28]-.5000000000*e[24]*ep2[29]-.5000000000*e[24]*ep2[31]; 
    A[36]=.5000000000*e[9]*ep2[21]+.5000000000*e[9]*ep2[24]+.5000000000*e[9]*ep2[19]+1.500000000*e[9]*ep2[18]+.5000000000*e[9]*ep2[20]-.5000000000*e[9]*ep2[26]-.5000000000*e[9]*ep2[23]-.5000000000*e[9]*ep2[22]-.5000000000*e[9]*ep2[25]+e[21]*e[18]*e[12]+e[21]*e[20]*e[14]+e[21]*e[11]*e[23]+e[21]*e[19]*e[13]+e[21]*e[10]*e[22]+e[24]*e[18]*e[15]+e[24]*e[20]*e[17]+e[24]*e[11]*e[26]+e[24]*e[19]*e[16]+e[24]*e[10]*e[25]+e[15]*e[19]*e[25]+e[15]*e[20]*e[26]+e[12]*e[19]*e[22]+e[12]*e[20]*e[23]+e[18]*e[20]*e[11]+e[18]*e[19]*e[10]-1.*e[18]*e[23]*e[14]-1.*e[18]*e[25]*e[16]-1.*e[18]*e[26]*e[17]-1.*e[18]*e[22]*e[13]; 
    A[182]=.5000000000*ep2[29]*e[26]+.5000000000*ep2[32]*e[26]+.5000000000*e[26]*ep2[33]+1.500000000*e[26]*ep2[35]+.5000000000*e[26]*ep2[34]-.5000000000*e[26]*ep2[27]-.5000000000*e[26]*ep2[28]-.5000000000*e[26]*ep2[31]-.5000000000*e[26]*ep2[30]+e[20]*e[27]*e[33]+e[20]*e[29]*e[35]+e[20]*e[28]*e[34]+e[29]*e[27]*e[24]+e[29]*e[18]*e[33]+e[29]*e[28]*e[25]+e[29]*e[19]*e[34]+e[23]*e[30]*e[33]+e[23]*e[32]*e[35]+e[23]*e[31]*e[34]+e[32]*e[30]*e[24]+e[32]*e[21]*e[33]+e[32]*e[31]*e[25]+e[32]*e[22]*e[34]+e[35]*e[33]*e[24]+e[35]*e[34]*e[25]-1.*e[35]*e[27]*e[18]-1.*e[35]*e[30]*e[21]-1.*e[35]*e[31]*e[22]-1.*e[35]*e[28]*e[19]; 
    A[37]=e[12]*e[19]*e[31]+e[12]*e[29]*e[23]+e[12]*e[20]*e[32]+3.*e[9]*e[27]*e[18]+e[9]*e[28]*e[19]+e[9]*e[29]*e[20]+e[21]*e[9]*e[30]+e[21]*e[29]*e[14]+e[21]*e[11]*e[32]+e[21]*e[28]*e[13]+e[21]*e[10]*e[31]+e[30]*e[20]*e[14]+e[30]*e[11]*e[23]+e[30]*e[19]*e[13]+e[30]*e[10]*e[22]+e[9]*e[33]*e[24]-1.*e[9]*e[35]*e[26]-1.*e[9]*e[31]*e[22]-1.*e[9]*e[32]*e[23]-1.*e[9]*e[34]*e[25]+e[18]*e[29]*e[11]+e[18]*e[28]*e[10]+e[27]*e[20]*e[11]+e[27]*e[19]*e[10]+e[15]*e[27]*e[24]+e[15]*e[18]*e[33]+e[15]*e[28]*e[25]+e[15]*e[19]*e[34]+e[15]*e[29]*e[26]+e[15]*e[20]*e[35]-1.*e[18]*e[31]*e[13]-1.*e[18]*e[32]*e[14]-1.*e[18]*e[34]*e[16]-1.*e[18]*e[35]*e[17]-1.*e[27]*e[23]*e[14]-1.*e[27]*e[25]*e[16]-1.*e[27]*e[26]*e[17]-1.*e[27]*e[22]*e[13]+e[24]*e[29]*e[17]+e[24]*e[11]*e[35]+e[24]*e[28]*e[16]+e[24]*e[10]*e[34]+e[33]*e[20]*e[17]+e[33]*e[11]*e[26]+e[33]*e[19]*e[16]+e[33]*e[10]*e[25]+e[12]*e[27]*e[21]+e[12]*e[18]*e[30]+e[12]*e[28]*e[22]; 
    A[183]=-.5000000000*e[17]*ep2[27]+.5000000000*e[17]*ep2[32]-.5000000000*e[17]*ep2[28]+.5000000000*e[17]*ep2[29]-.5000000000*e[17]*ep2[31]+.5000000000*e[17]*ep2[33]-.5000000000*e[17]*ep2[30]+.5000000000*e[17]*ep2[34]+1.500000000*e[17]*ep2[35]+e[32]*e[30]*e[15]+e[32]*e[12]*e[33]+e[32]*e[31]*e[16]+e[32]*e[13]*e[34]+e[14]*e[30]*e[33]+e[14]*e[31]*e[34]+e[11]*e[27]*e[33]+e[11]*e[29]*e[35]+e[11]*e[28]*e[34]+e[35]*e[33]*e[15]+e[35]*e[34]*e[16]+e[29]*e[27]*e[15]+e[29]*e[9]*e[33]+e[29]*e[28]*e[16]+e[29]*e[10]*e[34]-1.*e[35]*e[27]*e[9]-1.*e[35]*e[30]*e[12]-1.*e[35]*e[28]*e[10]-1.*e[35]*e[31]*e[13]+e[35]*e[32]*e[14]; 
    A[38]=.5000000000*e[9]*ep2[1]+1.500000000*e[9]*ep2[0]+.5000000000*e[9]*ep2[2]+.5000000000*e[9]*ep2[3]+.5000000000*e[9]*ep2[6]-.5000000000*e[9]*ep2[4]-.5000000000*e[9]*ep2[5]-.5000000000*e[9]*ep2[7]-.5000000000*e[9]*ep2[8]+e[6]*e[0]*e[15]+e[6]*e[10]*e[7]+e[6]*e[1]*e[16]+e[6]*e[11]*e[8]+e[6]*e[2]*e[17]+e[15]*e[1]*e[7]+e[15]*e[2]*e[8]+e[0]*e[11]*e[2]+e[0]*e[10]*e[1]-1.*e[0]*e[13]*e[4]-1.*e[0]*e[16]*e[7]-1.*e[0]*e[14]*e[5]-1.*e[0]*e[17]*e[8]+e[3]*e[0]*e[12]+e[3]*e[10]*e[4]+e[3]*e[1]*e[13]+e[3]*e[11]*e[5]+e[3]*e[2]*e[14]+e[12]*e[1]*e[4]+e[12]*e[2]*e[5]; 
    A[180]=.5000000000*e[35]*ep2[33]+.5000000000*e[35]*ep2[34]-.5000000000*e[35]*ep2[27]-.5000000000*e[35]*ep2[28]-.5000000000*e[35]*ep2[31]-.5000000000*e[35]*ep2[30]+e[32]*e[31]*e[34]+.5000000000*ep2[29]*e[35]+.5000000000*ep2[32]*e[35]+e[29]*e[28]*e[34]+e[32]*e[30]*e[33]+.5000000000*ep3[35]+e[29]*e[27]*e[33]; 
    A[39]=.5000000000*e[0]*ep2[19]+.5000000000*e[0]*ep2[20]+.5000000000*e[0]*ep2[24]-.5000000000*e[0]*ep2[26]-.5000000000*e[0]*ep2[23]-.5000000000*e[0]*ep2[22]-.5000000000*e[0]*ep2[25]+1.500000000*ep2[18]*e[0]+.5000000000*e[0]*ep2[21]+e[18]*e[19]*e[1]+e[18]*e[20]*e[2]+e[21]*e[18]*e[3]+e[21]*e[19]*e[4]+e[21]*e[1]*e[22]+e[21]*e[20]*e[5]+e[21]*e[2]*e[23]-1.*e[18]*e[26]*e[8]-1.*e[18]*e[22]*e[4]-1.*e[18]*e[25]*e[7]-1.*e[18]*e[23]*e[5]+e[18]*e[24]*e[6]+e[3]*e[19]*e[22]+e[3]*e[20]*e[23]+e[24]*e[19]*e[7]+e[24]*e[1]*e[25]+e[24]*e[20]*e[8]+e[24]*e[2]*e[26]+e[6]*e[19]*e[25]+e[6]*e[20]*e[26]; 
    A[181]=.5000000000*ep2[32]*e[8]-.5000000000*e[8]*ep2[27]-.5000000000*e[8]*ep2[28]+.5000000000*e[8]*ep2[29]-.5000000000*e[8]*ep2[31]+.5000000000*e[8]*ep2[33]-.5000000000*e[8]*ep2[30]+.5000000000*e[8]*ep2[34]+1.500000000*e[8]*ep2[35]+e[2]*e[27]*e[33]+e[2]*e[29]*e[35]+e[2]*e[28]*e[34]+e[5]*e[30]*e[33]+e[5]*e[32]*e[35]+e[5]*e[31]*e[34]+e[32]*e[30]*e[6]+e[32]*e[3]*e[33]+e[32]*e[31]*e[7]+e[32]*e[4]*e[34]+e[29]*e[27]*e[6]+e[29]*e[0]*e[33]+e[29]*e[28]*e[7]+e[29]*e[1]*e[34]+e[35]*e[33]*e[6]+e[35]*e[34]*e[7]-1.*e[35]*e[27]*e[0]-1.*e[35]*e[30]*e[3]-1.*e[35]*e[28]*e[1]-1.*e[35]*e[31]*e[4]; 
    A[32]=-.5000000000*e[18]*ep2[4]+1.500000000*e[18]*ep2[0]+.5000000000*e[18]*ep2[6]-.5000000000*e[18]*ep2[5]+.5000000000*e[18]*ep2[1]-.5000000000*e[18]*ep2[7]+.5000000000*e[18]*ep2[3]+.5000000000*e[18]*ep2[2]-.5000000000*e[18]*ep2[8]+e[3]*e[0]*e[21]+e[3]*e[19]*e[4]+e[3]*e[1]*e[22]+e[3]*e[20]*e[5]+e[3]*e[2]*e[23]+e[21]*e[1]*e[4]+e[21]*e[2]*e[5]+e[6]*e[0]*e[24]+e[6]*e[19]*e[7]+e[6]*e[1]*e[25]+e[6]*e[20]*e[8]+e[6]*e[2]*e[26]+e[24]*e[1]*e[7]+e[24]*e[2]*e[8]+e[0]*e[19]*e[1]+e[0]*e[20]*e[2]-1.*e[0]*e[26]*e[8]-1.*e[0]*e[22]*e[4]-1.*e[0]*e[25]*e[7]-1.*e[0]*e[23]*e[5]; 
    A[178]=e[10]*e[1]*e[7]+e[10]*e[0]*e[6]+e[10]*e[2]*e[8]+e[4]*e[12]*e[6]+e[4]*e[3]*e[15]+e[4]*e[13]*e[7]+e[4]*e[14]*e[8]+e[4]*e[5]*e[17]+e[13]*e[3]*e[6]+e[13]*e[5]*e[8]+e[7]*e[15]*e[6]+e[7]*e[17]*e[8]-1.*e[7]*e[11]*e[2]-1.*e[7]*e[9]*e[0]-1.*e[7]*e[14]*e[5]-1.*e[7]*e[12]*e[3]+e[1]*e[9]*e[6]+e[1]*e[0]*e[15]+e[1]*e[11]*e[8]+e[1]*e[2]*e[17]+1.500000000*e[16]*ep2[7]+.5000000000*e[16]*ep2[6]+.5000000000*e[16]*ep2[8]+.5000000000*ep2[1]*e[16]-.5000000000*e[16]*ep2[0]-.5000000000*e[16]*ep2[5]-.5000000000*e[16]*ep2[3]-.5000000000*e[16]*ep2[2]+.5000000000*ep2[4]*e[16]; 
    A[33]=e[0]*e[30]*e[21]-1.*e[0]*e[35]*e[26]-1.*e[0]*e[31]*e[22]-1.*e[0]*e[32]*e[23]-1.*e[0]*e[34]*e[25]-1.*e[18]*e[34]*e[7]-1.*e[18]*e[32]*e[5]-1.*e[18]*e[35]*e[8]-1.*e[18]*e[31]*e[4]-1.*e[27]*e[26]*e[8]-1.*e[27]*e[22]*e[4]-1.*e[27]*e[25]*e[7]-1.*e[27]*e[23]*e[5]+e[6]*e[28]*e[25]+e[6]*e[19]*e[34]+e[6]*e[29]*e[26]+e[6]*e[20]*e[35]+e[21]*e[28]*e[4]+e[21]*e[1]*e[31]+e[21]*e[29]*e[5]+e[21]*e[2]*e[32]+e[30]*e[19]*e[4]+e[30]*e[1]*e[22]+e[30]*e[20]*e[5]+e[30]*e[2]*e[23]+e[24]*e[27]*e[6]+e[24]*e[0]*e[33]+e[24]*e[28]*e[7]+e[24]*e[1]*e[34]+e[24]*e[29]*e[8]+e[24]*e[2]*e[35]+e[33]*e[18]*e[6]+e[33]*e[19]*e[7]+e[33]*e[1]*e[25]+e[33]*e[20]*e[8]+e[33]*e[2]*e[26]+3.*e[0]*e[27]*e[18]+e[0]*e[28]*e[19]+e[0]*e[29]*e[20]+e[18]*e[28]*e[1]+e[18]*e[29]*e[2]+e[27]*e[19]*e[1]+e[27]*e[20]*e[2]+e[3]*e[27]*e[21]+e[3]*e[18]*e[30]+e[3]*e[28]*e[22]+e[3]*e[19]*e[31]+e[3]*e[29]*e[23]+e[3]*e[20]*e[32]; 
    A[179]=e[19]*e[18]*e[6]+e[19]*e[0]*e[24]+e[19]*e[1]*e[25]+e[19]*e[20]*e[8]+e[19]*e[2]*e[26]+e[22]*e[21]*e[6]+e[22]*e[3]*e[24]+e[22]*e[4]*e[25]+e[22]*e[23]*e[8]+e[22]*e[5]*e[26]-1.*e[25]*e[21]*e[3]+e[25]*e[26]*e[8]-1.*e[25]*e[20]*e[2]-1.*e[25]*e[18]*e[0]-1.*e[25]*e[23]*e[5]+e[25]*e[24]*e[6]+e[1]*e[18]*e[24]+e[1]*e[20]*e[26]+e[4]*e[21]*e[24]+e[4]*e[23]*e[26]+.5000000000*ep2[19]*e[7]+.5000000000*ep2[22]*e[7]+1.500000000*ep2[25]*e[7]+.5000000000*e[7]*ep2[26]-.5000000000*e[7]*ep2[18]-.5000000000*e[7]*ep2[23]-.5000000000*e[7]*ep2[20]+.5000000000*e[7]*ep2[24]-.5000000000*e[7]*ep2[21]; 
    A[34]=.5000000000*e[18]*ep2[11]+1.500000000*e[18]*ep2[9]+.5000000000*e[18]*ep2[10]+.5000000000*e[18]*ep2[12]+.5000000000*e[18]*ep2[15]-.5000000000*e[18]*ep2[16]-.5000000000*e[18]*ep2[17]-.5000000000*e[18]*ep2[14]-.5000000000*e[18]*ep2[13]+e[12]*e[9]*e[21]+e[12]*e[20]*e[14]+e[12]*e[11]*e[23]+e[12]*e[19]*e[13]+e[12]*e[10]*e[22]+e[21]*e[11]*e[14]+e[21]*e[10]*e[13]+e[15]*e[9]*e[24]+e[15]*e[20]*e[17]+e[15]*e[11]*e[26]+e[15]*e[19]*e[16]+e[15]*e[10]*e[25]+e[24]*e[11]*e[17]+e[24]*e[10]*e[16]-1.*e[9]*e[23]*e[14]-1.*e[9]*e[25]*e[16]-1.*e[9]*e[26]*e[17]+e[9]*e[20]*e[11]+e[9]*e[19]*e[10]-1.*e[9]*e[22]*e[13]; 
    A[176]=e[13]*e[21]*e[24]+e[13]*e[23]*e[26]+e[19]*e[18]*e[15]+e[19]*e[9]*e[24]+e[19]*e[20]*e[17]+e[19]*e[11]*e[26]-1.*e[25]*e[23]*e[14]-1.*e[25]*e[20]*e[11]-1.*e[25]*e[18]*e[9]-1.*e[25]*e[21]*e[12]+e[22]*e[21]*e[15]+e[22]*e[12]*e[24]+e[22]*e[23]*e[17]+e[22]*e[14]*e[26]+e[22]*e[13]*e[25]+e[25]*e[24]*e[15]+e[25]*e[26]*e[17]+e[10]*e[19]*e[25]+e[10]*e[18]*e[24]+e[10]*e[20]*e[26]-.5000000000*e[16]*ep2[18]-.5000000000*e[16]*ep2[23]+.5000000000*e[16]*ep2[19]-.5000000000*e[16]*ep2[20]-.5000000000*e[16]*ep2[21]+.5000000000*ep2[22]*e[16]+1.500000000*ep2[25]*e[16]+.5000000000*e[16]*ep2[24]+.5000000000*e[16]*ep2[26]; 
    A[35]=.5000000000*e[0]*ep2[12]+.5000000000*e[0]*ep2[15]+.5000000000*e[0]*ep2[11]+1.500000000*e[0]*ep2[9]+.5000000000*e[0]*ep2[10]-.5000000000*e[0]*ep2[16]-.5000000000*e[0]*ep2[17]-.5000000000*e[0]*ep2[14]-.5000000000*e[0]*ep2[13]+e[12]*e[9]*e[3]+e[12]*e[10]*e[4]+e[12]*e[1]*e[13]+e[12]*e[11]*e[5]+e[12]*e[2]*e[14]+e[15]*e[9]*e[6]+e[15]*e[10]*e[7]+e[15]*e[1]*e[16]+e[15]*e[11]*e[8]+e[15]*e[2]*e[17]+e[6]*e[11]*e[17]+e[6]*e[10]*e[16]+e[3]*e[11]*e[14]+e[3]*e[10]*e[13]+e[9]*e[10]*e[1]+e[9]*e[11]*e[2]-1.*e[9]*e[13]*e[4]-1.*e[9]*e[16]*e[7]-1.*e[9]*e[14]*e[5]-1.*e[9]*e[17]*e[8]; 
    A[177]=e[19]*e[11]*e[35]+e[28]*e[18]*e[15]+e[28]*e[9]*e[24]+e[28]*e[20]*e[17]+e[28]*e[11]*e[26]-1.*e[25]*e[27]*e[9]-1.*e[25]*e[30]*e[12]-1.*e[25]*e[32]*e[14]+e[25]*e[33]*e[15]+e[25]*e[35]*e[17]-1.*e[25]*e[29]*e[11]-1.*e[34]*e[23]*e[14]+e[34]*e[24]*e[15]+e[34]*e[26]*e[17]-1.*e[34]*e[20]*e[11]-1.*e[34]*e[18]*e[9]-1.*e[34]*e[21]*e[12]+e[13]*e[30]*e[24]+e[13]*e[21]*e[33]+e[13]*e[31]*e[25]+e[13]*e[22]*e[34]+e[13]*e[32]*e[26]+e[13]*e[23]*e[35]+e[10]*e[27]*e[24]+e[10]*e[18]*e[33]+e[10]*e[28]*e[25]+e[10]*e[19]*e[34]+e[10]*e[29]*e[26]+e[10]*e[20]*e[35]+e[22]*e[30]*e[15]+e[22]*e[12]*e[33]+e[22]*e[32]*e[17]+e[22]*e[14]*e[35]+e[22]*e[31]*e[16]+e[31]*e[21]*e[15]+e[31]*e[12]*e[24]+e[31]*e[23]*e[17]+e[31]*e[14]*e[26]-1.*e[16]*e[27]*e[18]+e[16]*e[33]*e[24]-1.*e[16]*e[30]*e[21]-1.*e[16]*e[29]*e[20]+e[16]*e[35]*e[26]-1.*e[16]*e[32]*e[23]+e[16]*e[28]*e[19]+3.*e[16]*e[34]*e[25]+e[19]*e[27]*e[15]+e[19]*e[9]*e[33]+e[19]*e[29]*e[17]; 
    A[45]=e[4]*e[27]*e[3]+e[4]*e[0]*e[30]+e[4]*e[29]*e[5]+e[4]*e[2]*e[32]+e[31]*e[0]*e[3]+e[31]*e[2]*e[5]+e[7]*e[27]*e[6]+e[7]*e[0]*e[33]+e[7]*e[29]*e[8]+e[7]*e[2]*e[35]+e[34]*e[0]*e[6]+e[34]*e[2]*e[8]+e[1]*e[27]*e[0]+e[1]*e[29]*e[2]+e[1]*e[34]*e[7]-1.*e[1]*e[32]*e[5]-1.*e[1]*e[33]*e[6]-1.*e[1]*e[30]*e[3]-1.*e[1]*e[35]*e[8]+e[1]*e[31]*e[4]+1.500000000*e[28]*ep2[1]+.5000000000*e[28]*ep2[4]+.5000000000*e[28]*ep2[0]-.5000000000*e[28]*ep2[6]-.5000000000*e[28]*ep2[5]+.5000000000*e[28]*ep2[7]-.5000000000*e[28]*ep2[3]+.5000000000*e[28]*ep2[2]-.5000000000*e[28]*ep2[8]; 
    A[191]=-1.*e[35]*e[10]*e[1]-1.*e[35]*e[13]*e[4]+e[35]*e[16]*e[7]+e[35]*e[15]*e[6]-1.*e[35]*e[9]*e[0]-1.*e[35]*e[12]*e[3]+e[32]*e[12]*e[6]+e[32]*e[3]*e[15]+e[32]*e[13]*e[7]+e[32]*e[4]*e[16]-1.*e[8]*e[27]*e[9]-1.*e[8]*e[30]*e[12]-1.*e[8]*e[28]*e[10]-1.*e[8]*e[31]*e[13]+e[8]*e[29]*e[11]+e[11]*e[27]*e[6]+e[11]*e[0]*e[33]+e[11]*e[28]*e[7]+e[11]*e[1]*e[34]+e[29]*e[9]*e[6]+e[29]*e[0]*e[15]+e[29]*e[10]*e[7]+e[29]*e[1]*e[16]+e[5]*e[30]*e[15]+e[5]*e[12]*e[33]+e[5]*e[32]*e[17]+e[5]*e[14]*e[35]+e[5]*e[31]*e[16]+e[5]*e[13]*e[34]+e[8]*e[33]*e[15]+3.*e[8]*e[35]*e[17]+e[8]*e[34]*e[16]+e[2]*e[27]*e[15]+e[2]*e[9]*e[33]+e[2]*e[29]*e[17]+e[2]*e[11]*e[35]+e[2]*e[28]*e[16]+e[2]*e[10]*e[34]-1.*e[17]*e[27]*e[0]+e[17]*e[34]*e[7]+e[17]*e[33]*e[6]-1.*e[17]*e[30]*e[3]-1.*e[17]*e[28]*e[1]-1.*e[17]*e[31]*e[4]+e[14]*e[30]*e[6]+e[14]*e[3]*e[33]+e[14]*e[31]*e[7]+e[14]*e[4]*e[34]+e[14]*e[32]*e[8]; 
    A[44]=e[19]*e[11]*e[2]+e[4]*e[18]*e[12]+e[4]*e[9]*e[21]+e[4]*e[20]*e[14]+e[4]*e[11]*e[23]+e[4]*e[19]*e[13]+e[4]*e[10]*e[22]+e[7]*e[18]*e[15]+e[7]*e[9]*e[24]+e[7]*e[20]*e[17]+e[7]*e[11]*e[26]+e[7]*e[19]*e[16]+e[7]*e[10]*e[25]+e[1]*e[18]*e[9]+e[1]*e[20]*e[11]-1.*e[10]*e[21]*e[3]-1.*e[10]*e[26]*e[8]-1.*e[10]*e[23]*e[5]-1.*e[10]*e[24]*e[6]+e[13]*e[18]*e[3]+e[13]*e[0]*e[21]+e[13]*e[1]*e[22]+e[13]*e[20]*e[5]+e[13]*e[2]*e[23]-1.*e[19]*e[15]*e[6]-1.*e[19]*e[14]*e[5]-1.*e[19]*e[12]*e[3]-1.*e[19]*e[17]*e[8]+e[22]*e[9]*e[3]+e[22]*e[0]*e[12]+e[22]*e[11]*e[5]+e[22]*e[2]*e[14]+e[16]*e[18]*e[6]+e[16]*e[0]*e[24]+e[16]*e[1]*e[25]+e[16]*e[20]*e[8]+e[16]*e[2]*e[26]-1.*e[1]*e[23]*e[14]-1.*e[1]*e[24]*e[15]-1.*e[1]*e[26]*e[17]-1.*e[1]*e[21]*e[12]+e[25]*e[9]*e[6]+e[25]*e[0]*e[15]+e[25]*e[11]*e[8]+e[25]*e[2]*e[17]+e[10]*e[18]*e[0]+3.*e[10]*e[19]*e[1]+e[10]*e[20]*e[2]+e[19]*e[9]*e[0]; 
    A[190]=.5000000000*ep2[23]*e[26]+.5000000000*e[26]*ep2[25]+.5000000000*ep2[20]*e[26]-.5000000000*e[26]*ep2[18]+.5000000000*ep3[26]+.5000000000*e[26]*ep2[24]+e[20]*e[19]*e[25]-.5000000000*e[26]*ep2[19]-.5000000000*e[26]*ep2[21]+e[20]*e[18]*e[24]-.5000000000*e[26]*ep2[22]+e[23]*e[21]*e[24]+e[23]*e[22]*e[25]; 
    A[47]=e[16]*e[9]*e[33]+e[16]*e[29]*e[17]+e[16]*e[11]*e[35]+e[16]*e[10]*e[34]+e[34]*e[11]*e[17]+e[34]*e[9]*e[15]-1.*e[10]*e[30]*e[12]-1.*e[10]*e[32]*e[14]-1.*e[10]*e[33]*e[15]-1.*e[10]*e[35]*e[17]+e[10]*e[27]*e[9]+e[10]*e[29]*e[11]+e[13]*e[27]*e[12]+e[13]*e[9]*e[30]+e[13]*e[29]*e[14]+e[13]*e[11]*e[32]+e[13]*e[10]*e[31]+e[31]*e[11]*e[14]+e[31]*e[9]*e[12]+e[16]*e[27]*e[15]+1.500000000*e[28]*ep2[10]+.5000000000*e[28]*ep2[16]+.5000000000*e[28]*ep2[9]+.5000000000*e[28]*ep2[11]-.5000000000*e[28]*ep2[12]-.5000000000*e[28]*ep2[15]-.5000000000*e[28]*ep2[17]-.5000000000*e[28]*ep2[14]+.5000000000*e[28]*ep2[13]; 
    A[189]=.5000000000*ep2[20]*e[35]+.5000000000*ep2[23]*e[35]+1.500000000*e[35]*ep2[26]+.5000000000*e[35]*ep2[25]+.5000000000*e[35]*ep2[24]-.5000000000*e[35]*ep2[18]-.5000000000*e[35]*ep2[19]-.5000000000*e[35]*ep2[22]-.5000000000*e[35]*ep2[21]+e[20]*e[27]*e[24]+e[20]*e[18]*e[33]+e[20]*e[28]*e[25]+e[20]*e[19]*e[34]+e[20]*e[29]*e[26]+e[29]*e[19]*e[25]+e[29]*e[18]*e[24]+e[23]*e[30]*e[24]+e[23]*e[21]*e[33]+e[23]*e[31]*e[25]+e[23]*e[22]*e[34]+e[23]*e[32]*e[26]+e[32]*e[22]*e[25]+e[32]*e[21]*e[24]+e[26]*e[33]*e[24]+e[26]*e[34]*e[25]-1.*e[26]*e[27]*e[18]-1.*e[26]*e[30]*e[21]-1.*e[26]*e[31]*e[22]-1.*e[26]*e[28]*e[19]; 
    A[46]=e[4]*e[2]*e[5]+.5000000000*e[1]*ep2[0]-.5000000000*e[1]*ep2[6]+e[7]*e[0]*e[6]+.5000000000*e[1]*ep2[7]+.5000000000*e[1]*ep2[4]-.5000000000*e[1]*ep2[8]+.5000000000*e[1]*ep2[2]-.5000000000*e[1]*ep2[3]+.5000000000*ep3[1]+e[7]*e[2]*e[8]-.5000000000*e[1]*ep2[5]+e[4]*e[0]*e[3]; 
    A[188]=-.5000000000*e[17]*ep2[13]-.5000000000*e[17]*ep2[9]+.5000000000*e[17]*ep2[16]+.5000000000*e[17]*ep2[15]+.5000000000*ep3[17]-.5000000000*e[17]*ep2[10]+e[14]*e[13]*e[16]+e[14]*e[12]*e[15]+.5000000000*ep2[14]*e[17]+e[11]*e[10]*e[16]-.5000000000*e[17]*ep2[12]+.5000000000*ep2[11]*e[17]+e[11]*e[9]*e[15]; 
    A[41]=e[4]*e[27]*e[30]+e[4]*e[29]*e[32]+e[4]*e[28]*e[31]+e[31]*e[27]*e[3]+e[31]*e[0]*e[30]+e[31]*e[29]*e[5]+e[31]*e[2]*e[32]+e[7]*e[27]*e[33]+e[7]*e[29]*e[35]+e[7]*e[28]*e[34]+e[28]*e[27]*e[0]+e[28]*e[29]*e[2]+e[34]*e[27]*e[6]+e[34]*e[0]*e[33]+e[34]*e[29]*e[8]+e[34]*e[2]*e[35]-1.*e[28]*e[32]*e[5]-1.*e[28]*e[33]*e[6]-1.*e[28]*e[30]*e[3]-1.*e[28]*e[35]*e[8]+.5000000000*e[1]*ep2[27]+.5000000000*e[1]*ep2[29]+1.500000000*e[1]*ep2[28]+.5000000000*e[1]*ep2[31]-.5000000000*e[1]*ep2[32]-.5000000000*e[1]*ep2[33]-.5000000000*e[1]*ep2[30]+.5000000000*e[1]*ep2[34]-.5000000000*e[1]*ep2[35]; 
    A[187]=.5000000000*ep2[11]*e[35]+.5000000000*e[35]*ep2[16]-.5000000000*e[35]*ep2[9]-.5000000000*e[35]*ep2[12]+.5000000000*e[35]*ep2[15]+1.500000000*e[35]*ep2[17]-.5000000000*e[35]*ep2[10]+.5000000000*e[35]*ep2[14]-.5000000000*e[35]*ep2[13]+e[11]*e[27]*e[15]+e[11]*e[9]*e[33]+e[11]*e[29]*e[17]+e[11]*e[28]*e[16]+e[11]*e[10]*e[34]+e[29]*e[9]*e[15]+e[29]*e[10]*e[16]+e[14]*e[30]*e[15]+e[14]*e[12]*e[33]+e[14]*e[32]*e[17]+e[14]*e[31]*e[16]+e[14]*e[13]*e[34]+e[32]*e[12]*e[15]+e[32]*e[13]*e[16]+e[17]*e[33]*e[15]+e[17]*e[34]*e[16]-1.*e[17]*e[27]*e[9]-1.*e[17]*e[30]*e[12]-1.*e[17]*e[28]*e[10]-1.*e[17]*e[31]*e[13]; 
    A[40]=e[34]*e[27]*e[33]+e[34]*e[29]*e[35]-.5000000000*e[28]*ep2[30]-.5000000000*e[28]*ep2[35]+.5000000000*ep3[28]+.5000000000*e[28]*ep2[27]+.5000000000*e[28]*ep2[29]+e[31]*e[27]*e[30]+e[31]*e[29]*e[32]-.5000000000*e[28]*ep2[32]-.5000000000*e[28]*ep2[33]+.5000000000*e[28]*ep2[31]+.5000000000*e[28]*ep2[34]; 
    A[186]=.5000000000*ep2[5]*e[8]+e[2]*e[0]*e[6]+.5000000000*ep2[2]*e[8]+.5000000000*ep3[8]-.5000000000*e[8]*ep2[0]+e[5]*e[4]*e[7]+e[5]*e[3]*e[6]+.5000000000*e[8]*ep2[7]+e[2]*e[1]*e[7]-.5000000000*e[8]*ep2[1]-.5000000000*e[8]*ep2[4]-.5000000000*e[8]*ep2[3]+.5000000000*e[8]*ep2[6]; 
    A[43]=e[28]*e[27]*e[9]+e[28]*e[29]*e[11]-1.*e[28]*e[30]*e[12]+e[28]*e[31]*e[13]-1.*e[28]*e[32]*e[14]-1.*e[28]*e[33]*e[15]-1.*e[28]*e[35]*e[17]+e[31]*e[27]*e[12]+e[31]*e[9]*e[30]+e[31]*e[29]*e[14]+e[31]*e[11]*e[32]+e[13]*e[27]*e[30]+e[13]*e[29]*e[32]+e[16]*e[27]*e[33]+e[16]*e[29]*e[35]+e[34]*e[27]*e[15]+e[34]*e[9]*e[33]+e[34]*e[29]*e[17]+e[34]*e[11]*e[35]+e[34]*e[28]*e[16]+.5000000000*e[10]*ep2[27]+.5000000000*e[10]*ep2[29]+1.500000000*e[10]*ep2[28]-.5000000000*e[10]*ep2[32]+.5000000000*e[10]*ep2[31]-.5000000000*e[10]*ep2[33]-.5000000000*e[10]*ep2[30]+.5000000000*e[10]*ep2[34]-.5000000000*e[10]*ep2[35]; 
    A[185]=-.5000000000*e[35]*ep2[1]+.5000000000*e[35]*ep2[7]-.5000000000*e[35]*ep2[3]+.5000000000*ep2[2]*e[35]+1.500000000*e[35]*ep2[8]-.5000000000*e[35]*ep2[4]-.5000000000*e[35]*ep2[0]+.5000000000*e[35]*ep2[6]+.5000000000*e[35]*ep2[5]+e[2]*e[27]*e[6]+e[2]*e[0]*e[33]+e[2]*e[28]*e[7]+e[2]*e[1]*e[34]+e[2]*e[29]*e[8]-1.*e[8]*e[27]*e[0]+e[8]*e[34]*e[7]+e[8]*e[32]*e[5]+e[8]*e[33]*e[6]-1.*e[8]*e[30]*e[3]-1.*e[8]*e[28]*e[1]-1.*e[8]*e[31]*e[4]+e[29]*e[1]*e[7]+e[29]*e[0]*e[6]+e[5]*e[30]*e[6]+e[5]*e[3]*e[33]+e[5]*e[31]*e[7]+e[5]*e[4]*e[34]+e[32]*e[4]*e[7]+e[32]*e[3]*e[6]; 
    A[42]=e[28]*e[27]*e[18]+e[28]*e[29]*e[20]+e[22]*e[27]*e[30]+e[22]*e[29]*e[32]+e[22]*e[28]*e[31]+e[31]*e[27]*e[21]+e[31]*e[18]*e[30]+e[31]*e[29]*e[23]+e[31]*e[20]*e[32]+e[25]*e[27]*e[33]+e[25]*e[29]*e[35]+e[25]*e[28]*e[34]+e[34]*e[27]*e[24]+e[34]*e[18]*e[33]+e[34]*e[29]*e[26]+e[34]*e[20]*e[35]-1.*e[28]*e[33]*e[24]-1.*e[28]*e[30]*e[21]-1.*e[28]*e[35]*e[26]-1.*e[28]*e[32]*e[23]-.5000000000*e[19]*ep2[33]-.5000000000*e[19]*ep2[30]-.5000000000*e[19]*ep2[35]+.5000000000*e[19]*ep2[27]+.5000000000*e[19]*ep2[29]+1.500000000*e[19]*ep2[28]+.5000000000*e[19]*ep2[31]+.5000000000*e[19]*ep2[34]-.5000000000*e[19]*ep2[32]; 
    A[184]=e[23]*e[3]*e[15]-1.*e[17]*e[19]*e[1]-1.*e[17]*e[22]*e[4]-1.*e[17]*e[18]*e[0]+e[17]*e[25]*e[7]+e[17]*e[24]*e[6]+e[14]*e[21]*e[6]+e[14]*e[3]*e[24]+e[14]*e[22]*e[7]+e[14]*e[4]*e[25]+e[14]*e[23]*e[8]-1.*e[26]*e[10]*e[1]-1.*e[26]*e[13]*e[4]+e[26]*e[16]*e[7]+e[26]*e[15]*e[6]-1.*e[26]*e[9]*e[0]-1.*e[26]*e[12]*e[3]+e[23]*e[12]*e[6]+e[11]*e[18]*e[6]+e[11]*e[0]*e[24]+e[11]*e[19]*e[7]+e[11]*e[1]*e[25]+e[11]*e[20]*e[8]+e[11]*e[2]*e[26]+e[20]*e[9]*e[6]+e[20]*e[0]*e[15]+e[20]*e[10]*e[7]+e[20]*e[1]*e[16]+e[20]*e[2]*e[17]+e[5]*e[21]*e[15]+e[5]*e[12]*e[24]+e[5]*e[23]*e[17]+e[5]*e[14]*e[26]+e[5]*e[22]*e[16]+e[5]*e[13]*e[25]+e[8]*e[24]*e[15]+3.*e[8]*e[26]*e[17]+e[8]*e[25]*e[16]+e[2]*e[18]*e[15]+e[2]*e[9]*e[24]+e[2]*e[19]*e[16]+e[2]*e[10]*e[25]-1.*e[17]*e[21]*e[3]+e[23]*e[4]*e[16]+e[23]*e[13]*e[7]-1.*e[8]*e[18]*e[9]-1.*e[8]*e[21]*e[12]-1.*e[8]*e[19]*e[10]-1.*e[8]*e[22]*e[13]; 
    A[54]=e[13]*e[18]*e[12]+e[13]*e[9]*e[21]+e[13]*e[20]*e[14]+e[13]*e[11]*e[23]+e[13]*e[10]*e[22]+e[22]*e[11]*e[14]+e[22]*e[9]*e[12]+e[16]*e[18]*e[15]+e[16]*e[9]*e[24]+e[16]*e[20]*e[17]+e[16]*e[11]*e[26]+e[16]*e[10]*e[25]+e[25]*e[11]*e[17]+e[25]*e[9]*e[15]-1.*e[10]*e[23]*e[14]-1.*e[10]*e[24]*e[15]-1.*e[10]*e[26]*e[17]+e[10]*e[20]*e[11]+e[10]*e[18]*e[9]-1.*e[10]*e[21]*e[12]+.5000000000*e[19]*ep2[11]+.5000000000*e[19]*ep2[9]+1.500000000*e[19]*ep2[10]+.5000000000*e[19]*ep2[13]+.5000000000*e[19]*ep2[16]-.5000000000*e[19]*ep2[12]-.5000000000*e[19]*ep2[15]-.5000000000*e[19]*ep2[17]-.5000000000*e[19]*ep2[14]; 
    A[164]=e[10]*e[18]*e[6]+e[10]*e[0]*e[24]+e[10]*e[19]*e[7]+e[10]*e[1]*e[25]+e[10]*e[20]*e[8]+e[10]*e[2]*e[26]+e[19]*e[9]*e[6]+e[19]*e[0]*e[15]+e[19]*e[1]*e[16]+e[19]*e[11]*e[8]+e[19]*e[2]*e[17]+e[4]*e[21]*e[15]+e[4]*e[12]*e[24]+e[4]*e[23]*e[17]+e[4]*e[14]*e[26]+e[4]*e[22]*e[16]+e[4]*e[13]*e[25]+e[7]*e[24]*e[15]+e[7]*e[26]*e[17]+3.*e[7]*e[25]*e[16]+e[1]*e[18]*e[15]+e[1]*e[9]*e[24]+e[1]*e[20]*e[17]+e[1]*e[11]*e[26]-1.*e[16]*e[21]*e[3]+e[16]*e[26]*e[8]-1.*e[16]*e[20]*e[2]-1.*e[16]*e[18]*e[0]-1.*e[16]*e[23]*e[5]+e[16]*e[24]*e[6]+e[13]*e[21]*e[6]+e[13]*e[3]*e[24]+e[13]*e[22]*e[7]+e[13]*e[23]*e[8]+e[13]*e[5]*e[26]-1.*e[25]*e[11]*e[2]+e[25]*e[15]*e[6]-1.*e[25]*e[9]*e[0]-1.*e[25]*e[14]*e[5]-1.*e[25]*e[12]*e[3]+e[25]*e[17]*e[8]+e[22]*e[12]*e[6]+e[22]*e[3]*e[15]+e[22]*e[14]*e[8]+e[22]*e[5]*e[17]-1.*e[7]*e[23]*e[14]-1.*e[7]*e[20]*e[11]-1.*e[7]*e[18]*e[9]-1.*e[7]*e[21]*e[12]; 
    A[55]=e[13]*e[9]*e[3]+e[13]*e[0]*e[12]+e[13]*e[10]*e[4]+e[13]*e[11]*e[5]+e[13]*e[2]*e[14]+e[16]*e[9]*e[6]+e[16]*e[0]*e[15]+e[16]*e[10]*e[7]+e[16]*e[11]*e[8]+e[16]*e[2]*e[17]+e[7]*e[11]*e[17]+e[7]*e[9]*e[15]+e[4]*e[11]*e[14]+e[4]*e[9]*e[12]+e[10]*e[9]*e[0]+e[10]*e[11]*e[2]-1.*e[10]*e[15]*e[6]-1.*e[10]*e[14]*e[5]-1.*e[10]*e[12]*e[3]-1.*e[10]*e[17]*e[8]+.5000000000*e[1]*ep2[11]+.5000000000*e[1]*ep2[9]+1.500000000*e[1]*ep2[10]-.5000000000*e[1]*ep2[12]-.5000000000*e[1]*ep2[15]-.5000000000*e[1]*ep2[17]-.5000000000*e[1]*ep2[14]+.5000000000*e[1]*ep2[13]+.5000000000*e[1]*ep2[16]; 
    A[165]=e[1]*e[27]*e[6]+e[1]*e[0]*e[33]+e[1]*e[28]*e[7]+e[1]*e[29]*e[8]+e[1]*e[2]*e[35]-1.*e[7]*e[27]*e[0]-1.*e[7]*e[32]*e[5]+e[7]*e[33]*e[6]-1.*e[7]*e[30]*e[3]+e[7]*e[35]*e[8]-1.*e[7]*e[29]*e[2]+e[7]*e[31]*e[4]+e[28]*e[0]*e[6]+e[28]*e[2]*e[8]+e[4]*e[30]*e[6]+e[4]*e[3]*e[33]+e[4]*e[32]*e[8]+e[4]*e[5]*e[35]+e[31]*e[3]*e[6]+e[31]*e[5]*e[8]+.5000000000*ep2[1]*e[34]+1.500000000*e[34]*ep2[7]+.5000000000*e[34]*ep2[4]-.5000000000*e[34]*ep2[0]+.5000000000*e[34]*ep2[6]-.5000000000*e[34]*ep2[5]-.5000000000*e[34]*ep2[3]-.5000000000*e[34]*ep2[2]+.5000000000*e[34]*ep2[8]; 
    A[52]=e[4]*e[18]*e[3]+e[4]*e[0]*e[21]+e[4]*e[1]*e[22]+e[4]*e[20]*e[5]+e[4]*e[2]*e[23]+e[22]*e[0]*e[3]+e[22]*e[2]*e[5]+e[7]*e[18]*e[6]+e[7]*e[0]*e[24]+e[7]*e[1]*e[25]+e[7]*e[20]*e[8]+e[7]*e[2]*e[26]+e[25]*e[0]*e[6]+e[25]*e[2]*e[8]+e[1]*e[18]*e[0]+e[1]*e[20]*e[2]-1.*e[1]*e[21]*e[3]-1.*e[1]*e[26]*e[8]-1.*e[1]*e[23]*e[5]-1.*e[1]*e[24]*e[6]+.5000000000*e[19]*ep2[4]+.5000000000*e[19]*ep2[0]-.5000000000*e[19]*ep2[6]-.5000000000*e[19]*ep2[5]+1.500000000*e[19]*ep2[1]+.5000000000*e[19]*ep2[7]-.5000000000*e[19]*ep2[3]+.5000000000*e[19]*ep2[2]-.5000000000*e[19]*ep2[8]; 
    A[166]=-.5000000000*e[7]*ep2[0]+e[4]*e[5]*e[8]+.5000000000*ep2[4]*e[7]-.5000000000*e[7]*ep2[2]+.5000000000*e[7]*ep2[8]-.5000000000*e[7]*ep2[5]+.5000000000*e[7]*ep2[6]+e[1]*e[0]*e[6]+.5000000000*ep3[7]+e[4]*e[3]*e[6]+e[1]*e[2]*e[8]-.5000000000*e[7]*ep2[3]+.5000000000*ep2[1]*e[7]; 
    A[53]=-1.*e[1]*e[32]*e[23]-1.*e[19]*e[32]*e[5]-1.*e[19]*e[33]*e[6]-1.*e[19]*e[30]*e[3]-1.*e[19]*e[35]*e[8]-1.*e[28]*e[21]*e[3]-1.*e[28]*e[26]*e[8]-1.*e[28]*e[23]*e[5]-1.*e[28]*e[24]*e[6]+e[7]*e[27]*e[24]+e[7]*e[18]*e[33]+e[7]*e[29]*e[26]+e[7]*e[20]*e[35]+e[22]*e[27]*e[3]+e[22]*e[0]*e[30]+e[22]*e[29]*e[5]+e[22]*e[2]*e[32]+e[31]*e[18]*e[3]+e[31]*e[0]*e[21]+e[31]*e[20]*e[5]+e[31]*e[2]*e[23]+e[25]*e[27]*e[6]+e[25]*e[0]*e[33]+e[25]*e[28]*e[7]+e[25]*e[1]*e[34]+e[25]*e[29]*e[8]+e[25]*e[2]*e[35]+e[34]*e[18]*e[6]+e[34]*e[0]*e[24]+e[34]*e[19]*e[7]+e[34]*e[20]*e[8]+e[34]*e[2]*e[26]+e[1]*e[27]*e[18]+3.*e[1]*e[28]*e[19]+e[1]*e[29]*e[20]+e[19]*e[27]*e[0]+e[19]*e[29]*e[2]+e[28]*e[18]*e[0]+e[28]*e[20]*e[2]+e[4]*e[27]*e[21]+e[4]*e[18]*e[30]+e[4]*e[28]*e[22]+e[4]*e[19]*e[31]+e[4]*e[29]*e[23]+e[4]*e[20]*e[32]-1.*e[1]*e[33]*e[24]-1.*e[1]*e[30]*e[21]-1.*e[1]*e[35]*e[26]+e[1]*e[31]*e[22]; 
    A[167]=e[10]*e[27]*e[15]+e[10]*e[9]*e[33]+e[10]*e[29]*e[17]+e[10]*e[11]*e[35]+e[10]*e[28]*e[16]+e[28]*e[11]*e[17]+e[28]*e[9]*e[15]+e[13]*e[30]*e[15]+e[13]*e[12]*e[33]+e[13]*e[32]*e[17]+e[13]*e[14]*e[35]+e[13]*e[31]*e[16]+e[31]*e[14]*e[17]+e[31]*e[12]*e[15]+e[16]*e[33]*e[15]+e[16]*e[35]*e[17]-1.*e[16]*e[27]*e[9]-1.*e[16]*e[30]*e[12]-1.*e[16]*e[32]*e[14]-1.*e[16]*e[29]*e[11]+.5000000000*ep2[10]*e[34]+1.500000000*e[34]*ep2[16]-.5000000000*e[34]*ep2[9]-.5000000000*e[34]*ep2[11]-.5000000000*e[34]*ep2[12]+.5000000000*e[34]*ep2[15]+.5000000000*e[34]*ep2[17]-.5000000000*e[34]*ep2[14]+.5000000000*e[34]*ep2[13]; 
    A[50]=.5000000000*e[19]*ep2[18]+.5000000000*e[19]*ep2[25]+.5000000000*e[19]*ep2[22]+e[25]*e[20]*e[26]-.5000000000*e[19]*ep2[21]+.5000000000*e[19]*ep2[20]-.5000000000*e[19]*ep2[26]-.5000000000*e[19]*ep2[23]-.5000000000*e[19]*ep2[24]+.5000000000*ep3[19]+e[22]*e[20]*e[23]+e[25]*e[18]*e[24]+e[22]*e[18]*e[21]; 
    A[160]=.5000000000*e[34]*ep2[33]+.5000000000*e[34]*ep2[35]-.5000000000*e[34]*ep2[27]-.5000000000*e[34]*ep2[32]-.5000000000*e[34]*ep2[29]-.5000000000*e[34]*ep2[30]+.5000000000*ep2[28]*e[34]+e[31]*e[30]*e[33]+e[31]*e[32]*e[35]+e[28]*e[27]*e[33]+.5000000000*ep3[34]+e[28]*e[29]*e[35]+.5000000000*ep2[31]*e[34]; 
    A[51]=e[4]*e[28]*e[13]+e[4]*e[10]*e[31]+e[7]*e[27]*e[15]+e[7]*e[9]*e[33]+e[7]*e[29]*e[17]+e[7]*e[11]*e[35]+e[7]*e[28]*e[16]+e[7]*e[10]*e[34]+e[1]*e[27]*e[9]+e[1]*e[29]*e[11]+3.*e[1]*e[28]*e[10]+e[10]*e[27]*e[0]-1.*e[10]*e[32]*e[5]-1.*e[10]*e[33]*e[6]-1.*e[10]*e[30]*e[3]-1.*e[10]*e[35]*e[8]+e[10]*e[29]*e[2]+e[13]*e[27]*e[3]+e[13]*e[0]*e[30]+e[13]*e[1]*e[31]+e[13]*e[29]*e[5]+e[13]*e[2]*e[32]+e[28]*e[11]*e[2]-1.*e[28]*e[15]*e[6]+e[28]*e[9]*e[0]-1.*e[28]*e[14]*e[5]-1.*e[28]*e[12]*e[3]-1.*e[28]*e[17]*e[8]+e[31]*e[9]*e[3]+e[31]*e[0]*e[12]+e[31]*e[11]*e[5]+e[31]*e[2]*e[14]+e[16]*e[27]*e[6]+e[16]*e[0]*e[33]+e[16]*e[1]*e[34]+e[16]*e[29]*e[8]+e[16]*e[2]*e[35]-1.*e[1]*e[30]*e[12]-1.*e[1]*e[32]*e[14]-1.*e[1]*e[33]*e[15]-1.*e[1]*e[35]*e[17]+e[34]*e[9]*e[6]+e[34]*e[0]*e[15]+e[34]*e[11]*e[8]+e[34]*e[2]*e[17]+e[4]*e[27]*e[12]+e[4]*e[9]*e[30]+e[4]*e[29]*e[14]+e[4]*e[11]*e[32]; 
    A[161]=e[4]*e[30]*e[33]+e[4]*e[32]*e[35]+e[4]*e[31]*e[34]+e[31]*e[30]*e[6]+e[31]*e[3]*e[33]+e[31]*e[32]*e[8]+e[31]*e[5]*e[35]+e[28]*e[27]*e[6]+e[28]*e[0]*e[33]+e[28]*e[29]*e[8]+e[28]*e[2]*e[35]+e[34]*e[33]*e[6]+e[34]*e[35]*e[8]-1.*e[34]*e[27]*e[0]-1.*e[34]*e[32]*e[5]-1.*e[34]*e[30]*e[3]-1.*e[34]*e[29]*e[2]+e[1]*e[27]*e[33]+e[1]*e[29]*e[35]+e[1]*e[28]*e[34]+.5000000000*ep2[31]*e[7]-.5000000000*e[7]*ep2[27]-.5000000000*e[7]*ep2[32]+.5000000000*e[7]*ep2[28]-.5000000000*e[7]*ep2[29]+.5000000000*e[7]*ep2[33]-.5000000000*e[7]*ep2[30]+1.500000000*e[7]*ep2[34]+.5000000000*e[7]*ep2[35]; 
    A[48]=-.5000000000*e[10]*ep2[14]-.5000000000*e[10]*ep2[17]-.5000000000*e[10]*ep2[15]+e[13]*e[11]*e[14]+e[16]*e[11]*e[17]+.5000000000*e[10]*ep2[13]+e[13]*e[9]*e[12]-.5000000000*e[10]*ep2[12]+.5000000000*ep3[10]+e[16]*e[9]*e[15]+.5000000000*e[10]*ep2[16]+.5000000000*e[10]*ep2[11]+.5000000000*e[10]*ep2[9]; 
    A[162]=e[22]*e[32]*e[35]+e[22]*e[31]*e[34]+e[31]*e[30]*e[24]+e[31]*e[21]*e[33]+e[31]*e[32]*e[26]+e[31]*e[23]*e[35]+e[34]*e[33]*e[24]+e[34]*e[35]*e[26]-1.*e[34]*e[27]*e[18]-1.*e[34]*e[30]*e[21]-1.*e[34]*e[29]*e[20]-1.*e[34]*e[32]*e[23]+e[19]*e[27]*e[33]+e[19]*e[29]*e[35]+e[19]*e[28]*e[34]+e[28]*e[27]*e[24]+e[28]*e[18]*e[33]+e[28]*e[29]*e[26]+e[28]*e[20]*e[35]+e[22]*e[30]*e[33]+.5000000000*ep2[28]*e[25]+.5000000000*ep2[31]*e[25]+.5000000000*e[25]*ep2[33]+.5000000000*e[25]*ep2[35]+1.500000000*e[25]*ep2[34]-.5000000000*e[25]*ep2[27]-.5000000000*e[25]*ep2[32]-.5000000000*e[25]*ep2[29]-.5000000000*e[25]*ep2[30]; 
    A[49]=-1.*e[19]*e[35]*e[26]-1.*e[19]*e[32]*e[23]+e[19]*e[27]*e[18]+e[19]*e[29]*e[20]+e[22]*e[27]*e[21]+e[22]*e[18]*e[30]+e[22]*e[19]*e[31]+e[22]*e[29]*e[23]+e[22]*e[20]*e[32]+e[31]*e[18]*e[21]+e[31]*e[20]*e[23]+e[25]*e[27]*e[24]+e[25]*e[18]*e[33]+e[25]*e[19]*e[34]+e[25]*e[29]*e[26]+e[25]*e[20]*e[35]+e[34]*e[18]*e[24]+e[34]*e[20]*e[26]-1.*e[19]*e[33]*e[24]-1.*e[19]*e[30]*e[21]+1.500000000*e[28]*ep2[19]+.5000000000*e[28]*ep2[18]+.5000000000*e[28]*ep2[20]+.5000000000*e[28]*ep2[22]+.5000000000*e[28]*ep2[25]-.5000000000*e[28]*ep2[26]-.5000000000*e[28]*ep2[23]-.5000000000*e[28]*ep2[24]-.5000000000*e[28]*ep2[21]; 
    A[163]=e[10]*e[27]*e[33]+e[10]*e[29]*e[35]+e[10]*e[28]*e[34]+e[34]*e[33]*e[15]+e[34]*e[35]*e[17]+e[28]*e[27]*e[15]+e[28]*e[9]*e[33]+e[28]*e[29]*e[17]+e[28]*e[11]*e[35]-1.*e[34]*e[27]*e[9]-1.*e[34]*e[30]*e[12]+e[34]*e[31]*e[13]-1.*e[34]*e[32]*e[14]-1.*e[34]*e[29]*e[11]+e[31]*e[30]*e[15]+e[31]*e[12]*e[33]+e[31]*e[32]*e[17]+e[31]*e[14]*e[35]+e[13]*e[30]*e[33]+e[13]*e[32]*e[35]-.5000000000*e[16]*ep2[27]-.5000000000*e[16]*ep2[32]+.5000000000*e[16]*ep2[28]-.5000000000*e[16]*ep2[29]+.5000000000*e[16]*ep2[31]+.5000000000*e[16]*ep2[33]-.5000000000*e[16]*ep2[30]+1.500000000*e[16]*ep2[34]+.5000000000*e[16]*ep2[35]; 
    A[63]=e[29]*e[32]*e[14]-1.*e[29]*e[33]*e[15]-1.*e[29]*e[34]*e[16]+e[32]*e[27]*e[12]+e[32]*e[9]*e[30]+e[32]*e[28]*e[13]+e[32]*e[10]*e[31]+e[14]*e[27]*e[30]+e[14]*e[28]*e[31]+e[17]*e[27]*e[33]+e[17]*e[28]*e[34]+e[35]*e[27]*e[15]+e[35]*e[9]*e[33]+e[35]*e[29]*e[17]+e[35]*e[28]*e[16]+e[35]*e[10]*e[34]+e[29]*e[27]*e[9]+e[29]*e[28]*e[10]-1.*e[29]*e[30]*e[12]-1.*e[29]*e[31]*e[13]+.5000000000*e[11]*ep2[27]+1.500000000*e[11]*ep2[29]+.5000000000*e[11]*ep2[28]+.5000000000*e[11]*ep2[32]-.5000000000*e[11]*ep2[31]-.5000000000*e[11]*ep2[33]-.5000000000*e[11]*ep2[30]-.5000000000*e[11]*ep2[34]+.5000000000*e[11]*ep2[35]; 
    A[173]=e[1]*e[20]*e[35]+e[19]*e[27]*e[6]+e[19]*e[0]*e[33]+e[19]*e[28]*e[7]+e[19]*e[29]*e[8]+e[19]*e[2]*e[35]+e[28]*e[18]*e[6]+e[28]*e[0]*e[24]+e[28]*e[20]*e[8]+e[28]*e[2]*e[26]+e[4]*e[30]*e[24]+e[4]*e[21]*e[33]+e[4]*e[31]*e[25]+e[4]*e[22]*e[34]+e[4]*e[32]*e[26]+e[4]*e[23]*e[35]-1.*e[7]*e[27]*e[18]+e[7]*e[33]*e[24]-1.*e[7]*e[30]*e[21]-1.*e[7]*e[29]*e[20]+e[7]*e[35]*e[26]+e[7]*e[31]*e[22]-1.*e[7]*e[32]*e[23]-1.*e[25]*e[27]*e[0]-1.*e[25]*e[32]*e[5]-1.*e[25]*e[30]*e[3]-1.*e[25]*e[29]*e[2]-1.*e[34]*e[21]*e[3]-1.*e[34]*e[20]*e[2]-1.*e[34]*e[18]*e[0]-1.*e[34]*e[23]*e[5]+e[22]*e[30]*e[6]+e[22]*e[3]*e[33]+e[22]*e[32]*e[8]+e[22]*e[5]*e[35]+e[31]*e[21]*e[6]+e[31]*e[3]*e[24]+e[31]*e[23]*e[8]+e[31]*e[5]*e[26]+e[34]*e[26]*e[8]+e[1]*e[27]*e[24]+e[1]*e[18]*e[33]+e[1]*e[28]*e[25]+e[1]*e[19]*e[34]+e[1]*e[29]*e[26]+e[34]*e[24]*e[6]+e[25]*e[33]*e[6]+3.*e[25]*e[34]*e[7]+e[25]*e[35]*e[8]; 
    A[62]=.5000000000*e[20]*ep2[27]+1.500000000*e[20]*ep2[29]+.5000000000*e[20]*ep2[28]+.5000000000*e[20]*ep2[32]+.5000000000*e[20]*ep2[35]-.5000000000*e[20]*ep2[31]-.5000000000*e[20]*ep2[33]-.5000000000*e[20]*ep2[30]-.5000000000*e[20]*ep2[34]+e[29]*e[27]*e[18]+e[29]*e[28]*e[19]+e[23]*e[27]*e[30]+e[23]*e[29]*e[32]+e[23]*e[28]*e[31]+e[32]*e[27]*e[21]+e[32]*e[18]*e[30]+e[32]*e[28]*e[22]+e[32]*e[19]*e[31]+e[26]*e[27]*e[33]+e[26]*e[29]*e[35]+e[26]*e[28]*e[34]+e[35]*e[27]*e[24]+e[35]*e[18]*e[33]+e[35]*e[28]*e[25]+e[35]*e[19]*e[34]-1.*e[29]*e[33]*e[24]-1.*e[29]*e[30]*e[21]-1.*e[29]*e[31]*e[22]-1.*e[29]*e[34]*e[25]; 
    A[172]=e[19]*e[1]*e[7]+e[19]*e[0]*e[6]+e[19]*e[2]*e[8]+e[4]*e[21]*e[6]+e[4]*e[3]*e[24]+e[4]*e[22]*e[7]+e[4]*e[23]*e[8]+e[4]*e[5]*e[26]+e[22]*e[3]*e[6]+e[22]*e[5]*e[8]+e[7]*e[24]*e[6]+e[7]*e[26]*e[8]+e[1]*e[18]*e[6]+e[1]*e[0]*e[24]+e[1]*e[20]*e[8]+e[1]*e[2]*e[26]-1.*e[7]*e[21]*e[3]-1.*e[7]*e[20]*e[2]-1.*e[7]*e[18]*e[0]-1.*e[7]*e[23]*e[5]+.5000000000*e[25]*ep2[4]-.5000000000*e[25]*ep2[0]+.5000000000*e[25]*ep2[6]-.5000000000*e[25]*ep2[5]+.5000000000*e[25]*ep2[1]+1.500000000*e[25]*ep2[7]-.5000000000*e[25]*ep2[3]-.5000000000*e[25]*ep2[2]+.5000000000*e[25]*ep2[8]; 
    A[61]=e[5]*e[27]*e[30]+e[5]*e[29]*e[32]+e[5]*e[28]*e[31]+e[32]*e[27]*e[3]+e[32]*e[0]*e[30]+e[32]*e[28]*e[4]+e[32]*e[1]*e[31]+e[8]*e[27]*e[33]+e[8]*e[29]*e[35]+e[8]*e[28]*e[34]+e[29]*e[27]*e[0]+e[29]*e[28]*e[1]+e[35]*e[27]*e[6]+e[35]*e[0]*e[33]+e[35]*e[28]*e[7]+e[35]*e[1]*e[34]-1.*e[29]*e[34]*e[7]-1.*e[29]*e[33]*e[6]-1.*e[29]*e[30]*e[3]-1.*e[29]*e[31]*e[4]+.5000000000*e[2]*ep2[27]+1.500000000*e[2]*ep2[29]+.5000000000*e[2]*ep2[28]+.5000000000*e[2]*ep2[32]-.5000000000*e[2]*ep2[31]-.5000000000*e[2]*ep2[33]-.5000000000*e[2]*ep2[30]-.5000000000*e[2]*ep2[34]+.5000000000*e[2]*ep2[35]; 
    A[175]=e[13]*e[12]*e[6]+e[13]*e[3]*e[15]+e[13]*e[4]*e[16]+e[13]*e[14]*e[8]+e[13]*e[5]*e[17]+e[16]*e[15]*e[6]+e[16]*e[17]*e[8]+e[1]*e[11]*e[17]+e[1]*e[9]*e[15]+e[1]*e[10]*e[16]+e[4]*e[14]*e[17]+e[4]*e[12]*e[15]+e[10]*e[9]*e[6]+e[10]*e[0]*e[15]+e[10]*e[11]*e[8]+e[10]*e[2]*e[17]-1.*e[16]*e[11]*e[2]-1.*e[16]*e[9]*e[0]-1.*e[16]*e[14]*e[5]-1.*e[16]*e[12]*e[3]+.5000000000*ep2[13]*e[7]+1.500000000*ep2[16]*e[7]+.5000000000*e[7]*ep2[17]+.5000000000*e[7]*ep2[15]-.5000000000*e[7]*ep2[9]-.5000000000*e[7]*ep2[11]-.5000000000*e[7]*ep2[12]+.5000000000*e[7]*ep2[10]-.5000000000*e[7]*ep2[14]; 
    A[60]=.5000000000*e[29]*ep2[32]+.5000000000*e[29]*ep2[35]-.5000000000*e[29]*ep2[31]-.5000000000*e[29]*ep2[33]-.5000000000*e[29]*ep2[30]-.5000000000*e[29]*ep2[34]+e[32]*e[27]*e[30]+.5000000000*ep3[29]+.5000000000*e[29]*ep2[28]+e[35]*e[28]*e[34]+.5000000000*e[29]*ep2[27]+e[35]*e[27]*e[33]+e[32]*e[28]*e[31]; 
    A[174]=-1.*e[16]*e[21]*e[12]+e[10]*e[18]*e[15]+e[10]*e[9]*e[24]+e[10]*e[20]*e[17]+e[10]*e[11]*e[26]+e[19]*e[11]*e[17]+e[19]*e[9]*e[15]+e[19]*e[10]*e[16]+e[13]*e[21]*e[15]+e[13]*e[12]*e[24]+e[13]*e[23]*e[17]+e[13]*e[14]*e[26]+e[13]*e[22]*e[16]+e[22]*e[14]*e[17]+e[22]*e[12]*e[15]+e[16]*e[24]*e[15]+e[16]*e[26]*e[17]-1.*e[16]*e[23]*e[14]-1.*e[16]*e[20]*e[11]-1.*e[16]*e[18]*e[9]+.5000000000*ep2[13]*e[25]+1.500000000*e[25]*ep2[16]+.5000000000*e[25]*ep2[17]+.5000000000*e[25]*ep2[15]+.5000000000*ep2[10]*e[25]-.5000000000*e[25]*ep2[9]-.5000000000*e[25]*ep2[11]-.5000000000*e[25]*ep2[12]-.5000000000*e[25]*ep2[14]; 
    A[59]=e[19]*e[20]*e[2]+e[22]*e[18]*e[3]+e[22]*e[0]*e[21]+e[22]*e[19]*e[4]+e[22]*e[20]*e[5]+e[22]*e[2]*e[23]-1.*e[19]*e[21]*e[3]-1.*e[19]*e[26]*e[8]+e[19]*e[25]*e[7]-1.*e[19]*e[23]*e[5]-1.*e[19]*e[24]*e[6]+e[4]*e[18]*e[21]+e[4]*e[20]*e[23]+e[25]*e[18]*e[6]+e[25]*e[0]*e[24]+e[25]*e[20]*e[8]+e[25]*e[2]*e[26]+e[7]*e[18]*e[24]+e[7]*e[20]*e[26]+e[19]*e[18]*e[0]+1.500000000*ep2[19]*e[1]+.5000000000*e[1]*ep2[22]+.5000000000*e[1]*ep2[18]+.5000000000*e[1]*ep2[20]+.5000000000*e[1]*ep2[25]-.5000000000*e[1]*ep2[26]-.5000000000*e[1]*ep2[23]-.5000000000*e[1]*ep2[24]-.5000000000*e[1]*ep2[21]; 
    A[169]=e[19]*e[27]*e[24]+e[19]*e[18]*e[33]+e[19]*e[28]*e[25]+e[19]*e[29]*e[26]+e[19]*e[20]*e[35]+e[28]*e[18]*e[24]+e[28]*e[20]*e[26]+e[22]*e[30]*e[24]+e[22]*e[21]*e[33]+e[22]*e[31]*e[25]+e[22]*e[32]*e[26]+e[22]*e[23]*e[35]+e[31]*e[21]*e[24]+e[31]*e[23]*e[26]+e[25]*e[33]*e[24]+e[25]*e[35]*e[26]-1.*e[25]*e[27]*e[18]-1.*e[25]*e[30]*e[21]-1.*e[25]*e[29]*e[20]-1.*e[25]*e[32]*e[23]-.5000000000*e[34]*ep2[18]-.5000000000*e[34]*ep2[23]-.5000000000*e[34]*ep2[20]-.5000000000*e[34]*ep2[21]+.5000000000*ep2[19]*e[34]+.5000000000*ep2[22]*e[34]+1.500000000*e[34]*ep2[25]+.5000000000*e[34]*ep2[24]+.5000000000*e[34]*ep2[26]; 
    A[58]=e[16]*e[0]*e[6]+e[16]*e[2]*e[8]+e[1]*e[11]*e[2]-1.*e[1]*e[15]*e[6]+e[1]*e[9]*e[0]-1.*e[1]*e[14]*e[5]-1.*e[1]*e[12]*e[3]-1.*e[1]*e[17]*e[8]+e[4]*e[9]*e[3]+e[4]*e[0]*e[12]+e[4]*e[1]*e[13]+e[4]*e[11]*e[5]+e[4]*e[2]*e[14]+e[13]*e[0]*e[3]+e[13]*e[2]*e[5]+e[7]*e[9]*e[6]+e[7]*e[0]*e[15]+e[7]*e[1]*e[16]+e[7]*e[11]*e[8]+e[7]*e[2]*e[17]-.5000000000*e[10]*ep2[6]-.5000000000*e[10]*ep2[5]-.5000000000*e[10]*ep2[3]-.5000000000*e[10]*ep2[8]+1.500000000*e[10]*ep2[1]+.5000000000*e[10]*ep2[0]+.5000000000*e[10]*ep2[2]+.5000000000*e[10]*ep2[4]+.5000000000*e[10]*ep2[7]; 
    A[168]=e[13]*e[14]*e[17]+e[13]*e[12]*e[15]+e[10]*e[9]*e[15]+.5000000000*e[16]*ep2[15]-.5000000000*e[16]*ep2[11]-.5000000000*e[16]*ep2[12]-.5000000000*e[16]*ep2[14]+e[10]*e[11]*e[17]+.5000000000*ep2[10]*e[16]+.5000000000*ep3[16]-.5000000000*e[16]*ep2[9]+.5000000000*e[16]*ep2[17]+.5000000000*ep2[13]*e[16]; 
    A[57]=e[10]*e[29]*e[20]+e[22]*e[27]*e[12]+e[22]*e[9]*e[30]+e[22]*e[29]*e[14]+e[22]*e[11]*e[32]+e[22]*e[10]*e[31]+e[31]*e[18]*e[12]+e[31]*e[9]*e[21]+e[31]*e[20]*e[14]+e[31]*e[11]*e[23]-1.*e[10]*e[33]*e[24]-1.*e[10]*e[30]*e[21]-1.*e[10]*e[35]*e[26]-1.*e[10]*e[32]*e[23]+e[10]*e[34]*e[25]+e[19]*e[27]*e[9]+e[19]*e[29]*e[11]+e[28]*e[18]*e[9]+e[28]*e[20]*e[11]+e[16]*e[27]*e[24]+e[16]*e[18]*e[33]+e[16]*e[28]*e[25]+e[16]*e[19]*e[34]+e[16]*e[29]*e[26]+e[16]*e[20]*e[35]-1.*e[19]*e[30]*e[12]-1.*e[19]*e[32]*e[14]-1.*e[19]*e[33]*e[15]-1.*e[19]*e[35]*e[17]-1.*e[28]*e[23]*e[14]-1.*e[28]*e[24]*e[15]-1.*e[28]*e[26]*e[17]-1.*e[28]*e[21]*e[12]+e[25]*e[27]*e[15]+e[25]*e[9]*e[33]+e[25]*e[29]*e[17]+e[25]*e[11]*e[35]+e[34]*e[18]*e[15]+e[34]*e[9]*e[24]+e[34]*e[20]*e[17]+e[34]*e[11]*e[26]+e[13]*e[27]*e[21]+e[13]*e[18]*e[30]+e[13]*e[28]*e[22]+e[13]*e[19]*e[31]+e[13]*e[29]*e[23]+e[13]*e[20]*e[32]+e[10]*e[27]*e[18]+3.*e[10]*e[28]*e[19]; 
    A[171]=e[4]*e[30]*e[15]+e[4]*e[12]*e[33]+e[4]*e[32]*e[17]+e[4]*e[14]*e[35]+e[4]*e[31]*e[16]+e[4]*e[13]*e[34]+e[7]*e[33]*e[15]+e[7]*e[35]*e[17]+3.*e[7]*e[34]*e[16]+e[1]*e[27]*e[15]+e[1]*e[9]*e[33]+e[1]*e[29]*e[17]+e[1]*e[11]*e[35]+e[1]*e[28]*e[16]+e[1]*e[10]*e[34]-1.*e[16]*e[27]*e[0]-1.*e[16]*e[32]*e[5]+e[16]*e[33]*e[6]-1.*e[16]*e[30]*e[3]+e[16]*e[35]*e[8]-1.*e[16]*e[29]*e[2]+e[13]*e[30]*e[6]+e[13]*e[3]*e[33]+e[13]*e[31]*e[7]+e[13]*e[32]*e[8]+e[13]*e[5]*e[35]-1.*e[34]*e[11]*e[2]+e[34]*e[15]*e[6]-1.*e[34]*e[9]*e[0]-1.*e[34]*e[14]*e[5]-1.*e[34]*e[12]*e[3]+e[34]*e[17]*e[8]+e[31]*e[12]*e[6]+e[31]*e[3]*e[15]+e[31]*e[14]*e[8]+e[31]*e[5]*e[17]-1.*e[7]*e[27]*e[9]-1.*e[7]*e[30]*e[12]+e[7]*e[28]*e[10]-1.*e[7]*e[32]*e[14]+e[10]*e[27]*e[6]+e[10]*e[0]*e[33]+e[10]*e[29]*e[8]+e[10]*e[2]*e[35]+e[28]*e[9]*e[6]+e[28]*e[0]*e[15]+e[28]*e[11]*e[8]+e[28]*e[2]*e[17]-1.*e[7]*e[29]*e[11]; 
    A[56]=e[22]*e[18]*e[12]+e[22]*e[9]*e[21]+e[22]*e[20]*e[14]+e[22]*e[11]*e[23]+e[22]*e[19]*e[13]+e[25]*e[18]*e[15]+e[25]*e[9]*e[24]+e[25]*e[20]*e[17]+e[25]*e[11]*e[26]+e[25]*e[19]*e[16]+e[16]*e[18]*e[24]+e[16]*e[20]*e[26]+e[13]*e[18]*e[21]+e[13]*e[20]*e[23]+e[19]*e[18]*e[9]+e[19]*e[20]*e[11]-1.*e[19]*e[23]*e[14]-1.*e[19]*e[24]*e[15]-1.*e[19]*e[26]*e[17]-1.*e[19]*e[21]*e[12]+.5000000000*e[10]*ep2[22]+.5000000000*e[10]*ep2[25]+1.500000000*e[10]*ep2[19]+.5000000000*e[10]*ep2[18]+.5000000000*e[10]*ep2[20]-.5000000000*e[10]*ep2[26]-.5000000000*e[10]*ep2[23]-.5000000000*e[10]*ep2[24]-.5000000000*e[10]*ep2[21]; 
    A[170]=e[19]*e[20]*e[26]-.5000000000*e[25]*ep2[20]+e[22]*e[21]*e[24]+e[19]*e[18]*e[24]+.5000000000*ep2[22]*e[25]-.5000000000*e[25]*ep2[21]-.5000000000*e[25]*ep2[23]+.5000000000*ep2[19]*e[25]-.5000000000*e[25]*ep2[18]+.5000000000*e[25]*ep2[24]+.5000000000*e[25]*ep2[26]+.5000000000*ep3[25]+e[22]*e[23]*e[26]; 
    A[73]=-1.*e[20]*e[33]*e[6]-1.*e[20]*e[30]*e[3]-1.*e[20]*e[31]*e[4]-1.*e[29]*e[21]*e[3]-1.*e[29]*e[22]*e[4]-1.*e[29]*e[25]*e[7]-1.*e[29]*e[24]*e[6]+e[8]*e[27]*e[24]+e[8]*e[18]*e[33]+e[8]*e[28]*e[25]+e[8]*e[19]*e[34]+e[23]*e[27]*e[3]+e[23]*e[0]*e[30]+e[23]*e[28]*e[4]+e[23]*e[1]*e[31]+e[32]*e[18]*e[3]+e[32]*e[0]*e[21]+e[32]*e[19]*e[4]+e[32]*e[1]*e[22]+e[26]*e[27]*e[6]+e[26]*e[0]*e[33]+e[26]*e[28]*e[7]+e[26]*e[1]*e[34]+e[26]*e[29]*e[8]+e[26]*e[2]*e[35]+e[35]*e[18]*e[6]+e[35]*e[0]*e[24]+e[35]*e[19]*e[7]+e[35]*e[1]*e[25]+e[35]*e[20]*e[8]+e[2]*e[27]*e[18]+e[2]*e[28]*e[19]+3.*e[2]*e[29]*e[20]+e[20]*e[27]*e[0]+e[20]*e[28]*e[1]+e[29]*e[18]*e[0]+e[29]*e[19]*e[1]+e[5]*e[27]*e[21]+e[5]*e[18]*e[30]+e[5]*e[28]*e[22]+e[5]*e[19]*e[31]+e[5]*e[29]*e[23]+e[5]*e[20]*e[32]-1.*e[2]*e[33]*e[24]-1.*e[2]*e[30]*e[21]-1.*e[2]*e[31]*e[22]+e[2]*e[32]*e[23]-1.*e[2]*e[34]*e[25]-1.*e[20]*e[34]*e[7]; 
    A[72]=e[5]*e[18]*e[3]+e[5]*e[0]*e[21]+e[5]*e[19]*e[4]+e[5]*e[1]*e[22]+e[5]*e[2]*e[23]+e[23]*e[1]*e[4]+e[23]*e[0]*e[3]+e[8]*e[18]*e[6]+e[8]*e[0]*e[24]+e[8]*e[19]*e[7]+e[8]*e[1]*e[25]+e[8]*e[2]*e[26]+e[26]*e[1]*e[7]+e[26]*e[0]*e[6]+e[2]*e[18]*e[0]+e[2]*e[19]*e[1]-1.*e[2]*e[21]*e[3]-1.*e[2]*e[22]*e[4]-1.*e[2]*e[25]*e[7]-1.*e[2]*e[24]*e[6]-.5000000000*e[20]*ep2[4]+.5000000000*e[20]*ep2[0]-.5000000000*e[20]*ep2[6]+.5000000000*e[20]*ep2[5]+.5000000000*e[20]*ep2[1]-.5000000000*e[20]*ep2[7]-.5000000000*e[20]*ep2[3]+1.500000000*e[20]*ep2[2]+.5000000000*e[20]*ep2[8]; 
    A[75]=e[14]*e[9]*e[3]+e[14]*e[0]*e[12]+e[14]*e[10]*e[4]+e[14]*e[1]*e[13]+e[14]*e[11]*e[5]+e[17]*e[9]*e[6]+e[17]*e[0]*e[15]+e[17]*e[10]*e[7]+e[17]*e[1]*e[16]+e[17]*e[11]*e[8]+e[8]*e[9]*e[15]+e[8]*e[10]*e[16]+e[5]*e[9]*e[12]+e[5]*e[10]*e[13]+e[11]*e[9]*e[0]+e[11]*e[10]*e[1]-1.*e[11]*e[13]*e[4]-1.*e[11]*e[16]*e[7]-1.*e[11]*e[15]*e[6]-1.*e[11]*e[12]*e[3]+.5000000000*e[2]*ep2[14]+.5000000000*e[2]*ep2[17]+1.500000000*e[2]*ep2[11]+.5000000000*e[2]*ep2[9]+.5000000000*e[2]*ep2[10]-.5000000000*e[2]*ep2[16]-.5000000000*e[2]*ep2[12]-.5000000000*e[2]*ep2[15]-.5000000000*e[2]*ep2[13]; 
    A[74]=e[14]*e[18]*e[12]+e[14]*e[9]*e[21]+e[14]*e[11]*e[23]+e[14]*e[19]*e[13]+e[14]*e[10]*e[22]+e[23]*e[9]*e[12]+e[23]*e[10]*e[13]+e[17]*e[18]*e[15]+e[17]*e[9]*e[24]+e[17]*e[11]*e[26]+e[17]*e[19]*e[16]+e[17]*e[10]*e[25]+e[26]*e[9]*e[15]+e[26]*e[10]*e[16]-1.*e[11]*e[24]*e[15]-1.*e[11]*e[25]*e[16]+e[11]*e[18]*e[9]-1.*e[11]*e[21]*e[12]+e[11]*e[19]*e[10]-1.*e[11]*e[22]*e[13]+1.500000000*e[20]*ep2[11]+.5000000000*e[20]*ep2[9]+.5000000000*e[20]*ep2[10]+.5000000000*e[20]*ep2[14]+.5000000000*e[20]*ep2[17]-.5000000000*e[20]*ep2[16]-.5000000000*e[20]*ep2[12]-.5000000000*e[20]*ep2[15]-.5000000000*e[20]*ep2[13]; 
    A[77]=e[23]*e[10]*e[31]+e[32]*e[18]*e[12]+e[32]*e[9]*e[21]+e[32]*e[19]*e[13]+e[32]*e[10]*e[22]-1.*e[11]*e[33]*e[24]-1.*e[11]*e[30]*e[21]+e[11]*e[35]*e[26]-1.*e[11]*e[31]*e[22]-1.*e[11]*e[34]*e[25]+e[20]*e[27]*e[9]+e[20]*e[28]*e[10]+e[29]*e[18]*e[9]+e[29]*e[19]*e[10]+e[17]*e[27]*e[24]+e[17]*e[18]*e[33]+e[17]*e[28]*e[25]+e[17]*e[19]*e[34]+e[17]*e[29]*e[26]+e[17]*e[20]*e[35]-1.*e[20]*e[30]*e[12]-1.*e[20]*e[31]*e[13]-1.*e[20]*e[33]*e[15]-1.*e[20]*e[34]*e[16]-1.*e[29]*e[24]*e[15]-1.*e[29]*e[25]*e[16]-1.*e[29]*e[21]*e[12]-1.*e[29]*e[22]*e[13]+e[26]*e[27]*e[15]+e[26]*e[9]*e[33]+e[26]*e[28]*e[16]+e[26]*e[10]*e[34]+e[35]*e[18]*e[15]+e[35]*e[9]*e[24]+e[35]*e[19]*e[16]+e[35]*e[10]*e[25]+e[14]*e[27]*e[21]+e[14]*e[18]*e[30]+e[14]*e[28]*e[22]+e[14]*e[19]*e[31]+e[14]*e[29]*e[23]+e[14]*e[20]*e[32]+e[11]*e[27]*e[18]+e[11]*e[28]*e[19]+3.*e[11]*e[29]*e[20]+e[23]*e[27]*e[12]+e[23]*e[9]*e[30]+e[23]*e[11]*e[32]+e[23]*e[28]*e[13]; 
    A[76]=e[23]*e[18]*e[12]+e[23]*e[9]*e[21]+e[23]*e[20]*e[14]+e[23]*e[19]*e[13]+e[23]*e[10]*e[22]+e[26]*e[18]*e[15]+e[26]*e[9]*e[24]+e[26]*e[20]*e[17]+e[26]*e[19]*e[16]+e[26]*e[10]*e[25]+e[17]*e[19]*e[25]+e[17]*e[18]*e[24]+e[14]*e[19]*e[22]+e[14]*e[18]*e[21]+e[20]*e[18]*e[9]+e[20]*e[19]*e[10]-1.*e[20]*e[24]*e[15]-1.*e[20]*e[25]*e[16]-1.*e[20]*e[21]*e[12]-1.*e[20]*e[22]*e[13]+.5000000000*e[11]*ep2[23]+.5000000000*e[11]*ep2[26]+.5000000000*e[11]*ep2[19]+.5000000000*e[11]*ep2[18]+1.500000000*e[11]*ep2[20]-.5000000000*e[11]*ep2[22]-.5000000000*e[11]*ep2[24]-.5000000000*e[11]*ep2[21]-.5000000000*e[11]*ep2[25]; 
    A[79]=-1.*e[20]*e[21]*e[3]+e[20]*e[26]*e[8]-1.*e[20]*e[22]*e[4]-1.*e[20]*e[25]*e[7]-1.*e[20]*e[24]*e[6]+e[5]*e[19]*e[22]+e[5]*e[18]*e[21]+e[26]*e[18]*e[6]+e[26]*e[0]*e[24]+e[26]*e[19]*e[7]+e[26]*e[1]*e[25]+e[8]*e[19]*e[25]+e[8]*e[18]*e[24]+e[20]*e[18]*e[0]+e[20]*e[19]*e[1]+e[23]*e[18]*e[3]+e[23]*e[0]*e[21]+e[23]*e[19]*e[4]+e[23]*e[1]*e[22]+e[23]*e[20]*e[5]+1.500000000*ep2[20]*e[2]+.5000000000*e[2]*ep2[23]+.5000000000*e[2]*ep2[19]+.5000000000*e[2]*ep2[18]+.5000000000*e[2]*ep2[26]-.5000000000*e[2]*ep2[22]-.5000000000*e[2]*ep2[24]-.5000000000*e[2]*ep2[21]-.5000000000*e[2]*ep2[25]; 
    A[78]=-1.*e[2]*e[15]*e[6]+e[2]*e[9]*e[0]-1.*e[2]*e[12]*e[3]+e[5]*e[9]*e[3]+e[5]*e[0]*e[12]+e[5]*e[10]*e[4]+e[5]*e[1]*e[13]+e[5]*e[2]*e[14]+e[14]*e[1]*e[4]+e[14]*e[0]*e[3]+e[8]*e[9]*e[6]+e[8]*e[0]*e[15]+e[8]*e[10]*e[7]+e[8]*e[1]*e[16]+e[8]*e[2]*e[17]+e[17]*e[1]*e[7]+e[17]*e[0]*e[6]+e[2]*e[10]*e[1]-1.*e[2]*e[13]*e[4]-1.*e[2]*e[16]*e[7]+.5000000000*e[11]*ep2[1]+.5000000000*e[11]*ep2[0]+1.500000000*e[11]*ep2[2]+.5000000000*e[11]*ep2[5]+.5000000000*e[11]*ep2[8]-.5000000000*e[11]*ep2[4]-.5000000000*e[11]*ep2[6]-.5000000000*e[11]*ep2[7]-.5000000000*e[11]*ep2[3]; 
    A[64]=e[5]*e[19]*e[13]+e[5]*e[10]*e[22]+e[8]*e[18]*e[15]+e[8]*e[9]*e[24]+e[8]*e[20]*e[17]+e[8]*e[11]*e[26]+e[8]*e[19]*e[16]+e[8]*e[10]*e[25]+e[2]*e[18]*e[9]+e[2]*e[19]*e[10]-1.*e[11]*e[21]*e[3]-1.*e[11]*e[22]*e[4]-1.*e[11]*e[25]*e[7]-1.*e[11]*e[24]*e[6]+e[14]*e[18]*e[3]+e[14]*e[0]*e[21]+e[14]*e[19]*e[4]+e[14]*e[1]*e[22]+e[14]*e[2]*e[23]-1.*e[20]*e[13]*e[4]-1.*e[20]*e[16]*e[7]-1.*e[20]*e[15]*e[6]-1.*e[20]*e[12]*e[3]+e[23]*e[9]*e[3]+e[23]*e[0]*e[12]+e[23]*e[10]*e[4]+e[23]*e[1]*e[13]+e[17]*e[18]*e[6]+e[17]*e[0]*e[24]+e[17]*e[19]*e[7]+e[17]*e[1]*e[25]+e[17]*e[2]*e[26]-1.*e[2]*e[24]*e[15]-1.*e[2]*e[25]*e[16]-1.*e[2]*e[21]*e[12]-1.*e[2]*e[22]*e[13]+e[26]*e[9]*e[6]+e[26]*e[0]*e[15]+e[26]*e[10]*e[7]+e[26]*e[1]*e[16]+e[11]*e[18]*e[0]+e[11]*e[19]*e[1]+3.*e[11]*e[20]*e[2]+e[20]*e[9]*e[0]+e[20]*e[10]*e[1]+e[5]*e[18]*e[12]+e[5]*e[9]*e[21]+e[5]*e[20]*e[14]+e[5]*e[11]*e[23]; 
    A[65]=e[32]*e[1]*e[4]+e[32]*e[0]*e[3]+e[8]*e[27]*e[6]+e[8]*e[0]*e[33]+e[8]*e[28]*e[7]+e[8]*e[1]*e[34]+e[35]*e[1]*e[7]+e[35]*e[0]*e[6]+e[2]*e[27]*e[0]+e[2]*e[28]*e[1]-1.*e[2]*e[34]*e[7]+e[2]*e[32]*e[5]-1.*e[2]*e[33]*e[6]-1.*e[2]*e[30]*e[3]+e[2]*e[35]*e[8]-1.*e[2]*e[31]*e[4]+e[5]*e[27]*e[3]+e[5]*e[0]*e[30]+e[5]*e[28]*e[4]+e[5]*e[1]*e[31]+1.500000000*e[29]*ep2[2]-.5000000000*e[29]*ep2[4]+.5000000000*e[29]*ep2[0]-.5000000000*e[29]*ep2[6]+.5000000000*e[29]*ep2[5]+.5000000000*e[29]*ep2[1]-.5000000000*e[29]*ep2[7]-.5000000000*e[29]*ep2[3]+.5000000000*e[29]*ep2[8]; 
    A[66]=e[5]*e[0]*e[3]+e[8]*e[1]*e[7]+e[8]*e[0]*e[6]+e[5]*e[1]*e[4]-.5000000000*e[2]*ep2[4]+.5000000000*ep3[2]+.5000000000*e[2]*ep2[1]-.5000000000*e[2]*ep2[3]+.5000000000*e[2]*ep2[0]+.5000000000*e[2]*ep2[8]+.5000000000*e[2]*ep2[5]-.5000000000*e[2]*ep2[6]-.5000000000*e[2]*ep2[7]; 
    A[67]=e[35]*e[9]*e[15]+e[35]*e[10]*e[16]-1.*e[11]*e[30]*e[12]-1.*e[11]*e[31]*e[13]-1.*e[11]*e[33]*e[15]-1.*e[11]*e[34]*e[16]+e[11]*e[27]*e[9]+e[11]*e[28]*e[10]+e[14]*e[27]*e[12]+e[14]*e[9]*e[30]+e[14]*e[11]*e[32]+e[14]*e[28]*e[13]+e[14]*e[10]*e[31]+e[32]*e[9]*e[12]+e[32]*e[10]*e[13]+e[17]*e[27]*e[15]+e[17]*e[9]*e[33]+e[17]*e[11]*e[35]+e[17]*e[28]*e[16]+e[17]*e[10]*e[34]+1.500000000*e[29]*ep2[11]-.5000000000*e[29]*ep2[16]+.5000000000*e[29]*ep2[9]-.5000000000*e[29]*ep2[12]-.5000000000*e[29]*ep2[15]+.5000000000*e[29]*ep2[17]+.5000000000*e[29]*ep2[10]+.5000000000*e[29]*ep2[14]-.5000000000*e[29]*ep2[13]; 
    A[68]=e[14]*e[9]*e[12]+e[17]*e[10]*e[16]+e[17]*e[9]*e[15]+.5000000000*ep3[11]+e[14]*e[10]*e[13]+.5000000000*e[11]*ep2[10]-.5000000000*e[11]*ep2[15]+.5000000000*e[11]*ep2[14]-.5000000000*e[11]*ep2[13]-.5000000000*e[11]*ep2[12]+.5000000000*e[11]*ep2[9]-.5000000000*e[11]*ep2[16]+.5000000000*e[11]*ep2[17]; 
    A[69]=e[20]*e[27]*e[18]+e[20]*e[28]*e[19]+e[23]*e[27]*e[21]+e[23]*e[18]*e[30]+e[23]*e[28]*e[22]+e[23]*e[19]*e[31]+e[23]*e[20]*e[32]+e[32]*e[19]*e[22]+e[32]*e[18]*e[21]+e[26]*e[27]*e[24]+e[26]*e[18]*e[33]+e[26]*e[28]*e[25]+e[26]*e[19]*e[34]+e[26]*e[20]*e[35]+e[35]*e[19]*e[25]+e[35]*e[18]*e[24]-1.*e[20]*e[33]*e[24]-1.*e[20]*e[30]*e[21]-1.*e[20]*e[31]*e[22]-1.*e[20]*e[34]*e[25]+.5000000000*e[29]*ep2[23]+.5000000000*e[29]*ep2[26]-.5000000000*e[29]*ep2[22]-.5000000000*e[29]*ep2[24]-.5000000000*e[29]*ep2[21]-.5000000000*e[29]*ep2[25]+1.500000000*e[29]*ep2[20]+.5000000000*e[29]*ep2[19]+.5000000000*e[29]*ep2[18]; 
    A[70]=.5000000000*e[20]*ep2[26]+.5000000000*e[20]*ep2[18]+.5000000000*ep3[20]+.5000000000*e[20]*ep2[19]+e[26]*e[18]*e[24]+.5000000000*e[20]*ep2[23]-.5000000000*e[20]*ep2[25]+e[23]*e[19]*e[22]-.5000000000*e[20]*ep2[24]-.5000000000*e[20]*ep2[21]-.5000000000*e[20]*ep2[22]+e[23]*e[18]*e[21]+e[26]*e[19]*e[25]; 
    A[71]=e[8]*e[28]*e[16]+e[8]*e[10]*e[34]+e[2]*e[27]*e[9]+3.*e[2]*e[29]*e[11]+e[2]*e[28]*e[10]+e[11]*e[27]*e[0]-1.*e[11]*e[34]*e[7]-1.*e[11]*e[33]*e[6]-1.*e[11]*e[30]*e[3]+e[11]*e[28]*e[1]-1.*e[11]*e[31]*e[4]+e[14]*e[27]*e[3]+e[14]*e[0]*e[30]+e[14]*e[28]*e[4]+e[14]*e[1]*e[31]+e[14]*e[2]*e[32]+e[29]*e[10]*e[1]-1.*e[29]*e[13]*e[4]-1.*e[29]*e[16]*e[7]-1.*e[29]*e[15]*e[6]+e[29]*e[9]*e[0]-1.*e[29]*e[12]*e[3]+e[32]*e[9]*e[3]+e[32]*e[0]*e[12]+e[32]*e[10]*e[4]+e[32]*e[1]*e[13]+e[17]*e[27]*e[6]+e[17]*e[0]*e[33]+e[17]*e[28]*e[7]+e[17]*e[1]*e[34]+e[17]*e[2]*e[35]-1.*e[2]*e[30]*e[12]-1.*e[2]*e[31]*e[13]-1.*e[2]*e[33]*e[15]-1.*e[2]*e[34]*e[16]+e[35]*e[9]*e[6]+e[35]*e[0]*e[15]+e[35]*e[10]*e[7]+e[35]*e[1]*e[16]+e[5]*e[27]*e[12]+e[5]*e[9]*e[30]+e[5]*e[29]*e[14]+e[5]*e[11]*e[32]+e[5]*e[28]*e[13]+e[5]*e[10]*e[31]+e[8]*e[27]*e[15]+e[8]*e[9]*e[33]+e[8]*e[29]*e[17]+e[8]*e[11]*e[35]; 
    A[91]=-1.*e[12]*e[34]*e[7]+e[12]*e[32]*e[5]-1.*e[12]*e[35]*e[8]-1.*e[12]*e[29]*e[2]-1.*e[12]*e[28]*e[1]+e[12]*e[31]*e[4]-1.*e[30]*e[11]*e[2]-1.*e[30]*e[10]*e[1]+e[30]*e[13]*e[4]-1.*e[30]*e[16]*e[7]+e[30]*e[14]*e[5]-1.*e[30]*e[17]*e[8]+e[15]*e[3]*e[33]+e[15]*e[31]*e[7]+e[15]*e[4]*e[34]+e[15]*e[32]*e[8]+e[15]*e[5]*e[35]+e[3]*e[27]*e[9]-1.*e[3]*e[28]*e[10]-1.*e[3]*e[34]*e[16]-1.*e[3]*e[35]*e[17]-1.*e[3]*e[29]*e[11]+e[33]*e[13]*e[7]+e[33]*e[4]*e[16]+e[33]*e[14]*e[8]+e[33]*e[5]*e[17]+e[9]*e[28]*e[4]+e[9]*e[1]*e[31]+e[9]*e[29]*e[5]+e[9]*e[2]*e[32]+e[27]*e[10]*e[4]+e[27]*e[1]*e[13]+e[27]*e[11]*e[5]+e[27]*e[2]*e[14]+3.*e[3]*e[30]*e[12]+e[3]*e[32]*e[14]+e[3]*e[31]*e[13]+e[6]*e[30]*e[15]+e[6]*e[12]*e[33]+e[6]*e[32]*e[17]+e[6]*e[14]*e[35]+e[6]*e[31]*e[16]+e[6]*e[13]*e[34]+e[0]*e[27]*e[12]+e[0]*e[9]*e[30]+e[0]*e[29]*e[14]+e[0]*e[11]*e[32]+e[0]*e[28]*e[13]+e[0]*e[10]*e[31]; 
    A[90]=.5000000000*e[21]*ep2[24]-.5000000000*e[21]*ep2[25]+.5000000000*e[21]*ep2[23]-.5000000000*e[21]*ep2[26]+.5000000000*ep2[18]*e[21]+.5000000000*e[21]*ep2[22]-.5000000000*e[21]*ep2[20]+e[24]*e[22]*e[25]+e[24]*e[23]*e[26]-.5000000000*e[21]*ep2[19]+e[18]*e[19]*e[22]+e[18]*e[20]*e[23]+.5000000000*ep3[21]; 
    A[89]=-.5000000000*e[30]*ep2[26]-.5000000000*e[30]*ep2[19]-.5000000000*e[30]*ep2[20]-.5000000000*e[30]*ep2[25]+.5000000000*ep2[18]*e[30]+1.500000000*e[30]*ep2[21]+.5000000000*e[30]*ep2[22]+.5000000000*e[30]*ep2[23]+.5000000000*e[30]*ep2[24]+e[18]*e[27]*e[21]+e[18]*e[28]*e[22]+e[18]*e[19]*e[31]+e[18]*e[29]*e[23]+e[18]*e[20]*e[32]+e[27]*e[19]*e[22]+e[27]*e[20]*e[23]+e[21]*e[31]*e[22]+e[21]*e[32]*e[23]+e[24]*e[21]*e[33]+e[24]*e[31]*e[25]+e[24]*e[22]*e[34]+e[24]*e[32]*e[26]+e[24]*e[23]*e[35]+e[33]*e[22]*e[25]+e[33]*e[23]*e[26]-1.*e[21]*e[29]*e[20]-1.*e[21]*e[35]*e[26]-1.*e[21]*e[28]*e[19]-1.*e[21]*e[34]*e[25]; 
    A[88]=.5000000000*e[12]*ep2[15]-.5000000000*e[12]*ep2[17]+e[15]*e[13]*e[16]-.5000000000*e[12]*ep2[10]+e[15]*e[14]*e[17]-.5000000000*e[12]*ep2[16]-.5000000000*e[12]*ep2[11]+e[9]*e[10]*e[13]+.5000000000*e[12]*ep2[13]+.5000000000*ep2[9]*e[12]+.5000000000*ep3[12]+e[9]*e[11]*e[14]+.5000000000*e[12]*ep2[14]; 
    A[95]=e[12]*e[13]*e[4]+e[12]*e[14]*e[5]+e[15]*e[12]*e[6]+e[15]*e[13]*e[7]+e[15]*e[4]*e[16]+e[15]*e[14]*e[8]+e[15]*e[5]*e[17]+e[6]*e[14]*e[17]+e[6]*e[13]*e[16]+e[0]*e[11]*e[14]+e[0]*e[9]*e[12]+e[0]*e[10]*e[13]+e[9]*e[10]*e[4]+e[9]*e[1]*e[13]+e[9]*e[11]*e[5]+e[9]*e[2]*e[14]-1.*e[12]*e[11]*e[2]-1.*e[12]*e[10]*e[1]-1.*e[12]*e[16]*e[7]-1.*e[12]*e[17]*e[8]+1.500000000*ep2[12]*e[3]+.5000000000*e[3]*ep2[15]-.5000000000*e[3]*ep2[16]+.5000000000*e[3]*ep2[9]-.5000000000*e[3]*ep2[11]-.5000000000*e[3]*ep2[17]-.5000000000*e[3]*ep2[10]+.5000000000*e[3]*ep2[14]+.5000000000*e[3]*ep2[13]; 
    A[94]=e[18]*e[11]*e[14]+e[18]*e[9]*e[12]+e[18]*e[10]*e[13]+e[12]*e[23]*e[14]+e[12]*e[22]*e[13]+e[15]*e[12]*e[24]+e[15]*e[23]*e[17]+e[15]*e[14]*e[26]+e[15]*e[22]*e[16]+e[15]*e[13]*e[25]+e[24]*e[14]*e[17]+e[24]*e[13]*e[16]-1.*e[12]*e[25]*e[16]-1.*e[12]*e[26]*e[17]-1.*e[12]*e[20]*e[11]-1.*e[12]*e[19]*e[10]+e[9]*e[20]*e[14]+e[9]*e[11]*e[23]+e[9]*e[19]*e[13]+e[9]*e[10]*e[22]+.5000000000*ep2[9]*e[21]-.5000000000*e[21]*ep2[16]-.5000000000*e[21]*ep2[11]-.5000000000*e[21]*ep2[17]-.5000000000*e[21]*ep2[10]+1.500000000*e[21]*ep2[12]+.5000000000*e[21]*ep2[14]+.5000000000*e[21]*ep2[13]+.5000000000*e[21]*ep2[15]; 
    A[93]=-1.*e[21]*e[35]*e[8]-1.*e[21]*e[29]*e[2]-1.*e[21]*e[28]*e[1]+e[21]*e[31]*e[4]-1.*e[30]*e[26]*e[8]-1.*e[30]*e[20]*e[2]-1.*e[30]*e[19]*e[1]+e[30]*e[22]*e[4]-1.*e[30]*e[25]*e[7]+e[30]*e[23]*e[5]+e[6]*e[31]*e[25]+e[6]*e[22]*e[34]+e[6]*e[32]*e[26]+e[6]*e[23]*e[35]+e[24]*e[30]*e[6]+e[24]*e[3]*e[33]+e[24]*e[31]*e[7]+e[24]*e[4]*e[34]+e[24]*e[32]*e[8]+e[24]*e[5]*e[35]+e[33]*e[21]*e[6]+e[33]*e[22]*e[7]+e[33]*e[4]*e[25]+e[33]*e[23]*e[8]+e[33]*e[5]*e[26]+e[0]*e[27]*e[21]+e[0]*e[18]*e[30]+e[0]*e[28]*e[22]+e[0]*e[19]*e[31]+e[0]*e[29]*e[23]+e[0]*e[20]*e[32]+e[18]*e[27]*e[3]+e[18]*e[28]*e[4]+e[18]*e[1]*e[31]+e[18]*e[29]*e[5]+e[18]*e[2]*e[32]+e[27]*e[19]*e[4]+e[27]*e[1]*e[22]+e[27]*e[20]*e[5]+e[27]*e[2]*e[23]+3.*e[3]*e[30]*e[21]+e[3]*e[31]*e[22]+e[3]*e[32]*e[23]-1.*e[3]*e[29]*e[20]-1.*e[3]*e[35]*e[26]-1.*e[3]*e[28]*e[19]-1.*e[3]*e[34]*e[25]-1.*e[21]*e[34]*e[7]+e[21]*e[32]*e[5]; 
    A[92]=e[18]*e[1]*e[4]+e[18]*e[0]*e[3]+e[18]*e[2]*e[5]+e[3]*e[22]*e[4]+e[3]*e[23]*e[5]+e[6]*e[3]*e[24]+e[6]*e[22]*e[7]+e[6]*e[4]*e[25]+e[6]*e[23]*e[8]+e[6]*e[5]*e[26]+e[24]*e[4]*e[7]+e[24]*e[5]*e[8]+e[0]*e[19]*e[4]+e[0]*e[1]*e[22]+e[0]*e[20]*e[5]+e[0]*e[2]*e[23]-1.*e[3]*e[26]*e[8]-1.*e[3]*e[20]*e[2]-1.*e[3]*e[19]*e[1]-1.*e[3]*e[25]*e[7]+.5000000000*e[21]*ep2[4]+.5000000000*e[21]*ep2[0]+.5000000000*e[21]*ep2[6]+.5000000000*e[21]*ep2[5]-.5000000000*e[21]*ep2[1]-.5000000000*e[21]*ep2[7]+1.500000000*e[21]*ep2[3]-.5000000000*e[21]*ep2[2]-.5000000000*e[21]*ep2[8]; 
    A[82]=.5000000000*ep2[27]*e[21]+1.500000000*e[21]*ep2[30]+.5000000000*e[21]*ep2[32]+.5000000000*e[21]*ep2[31]+.5000000000*e[21]*ep2[33]-.5000000000*e[21]*ep2[28]-.5000000000*e[21]*ep2[29]-.5000000000*e[21]*ep2[34]-.5000000000*e[21]*ep2[35]+e[18]*e[27]*e[30]+e[18]*e[29]*e[32]+e[18]*e[28]*e[31]+e[27]*e[28]*e[22]+e[27]*e[19]*e[31]+e[27]*e[29]*e[23]+e[27]*e[20]*e[32]+e[30]*e[31]*e[22]+e[30]*e[32]*e[23]+e[24]*e[30]*e[33]+e[24]*e[32]*e[35]+e[24]*e[31]*e[34]+e[33]*e[31]*e[25]+e[33]*e[22]*e[34]+e[33]*e[32]*e[26]+e[33]*e[23]*e[35]-1.*e[30]*e[29]*e[20]-1.*e[30]*e[35]*e[26]-1.*e[30]*e[28]*e[19]-1.*e[30]*e[34]*e[25]; 
    A[192]=-.5000000000*e[26]*ep2[4]-.5000000000*e[26]*ep2[0]+.5000000000*e[26]*ep2[6]+.5000000000*e[26]*ep2[5]-.5000000000*e[26]*ep2[1]+.5000000000*e[26]*ep2[7]-.5000000000*e[26]*ep2[3]+.5000000000*e[26]*ep2[2]+1.500000000*e[26]*ep2[8]+e[20]*e[0]*e[6]+e[20]*e[2]*e[8]+e[5]*e[21]*e[6]+e[5]*e[3]*e[24]+e[5]*e[22]*e[7]+e[5]*e[4]*e[25]+e[5]*e[23]*e[8]+e[23]*e[4]*e[7]+e[23]*e[3]*e[6]+e[8]*e[24]*e[6]+e[8]*e[25]*e[7]+e[2]*e[18]*e[6]+e[2]*e[0]*e[24]+e[2]*e[19]*e[7]+e[2]*e[1]*e[25]-1.*e[8]*e[21]*e[3]-1.*e[8]*e[19]*e[1]-1.*e[8]*e[22]*e[4]-1.*e[8]*e[18]*e[0]+e[20]*e[1]*e[7]; 
    A[83]=e[9]*e[27]*e[30]+e[9]*e[29]*e[32]+e[9]*e[28]*e[31]+e[33]*e[30]*e[15]+e[33]*e[32]*e[17]+e[33]*e[14]*e[35]+e[33]*e[31]*e[16]+e[33]*e[13]*e[34]+e[27]*e[29]*e[14]+e[27]*e[11]*e[32]+e[27]*e[28]*e[13]+e[27]*e[10]*e[31]-1.*e[30]*e[28]*e[10]+e[30]*e[31]*e[13]+e[30]*e[32]*e[14]-1.*e[30]*e[34]*e[16]-1.*e[30]*e[35]*e[17]-1.*e[30]*e[29]*e[11]+e[15]*e[32]*e[35]+e[15]*e[31]*e[34]-.5000000000*e[12]*ep2[34]-.5000000000*e[12]*ep2[35]+.5000000000*e[12]*ep2[27]+.5000000000*e[12]*ep2[32]-.5000000000*e[12]*ep2[28]-.5000000000*e[12]*ep2[29]+.5000000000*e[12]*ep2[31]+.5000000000*e[12]*ep2[33]+1.500000000*e[12]*ep2[30]; 
    A[193]=e[23]*e[30]*e[6]+e[23]*e[3]*e[33]+e[23]*e[31]*e[7]+e[23]*e[4]*e[34]+e[32]*e[21]*e[6]+e[32]*e[3]*e[24]+e[32]*e[22]*e[7]+e[32]*e[4]*e[25]+e[26]*e[33]*e[6]+e[26]*e[34]*e[7]+3.*e[26]*e[35]*e[8]+e[35]*e[24]*e[6]+e[35]*e[25]*e[7]+e[2]*e[27]*e[24]+e[2]*e[18]*e[33]+e[2]*e[28]*e[25]+e[2]*e[19]*e[34]+e[2]*e[29]*e[26]+e[2]*e[20]*e[35]+e[20]*e[27]*e[6]+e[20]*e[0]*e[33]+e[20]*e[28]*e[7]+e[20]*e[1]*e[34]+e[20]*e[29]*e[8]+e[29]*e[18]*e[6]+e[29]*e[0]*e[24]+e[29]*e[19]*e[7]+e[29]*e[1]*e[25]+e[5]*e[30]*e[24]+e[5]*e[21]*e[33]+e[5]*e[31]*e[25]+e[5]*e[22]*e[34]+e[5]*e[32]*e[26]+e[5]*e[23]*e[35]-1.*e[8]*e[27]*e[18]+e[8]*e[33]*e[24]-1.*e[8]*e[30]*e[21]-1.*e[8]*e[31]*e[22]+e[8]*e[32]*e[23]-1.*e[8]*e[28]*e[19]+e[8]*e[34]*e[25]-1.*e[26]*e[27]*e[0]-1.*e[26]*e[30]*e[3]-1.*e[26]*e[28]*e[1]-1.*e[26]*e[31]*e[4]-1.*e[35]*e[21]*e[3]-1.*e[35]*e[19]*e[1]-1.*e[35]*e[22]*e[4]-1.*e[35]*e[18]*e[0]; 
    A[80]=e[27]*e[29]*e[32]+e[27]*e[28]*e[31]+e[33]*e[32]*e[35]+e[33]*e[31]*e[34]+.5000000000*ep3[30]-.5000000000*e[30]*ep2[28]-.5000000000*e[30]*ep2[29]-.5000000000*e[30]*ep2[34]+.5000000000*e[30]*ep2[33]+.5000000000*ep2[27]*e[30]+.5000000000*e[30]*ep2[32]+.5000000000*e[30]*ep2[31]-.5000000000*e[30]*ep2[35]; 
    A[194]=.5000000000*ep2[14]*e[26]+1.500000000*e[26]*ep2[17]+.5000000000*e[26]*ep2[15]+.5000000000*e[26]*ep2[16]+.5000000000*ep2[11]*e[26]-.5000000000*e[26]*ep2[9]-.5000000000*e[26]*ep2[12]-.5000000000*e[26]*ep2[10]-.5000000000*e[26]*ep2[13]+e[20]*e[11]*e[17]+e[20]*e[9]*e[15]+e[20]*e[10]*e[16]+e[14]*e[21]*e[15]+e[14]*e[12]*e[24]+e[14]*e[23]*e[17]+e[14]*e[22]*e[16]+e[14]*e[13]*e[25]+e[23]*e[12]*e[15]+e[23]*e[13]*e[16]+e[17]*e[24]*e[15]+e[17]*e[25]*e[16]-1.*e[17]*e[18]*e[9]-1.*e[17]*e[21]*e[12]-1.*e[17]*e[19]*e[10]-1.*e[17]*e[22]*e[13]+e[11]*e[18]*e[15]+e[11]*e[9]*e[24]+e[11]*e[19]*e[16]+e[11]*e[10]*e[25]; 
    A[81]=e[0]*e[27]*e[30]+e[0]*e[29]*e[32]+e[0]*e[28]*e[31]+e[30]*e[31]*e[4]+e[30]*e[32]*e[5]+e[6]*e[30]*e[33]+e[6]*e[32]*e[35]+e[6]*e[31]*e[34]+e[27]*e[28]*e[4]+e[27]*e[1]*e[31]+e[27]*e[29]*e[5]+e[27]*e[2]*e[32]+e[33]*e[31]*e[7]+e[33]*e[4]*e[34]+e[33]*e[32]*e[8]+e[33]*e[5]*e[35]-1.*e[30]*e[34]*e[7]-1.*e[30]*e[35]*e[8]-1.*e[30]*e[29]*e[2]-1.*e[30]*e[28]*e[1]+1.500000000*e[3]*ep2[30]+.5000000000*e[3]*ep2[32]+.5000000000*e[3]*ep2[31]+.5000000000*e[3]*ep2[27]-.5000000000*e[3]*ep2[28]-.5000000000*e[3]*ep2[29]+.5000000000*e[3]*ep2[33]-.5000000000*e[3]*ep2[34]-.5000000000*e[3]*ep2[35]; 
    A[195]=.5000000000*ep2[14]*e[8]+1.500000000*ep2[17]*e[8]+.5000000000*e[8]*ep2[15]+.5000000000*e[8]*ep2[16]-.5000000000*e[8]*ep2[9]+.5000000000*e[8]*ep2[11]-.5000000000*e[8]*ep2[12]-.5000000000*e[8]*ep2[10]-.5000000000*e[8]*ep2[13]+e[14]*e[12]*e[6]+e[14]*e[3]*e[15]+e[14]*e[13]*e[7]+e[14]*e[4]*e[16]+e[14]*e[5]*e[17]+e[17]*e[15]*e[6]+e[17]*e[16]*e[7]+e[2]*e[11]*e[17]+e[2]*e[9]*e[15]+e[2]*e[10]*e[16]+e[5]*e[12]*e[15]+e[5]*e[13]*e[16]+e[11]*e[9]*e[6]+e[11]*e[0]*e[15]+e[11]*e[10]*e[7]+e[11]*e[1]*e[16]-1.*e[17]*e[10]*e[1]-1.*e[17]*e[13]*e[4]-1.*e[17]*e[9]*e[0]-1.*e[17]*e[12]*e[3]; 
    A[86]=-.5000000000*e[3]*ep2[1]-.5000000000*e[3]*ep2[7]+.5000000000*ep3[3]-.5000000000*e[3]*ep2[8]+e[0]*e[2]*e[5]+.5000000000*e[3]*ep2[6]+.5000000000*e[3]*ep2[4]-.5000000000*e[3]*ep2[2]+e[0]*e[1]*e[4]+e[6]*e[4]*e[7]+.5000000000*ep2[0]*e[3]+.5000000000*e[3]*ep2[5]+e[6]*e[5]*e[8]; 
    A[196]=.5000000000*ep2[23]*e[17]+1.500000000*ep2[26]*e[17]+.5000000000*e[17]*ep2[25]+.5000000000*e[17]*ep2[24]-.5000000000*e[17]*ep2[18]-.5000000000*e[17]*ep2[19]+.5000000000*e[17]*ep2[20]-.5000000000*e[17]*ep2[22]-.5000000000*e[17]*ep2[21]+e[23]*e[21]*e[15]+e[23]*e[12]*e[24]+e[23]*e[14]*e[26]+e[23]*e[22]*e[16]+e[23]*e[13]*e[25]+e[26]*e[24]*e[15]+e[26]*e[25]*e[16]+e[11]*e[19]*e[25]+e[11]*e[18]*e[24]+e[11]*e[20]*e[26]+e[14]*e[22]*e[25]+e[14]*e[21]*e[24]+e[20]*e[18]*e[15]+e[20]*e[9]*e[24]+e[20]*e[19]*e[16]+e[20]*e[10]*e[25]-1.*e[26]*e[18]*e[9]-1.*e[26]*e[21]*e[12]-1.*e[26]*e[19]*e[10]-1.*e[26]*e[22]*e[13]; 
    A[87]=-1.*e[12]*e[34]*e[16]-1.*e[12]*e[35]*e[17]-1.*e[12]*e[29]*e[11]+e[9]*e[27]*e[12]+e[9]*e[29]*e[14]+e[9]*e[11]*e[32]+e[9]*e[28]*e[13]+e[9]*e[10]*e[31]+e[27]*e[11]*e[14]+e[27]*e[10]*e[13]+e[12]*e[32]*e[14]+e[12]*e[31]*e[13]+e[15]*e[12]*e[33]+e[15]*e[32]*e[17]+e[15]*e[14]*e[35]+e[15]*e[31]*e[16]+e[15]*e[13]*e[34]+e[33]*e[14]*e[17]+e[33]*e[13]*e[16]-1.*e[12]*e[28]*e[10]+.5000000000*ep2[9]*e[30]-.5000000000*e[30]*ep2[16]-.5000000000*e[30]*ep2[11]+1.500000000*e[30]*ep2[12]+.5000000000*e[30]*ep2[15]-.5000000000*e[30]*ep2[17]-.5000000000*e[30]*ep2[10]+.5000000000*e[30]*ep2[14]+.5000000000*e[30]*ep2[13]; 
    A[197]=e[32]*e[22]*e[16]+e[32]*e[13]*e[25]-1.*e[17]*e[27]*e[18]+e[17]*e[33]*e[24]-1.*e[17]*e[30]*e[21]+e[17]*e[29]*e[20]+3.*e[17]*e[35]*e[26]-1.*e[17]*e[31]*e[22]-1.*e[17]*e[28]*e[19]+e[17]*e[34]*e[25]+e[20]*e[27]*e[15]+e[20]*e[9]*e[33]+e[20]*e[28]*e[16]+e[20]*e[10]*e[34]+e[29]*e[18]*e[15]+e[29]*e[9]*e[24]+e[29]*e[19]*e[16]+e[29]*e[10]*e[25]-1.*e[26]*e[27]*e[9]-1.*e[26]*e[30]*e[12]-1.*e[26]*e[28]*e[10]-1.*e[26]*e[31]*e[13]+e[26]*e[33]*e[15]+e[26]*e[34]*e[16]+e[35]*e[24]*e[15]+e[35]*e[25]*e[16]-1.*e[35]*e[18]*e[9]-1.*e[35]*e[21]*e[12]-1.*e[35]*e[19]*e[10]-1.*e[35]*e[22]*e[13]+e[14]*e[30]*e[24]+e[14]*e[21]*e[33]+e[14]*e[31]*e[25]+e[14]*e[22]*e[34]+e[14]*e[32]*e[26]+e[14]*e[23]*e[35]+e[11]*e[27]*e[24]+e[11]*e[18]*e[33]+e[11]*e[28]*e[25]+e[11]*e[19]*e[34]+e[11]*e[29]*e[26]+e[11]*e[20]*e[35]+e[23]*e[30]*e[15]+e[23]*e[12]*e[33]+e[23]*e[32]*e[17]+e[23]*e[31]*e[16]+e[23]*e[13]*e[34]+e[32]*e[21]*e[15]+e[32]*e[12]*e[24]; 
    A[84]=e[6]*e[23]*e[17]+e[6]*e[14]*e[26]+e[6]*e[22]*e[16]+e[6]*e[13]*e[25]+e[0]*e[20]*e[14]+e[0]*e[11]*e[23]+e[0]*e[19]*e[13]+e[0]*e[10]*e[22]-1.*e[12]*e[26]*e[8]-1.*e[12]*e[20]*e[2]-1.*e[12]*e[19]*e[1]+e[12]*e[22]*e[4]-1.*e[12]*e[25]*e[7]+e[12]*e[23]*e[5]-1.*e[21]*e[11]*e[2]-1.*e[21]*e[10]*e[1]+e[21]*e[13]*e[4]-1.*e[21]*e[16]*e[7]+e[21]*e[14]*e[5]-1.*e[21]*e[17]*e[8]+e[15]*e[3]*e[24]+e[15]*e[22]*e[7]+e[15]*e[4]*e[25]+e[15]*e[23]*e[8]+e[15]*e[5]*e[26]-1.*e[3]*e[25]*e[16]-1.*e[3]*e[26]*e[17]-1.*e[3]*e[20]*e[11]-1.*e[3]*e[19]*e[10]+e[24]*e[13]*e[7]+e[24]*e[4]*e[16]+e[24]*e[14]*e[8]+e[24]*e[5]*e[17]+e[9]*e[18]*e[3]+e[9]*e[0]*e[21]+e[9]*e[19]*e[4]+e[9]*e[1]*e[22]+e[9]*e[20]*e[5]+e[9]*e[2]*e[23]+e[18]*e[0]*e[12]+e[18]*e[10]*e[4]+e[18]*e[1]*e[13]+e[18]*e[11]*e[5]+e[18]*e[2]*e[14]+3.*e[3]*e[21]*e[12]+e[3]*e[23]*e[14]+e[3]*e[22]*e[13]+e[6]*e[21]*e[15]+e[6]*e[12]*e[24]; 
    A[198]=.5000000000*ep2[5]*e[17]+1.500000000*e[17]*ep2[8]+.5000000000*e[17]*ep2[7]+.5000000000*e[17]*ep2[6]+.5000000000*ep2[2]*e[17]-.5000000000*e[17]*ep2[4]-.5000000000*e[17]*ep2[0]-.5000000000*e[17]*ep2[1]-.5000000000*e[17]*ep2[3]+e[11]*e[1]*e[7]+e[11]*e[0]*e[6]+e[11]*e[2]*e[8]+e[5]*e[12]*e[6]+e[5]*e[3]*e[15]+e[5]*e[13]*e[7]+e[5]*e[4]*e[16]+e[5]*e[14]*e[8]+e[14]*e[4]*e[7]+e[14]*e[3]*e[6]+e[8]*e[15]*e[6]+e[8]*e[16]*e[7]-1.*e[8]*e[10]*e[1]-1.*e[8]*e[13]*e[4]-1.*e[8]*e[9]*e[0]-1.*e[8]*e[12]*e[3]+e[2]*e[9]*e[6]+e[2]*e[0]*e[15]+e[2]*e[10]*e[7]+e[2]*e[1]*e[16]; 
    A[85]=e[6]*e[4]*e[34]+e[6]*e[32]*e[8]+e[6]*e[5]*e[35]+e[33]*e[4]*e[7]+e[33]*e[5]*e[8]+e[0]*e[27]*e[3]+e[0]*e[28]*e[4]+e[0]*e[1]*e[31]+e[0]*e[29]*e[5]+e[0]*e[2]*e[32]-1.*e[3]*e[34]*e[7]+e[3]*e[32]*e[5]+e[3]*e[33]*e[6]-1.*e[3]*e[35]*e[8]-1.*e[3]*e[29]*e[2]-1.*e[3]*e[28]*e[1]+e[3]*e[31]*e[4]+e[27]*e[1]*e[4]+e[27]*e[2]*e[5]+e[6]*e[31]*e[7]+.5000000000*e[30]*ep2[4]+.5000000000*e[30]*ep2[6]+.5000000000*e[30]*ep2[5]-.5000000000*e[30]*ep2[1]-.5000000000*e[30]*ep2[7]-.5000000000*e[30]*ep2[2]-.5000000000*e[30]*ep2[8]+.5000000000*ep2[0]*e[30]+1.500000000*e[30]*ep2[3]; 
    A[199]=.5000000000*ep2[23]*e[8]+1.500000000*ep2[26]*e[8]-.5000000000*e[8]*ep2[18]-.5000000000*e[8]*ep2[19]-.5000000000*e[8]*ep2[22]+.5000000000*e[8]*ep2[24]-.5000000000*e[8]*ep2[21]+.5000000000*e[8]*ep2[25]+.5000000000*ep2[20]*e[8]+e[20]*e[18]*e[6]+e[20]*e[0]*e[24]+e[20]*e[19]*e[7]+e[20]*e[1]*e[25]+e[20]*e[2]*e[26]+e[23]*e[21]*e[6]+e[23]*e[3]*e[24]+e[23]*e[22]*e[7]+e[23]*e[4]*e[25]+e[23]*e[5]*e[26]-1.*e[26]*e[21]*e[3]-1.*e[26]*e[19]*e[1]-1.*e[26]*e[22]*e[4]-1.*e[26]*e[18]*e[0]+e[26]*e[25]*e[7]+e[26]*e[24]*e[6]+e[2]*e[19]*e[25]+e[2]*e[18]*e[24]+e[5]*e[22]*e[25]+e[5]*e[21]*e[24]; 
    A[109]=e[19]*e[27]*e[21]+e[19]*e[18]*e[30]+e[19]*e[28]*e[22]+e[19]*e[29]*e[23]+e[19]*e[20]*e[32]+e[28]*e[18]*e[21]+e[28]*e[20]*e[23]+e[22]*e[30]*e[21]+e[22]*e[32]*e[23]+e[25]*e[30]*e[24]+e[25]*e[21]*e[33]+e[25]*e[22]*e[34]+e[25]*e[32]*e[26]+e[25]*e[23]*e[35]+e[34]*e[21]*e[24]+e[34]*e[23]*e[26]-1.*e[22]*e[27]*e[18]-1.*e[22]*e[33]*e[24]-1.*e[22]*e[29]*e[20]-1.*e[22]*e[35]*e[26]+.5000000000*ep2[19]*e[31]+1.500000000*e[31]*ep2[22]+.5000000000*e[31]*ep2[21]+.5000000000*e[31]*ep2[23]+.5000000000*e[31]*ep2[25]-.5000000000*e[31]*ep2[26]-.5000000000*e[31]*ep2[18]-.5000000000*e[31]*ep2[20]-.5000000000*e[31]*ep2[24]; 
    A[108]=-.5000000000*e[13]*ep2[15]+.5000000000*e[13]*ep2[16]+.5000000000*e[13]*ep2[12]+e[16]*e[12]*e[15]+.5000000000*ep3[13]+e[10]*e[11]*e[14]+.5000000000*e[13]*ep2[14]-.5000000000*e[13]*ep2[17]-.5000000000*e[13]*ep2[11]-.5000000000*e[13]*ep2[9]+.5000000000*ep2[10]*e[13]+e[10]*e[9]*e[12]+e[16]*e[14]*e[17]; 
    A[111]=-1.*e[13]*e[29]*e[2]-1.*e[31]*e[11]*e[2]-1.*e[31]*e[15]*e[6]-1.*e[31]*e[9]*e[0]+e[31]*e[14]*e[5]+e[31]*e[12]*e[3]-1.*e[31]*e[17]*e[8]+e[16]*e[30]*e[6]+e[16]*e[3]*e[33]+e[16]*e[4]*e[34]+e[16]*e[32]*e[8]+e[16]*e[5]*e[35]-1.*e[4]*e[27]*e[9]+e[4]*e[28]*e[10]-1.*e[4]*e[33]*e[15]-1.*e[4]*e[35]*e[17]-1.*e[4]*e[29]*e[11]+e[34]*e[12]*e[6]+e[34]*e[3]*e[15]+e[34]*e[14]*e[8]+e[34]*e[5]*e[17]+e[10]*e[27]*e[3]+e[10]*e[0]*e[30]+e[10]*e[29]*e[5]+e[10]*e[2]*e[32]+e[28]*e[9]*e[3]+e[28]*e[0]*e[12]+e[28]*e[11]*e[5]+e[28]*e[2]*e[14]+e[4]*e[30]*e[12]+e[4]*e[32]*e[14]+3.*e[4]*e[31]*e[13]+e[7]*e[30]*e[15]+e[7]*e[12]*e[33]+e[7]*e[32]*e[17]+e[7]*e[14]*e[35]+e[7]*e[31]*e[16]+e[7]*e[13]*e[34]+e[1]*e[27]*e[12]+e[1]*e[9]*e[30]+e[1]*e[29]*e[14]+e[1]*e[11]*e[32]+e[1]*e[28]*e[13]+e[1]*e[10]*e[31]-1.*e[13]*e[27]*e[0]+e[13]*e[32]*e[5]-1.*e[13]*e[33]*e[6]+e[13]*e[30]*e[3]-1.*e[13]*e[35]*e[8]; 
    A[110]=e[25]*e[23]*e[26]+e[19]*e[20]*e[23]+e[19]*e[18]*e[21]+e[25]*e[21]*e[24]+.5000000000*ep3[22]+.5000000000*e[22]*ep2[23]+.5000000000*ep2[19]*e[22]-.5000000000*e[22]*ep2[18]-.5000000000*e[22]*ep2[24]+.5000000000*e[22]*ep2[21]+.5000000000*e[22]*ep2[25]-.5000000000*e[22]*ep2[20]-.5000000000*e[22]*ep2[26]; 
    A[105]=e[34]*e[5]*e[8]+e[1]*e[27]*e[3]+e[1]*e[0]*e[30]+e[1]*e[28]*e[4]+e[1]*e[29]*e[5]+e[1]*e[2]*e[32]-1.*e[4]*e[27]*e[0]+e[4]*e[34]*e[7]+e[4]*e[32]*e[5]-1.*e[4]*e[33]*e[6]+e[4]*e[30]*e[3]-1.*e[4]*e[35]*e[8]-1.*e[4]*e[29]*e[2]+e[28]*e[0]*e[3]+e[28]*e[2]*e[5]+e[7]*e[30]*e[6]+e[7]*e[3]*e[33]+e[7]*e[32]*e[8]+e[7]*e[5]*e[35]+e[34]*e[3]*e[6]+.5000000000*ep2[1]*e[31]+1.500000000*e[31]*ep2[4]-.5000000000*e[31]*ep2[0]-.5000000000*e[31]*ep2[6]+.5000000000*e[31]*ep2[5]+.5000000000*e[31]*ep2[7]+.5000000000*e[31]*ep2[3]-.5000000000*e[31]*ep2[2]-.5000000000*e[31]*ep2[8]; 
    A[104]=e[1]*e[20]*e[14]+e[1]*e[11]*e[23]+e[13]*e[21]*e[3]-1.*e[13]*e[26]*e[8]-1.*e[13]*e[20]*e[2]-1.*e[13]*e[18]*e[0]+e[13]*e[23]*e[5]-1.*e[13]*e[24]*e[6]-1.*e[22]*e[11]*e[2]-1.*e[22]*e[15]*e[6]-1.*e[22]*e[9]*e[0]+e[22]*e[14]*e[5]+e[22]*e[12]*e[3]-1.*e[22]*e[17]*e[8]+e[16]*e[21]*e[6]+e[16]*e[3]*e[24]+e[16]*e[4]*e[25]+e[16]*e[23]*e[8]+e[16]*e[5]*e[26]-1.*e[4]*e[24]*e[15]-1.*e[4]*e[26]*e[17]-1.*e[4]*e[20]*e[11]-1.*e[4]*e[18]*e[9]+e[25]*e[12]*e[6]+e[25]*e[3]*e[15]+e[25]*e[14]*e[8]+e[25]*e[5]*e[17]+e[10]*e[18]*e[3]+e[10]*e[0]*e[21]+e[10]*e[19]*e[4]+e[10]*e[1]*e[22]+e[10]*e[20]*e[5]+e[10]*e[2]*e[23]+e[19]*e[9]*e[3]+e[19]*e[0]*e[12]+e[19]*e[1]*e[13]+e[19]*e[11]*e[5]+e[19]*e[2]*e[14]+e[4]*e[21]*e[12]+e[4]*e[23]*e[14]+3.*e[4]*e[22]*e[13]+e[7]*e[21]*e[15]+e[7]*e[12]*e[24]+e[7]*e[23]*e[17]+e[7]*e[14]*e[26]+e[7]*e[22]*e[16]+e[7]*e[13]*e[25]+e[1]*e[18]*e[12]+e[1]*e[9]*e[21]; 
    A[107]=e[10]*e[27]*e[12]+e[10]*e[9]*e[30]+e[10]*e[29]*e[14]+e[10]*e[11]*e[32]+e[10]*e[28]*e[13]+e[28]*e[11]*e[14]+e[28]*e[9]*e[12]+e[13]*e[30]*e[12]+e[13]*e[32]*e[14]+e[16]*e[30]*e[15]+e[16]*e[12]*e[33]+e[16]*e[32]*e[17]+e[16]*e[14]*e[35]+e[16]*e[13]*e[34]+e[34]*e[14]*e[17]+e[34]*e[12]*e[15]-1.*e[13]*e[27]*e[9]-1.*e[13]*e[33]*e[15]-1.*e[13]*e[35]*e[17]-1.*e[13]*e[29]*e[11]+.5000000000*ep2[10]*e[31]+.5000000000*e[31]*ep2[16]-.5000000000*e[31]*ep2[9]-.5000000000*e[31]*ep2[11]+.5000000000*e[31]*ep2[12]-.5000000000*e[31]*ep2[15]-.5000000000*e[31]*ep2[17]+.5000000000*e[31]*ep2[14]+1.500000000*e[31]*ep2[13]; 
    A[106]=-.5000000000*e[4]*ep2[6]-.5000000000*e[4]*ep2[0]+e[1]*e[2]*e[5]+.5000000000*e[4]*ep2[7]+e[1]*e[0]*e[3]+e[7]*e[5]*e[8]-.5000000000*e[4]*ep2[8]+.5000000000*e[4]*ep2[3]+.5000000000*e[4]*ep2[5]+e[7]*e[3]*e[6]-.5000000000*e[4]*ep2[2]+.5000000000*ep3[4]+.5000000000*ep2[1]*e[4]; 
    A[100]=e[34]*e[32]*e[35]-.5000000000*e[31]*ep2[35]+.5000000000*e[31]*ep2[34]+.5000000000*ep2[28]*e[31]+.5000000000*ep3[31]+.5000000000*e[31]*ep2[32]+e[34]*e[30]*e[33]-.5000000000*e[31]*ep2[27]+.5000000000*e[31]*ep2[30]-.5000000000*e[31]*ep2[33]-.5000000000*e[31]*ep2[29]+e[28]*e[29]*e[32]+e[28]*e[27]*e[30]; 
    A[101]=e[1]*e[27]*e[30]+e[1]*e[29]*e[32]+e[1]*e[28]*e[31]+e[31]*e[30]*e[3]+e[31]*e[32]*e[5]+e[7]*e[30]*e[33]+e[7]*e[32]*e[35]+e[7]*e[31]*e[34]+e[28]*e[27]*e[3]+e[28]*e[0]*e[30]+e[28]*e[29]*e[5]+e[28]*e[2]*e[32]+e[34]*e[30]*e[6]+e[34]*e[3]*e[33]+e[34]*e[32]*e[8]+e[34]*e[5]*e[35]-1.*e[31]*e[27]*e[0]-1.*e[31]*e[33]*e[6]-1.*e[31]*e[35]*e[8]-1.*e[31]*e[29]*e[2]+.5000000000*e[4]*ep2[30]+.5000000000*e[4]*ep2[32]+1.500000000*e[4]*ep2[31]-.5000000000*e[4]*ep2[27]+.5000000000*e[4]*ep2[28]-.5000000000*e[4]*ep2[29]-.5000000000*e[4]*ep2[33]+.5000000000*e[4]*ep2[34]-.5000000000*e[4]*ep2[35]; 
    A[102]=.5000000000*e[22]*ep2[30]+.5000000000*e[22]*ep2[32]+1.500000000*e[22]*ep2[31]+.5000000000*e[22]*ep2[34]-.5000000000*e[22]*ep2[27]-.5000000000*e[22]*ep2[29]-.5000000000*e[22]*ep2[33]-.5000000000*e[22]*ep2[35]+e[28]*e[18]*e[30]+e[28]*e[29]*e[23]+e[28]*e[20]*e[32]+e[31]*e[30]*e[21]+e[31]*e[32]*e[23]+e[25]*e[30]*e[33]+e[25]*e[32]*e[35]+e[25]*e[31]*e[34]+e[34]*e[30]*e[24]+e[34]*e[21]*e[33]+e[34]*e[32]*e[26]+e[34]*e[23]*e[35]-1.*e[31]*e[27]*e[18]-1.*e[31]*e[33]*e[24]-1.*e[31]*e[29]*e[20]-1.*e[31]*e[35]*e[26]+e[19]*e[27]*e[30]+e[19]*e[29]*e[32]+e[19]*e[28]*e[31]+e[28]*e[27]*e[21]+.5000000000*ep2[28]*e[22]; 
    A[103]=e[16]*e[30]*e[33]+e[16]*e[32]*e[35]+e[10]*e[27]*e[30]+e[10]*e[29]*e[32]+e[10]*e[28]*e[31]+e[34]*e[30]*e[15]+e[34]*e[12]*e[33]+e[34]*e[32]*e[17]+e[34]*e[14]*e[35]+e[34]*e[31]*e[16]+e[28]*e[27]*e[12]+e[28]*e[9]*e[30]+e[28]*e[29]*e[14]+e[28]*e[11]*e[32]-1.*e[31]*e[27]*e[9]+e[31]*e[30]*e[12]+e[31]*e[32]*e[14]-1.*e[31]*e[33]*e[15]-1.*e[31]*e[35]*e[17]-1.*e[31]*e[29]*e[11]-.5000000000*e[13]*ep2[27]+.5000000000*e[13]*ep2[32]+.5000000000*e[13]*ep2[28]-.5000000000*e[13]*ep2[29]+1.500000000*e[13]*ep2[31]-.5000000000*e[13]*ep2[33]+.5000000000*e[13]*ep2[30]+.5000000000*e[13]*ep2[34]-.5000000000*e[13]*ep2[35]; 
    A[96]=e[21]*e[23]*e[14]+e[21]*e[22]*e[13]+e[24]*e[21]*e[15]+e[24]*e[23]*e[17]+e[24]*e[14]*e[26]+e[24]*e[22]*e[16]+e[24]*e[13]*e[25]+e[15]*e[22]*e[25]+e[15]*e[23]*e[26]+e[9]*e[19]*e[22]+e[9]*e[18]*e[21]+e[9]*e[20]*e[23]+e[18]*e[20]*e[14]+e[18]*e[11]*e[23]+e[18]*e[19]*e[13]+e[18]*e[10]*e[22]-1.*e[21]*e[25]*e[16]-1.*e[21]*e[26]*e[17]-1.*e[21]*e[20]*e[11]-1.*e[21]*e[19]*e[10]+1.500000000*ep2[21]*e[12]+.5000000000*e[12]*ep2[24]-.5000000000*e[12]*ep2[26]+.5000000000*e[12]*ep2[18]+.5000000000*e[12]*ep2[23]-.5000000000*e[12]*ep2[19]-.5000000000*e[12]*ep2[20]+.5000000000*e[12]*ep2[22]-.5000000000*e[12]*ep2[25]; 
    A[97]=-1.*e[12]*e[29]*e[20]-1.*e[12]*e[35]*e[26]-1.*e[12]*e[28]*e[19]-1.*e[12]*e[34]*e[25]+e[18]*e[29]*e[14]+e[18]*e[11]*e[32]+e[18]*e[28]*e[13]+e[18]*e[10]*e[31]+e[27]*e[20]*e[14]+e[27]*e[11]*e[23]+e[27]*e[19]*e[13]+e[27]*e[10]*e[22]+e[15]*e[30]*e[24]+e[15]*e[21]*e[33]+e[15]*e[31]*e[25]+e[15]*e[22]*e[34]+e[15]*e[32]*e[26]+e[15]*e[23]*e[35]-1.*e[21]*e[28]*e[10]-1.*e[21]*e[34]*e[16]-1.*e[21]*e[35]*e[17]-1.*e[21]*e[29]*e[11]-1.*e[30]*e[25]*e[16]-1.*e[30]*e[26]*e[17]-1.*e[30]*e[20]*e[11]-1.*e[30]*e[19]*e[10]+e[24]*e[32]*e[17]+e[24]*e[14]*e[35]+e[24]*e[31]*e[16]+e[24]*e[13]*e[34]+e[33]*e[23]*e[17]+e[33]*e[14]*e[26]+e[33]*e[22]*e[16]+e[33]*e[13]*e[25]+3.*e[12]*e[30]*e[21]+e[12]*e[31]*e[22]+e[12]*e[32]*e[23]+e[9]*e[27]*e[21]+e[9]*e[18]*e[30]+e[9]*e[28]*e[22]+e[9]*e[19]*e[31]+e[9]*e[29]*e[23]+e[9]*e[20]*e[32]+e[21]*e[32]*e[14]+e[21]*e[31]*e[13]+e[30]*e[23]*e[14]+e[30]*e[22]*e[13]+e[12]*e[27]*e[18]+e[12]*e[33]*e[24]; 
    A[98]=e[0]*e[11]*e[5]+e[0]*e[2]*e[14]+e[9]*e[1]*e[4]+e[9]*e[0]*e[3]+e[9]*e[2]*e[5]+e[3]*e[13]*e[4]+e[3]*e[14]*e[5]+e[6]*e[3]*e[15]+e[6]*e[13]*e[7]+e[6]*e[4]*e[16]+e[6]*e[14]*e[8]+e[6]*e[5]*e[17]+e[15]*e[4]*e[7]+e[15]*e[5]*e[8]-1.*e[3]*e[11]*e[2]-1.*e[3]*e[10]*e[1]-1.*e[3]*e[16]*e[7]-1.*e[3]*e[17]*e[8]+e[0]*e[10]*e[4]+e[0]*e[1]*e[13]+1.500000000*e[12]*ep2[3]+.5000000000*e[12]*ep2[4]+.5000000000*e[12]*ep2[5]+.5000000000*e[12]*ep2[6]+.5000000000*ep2[0]*e[12]-.5000000000*e[12]*ep2[1]-.5000000000*e[12]*ep2[7]-.5000000000*e[12]*ep2[2]-.5000000000*e[12]*ep2[8]; 
    A[99]=e[21]*e[24]*e[6]+e[0]*e[19]*e[22]+e[0]*e[20]*e[23]+e[24]*e[22]*e[7]+e[24]*e[4]*e[25]+e[24]*e[23]*e[8]+e[24]*e[5]*e[26]+e[6]*e[22]*e[25]+e[6]*e[23]*e[26]+e[18]*e[0]*e[21]+e[18]*e[19]*e[4]+e[18]*e[1]*e[22]+e[18]*e[20]*e[5]+e[18]*e[2]*e[23]+e[21]*e[22]*e[4]+e[21]*e[23]*e[5]-1.*e[21]*e[26]*e[8]-1.*e[21]*e[20]*e[2]-1.*e[21]*e[19]*e[1]-1.*e[21]*e[25]*e[7]+1.500000000*ep2[21]*e[3]+.5000000000*e[3]*ep2[22]+.5000000000*e[3]*ep2[23]+.5000000000*e[3]*ep2[24]-.5000000000*e[3]*ep2[26]-.5000000000*e[3]*ep2[19]-.5000000000*e[3]*ep2[20]-.5000000000*e[3]*ep2[25]+.5000000000*ep2[18]*e[3]; 
    A[127]=e[11]*e[27]*e[12]+e[11]*e[9]*e[30]+e[11]*e[29]*e[14]+e[11]*e[28]*e[13]+e[11]*e[10]*e[31]+e[29]*e[9]*e[12]+e[29]*e[10]*e[13]+e[14]*e[30]*e[12]+e[14]*e[31]*e[13]+e[17]*e[30]*e[15]+e[17]*e[12]*e[33]+e[17]*e[14]*e[35]+e[17]*e[31]*e[16]+e[17]*e[13]*e[34]+e[35]*e[12]*e[15]+e[35]*e[13]*e[16]-1.*e[14]*e[27]*e[9]-1.*e[14]*e[28]*e[10]-1.*e[14]*e[33]*e[15]-1.*e[14]*e[34]*e[16]+.5000000000*ep2[11]*e[32]-.5000000000*e[32]*ep2[16]-.5000000000*e[32]*ep2[9]+.5000000000*e[32]*ep2[12]-.5000000000*e[32]*ep2[15]+.5000000000*e[32]*ep2[17]-.5000000000*e[32]*ep2[10]+1.500000000*e[32]*ep2[14]+.5000000000*e[32]*ep2[13]; 
    A[126]=e[8]*e[3]*e[6]+.5000000000*ep2[2]*e[5]-.5000000000*e[5]*ep2[0]+.5000000000*e[5]*ep2[4]-.5000000000*e[5]*ep2[6]+.5000000000*e[5]*ep2[8]+e[8]*e[4]*e[7]+.5000000000*ep3[5]+e[2]*e[0]*e[3]+.5000000000*e[5]*ep2[3]-.5000000000*e[5]*ep2[7]+e[2]*e[1]*e[4]-.5000000000*e[5]*ep2[1]; 
    A[125]=e[2]*e[27]*e[3]+e[2]*e[0]*e[30]+e[2]*e[28]*e[4]+e[2]*e[1]*e[31]+e[2]*e[29]*e[5]-1.*e[5]*e[27]*e[0]-1.*e[5]*e[34]*e[7]-1.*e[5]*e[33]*e[6]+e[5]*e[30]*e[3]+e[5]*e[35]*e[8]-1.*e[5]*e[28]*e[1]+e[5]*e[31]*e[4]+e[29]*e[1]*e[4]+e[29]*e[0]*e[3]+e[8]*e[30]*e[6]+e[8]*e[3]*e[33]+e[8]*e[31]*e[7]+e[8]*e[4]*e[34]+e[35]*e[4]*e[7]+e[35]*e[3]*e[6]+.5000000000*ep2[2]*e[32]+1.500000000*e[32]*ep2[5]+.5000000000*e[32]*ep2[4]-.5000000000*e[32]*ep2[0]-.5000000000*e[32]*ep2[6]-.5000000000*e[32]*ep2[1]-.5000000000*e[32]*ep2[7]+.5000000000*e[32]*ep2[3]+.5000000000*e[32]*ep2[8]; 
    A[124]=-1.*e[14]*e[19]*e[1]+e[14]*e[22]*e[4]-1.*e[14]*e[18]*e[0]-1.*e[14]*e[25]*e[7]-1.*e[14]*e[24]*e[6]-1.*e[23]*e[10]*e[1]+e[23]*e[13]*e[4]-1.*e[23]*e[16]*e[7]-1.*e[23]*e[15]*e[6]-1.*e[23]*e[9]*e[0]+e[23]*e[12]*e[3]+e[17]*e[21]*e[6]+e[17]*e[3]*e[24]+e[17]*e[22]*e[7]+e[17]*e[4]*e[25]+e[17]*e[5]*e[26]-1.*e[5]*e[24]*e[15]-1.*e[5]*e[25]*e[16]-1.*e[5]*e[18]*e[9]-1.*e[5]*e[19]*e[10]+e[26]*e[12]*e[6]+e[26]*e[3]*e[15]+e[26]*e[13]*e[7]+e[26]*e[4]*e[16]+e[11]*e[18]*e[3]+e[11]*e[0]*e[21]+e[11]*e[19]*e[4]+e[11]*e[1]*e[22]+e[11]*e[20]*e[5]+e[11]*e[2]*e[23]+e[20]*e[9]*e[3]+e[20]*e[0]*e[12]+e[20]*e[10]*e[4]+e[20]*e[1]*e[13]+e[20]*e[2]*e[14]+e[5]*e[21]*e[12]+3.*e[5]*e[23]*e[14]+e[5]*e[22]*e[13]+e[8]*e[21]*e[15]+e[8]*e[12]*e[24]+e[8]*e[23]*e[17]+e[8]*e[14]*e[26]+e[8]*e[22]*e[16]+e[8]*e[13]*e[25]+e[2]*e[18]*e[12]+e[2]*e[9]*e[21]+e[2]*e[19]*e[13]+e[2]*e[10]*e[22]+e[14]*e[21]*e[3]; 
    A[123]=-.5000000000*e[14]*ep2[27]+1.500000000*e[14]*ep2[32]-.5000000000*e[14]*ep2[28]+.5000000000*e[14]*ep2[29]+.5000000000*e[14]*ep2[31]-.5000000000*e[14]*ep2[33]+.5000000000*e[14]*ep2[30]-.5000000000*e[14]*ep2[34]+.5000000000*e[14]*ep2[35]+e[11]*e[27]*e[30]+e[11]*e[29]*e[32]+e[11]*e[28]*e[31]+e[35]*e[30]*e[15]+e[35]*e[12]*e[33]+e[35]*e[32]*e[17]+e[35]*e[31]*e[16]+e[35]*e[13]*e[34]+e[29]*e[27]*e[12]+e[29]*e[9]*e[30]+e[29]*e[28]*e[13]+e[29]*e[10]*e[31]-1.*e[32]*e[27]*e[9]+e[32]*e[30]*e[12]-1.*e[32]*e[28]*e[10]+e[32]*e[31]*e[13]-1.*e[32]*e[33]*e[15]-1.*e[32]*e[34]*e[16]+e[17]*e[30]*e[33]+e[17]*e[31]*e[34]; 
    A[122]=-.5000000000*e[23]*ep2[33]-.5000000000*e[23]*ep2[34]+.5000000000*ep2[29]*e[23]+.5000000000*e[23]*ep2[30]+1.500000000*e[23]*ep2[32]+.5000000000*e[23]*ep2[31]+.5000000000*e[23]*ep2[35]-.5000000000*e[23]*ep2[27]-.5000000000*e[23]*ep2[28]+e[32]*e[30]*e[21]+e[32]*e[31]*e[22]+e[26]*e[30]*e[33]+e[26]*e[32]*e[35]+e[26]*e[31]*e[34]+e[35]*e[30]*e[24]+e[35]*e[21]*e[33]+e[35]*e[31]*e[25]+e[35]*e[22]*e[34]-1.*e[32]*e[27]*e[18]-1.*e[32]*e[33]*e[24]-1.*e[32]*e[28]*e[19]-1.*e[32]*e[34]*e[25]+e[20]*e[27]*e[30]+e[20]*e[29]*e[32]+e[20]*e[28]*e[31]+e[29]*e[27]*e[21]+e[29]*e[18]*e[30]+e[29]*e[28]*e[22]+e[29]*e[19]*e[31]; 
    A[121]=e[2]*e[27]*e[30]+e[2]*e[29]*e[32]+e[2]*e[28]*e[31]+e[32]*e[30]*e[3]+e[32]*e[31]*e[4]+e[8]*e[30]*e[33]+e[8]*e[32]*e[35]+e[8]*e[31]*e[34]+e[29]*e[27]*e[3]+e[29]*e[0]*e[30]+e[29]*e[28]*e[4]+e[29]*e[1]*e[31]+e[35]*e[30]*e[6]+e[35]*e[3]*e[33]+e[35]*e[31]*e[7]+e[35]*e[4]*e[34]-1.*e[32]*e[27]*e[0]-1.*e[32]*e[34]*e[7]-1.*e[32]*e[33]*e[6]-1.*e[32]*e[28]*e[1]+.5000000000*e[5]*ep2[30]+1.500000000*e[5]*ep2[32]+.5000000000*e[5]*ep2[31]-.5000000000*e[5]*ep2[27]-.5000000000*e[5]*ep2[28]+.5000000000*e[5]*ep2[29]-.5000000000*e[5]*ep2[33]-.5000000000*e[5]*ep2[34]+.5000000000*e[5]*ep2[35]; 
    A[120]=.5000000000*e[32]*ep2[31]+.5000000000*e[32]*ep2[35]-.5000000000*e[32]*ep2[27]+e[29]*e[27]*e[30]+e[29]*e[28]*e[31]+e[35]*e[30]*e[33]+e[35]*e[31]*e[34]+.5000000000*ep2[29]*e[32]+.5000000000*ep3[32]-.5000000000*e[32]*ep2[33]-.5000000000*e[32]*ep2[34]+.5000000000*e[32]*ep2[30]-.5000000000*e[32]*ep2[28]; 
    A[118]=e[10]*e[1]*e[4]+e[10]*e[0]*e[3]+e[10]*e[2]*e[5]+e[4]*e[12]*e[3]+e[4]*e[14]*e[5]+e[7]*e[12]*e[6]+e[7]*e[3]*e[15]+e[7]*e[4]*e[16]+e[7]*e[14]*e[8]+e[7]*e[5]*e[17]+e[16]*e[3]*e[6]+e[16]*e[5]*e[8]-1.*e[4]*e[11]*e[2]-1.*e[4]*e[15]*e[6]-1.*e[4]*e[9]*e[0]-1.*e[4]*e[17]*e[8]+e[1]*e[9]*e[3]+e[1]*e[0]*e[12]+e[1]*e[11]*e[5]+e[1]*e[2]*e[14]+1.500000000*e[13]*ep2[4]+.5000000000*e[13]*ep2[3]+.5000000000*e[13]*ep2[5]+.5000000000*e[13]*ep2[7]+.5000000000*ep2[1]*e[13]-.5000000000*e[13]*ep2[0]-.5000000000*e[13]*ep2[6]-.5000000000*e[13]*ep2[2]-.5000000000*e[13]*ep2[8]; 
    A[119]=e[25]*e[21]*e[6]+e[25]*e[3]*e[24]+e[25]*e[23]*e[8]+e[25]*e[5]*e[26]+e[7]*e[21]*e[24]+e[7]*e[23]*e[26]+e[19]*e[18]*e[3]+e[19]*e[0]*e[21]+e[19]*e[1]*e[22]+e[19]*e[20]*e[5]+e[19]*e[2]*e[23]+e[22]*e[21]*e[3]+e[22]*e[23]*e[5]-1.*e[22]*e[26]*e[8]-1.*e[22]*e[20]*e[2]-1.*e[22]*e[18]*e[0]+e[22]*e[25]*e[7]-1.*e[22]*e[24]*e[6]+e[1]*e[18]*e[21]+e[1]*e[20]*e[23]+.5000000000*e[4]*ep2[25]-.5000000000*e[4]*ep2[26]-.5000000000*e[4]*ep2[18]-.5000000000*e[4]*ep2[20]-.5000000000*e[4]*ep2[24]+.5000000000*ep2[19]*e[4]+1.500000000*ep2[22]*e[4]+.5000000000*e[4]*ep2[21]+.5000000000*e[4]*ep2[23]; 
    A[116]=e[22]*e[21]*e[12]+e[22]*e[23]*e[14]+e[25]*e[21]*e[15]+e[25]*e[12]*e[24]+e[25]*e[23]*e[17]+e[25]*e[14]*e[26]+e[25]*e[22]*e[16]+e[16]*e[21]*e[24]+e[16]*e[23]*e[26]+e[10]*e[19]*e[22]+e[10]*e[18]*e[21]+e[10]*e[20]*e[23]+e[19]*e[18]*e[12]+e[19]*e[9]*e[21]+e[19]*e[20]*e[14]+e[19]*e[11]*e[23]-1.*e[22]*e[24]*e[15]-1.*e[22]*e[26]*e[17]-1.*e[22]*e[20]*e[11]-1.*e[22]*e[18]*e[9]-.5000000000*e[13]*ep2[26]-.5000000000*e[13]*ep2[18]+.5000000000*e[13]*ep2[23]+.5000000000*e[13]*ep2[19]-.5000000000*e[13]*ep2[20]-.5000000000*e[13]*ep2[24]+.5000000000*e[13]*ep2[21]+1.500000000*ep2[22]*e[13]+.5000000000*e[13]*ep2[25]; 
    A[117]=e[13]*e[30]*e[21]+3.*e[13]*e[31]*e[22]+e[13]*e[32]*e[23]+e[10]*e[27]*e[21]+e[10]*e[18]*e[30]+e[10]*e[28]*e[22]+e[10]*e[19]*e[31]+e[10]*e[29]*e[23]+e[10]*e[20]*e[32]+e[22]*e[30]*e[12]+e[22]*e[32]*e[14]+e[31]*e[21]*e[12]+e[31]*e[23]*e[14]-1.*e[13]*e[27]*e[18]-1.*e[13]*e[33]*e[24]-1.*e[13]*e[29]*e[20]-1.*e[13]*e[35]*e[26]+e[13]*e[28]*e[19]+e[13]*e[34]*e[25]+e[19]*e[27]*e[12]+e[19]*e[9]*e[30]+e[19]*e[29]*e[14]+e[19]*e[11]*e[32]+e[28]*e[18]*e[12]+e[28]*e[9]*e[21]+e[28]*e[20]*e[14]+e[28]*e[11]*e[23]+e[16]*e[30]*e[24]+e[16]*e[21]*e[33]+e[16]*e[31]*e[25]+e[16]*e[22]*e[34]+e[16]*e[32]*e[26]+e[16]*e[23]*e[35]-1.*e[22]*e[27]*e[9]-1.*e[22]*e[33]*e[15]-1.*e[22]*e[35]*e[17]-1.*e[22]*e[29]*e[11]-1.*e[31]*e[24]*e[15]-1.*e[31]*e[26]*e[17]-1.*e[31]*e[20]*e[11]-1.*e[31]*e[18]*e[9]+e[25]*e[30]*e[15]+e[25]*e[12]*e[33]+e[25]*e[32]*e[17]+e[25]*e[14]*e[35]+e[34]*e[21]*e[15]+e[34]*e[12]*e[24]+e[34]*e[23]*e[17]+e[34]*e[14]*e[26]; 
    A[114]=e[19]*e[11]*e[14]+e[19]*e[9]*e[12]+e[19]*e[10]*e[13]+e[13]*e[21]*e[12]+e[13]*e[23]*e[14]+e[16]*e[21]*e[15]+e[16]*e[12]*e[24]+e[16]*e[23]*e[17]+e[16]*e[14]*e[26]+e[16]*e[13]*e[25]+e[25]*e[14]*e[17]+e[25]*e[12]*e[15]-1.*e[13]*e[24]*e[15]-1.*e[13]*e[26]*e[17]-1.*e[13]*e[20]*e[11]-1.*e[13]*e[18]*e[9]+e[10]*e[18]*e[12]+e[10]*e[9]*e[21]+e[10]*e[20]*e[14]+e[10]*e[11]*e[23]+1.500000000*e[22]*ep2[13]+.5000000000*e[22]*ep2[14]+.5000000000*e[22]*ep2[12]+.5000000000*e[22]*ep2[16]+.5000000000*ep2[10]*e[22]-.5000000000*e[22]*ep2[9]-.5000000000*e[22]*ep2[11]-.5000000000*e[22]*ep2[15]-.5000000000*e[22]*ep2[17]; 
    A[115]=e[13]*e[12]*e[3]+e[13]*e[14]*e[5]+e[16]*e[12]*e[6]+e[16]*e[3]*e[15]+e[16]*e[13]*e[7]+e[16]*e[14]*e[8]+e[16]*e[5]*e[17]+e[7]*e[14]*e[17]+e[7]*e[12]*e[15]+e[1]*e[11]*e[14]+e[1]*e[9]*e[12]+e[1]*e[10]*e[13]+e[10]*e[9]*e[3]+e[10]*e[0]*e[12]+e[10]*e[11]*e[5]+e[10]*e[2]*e[14]-1.*e[13]*e[11]*e[2]-1.*e[13]*e[15]*e[6]-1.*e[13]*e[9]*e[0]-1.*e[13]*e[17]*e[8]+1.500000000*ep2[13]*e[4]+.5000000000*e[4]*ep2[16]-.5000000000*e[4]*ep2[9]-.5000000000*e[4]*ep2[11]+.5000000000*e[4]*ep2[12]-.5000000000*e[4]*ep2[15]-.5000000000*e[4]*ep2[17]+.5000000000*e[4]*ep2[10]+.5000000000*e[4]*ep2[14]; 
    A[112]=e[19]*e[1]*e[4]+e[19]*e[0]*e[3]+e[19]*e[2]*e[5]+e[4]*e[21]*e[3]+e[4]*e[23]*e[5]+e[7]*e[21]*e[6]+e[7]*e[3]*e[24]+e[7]*e[4]*e[25]+e[7]*e[23]*e[8]+e[7]*e[5]*e[26]+e[25]*e[3]*e[6]+e[25]*e[5]*e[8]+e[1]*e[18]*e[3]+e[1]*e[0]*e[21]+e[1]*e[20]*e[5]+e[1]*e[2]*e[23]-1.*e[4]*e[26]*e[8]-1.*e[4]*e[20]*e[2]-1.*e[4]*e[18]*e[0]-1.*e[4]*e[24]*e[6]+1.500000000*e[22]*ep2[4]-.5000000000*e[22]*ep2[0]-.5000000000*e[22]*ep2[6]+.5000000000*e[22]*ep2[5]+.5000000000*e[22]*ep2[1]+.5000000000*e[22]*ep2[7]+.5000000000*e[22]*ep2[3]-.5000000000*e[22]*ep2[2]-.5000000000*e[22]*ep2[8]; 
    A[113]=-1.*e[31]*e[20]*e[2]-1.*e[31]*e[18]*e[0]+e[31]*e[23]*e[5]-1.*e[31]*e[24]*e[6]+e[7]*e[30]*e[24]+e[7]*e[21]*e[33]+e[7]*e[32]*e[26]+e[7]*e[23]*e[35]+e[25]*e[30]*e[6]+e[25]*e[3]*e[33]+e[25]*e[31]*e[7]+e[25]*e[4]*e[34]+e[25]*e[32]*e[8]+e[25]*e[5]*e[35]+e[34]*e[21]*e[6]+e[34]*e[3]*e[24]+e[34]*e[22]*e[7]+e[34]*e[23]*e[8]+e[34]*e[5]*e[26]+e[1]*e[27]*e[21]+e[1]*e[18]*e[30]+e[1]*e[28]*e[22]+e[1]*e[19]*e[31]+e[1]*e[29]*e[23]+e[1]*e[20]*e[32]+e[19]*e[27]*e[3]+e[19]*e[0]*e[30]+e[19]*e[28]*e[4]+e[19]*e[29]*e[5]+e[19]*e[2]*e[32]+e[28]*e[18]*e[3]+e[28]*e[0]*e[21]+e[28]*e[20]*e[5]+e[28]*e[2]*e[23]+e[4]*e[30]*e[21]+3.*e[4]*e[31]*e[22]+e[4]*e[32]*e[23]-1.*e[4]*e[27]*e[18]-1.*e[4]*e[33]*e[24]-1.*e[4]*e[29]*e[20]-1.*e[4]*e[35]*e[26]-1.*e[22]*e[27]*e[0]+e[22]*e[32]*e[5]-1.*e[22]*e[33]*e[6]+e[22]*e[30]*e[3]-1.*e[22]*e[35]*e[8]-1.*e[22]*e[29]*e[2]+e[31]*e[21]*e[3]-1.*e[31]*e[26]*e[8]; 

    int perm[20] = {6, 8, 18, 15, 12, 5, 14, 7, 4, 11, 19, 13, 1, 16, 17, 3, 10, 9, 2, 0}; 
    double AA[200]; 
    for (int i = 0; i < 20; i++)
    {
        for (int j = 0; j < 10; j++) AA[i + j * 20] = A[perm[i] + j * 20];             
    }
    
    for (int i = 0; i < 200; i++)
    {
        A[i] = AA[i]; 
    }
}

