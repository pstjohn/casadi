Function objects
================

`CasADi` allows the user to create function objects, in C++ terminology often referred to as *functors*. This includes functions that are defined by a symbolic expression, ODE/DAE integrators, QP solvers, NLP solvers etc.

Function objects are typically created with the syntax:

``` python
f = functionname(name, arguments, ..., [options])
```

The name is mainly a display name that will show up in e.g. error messages or as comments in generated C code. This is followed by a set of arguments, which is class dependent. Finally, the user can pass an options structure for customizing the behavior of the class. The options structure is a dictionary type in Python, a struct in MATLAB or `CasADi`’s `Dict` type in C++.

A `Function` can be constructed by passing a list of input expressions and a list of output expressions:

``` python
# Python
x = SX.sym('x',2)
y = SX.sym('y')
f = Function('f',[x,y],\
           [x,sin(y)*x])
```

``` matlab
% MATLAB
x = SX.sym('x',2);
y = SX.sym('y');
f = Function('f',{x,y},...
           {x,sin(y)*x});
```

which defines a function $f$ : $f : \mathbb{R}^{2} \times \mathbb{R} \rightarrow \mathbb{R}^{2} \times \mathbb{R}^{2}, \quad  (x,y) \mapsto (x,\sin(y) x)$. Note that all function objects in `CasADi`, including the above, are multiple matrix-valued input, multiple, matrix-valued output.

`MX` expression graphs work the same way:

``` python
# Python
x = MX.sym('x',2)
y = MX.sym('y')
f = Function('f',[x,y],\
             [x,sin(y)*x])
```

``` matlab
% MATLAB
x = MX.sym('x',2);
y = MX.sym('y');
f = Function('f',{x,y},...
             {x,sin(y)*x});
```

When creating a `Function` from expressions like that, it is always advisory to *name* the inputs and outputs as follows:

``` python
# Python
x = MX.sym('x',2)
y = MX.sym('y')
f = Function('f',[x,y],\
      [x,sin(y)*x],\
      ['x','y'],['r','q'])
```

``` matlab
% MATLAB
x = MX.sym('x',2);
y = MX.sym('y');
f = Function('f',{x,y},...
      {x,sin(y)*x},...
      {'x','y'},{'r','q'});
```

Naming inputs and outputs is preferred for a number of reasons:

-   No need to remember the number or order of arguments

-   Inputs or outputs that are absent can be left unset

-   More readable and less error prone syntax. E.g. `f.jacobian('x','q')` instead of `f.jacobian(0,1)`.

For `Function` instances – to be encountered later – that are *not* created directly from expressions, the inputs and outputs are named automatically.

Calling function objects
------------------------

`MX` expressions may contain calls to `Function`-derived functions. Calling a function object is both done for the numerical evaluation and, by passing symbolic arguments, for embedding a *call* to the function object into an expression graph (cf. also Section integrator).

To call a function object, you either pass the argument in the correct order:

``` python
# Python
r0, q0 = f(1.1,3.3)
print('r0:',r0)
print('q0:',q0)
```

``` matlab
% MATLAB
[r0, q0] = f(1.1,3.3);
display(r0)
display(q0)
```

or the arguments and their names as follows, which will result in a dictionary (`dict` in Python, `struct` in MATLAB and `std::map<std::string, MatrixType>` in C++):

``` python
# Python
res = f(x=1.1, y=3.3)
print('res:', res)
```

``` matlab
% MATLAB
res = f('x',1.1,'y',3.3);
display(res)
```

When calling a function object, the dimensions (but not necessarily the sparsity patterns) of the evaluation arguments have to match those of the function inputs, with two exceptions:

-   A row vector can be passed instead of a column vector and vice versa.

-   A scalar argument can always be passed, regardless of the input dimension. This has the meaning of setting all elements of the input matrix to that value.

When the number of inputs to a function object is large or changing, an alternative syntax to the above is to use the *call* function which takes a Python list / MATLAB cell array or, alternatively, a Python dict / MATLAB struct. The return value will have the same type:

``` python
# Python
arg = [1.1,3.3]
res = f.call(arg)
print('res:', res)
arg = {'x':1.1,'y':3.3}
res = f.call(arg)
print('res:', res)
```

``` matlab
% MATLAB
arg = {1.1,3.3};
res = f.call(arg);
display(res)
arg = struct('x',1.1,'y',3.3);
res = f.call(arg);
display(res)
```

