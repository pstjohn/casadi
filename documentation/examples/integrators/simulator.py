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
#! Simulator
#! =====================
from casadi import *
from numpy import *
from pylab import *

#! We will investigate the working of Simulator with the help of the parametrically exited Duffing equation:
#!
#$ $\ddot{u}+\dot{u}-\epsilon (2 \mu \dot{u}+\alpha u^3+2 k u \cos(\Omega t))$ with $\Omega = 2 + \epsilon \sigma$.

t = ssym("t")

u = ssym("u") 
v = ssym("v") 
states = vertcat([u,v])

eps   = ssym("eps")
mu    = ssym("mu")
alpha = ssym("alpha")
k     = ssym("k")
sigma = ssym("sigma")
Omega = 2 + eps*sigma

params = vertcat([eps,mu,alpha,k,sigma])
rhs    = vertcat([v,-u-eps*(2*mu*v+alpha*u**3+2*k*u*cos(Omega*t))])

f=SXFunction(daeIn(x=states,p=params,t=t),daeOut(ode=rhs))
f.init()

integrator = CVodesIntegrator(f)

#! We will simulate over 50 seconds, 1000 timesteps.
ts = linspace(0,50,1000)

sim=Simulator(integrator,ts)
sim.init()
sim.setInput([1,0],INTEGRATOR_X0)
sim.setInput([0.1,0.1,0.1,0.3,0.1],INTEGRATOR_P)
sim.evaluate()

#! Plot the solution
plot(array(sim.output())[:,0],array(sim.output())[:,1])
xlabel("u")
ylabel("u_dot")
show()

