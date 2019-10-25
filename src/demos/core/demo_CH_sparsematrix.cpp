// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Demo for working with Eigen references.
// (Mostly for Chrono developers)
//
// =============================================================================

#include <iostream>
#include <vector>
#include "chrono/core/ChMatrix.h"
#include "chrono/core/ChSparsityPatternLearner.h"


using namespace chrono;

// ------------------------------------------------------------------

int main(int argc, char* argv[]) {

	ChSparsityPatternLearner spl(3,3);

	spl.SetElement(0,0,1.1);
	spl.SetElement(1,1,2.2);
	spl.SetElement(2,2,3.3);

	ChSparseMatrix spmat;

	spmat.LoadSparsityPattern(spl.GetSparsityPattern());



    return 0;
}
