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

# Excercise 1, chapter 10 from Larry Biegler's book
print "program started"

# Test with different number of elements
for N in range(1,11):
  print "N = ", N
  
  # Degree of interpolating polynomial
  K = 2
  
  # Legrandre roots
  tau_root = [0., 0.211325, 0.788675]

  # Radau roots (K=3)
  #tau_root = [0, 0.155051, 0.644949, 1]

  # Time
  t = ssym("t")
  
  # Differential equation
  z = ssym("z")
  F = SXFunction([z],[z*z - 2*z + 1])
  F.setOption("name","dz/dt")
  F.init()
  
  z0 = -3
  
  # Analytic solution
  z_analytic = SXFunction([t], [(4*t-3)/(3*t+1)])
  z_analytic.setOption("name","analytic solution")
  
  # Collocation point
  tau = ssym("tau")

  # Step size
  h = 1.0/N
  
  # Lagrange polynomials
  l = []
  for j in range(K+1):
    L = 1
    for k in range(K+1):
      if(k != j):
        L *= (tau-tau_root[k])/(tau_root[j]-tau_root[k])

    print "l(", j, ") = ", L

    f = SXFunction([tau],[L])
    f.setOption("name", "l(" + str(j) + ")")
    
    # initialize
    f.init()
    l.append(f)
  
  # Get the coefficients of the continuity equation
  D = DMatrix.zeros(K+1)
  for j in range(K+1):
    l[j].setInput(1.)
    l[j].evaluate()
    D[j] = l[j].output()
  print "D = ", D

  # Get the coefficients of the collocation equation using AD
  C = DMatrix.zeros(K+1,K+1)
  for j in range(K+1):
    for k in range(K+1):
      l[j].setInput(tau_root[k])
      l[j].setFwdSeed(1.0)
      l[j].evaluate(1,0)
      C[j,k] = l[j].fwdSens()
  print "C = ", C
  
  # Collocated states
  Z = ssym("Z",N,K+1)
    
  # Construct the NLP
  x = vec(Z.T)
  g = SXMatrix()
  for i in range(N):
    for k in range(1,K+1):
      # Add collocation equations to NLP
      rhs = 0
      for j in range(K+1):
        rhs += Z[i,j]*C[j,k]
      [FF] = F.eval([Z[i,k]])
      g.append(h*FF-rhs)

    # Add continuity equation to NLP
    rhs = 0
    for j in range(K+1):
      rhs += D[j]*Z[i,j]

    if(i<N-1):
      g.append(Z[i+1,0] - rhs)

  print "g = ", g
  
  # Constraint function
  gfcn = SXFunction([x],[g])

  # Dummy objective function
  obj = SXFunction([x], [x[0]*x[0]])
  
  ## ----
  ## SOLVE THE NLP
  ## ----
  
  # Allocate an NLP solver
  solver = IpoptSolver(obj,gfcn)

  # Set options
  solver.setOption("tol",1e-10)

  # initialize the solver
  solver.init()

  # Initial condition
  xinit = x.size() * [0]
  solver.setInput(xinit,NLP_X_INIT)

  # Bounds on x
  lbx = x.size()*[-100]
  ubx = x.size()*[100]
  lbx[0] = ubx[0] = z0
  solver.setInput(lbx,NLP_LBX)
  solver.setInput(ubx,NLP_UBX)
  
  # Bounds on the constraints
  lubg = g.size()*[0]
  solver.setInput(lubg,NLP_LBG)
  solver.setInput(lubg,NLP_UBG)
  
  # Solve the problem
  solver.solve()
  
  ## Print the time points
  t_opt = N*(K+1) * [0]
  for i in range(N):
    for j in range(K+1):
      t_opt[j + (K+1)*i] = h*(i + tau_root[j])
  
  print "time points: ", t_opt

  # Print the optimal cost
  print "optimal cost: ", float(solver.output(NLP_COST))

  # Print the optimal solution
  xopt = solver.output(NLP_X_OPT).data()
  print "optimal solution: ", xopt
 
  # plot to screen
  plt.plot(t_opt,xopt)

# show the plots
plt.show()
  