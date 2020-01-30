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
 *_
 */

#ifndef ID_TYPE_H
#define ID_TYPE_H

#include <cstdint>

namespace similarity {

using std::numeric_limits;

/* 
 * We are not gonna have billions of records or labels in the foreseeable future
 * Negative values would represent missing labels and ids. In principle, it is probably
 * easy to switch to 8-byte IDs by simply changing below typedef declarations.
 * However, some caution needs to be excecised because
 * 1) We want to preserve alignment (extra padding in the Object may be needed (see the comment in object.h).
 * 2) We need to carefully check all the contexts where IDs are used. It is not impossible that some
 *    code declares IDs as int-type.
 */
typedef int32_t  IdType;
typedef uint32_t IdTypeUnsign;
const IdTypeUnsign MAX_DATASET_QTY = numeric_limits<IdType>::max();

typedef int32_t LabelType;

#define LABEL_PREFIX "label:"
#define EMPTY_LABEL  numeric_limits<LabelType>::min()

const size_t ID_SIZE         = sizeof(IdType);
const size_t LABEL_SIZE      = sizeof(LabelType);
const size_t DATALENGTH_SIZE = sizeof(size_t);

}   // namespace similarity

#endif    // _OBJECT_H_

