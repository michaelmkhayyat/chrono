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

#include <iomanip>

#include "chrono/solver/ChSystemDescriptor.h"
#include "chrono/solver/ChConstraintTwoTuplesContactN.h"
#include "chrono/solver/ChConstraintTwoTuplesFrictionT.h"
#include "chrono/core/ChMatrix.h"

namespace chrono {

// Register into the object factory, to enable run-time
// dynamic creation and persistence
CH_FACTORY_REGISTER(ChSystemDescriptor)

#define CH_SPINLOCK_HASHSIZE 203

ChSystemDescriptor::ChSystemDescriptor() : n_q(0), n_c(0), c_a(1.0), freeze_count(false) {
    m_constraints.clear();
    m_variables.clear();
    m_KRMblocks.clear();
}

ChSystemDescriptor::~ChSystemDescriptor() {
    m_constraints.clear();
    m_variables.clear();
    m_KRMblocks.clear();
}

void ChSystemDescriptor::ComputeFeasabilityViolation(double& resulting_maxviolation, double& resulting_feasability) {
    resulting_maxviolation = 0;
    resulting_feasability = 0;

    for (const auto& constr : m_constraints) {
        // the the residual of the constraint..
        double mres_i = constr->Compute_c_i();

        double candidate_violation = fabs(constr->Violation(mres_i));

        if (candidate_violation > resulting_maxviolation)
            resulting_maxviolation = candidate_violation;

        if (constr->IsUnilateral()) {
            double candidate_feas = fabs(mres_i * constr->Get_l_i());  // =|c*l|
            if (candidate_feas > resulting_feasability)
                resulting_feasability = candidate_feas;
        }
    }
}

unsigned int ChSystemDescriptor::CountActiveVariables() const {
    if (freeze_count)  // optimization, avoid list count all times
        return n_q;

    n_q = 0;
    for (auto& var : m_variables) {
        if (var->IsActive()) {
            var->SetOffset(n_q);  // also store offsets in state and MC matrix
            n_q += var->GetDOF();
        }
    }
    return n_q;
}

unsigned int ChSystemDescriptor::CountActiveConstraints() const {
    if (freeze_count)  // optimization, avoid list count all times
        return n_c;

    n_c = 0;
    for (auto& constr : m_constraints) {
        if (constr->IsActive()) {
            constr->SetOffset(n_c);  // also store offsets in state and MC matrix
            n_c++;
        }
    }
    return n_c;
}

void ChSystemDescriptor::UpdateCountsAndOffsets() {
    freeze_count = false;
    CountActiveVariables();
    CountActiveConstraints();
    freeze_count = true;
}

void ChSystemDescriptor::PasteMassKRMMatrixInto(ChSparseMatrix& Z,
                                                unsigned int row_offset,
                                                unsigned int col_offset) const {

    //// Contribution of mass or rigid bodies and node-concentrated masses
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            var->PasteMassInto(Z, row_offset, col_offset, c_a);
        }
    }

    // Contribution of stiffness, damping and mass matrices
    for (const auto& KRMBlock : m_KRMblocks) {
        KRMBlock->PasteInto(Z, row_offset, col_offset, false);
    }
}

unsigned int ChSystemDescriptor::PasteConstraintsJacobianMatrixInto(ChSparseMatrix& Z,
                                                                    unsigned int row_offset,
                                                                    unsigned int col_offset,
                                                                    bool only_bilateral) const {
    unsigned int s_c = 0;
    for (const auto& constr : m_constraints) {
        if (constr->IsActive() && !(only_bilateral && !constr->GetMode() == CONSTRAINT_LOCK)) {
            constr->PasteJacobianInto(Z, s_c + row_offset, col_offset);
            s_c++;
        }
    }

    return s_c;
}

unsigned int ChSystemDescriptor::PasteConstraintsJacobianMatrixTransposedInto(ChSparseMatrix& Z,
                                                                              unsigned int row_offset,
                                                                              unsigned int col_offset,
                                                                              bool only_bilateral) const {
    unsigned int s_c = 0;
    for (const auto& constr : m_constraints) {
        if (constr->IsActive() && !(only_bilateral && !constr->GetMode() == CONSTRAINT_LOCK)) {
            constr->PasteJacobianTransposedInto(Z, row_offset, s_c + col_offset);
            s_c++;
        }
    }

    return s_c;
}

void ChSystemDescriptor::PasteComplianceMatrixInto(ChSparseMatrix& Z,
                                                   unsigned int row_offset,
                                                   unsigned int col_offset,
                                                   bool only_bilateral) const {
    int s_c = 0;
    for (const auto& constr : m_constraints) {
        if (constr->IsActive() && !(only_bilateral && !constr->GetMode() == CONSTRAINT_LOCK)) {
            Z.SetElement(row_offset + s_c, col_offset + s_c, constr->Get_cfm_i());
            s_c++;
        }
    }
}

