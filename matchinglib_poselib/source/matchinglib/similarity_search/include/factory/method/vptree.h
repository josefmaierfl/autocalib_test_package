/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _FACTORY_VPTREE_H_
#define _FACTORY_VPTREE_H_

#include "searchoracle.h"
#include <method/vptree.h>

namespace similarity {

/* 
 * We have different creating functions, 
 * b/c there are different pruning methods.
 */
template <typename dist_t>
Index<dist_t>* CreateVPTree(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {
    return new VPTree<dist_t,PolynomialPruner<dist_t>>(PrintProgress, space, DataObjects);
}

/*
 * End of creating functions.
 */
}

#endif
