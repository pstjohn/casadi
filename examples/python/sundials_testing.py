#
#     This file is part of CasADi.
# 
#     CasADi -- A symbolic framework for dynamic optimization.
#     Copyright (C) 2010 by Joel Andersson, Moritz Diehl, K.U.Leuven. All rights reserved.
# 
#     CasADi is free software; you can redistribute it and/or
#     modify it under the terms of the GNU Lesser General Public
#     License as published by the Free Software Foundation; either
#     version 3 of the License, or (at your option) any later version.
# 
#     CasADi is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#     Lesser General Public License for more details.
# 
#     You should have received a copy of the GNU Lesser General Public
#     License along with CasADi; if not, write to the Free Software
#     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
# 
# 
# -*- coding: utf-8 -*-
from casadi import *
from numpy import *
import matplotlib.pyplot as plt

# This demonstration is intended to demonstrate CasADi's Sundials interface
# The flags set in the beginning of the script demonstrates the most important features.
# Joel Andersson, 2010

# Use CVodes or IDAS
implicit_integrator = False

# Use a collocation based integrator + KINSOL
collocation_integrator = False

# test adjoint sensitivities
with_asens = False

# use exact jacobian
exact_jacobian = True

# Calculate the forward sensitivities using finite differences
finite_difference_fsens = not exact_jacobian

# Calculate initial condition (for IDAS only)
calc_ic = False

# Perturb x or u
perturb_u = True

# Use a user_defined linear solver
user_defined_solver = True

# Use sparse direct solver (CSparse)
sparse_direct = True

# Second order sensitivities by a symbolic-numeric approach
second_order = False

# Derivatives via source code transformation
with_sct = True

# Create an IDAS instance (fully implicit integrator)
def create_IDAS():
  # Time 
  t = SX("t")

  # Differential states
  s = ssym("s")
  v = ssym("v")
  m = ssym("m")
  y = vertcat([s,v,m])
  
  # Control
  u = ssym("u")
  
  # Reference trajectory
  u_ref = 3-sin(t)
  
  # Square deviation from the state trajectory
  u_dev = u-u_ref
  u_dev *= u_dev
  
  # Differential equation (fully implicit form)
  ode_res = vertcat([v,  (u-0.02*v*v)/m, -0.01*u*u])

  # DAE residual function
  ffcn = SXFunction(daeIn(t=t,x=y,p=u),daeOut(ode=ode_res,quad=u_dev))
  
  if collocation_integrator:
    # Create a collocation integrator
    integrator = CollocationIntegrator(ffcn)
  
    # Set some options
    integrator.setOption("implicit_solver",KinsolSolver)
    integrator.setOption("implicit_solver_options",{'linear_solver_creator' : CSparse})
    integrator.setOption("quadrature_solver",CSparse)
    integrator.setOption("startup_integrator",IdasIntegrator)
    integrator.setOption("expand_f",True)
    integrator.setOption("expand_q",True)
    
  else:
    # Create an integrator
    integrator = IdasIntegrator(ffcn)

    # Set IDAS specific options
    integrator.setOption("calc_ic",calc_ic)

  # Return the integrator
  return integrator

# Create an CVODES instance (ODE integrator)
def create_CVODES():
  # Time 
  t = ssym("t")

  # Differential states
  s = ssym("s")
  v = ssym("v")
  m = ssym("m")
  x = vertcat([s,v,m])
  
  # Control
  u = ssym("u")
  
  # Reference trajectory
  u_ref = 3-sin(t)
  
  # Square deviation from the state trajectory
  quad = (u-u_ref)**2
  
  # Differential equation (fully implicit form)
  ode = vertcat([v, (u-0.02*v*v)/m, -0.01*u*u])

  # DAE residual function
  ffcn = SXFunction(daeIn(t=t,x=x,p=u),daeOut(ode=ode,quad=quad))
  
  if collocation_integrator:
    # Create a collocation integrator
    integrator = CollocationIntegrator(ffcn)
  
    # Set some options
    integrator.setOption("implicit_solver",KinsolSolver)
    integrator.setOption("implicit_solver_options",{'linear_solver_creator' : CSparse})
    integrator.setOption("quadrature_solver",CSparse)
    integrator.setOption("startup_integrator",CVodesIntegrator)
    integrator.setOption("expand_f",True)
    integrator.setOption("expand_q",True)
    
  else:
    # Create an integrator
    integrator = CVodesIntegrator(ffcn)
  
  # Return the integrator
  return integrator

# Time horizon
t0 = 0
tf = 10

# Bounds on the control
u_lb = -0.5
u_ub = 1.3
u_init = 1

# Initial conditions
x0 = [0,0,1]
  
# Integrator
if implicit_integrator:
  integrator = create_IDAS()
else:
  integrator = create_CVODES()


# Attach user-defined linear solver
if user_defined_solver:
  if sparse_direct:
    #integrator.setOption("linear_solver_creator",SuperLU)
    integrator.setOption("linear_solver_creator",CSparse)
  else:
    integrator.setOption("linear_solver_creator",LapackLUDense)

