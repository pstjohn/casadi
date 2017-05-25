The `DaeBuilder` class
======================

The `DaeBuilder` class in `CasADi` is an auxiliary class intended to facilitate the modeling complex dynamical systems for later use with optimal control algorithms. This class can be seen as a low-level alternative to a physical modeling language such as Modelica (cf. Section modelica), while still being higher level than working directly with `CasADi` symbolic expressions. Another important usage it to provide an interface to physical modeling languages and software and be a building blocks for developing domain specific modeling environments.

Using the `DaeBuilder` class consists of the following steps:

-   Step-by-step constructing a structured system of differential-algebraic equations (DAE) or, alternatively, importing an existing model from Modelica

-   Symbolically reformulate the DAE

-   Generate a chosen set of `CasADi` functions to be used for e.g. optimal control or C code generation

In the following sections, we describe the mathematical formulation of the class and its intended usage.

Mathematical formulation
------------------------

The `DaeBuilder` class uses a relatively rich problem formulation that consists of a set of input expressions and a set of output expressions, each defined by a string identifier. The choice of expressions was inspired by the *functional mockup interface* (FMI) version 2.0 [3]

### Input expressions

1.  Time *t*

2.  Named constants *c*

3.  Independent parameters *p*

4.  Dependent parameters *d*, depends only on *p* and *c* and, acyclically, on other *d*

5.  Differential state *x*, defined by an explicit ODE

6.  Differential state *s*, defined by an implicit ODE

7.  Time derivatives implicitly defined differential state $\\dot{s}$

8.  Algebraic variable, defined by an algebraic equation

9.  Quadrature state *q*. A differential state that may not appear in the right-hand-side and hence can be calculated by quadrature formulas.

10. Local variables *w*. Calculated from time and time dependent variables. They may also depend, acyclically, on other *w*.

11. Output variables *y*

### Output expressions

The above input expressions are used to define the following output expressions:

1.  Explicit expression for calculating *d*

2.  Explicit expression for calculating *w*

3.  The explicit ODE right-hand-side: $\\dot{x} = \\text{ode}(t,w,x,s,z,u,p,d)$

4.  The implicit ODE right-hand-side: $\\text{dae}(t,w,x,s,z,u,p,d,\\dot{s}) =0$

5.  The algebraic equations: alg(*t*, *w*, *x*, *s*, *z*, *u*, *p*, *d*)=0

6.  The quadrature equations: $\\dot{q} = \\text{quad}(t,w,x,s,z,u,p,d)$

7.  Explicit expressions for calculating *y*

Constructing a `DaeBuilder` instance
------------------------------------

Consider the following simple DAE corresponding to a controlled rocket subject to quadratic air friction term and gravity, which loses mass as it uses up fuel:

$${\\realLaTeX@begin}{aligned}
 \\dot{h} &= v,                    \\qquad &h(0) = 0 \\\\
 \\dot{v} &= (u - a \\, v^2)/m - g, \\qquad &v(0) = 0 \\\\
 \\dot{m} &= -b \\, u^2,            \\qquad &m(0) = 1\\end{aligned}$$

where the three states correspond to height, velocity and mass, respectively. *u* is the thrust of the rocket and (*a*, *b*) are parameters.

To construct a DAE formulation for this problem, start with an empty `DaeBuilder` instance and add the input and output expressions step-by-step as follows.

``` python
# Python
dae = DaeBuilder()
# Add input expressions
a = dae.add_p('a')
b = dae.add_p('b')
u = dae.add_u('u')
h = dae.add_x('h')
v = dae.add_x('v')
m = dae.add_x('m')
# Add output expressions
hdot = v
vdot = (u-a*v**2)/m-g
mdot = -b*u**2
dae.add_ode(hdot)
dae.add_ode(vdot)
dae.add_ode(mdot)
# Specify initial conditions
dae.set_start('h', 0)
dae.set_start('v', 0)
dae.set_start('m', 1)
# Add meta information
dae.set_unit('h','m')
dae.set_unit('v','m/s')
dae.set_unit('m','kg')
```

``` matlab
% MATLAB
dae = DaeBuilder;
% Add input expressions
a = dae.add_p('a');
b = dae.add_p('b');
u = dae.add_u('u');
h = dae.add_x('h');
v = dae.add_x('v');
m = dae.add_x('m');
% Add output expressions
hdot = v;
vdot = (u-a*v^2)/m-g;
mdot = -b*u^2;
dae.add_ode(hdot);
dae.add_ode(vdot);
dae.add_ode(mdot);
% Specify initial conditions
dae.set_start('h', 0);
dae.set_start('v', 0);
dae.set_start('m', 1);
% Add meta information
dae.set_unit('h','m');
dae.set_unit('v','m/s');
dae.set_unit('m','kg');
```