void ChSystemDescriptor::BuildSystemMatrix(ChSparseMatrix* Z, ChVectorDynamic<>* rhs) const {
    n_q = CountActiveVariables();

    n_c = CountActiveConstraints();

    if (Z) {
        Z->conservativeResize(n_q + n_c, n_q + n_c);

        Z->setZeroValues();

        PasteMassKRMMatrixInto(*Z, 0, 0);

        PasteConstraintsJacobianMatrixInto(*Z, n_q, 0);

        PasteConstraintsJacobianMatrixTransposedInto(*Z, 0, n_q);

        PasteComplianceMatrixInto(*Z, n_q, n_q);
    }

    if (rhs) {
        rhs->setZero(n_q + n_c, 1);

        BuildDiVector(*rhs);
    }
}

unsigned int ChSystemDescriptor::BuildFbVector(ChVectorDynamic<>& Fvector, unsigned int row_offset) const {
    n_q = CountActiveVariables();
    Fvector.setZero(n_q);

    // Fills the 'f' vector
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            Fvector.segment(row_offset + var->GetOffset(), var->GetDOF()) = var->Get_fb();
        }
    }
    return n_q;
}

unsigned int ChSystemDescriptor::BuildBiVector(ChVectorDynamic<>& Bvector, unsigned int row_offset) const {
    n_c = CountActiveConstraints();
    Bvector.setZero(n_c);

    // Fill the 'b' vector
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            Bvector(row_offset + constr->GetOffset()) = constr->Get_b_i();
        }
    }

    return n_c;
}

unsigned int ChSystemDescriptor::BuildDiVector(ChVectorDynamic<>& Dvector) const {
    n_q = CountActiveVariables();
    n_c = CountActiveConstraints();
    Dvector.setZero(n_q + n_c);

    // Fills the 'f' vector part
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            Dvector.segment(var->GetOffset(), var->GetDOF()) = var->Get_fb();
        }
    }

    // Fill the '-b' vector (with flipped sign!)
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            Dvector(constr->GetOffset() + n_q) = -constr->Get_b_i();
        }
    }

    return n_q + n_c;
}

unsigned int ChSystemDescriptor::BuildDiagonalVector(ChVectorDynamic<>& Diagonal_vect) const {
    n_q = CountActiveVariables();
    n_c = CountActiveConstraints();
    Diagonal_vect.setZero(n_q + n_c);

    // Fill the diagonal values given by ChKRMBlock objects , if any
    // (This cannot be easily parallelized because of possible write concurrency).
    for (const auto& krm_block : m_KRMblocks) {
        krm_block->DiagonalAdd(Diagonal_vect);
    }

    // Get the 'M' diagonal terms given by ChVariables objects
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            var->DiagonalAdd(Diagonal_vect, c_a);
        }
    }

    // Get the 'E' diagonal terms (E_i = cfm_i )
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            Diagonal_vect(constr->GetOffset() + n_q) = constr->Get_cfm_i();
        }
    }

    return n_q + n_c;
}

unsigned int ChSystemDescriptor::FromVariablesToVector(ChVectorDynamic<>& mvector, bool resize_vector) const {
    // Count active variables and resize vector if necessary
    if (resize_vector) {
        n_q = CountActiveVariables();
        mvector.setZero(n_q);
    }

    // Fill the vector
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            mvector.segment(var->GetOffset(), var->GetDOF()) = var->Get_qb();
        }
    }

    return n_q;
}

unsigned int ChSystemDescriptor::FromVectorToVariables(const ChVectorDynamic<>& mvector) {
    assert(CountActiveVariables() == mvector.rows());

    // fetch from the vector
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            var->Get_qb() = mvector.segment(var->GetOffset(), var->GetDOF());
        }
    }

    return n_q;
}

unsigned int ChSystemDescriptor::FromConstraintsToVector(ChVectorDynamic<>& mvector, bool resize_vector) const {
    // Count active constraints and resize vector if necessary
    if (resize_vector) {
        n_c = CountActiveConstraints();
        mvector.setZero(n_c);
    }

    // Fill the vector
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            mvector(constr->GetOffset()) = constr->Get_l_i();
        }
    }

    return n_c;
}

unsigned int ChSystemDescriptor::FromVectorToConstraints(const ChVectorDynamic<>& mvector) {
    n_c = CountActiveConstraints();

    assert(n_c == mvector.size());

    // Fill the vector
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            constr->Set_l_i(mvector(constr->GetOffset()));
        }
    }

    return n_c;
}