# Set common integrator options
integrator.setOption("fsens_err_con",True)
integrator.setOption("quad_err_con",True)
integrator.setOption("abstol",1e-12)
integrator.setOption("reltol",1e-12)
integrator.setOption("fsens_abstol",1e-6)
integrator.setOption("fsens_reltol",1e-6)
integrator.setOption("asens_abstol",1e-6)
integrator.setOption("asens_reltol",1e-6)
integrator.setOption("exact_jacobian",exact_jacobian)
integrator.setOption("finite_difference_fsens",finite_difference_fsens)
integrator.setOption("max_num_steps",100000)

# Set time horizon
integrator.setOption("t0",t0)
integrator.setOption("tf",tf)

if(user_defined_solver):
  integrator.setOption("linear_solver","user_defined")

# Initialize the integrator
integrator.init()

# Set parameters
integrator.setInput(u_init,INTEGRATOR_P)

# Set inital state
integrator.setInput(x0,INTEGRATOR_X0)

# Integrate
integrator.evaluate()

# Save the result
res0 = DMatrix(integrator.output())

# Perturb in some direction
if perturb_u:
  u_pert = u_init + 0.01
  integrator.setInput(u_pert,INTEGRATOR_P)
else:
  x_pert = list(x0)
  x_pert[1] += 0.01
  integrator.setInput(x_pert,INTEGRATOR_X0)
  
# Integrate again
integrator.evaluate()
  
# Print statistics
integrator.printStats()

# Calculate finite difference approximation
fd = (integrator.output() - res0) / 0.01
  
print "unperturbed                     ", res0
print "perturbed                       ", integrator.output()
print "finite_difference approximation ", fd

# forward seeds
if perturb_u:
  u_seed = 1
  integrator.setFwdSeed(u_seed,INTEGRATOR_P)
else:
  x0_seed = len(x0)*[0.]
  x0_seed[1] = 1
  integrator.setFwdSeed(x0_seed,INTEGRATOR_X0)
  
# Reset parameters
integrator.setInput(u_init,INTEGRATOR_P)
  
# Reset initial state
integrator.setInput(x0,INTEGRATOR_X0)

# backward seeds
bseed = len(x0)*[0.]
bseed[0] = 1
integrator.setAdjSeed(bseed,INTEGRATOR_XF)

if with_asens:
  # evaluate with forward and adjoint sensitivities
  integrator.evaluate(1,1)
else:
  # evaluate with only forward sensitivities
  integrator.evaluate(1,0)
    
print "forward sensitivities           ", integrator.fwdSens()

if with_asens:
  print "adjoint sensitivities           ",
  print integrator.adjSens(INTEGRATOR_X0), " ",
  print integrator.adjSens(INTEGRATOR_P), " "
  
# Derivatives via source code transformation
if with_sct:
  # Forward seeds
  fintegrator = integrator.derivative(1,1)
  fintegrator.setInput(integrator.input(INTEGRATOR_X0),INTEGRATOR_X0)
  fintegrator.setInput(integrator.input(INTEGRATOR_P),INTEGRATOR_P)
  fintegrator.setInput(integrator.fwdSeed(INTEGRATOR_X0),INTEGRATOR_NUM_IN+INTEGRATOR_X0)
  fintegrator.setInput(integrator.fwdSeed(INTEGRATOR_P),INTEGRATOR_NUM_IN+INTEGRATOR_P)
  fintegrator.setInput(integrator.adjSeed(INTEGRATOR_XF),2*INTEGRATOR_NUM_IN+INTEGRATOR_XF)
  fintegrator.setInput(integrator.adjSeed(INTEGRATOR_QF),2*INTEGRATOR_NUM_IN+INTEGRATOR_QF)
  
  fintegrator.evaluate()

  print "forward sensitivities via sct   ", fintegrator.output(INTEGRATOR_NUM_OUT+INTEGRATOR_XF)
  print "adjoint sensitivities via sct   ",
  print fintegrator.output(2*INTEGRATOR_NUM_OUT+INTEGRATOR_X0), " ",
  print fintegrator.output(2*INTEGRATOR_NUM_OUT+INTEGRATOR_P), " "
  
if second_order:
  # Generate the jacobian by creating a new integrator for the sensitivity equations by source transformation
  intjac = integrator.jacobian(INTEGRATOR_P,INTEGRATOR_XF)

  # Set options
  intjac.setOption("number_of_fwd_dir",0)
  intjac.setOption("number_of_adj_dir",1)
    
  # Initialize the integrator
  intjac.init()

  # Set inputs
  intjac.setInput(u_init,INTEGRATOR_P)
  intjac.setInput(x0,INTEGRATOR_X0)
    
  # Set adjoint seed
  jacseed = 4*[0]
  jacseed[0] = 1;
  intjac.setAdjSeed(jacseed)
    
  # Evaluate the Jacobian
  intjac.evaluate(0,1)

  # Get the results
  print "second order (fwd-over-adj)     ",
  print intjac.adjSens(INTEGRATOR_X0), ", ",
  print intjac.adjSens(INTEGRATOR_P)

  
  
  
