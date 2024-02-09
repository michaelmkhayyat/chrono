// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2023 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================

#ifndef CHFUNCT_SINE_STEP_H
#define CHFUNCT_SINE_STEP_H

#include "chrono/core/ChVector2.h"
#include "chrono/motion_functions/ChFunctionBase.h"

namespace chrono {

/// @addtogroup chrono_functions
/// @{

/// Sinusoidal ramp between two (x,y) points p1 and p2.
class ChApi ChFunctionSineStep : public ChFunction {
  public:
    ChFunctionSineStep(const ChVector2d& p1, const ChVector2d& p2);
    ChFunctionSineStep(const ChFunctionSineStep& other);
    ~ChFunctionSineStep() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChFunctionSineStep* Clone() const override { return new ChFunctionSineStep(*this); }

    virtual FunctionType Get_Type() const override { return FUNCT_SINE_STEP; }

    virtual double Get_y(double x) const override;
    virtual double Get_y_dx(double x) const override;
    virtual double Get_y_dxdx(double x) const override;

    void SetP1(const ChVector2d& p1);
    void SetP2(const ChVector2d& p2);

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& marchive) override;

    /// Method to allow de-serialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& marchive) override;

    static double Eval(double x, double x1, double y1, double x2, double y2);

  private:
    ChVector2d m_p1;
    ChVector2d m_p2;
    ChVector2d m_dp;
};

/// @} chrono_functions

}  // end namespace chrono

#endif