unsigned int ChSystemDescriptor::FromUnknownsToVector(ChVectorDynamic<>& mvector, bool resize_vector) const {
    // Count active variables & constraints and resize vector if necessary
    n_q = CountActiveVariables();
    n_c = CountActiveConstraints();

    if (resize_vector) {
        mvector.setZero(n_q + n_c);
    }

    // Fill the first part of vector, x.q ,with variables q
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            mvector.segment(var->GetOffset(), var->GetDOF()) = var->Get_qb();
        }
    }

    // Fill the second part of vector, x.l, with constraint multipliers -l (with flipped sign!)
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            mvector(constr->GetOffset() + n_q) = -constr->Get_l_i();
        }
    }

    return n_q + n_c;
}

unsigned int ChSystemDescriptor::FromVectorToUnknowns(const ChVectorDynamic<>& mvector) {
    n_q = CountActiveVariables();
    n_c = CountActiveConstraints();

    assert((n_q + n_c) == mvector.size());

    // Fetch from the first part of vector (x.q = q)
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            var->Get_qb() = mvector.segment(var->GetOffset(), var->GetDOF());
        }
    }

    // Fetch from the second part of vector (x.l = -l), with flipped sign!
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            constr->Set_l_i(-mvector(constr->GetOffset() + n_q));
        }
    }

    return n_q + n_c;
}

void ChSystemDescriptor::SchurComplementProduct(ChVectorDynamic<>& result,
                                                const ChVectorDynamic<>& lvector,
                                                std::vector<bool>* enabled) {
    // currently, the case with ChKRMBlock items is not supported (only diagonal M is supported, no K)
    assert(m_KRMblocks.size() == 0);
    assert(lvector.size() == CountActiveConstraints());

    result.setZero(n_c);

    // Performs the sparse product    result = [N]*l = [ [Cq][M^(-1)][Cq'] - [E] ] *l
    // in different phases:

    // 1 - set the qb vector (aka speeds, in each ChVariable sparse data) as zero

    for (const auto& var : m_variables) {
        if (var->IsActive())
            var->Get_qb().setZero();
    }

    // 2 - performs    qb=[M^(-1)][Cq']*l  by
    //     iterating over all constraints (when implemented in parallel this
    //     could be non-trivial because race conditions might occur -> reduction buffer etc.)
    //     Also, begin to add the cfm term ( -[E]*l ) to the result.

    // ATTENTION:  this loop cannot be parallelized! Concurrent write to some q may happen
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            int s_c = constr->GetOffset();

            bool process = (!enabled) || (*enabled)[s_c];

            if (process) {
                double li = lvector(s_c);

                // Compute qb += [M^(-1)][Cq']*l_i
                //  NOTE! concurrent update to same q data, risk of collision if parallel.
                constr->Increment_q(li);  // computationally intensive

                // Add constraint force mixing term  result = cfm * l_i = [E]*l_i
                result(s_c) = constr->Get_cfm_i() * li;
            }
        }
    }

    // 3 - performs    result=[Cq']*qb    by
    //     iterating over all constraints

    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            bool process = (!enabled) || (*enabled)[constr->GetOffset()];

            if (process)
                result(constr->GetOffset()) += constr->Compute_Cq_q();  // computationally intensive
            else
                result(constr->GetOffset()) = 0;  // not enabled constraints, just set to 0 result
        }
    }
}

void ChSystemDescriptor::SystemProduct(ChVectorDynamic<>& result, const ChVectorDynamic<>& x) {
    n_q = CountActiveVariables();
    n_c = CountActiveConstraints();

    result.setZero(n_q + n_c);

    // 1) First row: result.q part =  [M + K]*x.q + [Cq']*x.l

    // 1.1)  do  M*x.q
    for (const auto& var : m_variables) {
        if (var->IsActive()) {
            var->MultiplyAndAdd(result, x, c_a);
        }
    }

    // 1.2)  add also K*x.q  (NON straight parallelizable - risk of concurrency in writing)
    for (const auto& krm_block : m_KRMblocks) {
        krm_block->MultiplyAndAdd(result, x);
    }

    // 1.3)  add also [Cq]'*x.l  (NON straight parallelizable - risk of concurrency in writing)
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            constr->MultiplyTandAdd(result, x(constr->GetOffset() + n_q));
        }
    }

    // 2) Second row: result.l part =  [C_q]*x.q + [E]*x.l
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            int s_c = constr->GetOffset() + n_q;
            constr->MultiplyAndAdd(result(s_c), x);       // result.l_i += [C_q_i]*x.q
            result(s_c) += constr->Get_cfm_i() * x(s_c);  // result.l_i += [E]*x.l_i
        }
    }
}

void ChSystemDescriptor::ConstraintsProject(ChVectorDynamic<>& multipliers) {
    FromVectorToConstraints(multipliers);

    for (const auto& constr : m_constraints) {
        if (constr->IsActive())
            constr->Project();
    }

    FromConstraintsToVector(multipliers, false);
}

