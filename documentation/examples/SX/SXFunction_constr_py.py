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
#! SXFunction constructors
#! =======================
from casadi import *

x = SX("x") # A scalar symbolic
y = ssym("y",2,1) # A matrix symbolic


ins = [x,y] # function inputs
outs = [x,y,SXMatrix([[x,x],[x,x]]),y*x,0]

print outs

f = SXFunction(ins,outs)
f.init()

#! f now has two inputs and a 4 outputs:
print f.getNumInputs()
print f.getNumOutputs()

#! The outputs has the following string representation.
#! Note how all elements of out have been converted to SXMatrix by
#! automatic typecasting functionality

for i in range(3):
  print f.outputExpr(i)