Other input and output expressions can be added in an analogous way. For a full list of functions, see the C++ API documentation for `DaeBuilder`.

Import of OCPs from Modelica
----------------------------

An alternative to model directly in `CasADi`, as above, is to use an advanced physical modeling language such as Modelica to specity the model. For this, `CasADi` offers interoperability with the open-source [JModelica.org](http://www.jmodelica.org/) compiler, which is written specifically with optimal control in mind. Model inport from JModelica.org is possible in two different ways; using the JModelica.org’s `CasadiInterface` or via `DaeBuilder`’s `parse_fmi` command.

We recommend the former approach, since it is being actively maintained and refer to JModelica.org’s user guide for details on how to extract `CasADi` expressions.

In the following, we will outline the legacy approach, using `parse_fmi`.

### Legacy import of a `modelDescription.xml` file

To see how to use the Modelica import, look at [thermodynamics\_example.py](https://github.com/casadi/casadi/blob/tested/examples/python/modelica/fritzson_application_examples/thermodynamics_example.py) and [cstr.cpp](https://github.com/casadi/casadi/blob/tested/examples/cplusplus/cstr.cpp) in `CasADi`’s example collection.

Assuming that the Modelica/Optimica model `ModelicaClass.ModelicaModel` is defined in the files `file1.mo` and `file2.mop`, the Python compile command is:

``` python
from pymodelica import compile_jmu
jmu_name=compile_jmu('ModelicaClass.ModelicaModel', \
  ['file1.mo','file2.mop'],'auto','ipopt',\
  {'generate_xml_equations':True, 'generate_fmi_me_xml':False})
```

This will generate a `jmu`-file, which is essentially a zip file containing, among other things, the file `modelDescription.xml`. This XML-file contains a symbolic representation of the optimal control problem and can be inspected in a standard XML editor.

``` python
from zipfile import ZipFile
sfile = ZipFile(jmu_name','r')
mfile = sfile.extract('modelDescription.xml','.')
```

Once a `modelDescription.xml` file is available, it can be imported using the command:

``` python
dae = DaeBuilder()
ocp.parse_fmi('modelDescription.xml')
```

Symbolic reformulation
----------------------

One of the original purposes of the `DaeBuilder` class was to reformulate a *fully-implicit DAE*, typically coming from Modelica, to a semi-explicit DAE that can be used more readily in optimal control algorithms.

This can be done by the command:

``` python
# Python
ocp.make_explicit()
```

``` matlab
% MATLAB
ocp.make_explicit();
```

Other useful commands available for an instance `ocp` of `DaeBuilder` include:

print `ocp`  
Print the optimal optimal control problem to screen

`ocp`.scale\_variables()  
Scale all variables using the *nominal* attribute for each variable

`ocp`.eliminate\_d()  
Eliminate all independent parameters from the symbolic expressions

For a more detailed description of this class and its functionalities, we again refer to the API documentation.

Function factory
----------------

Once a `DaeBuilder` has been formulated and possibily reformulated to a satisfactory form, we can generate `CasADi` functions corresponding to the input and output expressions outlined in Section daebuilder\_io. For example, to create a function for the ODE right-hand-side for the rocket model in Section daebuilder\_syntax, simply provide a display name of the function being created, a list of input expressions and a list of output expressions:

``` python
# Python
f = dae.create('f',\
     ['x','u','p'],['ode'])
```

``` matlab
% MATLAB
f = dae.create('f',...
     {'x','u','p'},{'ode'});
```

Using a naming convention, we can also create Jacobians, e.g. for the ’ode’ output with respect to ’x’:

``` python
# Python
f = dae.create('f',\
     ['x','u','p'],\
     ['jac_ode_x'])
```

``` matlab
% MATLAB
f = dae.create('f',...
     {'x','u','p'},
     {'jac_ode_x'});
```

Functions with second order information can be extracted by first creating a named linear combination of the output expressions using and then requesting its Hessian:

``` python
# Python
dae.add_lc('gamma',['ode'])
hes = dae.create('hes’,\
  ['x','u','p','lam_ode'],\
  ['hes_gamma_x_x'])
```

``` matlab
% MATLAB
dae.add_lc('gamma,{'ode'});
hes = dae.create(’hes’,...
  {'x','u','p','lam_ode'},...
  {'hes_gamma_x_x'});
```

It is also possible to simply extract the symbolic expressions from the `DaeBuilder` instance and manually create `CasADi` functions. For example, contains all the expressions corresponding to ’x’, contains the expressions corresponding to ’ode’, etc.

