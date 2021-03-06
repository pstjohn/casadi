/*
 *    This file is part of CasADi.
 *
 *    CasADi -- A symbolic framework for dynamic optimization.
 *    Copyright (C) 2010 by Joel Andersson, Moritz Diehl, K.U.Leuven. All rights reserved.
 *
 *    CasADi is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    CasADi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with CasADi; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "sundials_internal.hpp"
#include "symbolic/stl_vector_tools.hpp"
#include "symbolic/matrix/matrix_tools.hpp"
#include "symbolic/mx/mx_tools.hpp"
#include "symbolic/sx/sx_tools.hpp"
#include "symbolic/fx/mx_function.hpp"
#include "symbolic/fx/sx_function.hpp"

INPUTSCHEME(IntegratorInput)
OUTPUTSCHEME(IntegratorOutput)

using namespace std;
namespace CasADi{
  
SundialsInternal::SundialsInternal(const FX& f, const FX& g) : IntegratorInternal(f,g){
  addOption("max_num_steps",               OT_INTEGER,          10000,          "Maximum number of integrator steps");
  addOption("reltol",                      OT_REAL,             1e-6,           "Relative tolerence for the IVP solution");
  addOption("abstol",                      OT_REAL,             1e-8,           "Absolute tolerence  for the IVP solution");
  addOption("exact_jacobian",              OT_BOOLEAN,          true,           "Use exact Jacobian information for the integration");
  addOption("upper_bandwidth",             OT_INTEGER,          GenericType(),  "Upper band-width of banded Jacobian (estimations)");
  addOption("lower_bandwidth",             OT_INTEGER,          GenericType(),  "Lower band-width of banded Jacobian (estimations)");
  addOption("linear_solver_type",          OT_STRING,           "dense",        "","user_defined|dense|banded|iterative");
  addOption("iterative_solver",            OT_STRING,           "gmres",        "","gmres|bcgstab|tfqmr");
  addOption("pretype",                     OT_STRING,           "none",         "","none|left|right|both");
  addOption("max_krylov",                  OT_INTEGER,          10,             "Maximum Krylov subspace size");
  addOption("sensitivity_method",          OT_STRING,           "simultaneous", "","simultaneous|staggered");
  addOption("max_multistep_order",         OT_INTEGER,          5);
  addOption("use_preconditioner",          OT_BOOLEAN,          false,          "Precondition an iterative solver");
  addOption("stop_at_end",                 OT_BOOLEAN,          false,          "Stop the integrator at the end of the interval");
  
  // Quadratures
  addOption("quad_err_con",                OT_BOOLEAN,          false,          "Should the quadratures affect the step size control");

  // Forward sensitivity problem
  addOption("fsens_err_con",               OT_BOOLEAN,          true,           "include the forward sensitivities in all error controls");
  addOption("finite_difference_fsens",     OT_BOOLEAN,          false,          "Use finite differences to approximate the forward sensitivity equations (if AD is not available)");
  addOption("fsens_reltol",                OT_REAL,             GenericType(),  "Relative tolerence for the forward sensitivity solution [default: equal to reltol]");
  addOption("fsens_abstol",                OT_REAL,             GenericType(),  "Absolute tolerence for the forward sensitivity solution [default: equal to abstol]");
  addOption("fsens_scaling_factors",       OT_REALVECTOR,       GenericType(),  "Scaling factor for the components if finite differences is used");
  addOption("fsens_sensitiviy_parameters", OT_INTEGERVECTOR,    GenericType(),  "Specifies which components will be used when estimating the sensitivity equations");

  // Adjoint sensivity problem
  addOption("steps_per_checkpoint",        OT_INTEGER,          20,             "Number of steps between two consecutive checkpoints");
  addOption("interpolation_type",          OT_STRING,           "hermite",      "Type of interpolation for the adjoint sensitivities","hermite|polynomial");
  addOption("asens_upper_bandwidth",       OT_INTEGER,          GenericType(),  "Upper band-width of banded jacobians for backward integration");
  addOption("asens_lower_bandwidth",       OT_INTEGER,          GenericType(),  "lower band-width of banded jacobians for backward integration");
  addOption("asens_linear_solver_type",    OT_STRING,           "dense",        "","dense|banded|iterative");
  addOption("asens_iterative_solver",      OT_STRING,           "gmres",        "","gmres|bcgstab|tfqmr");
  addOption("asens_pretype",               OT_STRING,           "none",         "","none|left|right|both");
  addOption("asens_max_krylov",            OT_INTEGER,          10,             "Maximum krylov subspace size");
  addOption("asens_reltol",                OT_REAL,             GenericType(),  "Relative tolerence for the adjoint sensitivity solution [default: equal to reltol]");
  addOption("asens_abstol",                OT_REAL,             GenericType(),  "Absolute tolerence for the adjoint sensitivity solution [default: equal to abstol]");
  addOption("linear_solver",       OT_LINEARSOLVER,     GenericType(),  "An linear solver creator function");
  addOption("linear_solver_options",       OT_DICTIONARY,       GenericType(),  "Options to be passed to the linear solver");
}

SundialsInternal::~SundialsInternal(){ 
}

void SundialsInternal::init(){
  // Call the base class method
  IntegratorInternal::init();
 
  // Reset checkpoints counter
  ncheck_ = 0;

  // Read options
  abstol_ = getOption("abstol");
  reltol_ = getOption("reltol");
  exact_jacobian_ = getOption("exact_jacobian");
  max_num_steps_ = getOption("max_num_steps");
  finite_difference_fsens_ = getOption("finite_difference_fsens");
  fsens_abstol_ = hasSetOption("fsens_abstol") ? double(getOption("fsens_abstol")) : abstol_;
  fsens_reltol_ = hasSetOption("fsens_reltol") ? double(getOption("fsens_reltol")) : reltol_;
  asens_abstol_ = hasSetOption("asens_abstol") ? double(getOption("asens_abstol")) : abstol_;
  asens_reltol_ = hasSetOption("asens_reltol") ? double(getOption("asens_reltol")) : reltol_;
  stop_at_end_ = getOption("stop_at_end");
  use_preconditioner_ = getOption("use_preconditioner");

  
  // Linear solver for forward integration
  if(getOption("linear_solver_type")=="dense"){
    linsol_f_ = SD_DENSE;
  } else if(getOption("linear_solver_type")=="banded") {
    linsol_f_ = SD_BANDED;
  } else if(getOption("linear_solver_type")=="iterative") {
    linsol_f_ = SD_ITERATIVE;

    // Iterative solver
    if(getOption("iterative_solver")=="gmres"){
      itsol_f_ = SD_GMRES;
    } else if(getOption("iterative_solver")=="bcgstab") {
      itsol_f_ = SD_BCGSTAB;
    } else if(getOption("iterative_solver")=="tfqmr") {
      itsol_f_ = SD_TFQMR;
    } else throw CasadiException("Unknown sparse solver for forward integration");
      
    // Preconditioning type
    if(getOption("pretype")=="none")               pretype_f_ = PREC_NONE;
    else if(getOption("pretype")=="left")          pretype_f_ = PREC_LEFT;
    else if(getOption("pretype")=="right")         pretype_f_ = PREC_RIGHT;
    else if(getOption("pretype")=="both")          pretype_f_ = PREC_BOTH;
    else                                           throw CasadiException("Unknown preconditioning type for forward integration");
  } else if(getOption("linear_solver_type")=="user_defined") {
    linsol_f_ = SD_USER_DEFINED;
  } else throw CasadiException("Unknown linear solver for forward integration");
  
  // Linear solver for backward integration
  if(getOption("asens_linear_solver_type")=="dense"){
    linsol_g_ = SD_DENSE;
  } else if(getOption("asens_linear_solver_type")=="banded") {
    linsol_g_ = SD_BANDED;
  } else if(getOption("asens_linear_solver_type")=="iterative") {
    linsol_g_ = SD_ITERATIVE;

    // Iterative solver
    if(getOption("asens_iterative_solver")=="gmres"){
      itsol_g_ = SD_GMRES;
    } else if(getOption("asens_iterative_solver")=="bcgstab") {
      itsol_g_ = SD_BCGSTAB;
    } else if(getOption("asens_iterative_solver")=="tfqmr") {
      itsol_g_ = SD_TFQMR;
    } else throw CasadiException("Unknown sparse solver for backward integration");
    
    // Preconditioning type
    if(getOption("asens_pretype")=="none")               pretype_g_ = PREC_NONE;
    else if(getOption("asens_pretype")=="left")          pretype_g_ = PREC_LEFT;
    else if(getOption("asens_pretype")=="right")         pretype_g_ = PREC_RIGHT;
    else if(getOption("asens_pretype")=="both")          pretype_g_ = PREC_BOTH;
    else                                           throw CasadiException("Unknown preconditioning type for backward integration");
  } else if(getOption("asens_linear")=="user_defined") {
    linsol_g_ = SD_USER_DEFINED;
  } else throw CasadiException("Unknown linear solver for backward integration");
  
  // Get the linear solver creator function
  if(linsol_.isNull() && hasSetOption("linear_solver")){
    linearSolverCreator linear_solver_creator = getOption("linear_solver");
  
    // Allocate an NLP solver
    linsol_ = linear_solver_creator(CRSSparsity());
  
    // Pass options
    if(hasSetOption("linear_solver_options")){
      const Dictionary& linear_solver_options = getOption("linear_solver_options");
      linsol_.setOption(linear_solver_options);
    }
  }
}

void SundialsInternal::deepCopyMembers(std::map<SharedObjectNode*,SharedObject>& already_copied){
  IntegratorInternal::deepCopyMembers(already_copied);
  linsol_ = deepcopy(linsol_,already_copied);
}

void SundialsInternal::reset(int nsens, int nsensB, int nsensB_store){
  // Reset the base classes
  IntegratorInternal::reset(nsens,nsensB,nsensB_store);
  
  // Go to the start time
  t_ = t0_;
}

} // namespace CasADi


