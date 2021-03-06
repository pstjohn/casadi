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
from casadi import *
from time import *
import numpy as NP

# Calculates the determinant by minor expansion
def f(A):
  n = A.shape[0]    # Number of rows
  if n==1: return A # If scalar
  
  # Expand along the first column
  R = 0             # Return value
  for i in range(n):
    # Remove row i and column 0 from A
    M = A[range(i)+range(i+1,n),1:,]
    
    # Add/subtract the minor
    if i % 2 == 0:  R += A[i,0]*f(M)
    else:           R -= A[i,0]*f(M)
  return R

# Random matrices of different sizes
x0 = list(NP.random.rand(n,n) for n in range(15))

# What to check
check_sx_oo = True
check_sx_sct = False
check_mx_oo = False
check_mx_sx_oo = False

# Check scalar representation
if check_sx_oo:
  print "SX OO"
  for n in range(5,11):
    # Create function
    x = ssym("X",n,n)
    F = SXFunction([x],[f(x)])
    F.setOption("live_variables",True)
    F.init()
    
    # Calculate gradient
    F.setInput(x0[n])
    F.setAdjSeed(1.0)
    print "starting evaluation: ", F.countNodes(), " nodes"
    t1 = time()
    n_repeats = 100
    for _ in range(n_repeats):
      F.evaluate(0,1)
    t2 = time()
    print n, ": ", (t2-t1)/n_repeats, " s, ", F.adjSens()

if check_sx_sct:
  print "SX SCT"
  for n in range(5,10):
    # Create function
    x = ssym("X",n,n)
    F = SXFunction([x],[gradient(f(x),x)])
    F.setOption("live_variables",True)
    F.init()
    
    # Calculate gradient
    F.setInput(x0[n])
    print "starting evaluation: ", F.countNodes(), " nodes"
    t1 = time()
    n_repeats = 100
    for _ in range(n_repeats):
      F.evaluate(0)
    t2 = time()
    print n, ": ", (t2-t1)/n_repeats, " s, ", F.output()

# Check matrix representation
if check_mx_oo:
  print "MX OO"
  for n in range(5,9):
    # Create function
    x = msym("X",n,n)
    F = MXFunction([x],[f(x)])
    F.init()
    
    # Calculate gradient
    F.setInput(x0[n])
    F.setAdjSeed(1.0)
    print "starting evaluation: ", F.countNodes(), " nodes"
    t1 = time()
    n_repeats = 100
    for _ in range(n_repeats):
      F.evaluate(0,1)
    t2 = time()
    print n, ": ", (t2-t1)/n_repeats, " s, ", F.adjSens()

# Create a function for calculating the determinant of a "small" matrix
n_small = 7
x_small = ssym("x",n_small,n_small)
F_small = SXFunction([x_small],[f(x_small)])
F_small.init()

# Calculates the determinant by minor expansion
def f_mod(A):
  n = A.shape[0]    # Number of rows
  if n==n_small:
    [d] = F_small.call([A]) # Create a function call
    return d
  
  # Expand along the first column
  R = 0             # Return value
  for i in range(n):
    # Remove row i and column 0 from A
    M = A[range(i)+range(i+1,n),1:,]
    
    # Add/subtract the minor
    if i % 2 == 0:  R += A[i,0]*f_mod(M)
    else:           R -= A[i,0]*f_mod(M)
  return R
    
# Check matrix representation
if check_mx_sx_oo:
  print "MX+SX OO"
  for n in range(n_small,12):
    # Create function
    x = msym("X",n,n)
    F = MXFunction([x],[f_mod(x)])
    F.init()
    
    # Calculate gradient
    F.setInput(x0[n])
    F.setAdjSeed(1.0)
    print "starting evaluation: ", F.countNodes(), " nodes"
    t1 = time()
    n_repeats = 100
    for _ in range(n_repeats):
      F.evaluate(0,1)
    t2 = time()
    print n, ": ", (t2-t1)/n_repeats, " s, ", F.adjSens()
  
#X = ssym("X",2,2)
#print "f(X) = ", f(X)

#X = msym("X",2,2)
#print "f(X) = ", f(X)

