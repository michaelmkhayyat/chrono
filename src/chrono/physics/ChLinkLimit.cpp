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
// Authors: Alessandro Tasora, Radu Serban
// =============================================================================

#include "chrono/physics/ChLinkLimit.h"

namespace chrono {

ChLinkLimit::ChLinkLimit()
    : m_active(false),
      m_penalty_only(false),
      m_polar(false),
      m_rotation(false),
      m_max(1),
      m_min(-1),
      m_maxCushion(0),
      m_minCushion(0),
      m_Kmax(1000),
      m_Kmin(1000),
      m_Rmax(100),
      m_Rmin(100) {
    // Default: no modulation
    m_Kmax_modul = chrono_types::make_shared<ChFunctionConst>(1);
    m_Kmin_modul = chrono_types::make_shared<ChFunctionConst>(1);
    m_Rmax_modul = chrono_types::make_shared<ChFunctionConst>(1);
    m_Rmin_modul = chrono_types::make_shared<ChFunctionConst>(1);
    m_polarMax_funct = chrono_types::make_shared<ChFunctionConst>(1);

    constr_upper.SetMode(ChConstraint::Mode::UNILATERAL);
    constr_lower.SetMode(ChConstraint::Mode::UNILATERAL);
}

ChLinkLimit::ChLinkLimit(const ChLinkLimit& other) {
    m_active = other.m_active;
    m_penalty_only = other.m_penalty_only;
    m_polar = other.m_polar;
    m_rotation = other.m_rotation;

    m_max = other.m_max;
    m_min = other.m_min;
    m_maxCushion = other.m_maxCushion;
    m_minCushion = other.m_minCushion;
    m_Kmax = other.m_Kmax;
    m_Kmin = other.m_Kmin;
    m_Rmax = other.m_Rmax;
    m_Rmin = other.m_Rmin;

    m_Kmax_modul = std::shared_ptr<ChFunction>(other.m_Kmax_modul->Clone());
    m_Kmin_modul = std::shared_ptr<ChFunction>(other.m_Kmin_modul->Clone());
    m_Rmax_modul = std::shared_ptr<ChFunction>(other.m_Rmax_modul->Clone());
    m_Rmin_modul = std::shared_ptr<ChFunction>(other.m_Rmin_modul->Clone());
    m_polarMax_funct = std::shared_ptr<ChFunction>(other.m_polarMax_funct->Clone());
}

void ChLinkLimit::SetMax(double val) {
    m_max = val;
    if (m_max < m_min)
        m_min = m_max;
    if (m_max - m_maxCushion < m_min)
        m_maxCushion = m_max - m_min;
    if (m_max - m_maxCushion < m_min + m_minCushion)
        m_minCushion = m_max - m_min - m_maxCushion;
    constr_upper.SetActive(true);
}

void ChLinkLimit::SetMin(double val) {
    m_min = val;
    if (m_min > m_max)
        m_max = m_min;
    if (m_min + m_minCushion > m_max)
        m_minCushion = m_max - m_min;
    if (m_min + m_minCushion > m_max - m_maxCushion)
        m_maxCushion = m_max - m_min - m_minCushion;
    constr_lower.SetActive(true);
}

void ChLinkLimit::SetMaxCushion(double val) {
    m_maxCushion = val;
    if (m_max - m_maxCushion < m_min)
        m_maxCushion = m_max - m_min;
    if (m_max - m_maxCushion < m_min + m_minCushion)
        m_minCushion = m_max - m_min - m_maxCushion;
}

void ChLinkLimit::SetMinCushion(double val) {
    m_minCushion = val;
    if (m_min + m_minCushion > m_max)
        m_minCushion = m_max - m_min;
    if (m_min + m_minCushion > m_max - m_maxCushion)
        m_maxCushion = m_max - m_min - m_minCushion;
}

// file parsing / dumping
void ChLinkLimit::ArchiveOut(ChArchiveOut& archive_out) {
    // class version number
    archive_out.VersionWrite<ChLinkLimit>();

    // stream out all member data
    archive_out << CHNVP(m_active);
    archive_out << CHNVP(m_penalty_only);
    archive_out << CHNVP(m_polar);
    archive_out << CHNVP(m_rotation);
    archive_out << CHNVP(m_max);
    archive_out << CHNVP(m_min);
    archive_out << CHNVP(m_maxCushion);
    archive_out << CHNVP(m_minCushion);
    archive_out << CHNVP(m_Kmax);
    archive_out << CHNVP(m_Kmin);
    archive_out << CHNVP(m_Rmax);
    archive_out << CHNVP(m_Rmin);
    archive_out << CHNVP(m_Kmax_modul);
    archive_out << CHNVP(m_Kmin_modul);
    archive_out << CHNVP(m_Rmax_modul);
    archive_out << CHNVP(m_Rmin_modul);
    archive_out << CHNVP(m_polarMax_funct);
}

void ChLinkLimit::ArchiveIn(ChArchiveIn& archive_in) {
    // class version number
    /*int version =*/archive_in.VersionRead<ChLinkLimit>();

    // stream in all member data
    archive_in >> CHNVP(m_active);
    archive_in >> CHNVP(m_penalty_only);
    archive_in >> CHNVP(m_polar);
    archive_in >> CHNVP(m_rotation);
    archive_in >> CHNVP(m_max);
    archive_in >> CHNVP(m_min);
    archive_in >> CHNVP(m_maxCushion);
    archive_in >> CHNVP(m_minCushion);
    archive_in >> CHNVP(m_Kmax);
    archive_in >> CHNVP(m_Kmin);
    archive_in >> CHNVP(m_Rmax);
    archive_in >> CHNVP(m_Rmin);
    archive_in >> CHNVP(m_Kmax_modul);
    archive_in >> CHNVP(m_Kmin_modul);
    archive_in >> CHNVP(m_Rmax_modul);
    archive_in >> CHNVP(m_Rmin_modul);
    archive_in >> CHNVP(m_polarMax_funct);
}

double ChLinkLimit::GetViolation(double x) const {
    if (!m_active || m_penalty_only)
        return 0;

    if (x > m_min && x < m_max)
        return 0;
    if (x <= m_min)
        return (x - m_min);
    if (x >= m_max)
        return (x - m_max);

    return 0;
}

double ChLinkLimit::GetForceTorque(double x, double x_dt) const {
    double cush_coord;
    double cush_coord_norm;
    double force;
    double min_val, max_val;

    if (!m_penalty_only) {
        min_val = m_min;
        max_val = m_max;
    } else {
        min_val = -999999999;
        max_val = 999999999;
    }

    if (x > min_val && x < m_min + m_minCushion) {
        cush_coord = (m_min + m_minCushion) - x;

        if (m_minCushion >= 0.0000001)
            cush_coord_norm = cush_coord / m_minCushion;
        else
            cush_coord_norm = 1;

        if (cush_coord_norm > 1)
            cush_coord_norm = 1;  // clip cushion forces at stopper limit

        force = cush_coord * m_Kmin * m_Kmin_modul->GetVal(cush_coord_norm);
        force += (-x_dt) * m_Rmin * m_Rmin_modul->GetVal(cush_coord_norm);
        if (force < 0) {
            force = 0;
        }  // damping could cause neg force while going away,
           // so -as the limit is not "sticky"- clip force sign.

        return (force);
    }

    if (x < max_val && x > m_max - m_maxCushion) {
        cush_coord = x - (m_max - m_maxCushion);

        if (m_maxCushion >= 0.0000001)
            cush_coord_norm = cush_coord / m_maxCushion;
        else
            cush_coord_norm = 1;

        if (cush_coord_norm > 1)
            cush_coord_norm = 1;  // clip cushion forces at stopper limit

        force = (-cush_coord) * m_Kmax * m_Kmax_modul->GetVal(cush_coord_norm);
        force += (-x_dt) * m_Rmax * m_Rmax_modul->GetVal(cush_coord_norm);
        if (force > 0) {
            force = 0;
        }  // damping could cause pos force while going away,
           // so -as the limit is not "sticky"- clip force sign.
        return (force);
    }
    return 0;
}

double ChLinkLimit::GetPolarAngleMax(double pol_ang) const {
    if (!m_polarMax_funct)
        return 0.001;
    return m_polarMax_funct->GetVal(pol_ang);
}

// The same, but for conical limits, in polar coordinates
double ChLinkLimit::GetPolarForce(double x, double x_dt, double pol_ang) const {
    double cush_coord;
    double cush_coord_norm;
    double cushion_thick;
    double force;
    double max_val;
    double ang_max;

    if (!m_polarMax_funct)
        return 0;

    if (!m_penalty_only) {
        max_val = m_max;
    } else {
        max_val = 999999999;
    }

    ang_max = m_polarMax_funct->GetVal(pol_ang);

    if (x < max_val && x > ang_max - m_maxCushion) {
        cushion_thick = m_maxCushion;
        if (cushion_thick > ang_max)
            cushion_thick = ang_max;

        cush_coord = x - (ang_max - m_maxCushion);

        if (cushion_thick >= 0.0000001)
            cush_coord_norm = cush_coord / cushion_thick;
        else
            cush_coord_norm = 1;

        // clip cushion forces at stopper limit
        if (cush_coord_norm > 1)
            cush_coord_norm = 1;

        force = (-cush_coord) * m_Kmax * m_Kmax_modul->GetVal(cush_coord_norm);
        force += (-x_dt) * m_Rmax * m_Rmax_modul->GetVal(cush_coord_norm);

        // damping could cause pos force while going away,
        // so, since the limit is not "sticky", clip force sign.
        if (force > 0) {
            force = 0;
        }
        return (force);
    }

    return 0;
}

}  // end namespace chrono
