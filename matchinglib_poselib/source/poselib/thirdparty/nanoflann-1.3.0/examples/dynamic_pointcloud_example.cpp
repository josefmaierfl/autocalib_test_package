/***********************************************************************
 * Software License Agreement (BSD License)
 *
 * Copyright 2011-2016 Jose Luis Blanco (joseluisblancoc@gmail.com).
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************/

#include <nanoflann.hpp>
#include "utils.h"
#include <ctime>
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace nanoflann;

template <typename num_t>
void kdtree_demo(const size_t N)
{
	PointCloud<num_t> cloud;

	// construct a kd-tree index:
	typedef KDTreeSingleIndexDynamicAdaptor<
		L2_Simple_Adaptor<num_t, PointCloud<num_t> > ,
		PointCloud<num_t>,
		3 /* dim */
		> my_kd_tree_t;

	dump_mem_usage();

	my_kd_tree_t   index(3 /*dim*/, cloud, KDTreeSingleIndexAdaptorParams(10 /* max leaf */) );

	// Generate points:
	generateRandomPointCloud(cloud, N);

	num_t query_pt[3] = { 0.5, 0.5, 0.5 };

	// add points in chunks at a time
	int chunk_size = 100;
	for(int i = 0; i < N; i = i + chunk_size)
	{
		size_t end = min(size_t(i + chunk_size), N - 1);
		// Inserts all points from [i, end]
		index.addPoints(i, end);
	}

	// remove a point
	size_t removePointIndex = N - 1;
	index.removePoint(removePointIndex);

	dump_mem_usage();
	{
		// do a knn search
		const size_t num_results = 1;
		size_t ret_index;
		num_t out_dist_sqr;
		nanoflann::KNNResultSet<num_t> resultSet(num_results);
		resultSet.init(&ret_index, &out_dist_sqr );
		index.findNeighbors(resultSet, query_pt, nanoflann::SearchParams(10));

		std::cout << "knnSearch(nn="<<num_results<<"): \n";
		std::cout << "ret_index=" << ret_index << " out_dist_sqr=" << out_dist_sqr << endl;
	}
	{
		// Unsorted radius search:
		const num_t radius = 1;
		std::vector<std::pair<size_t, num_t> > indices_dists;
		RadiusResultSet<num_t, size_t> resultSet(radius, indices_dists);

		index.findNeighbors(resultSet, query_pt, nanoflann::SearchParams());

		// Get worst (furthest) point, without sorting:
		std::pair<size_t,num_t> worst_pair = resultSet.worst_item();
		cout << "Worst pair: idx=" << worst_pair.first << " dist=" << worst_pair.second << endl;
	}
}

int main()
{
	// Randomize Seed
	srand(time(NULL));
	kdtree_demo<float>(1000000);
	kdtree_demo<double>(1000000);
	return 0;
}