Converting `MX` to `SX`
-----------------------

A function object defined by an `MX` graph that only contains built-in operations (e.g. elementwise operations such as addition, square root, matrix multiplications and calls to `SX` functions, can be converted into a function defined purely by an `SX` graph using the syntax:

``` python
sx_function = mx_function.expand()
```

This might speed up the calculations significantly, but might also cause extra memory overhead.

Nonlinear root-finding problems
-------------------------------

Consider the following system of equations:
$$
\begin{aligned}
&g_0(z, x\_1, x\_2, \ldots, x_n) &&= 0 \\
&g_1(z, x\_1, x\_2, \ldots, x_n) &&= y\_1 \\
&g_2(z, x\_1, x\_2, \ldots, x_n) &&= y\_2 \\
&\qquad \vdots \qquad &&\qquad \\
&g\_m(z, x_1, x_2, \ldots, x_n) &&= y_m,
\end{aligned}$$
where the first equation uniquely defines $z$ as a function of $x_1, \ldots, x_n$ by the *implicit function theorem*
and the remaining equations define the auxiliary outputs $y_1, \ldots, y_m$.

Given a function $g$ for evaluating $g_0, \ldots, g_m$, we can use `CasADi` to automatically formulate a function
$G: \{z_{\text{guess}}, x_1, x_2, \ldots, x_n\} \rightarrow \{z, y_1, y_2, \ldots, y_m\}$.
This function includes a guess for $z$ to handle the case when the solution is non-unique.
The syntax for this, assuming $n=m=1$ for simplicity, is:

``` python
# Python
z = SX.sym('x',nz)
x = SX.sym('x',nx)
g0 = (an expression of x, z)
g1 = (an expression of x, z)
g = Function('g',[z,x],[g0,g1])
G = rootfinder('G','newton',g)
```

``` matlab
% MATLAB
z = SX.sym('x',nz);
x = SX.sym('x',nx);
g0 = (an expression of x, z)
g1 = (an expression of x, z)
g = Function('g',{z,x},{g0,g1});
G = rootfinder('G','newton',g);
```

where the `rootfinder` function expects a display name, the name of a solver plugin (here a simple full-step Newton method) and the residual function.

Rootfinding objects in `CasADi` are differential objects and derivatives can be calculated exactly to arbitrary order.

Initial-value problems and sensitivity analysis
-----------------------------------------------

`CasADi` can be used to solve initial-value problems in ODE or DAE. The problem formulation used is a DAE of semi-explicit form with quadratures:

$$
\begin{align}
 \dot{x} &= f_{\text{ode}}(t,x,z,p), \qquad x(0) = x_0 \\
      0  &= f_{\text{alg}}(t,x,z,p) \\
 \dot{q} &= f_{\text{quad}}(t,x,z,p), \qquad q(0) = 0
\end{align}
$$

For solvers of *ordinary* differential equations, the second equation and the algebraic variables $z$ must be absent.

An integrator in `CasADi` is a function that takes the state at the initial time `x0`, a set of parameters `p`, and a guess for the algebraic variables (only for DAEs) `z0` and returns the state vector `xf`, algebraic variables `zf` and the quadrature state `qf`, all at the final time.

The freely available [SUNDIALS suite](https://computation.llnl.gov/casc/sundials/description/description.html) (distributed along with `CasADi`) contains the two popular integrators CVodes and IDAS for ODEs and DAEs respectively. These integrators have support for forward and adjoint sensitivity analysis and when used via `CasADi`’s Sundials interface, `CasADi` will automatically formulate the Jacobian information, which is needed by the backward differentiation formula (BDF) that CVodes and IDAS use. Also automatically formulated will be the forward and adjoint sensitivity equations.

### Creating integrators

Integrators are created using `CasADi`’s `integrator` function. Different integrators schemes and interfaces are implemented as *plugins*, essentially shared libraries that are loaded at runtime.

Consider for example the DAE:

$$
\begin{align}
 \dot{x} &= z+p, \\
      0  &= z \, \cos(z)-x
\end{align}
$$

An integrator, using the ”idas” plugin, can be created using the syntax:

``` python
# Python
x = SX.sym('x'); z = SX.sym('z'); p = SX.sym('p')
dae = {'x':x, 'z':z, 'p':p, 'ode':z+p, 'alg':z*cos(z)-x}
F = integrator('F', 'idas', dae)
```

``` matlab
% MATLAB
x = SX.sym('x'); z = SX.sym('z'); p = SX.sym('p');
dae = struct('x',x,'z',z,'p',p,'ode',z+p,'alg',z*cos(z)-x);
F = integrator('F', 'idas', dae);
```

Integrating this DAE from 0 to 1 with $x(0)=0$, $p=0.1$ and using the guess $z(0)=0$, can
be done by evaluating the created function object:

``` python
# Python
r = F(x0=0, z0=0, p=0.1)
print(r['xf'])
```

``` matlab
% MATLAB
r = F('x0',0,'z0',0,'p',0.1);
disp(r.xf)
```

The time horizon is assumed to be fixed[1] and can be changed from its default $[0, 1]$ by setting the options `t0` and `tf`.

### Sensitivity analysis

From a usage point of view, an integrator behaves just like the function objects created from expressions earlier in the chapter. You can use member functions in the Function class to generate new function objects corresponding to directional derivatives (forward or reverse mode) or complete Jacobians. Then evaluate these function objects numerically to obtain sensitivity information. The documented example “sensitivity_analysis” (available in `CasADi`’s example collection for Python, MATLAB and C++) demonstrate how `CasADi` can be used to calculate first and second order derivative information (forward-over-forward, forward-over-adjoint, adjoint-over-adjoint) for a simple DAE.

Nonlinear programming
---------------------

The NLP solvers distributed with or interfaced to `CasADi` solves parametric NLPs of the following form:
$$
\begin{array}{cc}
\begin{array}{c}
\text{minimize:} \\
x
\end{array}
&
f(x,p)
\\
\begin{array}{c}
\text{subject to:}
\end{array}
&
\begin{array}{rcl}
  x_{lb} \le &  x   & \le x_{ub} \\
  g_{lb} \le &g(x,p)& \le g_{ub}
\end{array}
\end{array}
$$

where $x \in \mathbb{R}^{nx}$ is the decision variable and $p \in \mathbb{R}^{np}$ is a known parameter vector.

An NLP solver in `CasADi` is a function that takes the parameter value (`p`), the bounds (`lbx`, `ubx`, `lbg`, `ubg`) and a guess for the primal-dual solution (`x0`, `lam_x0`, `lam_g0`) and returns the optimal solution. Unlike integrator objects, NLP solver functions are currently not differentiable functions in `CasADi`.

There are several NLP solvers interfaced with `CasADi`. The most popular one is IPOPT, an open-source primal-dual interior point method which is included in `CasADi` installations. Others, that require the installation of third-party software, include SNOPT, WORHP and KNITRO. Whatever the NLP solver used, the interface will automatically generate the information that it needs to solve the NLP, which may be solver and option dependent. Typically an NLP solver will need a function that gives the Jacobian of the constraint function and a Hessian of the Lagrangian function $L(x,\lambda) = f(x) + \lambda^{\text{T}} \, g(x))$ with respect to $x$

### Creating NLP solvers

NLP solvers are created using `CasADi`’s `nlpsol` function. Different solvers and interfaces are implemented as *plugins*. Consider the following form of the so-called Rosenbrock problem:

$$
\begin{array}{cc}
\begin{array}{c}
\text{minimize:} \\
x,y,z
\end{array}
&
x^2 + 100 \, z^2  \\
\begin{array}{c}
\text{subject to:}
\end{array}
&  z+(1-x)^2-y = 0
\end{array}
$$

A solver for this problem, using the ”ipopt” plugin, can be created using the syntax:

``` python
# Python
x = SX.sym('x'); y = SX.sym('y'); z = SX.sym('z')
nlp = {'x':vertcat(x,y,z), 'f':x**2+100*z**2, 'g':z+(1-x)**2-y}
S = nlpsol('S', 'ipopt', nlp)
```

``` matlab
% MATLAB
x = SX.sym('x'); y = SX.sym('y'); z = SX.sym('z');
nlp = struct('x',[x;y;z], 'f':x^2+100*z^2, 'g',z+(1-x)^2-y)
S = nlpsol('S', 'ipopt', nlp)
```

Once the solver has been created, we can solve the NLP, using $[2.5, 3.0, 0.75]$ as an initial guess, by evaluating the function `S`:

``` python
# Python
r = S(x0=[2.5,3.0,0.75],\
      lbg=0, ubg=0)
x_opt = r['x']
print('x_opt: ', x_opt)
```

``` matlab
% MATLAB
r = S('x0',[2.5,3.0,0.75],...
      'lbg',0,'ubg',0);
x_opt = r.x;
display(x_opt)
```

Quadratic programming
---------------------

`CasADi` provides interfaces to solve quadratic programs (QPs). Supported solvers are the open-source solvers qpOASES (distributed with `CasADi`) and OOQP as well as the commercial solvers CPLEX and GUROBI.

There are two different ways to solve QPs in `CasADi`, using a high-level interface and a low-level interface. They are described in the following.

### High-level interface

The high-level interface for quadratic programming mirrors that of nonlinear programming, i.e. expects a problem of the form , with the restriction that objective function *f*(*x*, *p*) must be a convex quadratic function in *x* and the constraint function *g*(*x*, *p*) must be linear in *x*. If the functions are not quadratic and linear, respectively, the solution is done at the current linearization point, given by the “initial guess” for *x*.

If the objective function is not convex, the solver may or may not fail to find a solution or the solution may not be unique.

To illustrate the syntax, we consider the following convex QP:
$$
\begin{array}{cc}
\begin{array}{c}
\text{minimize:} \\
x,y
\end{array}
&
x^2 + y^2  \\
\begin{array}{c}
\text{subject to:}
\end{array}
& x+y-10 \ge 0
\end{array}
$$

To solve this problem with the high-level interface, we simply replace `nlpsol` with `qpsol` and use a QP solver plugin such as the with `CasADi` distributed qpOASES:

``` python
# Python
x = SX.sym('x'); y = SX.sym('y')
qp = {'x':vertcat(x,y), 'f':x**2+y**2, 'g':x+y-10}
S = qpsol('S', 'qpoases', qp)
```

``` matlab
% MATLAB
x = SX.sym('x'); y = SX.sym('y')
qp = struct('x',[x;y], 'f':x^2+y^2, 'g',x+y-10)
S = qpsol('S', 'qpoases', qp)
```

The created solver object `S` will have the same input and output signature as the solver objects created with `nlpsol`. Since the solution is unique, it is less important to provide an initial guess:

``` python
# Python
r = S(lbg=0)
x_opt = r['x']
print('x_opt: ', x_opt)
```

``` matlab
% MATLAB
r = S('lbg',0);
x_opt = r.x;
display(x_opt)
```

### Low-level interface

The low-level interface, on the other hand, solves QPs of the following form:
$$
\begin{array}{cc}
\begin{array}{c}
\text{minimize:} \\
x
\end{array}
&
\frac{1}{2} x^\T \, H \, x + g^\T \, x
\\
\begin{array}{c}
\text{subject to:}
\end{array}
&
\begin{array}{rcl}
  x_{\lb} \le &  x   & \le x_{\ub} \\
  a_{\lb} \le & A \, x& \le a_{\ub}
\end{array}
\end{array}
$$

Encoding problem in this form, omitting bounds that are infinite, is straightforward:

``` python
# Python
H = 2*DM.eye(2)
A = DM.ones(1,2)
g = DM.zeros(2)
lba = 10.
```

``` matlab
% MATLAB
H = 2*DM.eye(2);
A = DM.ones(1,2);
g = DM.zeros(2);
lba = 10;
```

To create a solver instance, instead of passing symbolic expressions for the QP, we now pass the sparsity patterns of the matrices $H$ and $A$. Since we used `CasADi`’s `DM`-type above, we can simply query the sparsity patterns:

``` python
# Python
qp = {}
qp['h'] = H.sparsity()
qp['a'] = A.sparsity()
S = conic('S','qpoases',qp)
```

``` matlab
% MATLAB
qp = struct;
qp.h = H.sparsity();
qp.a = A.sparsity();
S = conic('S','qpoases',qp);
```

The returned `Function` instance will have a *different* input/output signature compared to the high-level interface, one that includes the matrices $H$ and $A$:

``` python
# Python
r = S(h=H, g=g, \
      a=A, lba=lba)
x_opt = r['x']
print('x_opt: ', x_opt)
```

``` matlab
% MATLAB
r = S('h', H, 'g', g,...
      'a', A, 'lba', lba);
x_opt = r.x;
display(x_opt)
```