void ChSystemDescriptor::UnknownsProject(ChVectorDynamic<>& mx) {
    n_q = CountActiveVariables();

    // vector -> constraints
    // Fetch from the second part of vector (x.l = -l), with flipped sign!
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            constr->Set_l_i(-mx(constr->GetOffset() + n_q));
        }
    }

    // constraint projection!
    for (const auto& constr : m_constraints) {
        if (constr->IsActive())
            constr->Project();
    }

    // constraints -> vector
    // Fill the second part of vector, x.l, with constraint multipliers -l (with flipped sign!)
    for (const auto& constr : m_constraints) {
        if (constr->IsActive()) {
            mx(constr->GetOffset() + n_q) = -constr->Get_l_i();
        }
    }
}

// -----------------------------------------------------------------------------

void ChSystemDescriptor::WriteMatrixBlocks(const std::string& path, const std::string& prefix, bool one_indexed) {
    ChSparseMatrix mass_matrix(CountActiveVariables(), CountActiveVariables());
    ChSparseMatrix jacob_matrix(CountActiveConstraints(), CountActiveVariables());
    ChSparseMatrix compl_matrix(CountActiveConstraints(), CountActiveConstraints());
    ChVectorDynamic<double> f;
    ChVectorDynamic<double> b;

    mass_matrix.setZeroValues();
    jacob_matrix.setZeroValues();
    f.setZero(CountActiveVariables());
    b.setZero(CountActiveConstraints());

    PasteMassKRMMatrixInto(mass_matrix);
    PasteConstraintsJacobianMatrixInto(jacob_matrix);
    PasteComplianceMatrixInto(compl_matrix);
    BuildFbVector(f);
    BuildBiVector(b);

    std::ofstream file_M(path + "/" + prefix + "_M.dat");
    file_M << std::setprecision(12) << std::scientific;
    StreamOut(mass_matrix, file_M, one_indexed);

    std::ofstream file_Cq(path + "/" + prefix + "_Cq.dat");
    file_Cq << std::setprecision(12) << std::scientific;
    StreamOut(jacob_matrix, file_Cq, one_indexed);

    std::ofstream file_E(path + "/" + prefix + "_E.dat");
    file_E << std::setprecision(12) << std::scientific;
    StreamOut(compl_matrix, file_E, one_indexed);

    std::ofstream file_f(path + "/" + prefix + "_f.dat");
    file_f << std::setprecision(12) << std::scientific;
    StreamOut(f, file_f);

    std::ofstream file_b(path + "/" + prefix + "_b.dat");
    file_b << std::setprecision(12) << std::scientific;
    StreamOut(b, file_b);
}

void ChSystemDescriptor::WriteMatrix(const std::string& path, const std::string& prefix, bool one_indexed) {
    ChSparseMatrix Z;
    ChVectorDynamic<double> rhs;
    BuildSystemMatrix(&Z, &rhs);

    std::ofstream file_Z(path + "/" + prefix + "_Z.dat");
    file_Z << std::setprecision(12) << std::scientific;
    StreamOut(Z, file_Z, one_indexed);

    std::ofstream file_rhs(path + "/" + prefix + "_rhs.dat");
    file_rhs << std::setprecision(12) << std::scientific;
    StreamOut(rhs, file_rhs);
}

void ChSystemDescriptor::WriteMatrixSpmv(const std::string& path, const std::string& prefix, bool one_indexed) {
    // Count constraints.
    int mn_c = 0;
    for (auto& cnstr : m_constraints) {
        if (cnstr->IsActive())
            mn_c++;
    }

    // Generate dense system matrix, column by column
    int size = n_q + mn_c;
    ChMatrixDynamic<> A(size, size);
    ChVectorDynamic<> v(size);
    ChVectorDynamic<> e(size);
    e.setZero();

    for (int i = 0; i < size; i++) {
        e[i] = 1;
        SystemProduct(v, e);
        A.col(i) = v;
        e[i] = 0;
    }

    // Convert to Eigen sparse matrix
    ChSparseMatrix Z = A.sparseView();

    // Write sparse matrix to file
    std::ofstream file_Z(path + "/" + prefix + "_Z.dat");
    file_Z << std::setprecision(12) << std::scientific;
    StreamOut(Z, file_Z, one_indexed);

    // Write RHS to file
    ChVectorDynamic<double> rhs;
    BuildSystemMatrix(nullptr, &rhs);
    std::ofstream file_rhs(path + "/" + prefix + "_rhs.dat");
    file_rhs << std::setprecision(12) << std::scientific;
    StreamOut(rhs, file_rhs);
}

}  // end namespace chrono
