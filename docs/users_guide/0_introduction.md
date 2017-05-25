Introduction
============

`CasADi` is an open-source software tool for numerical optimization in general and optimal control (i.e. optimization involving differential equations) in particular. The project was started by Joel Andersson and Joris Gillis while PhD students at the Optimization in Engineering Center (OPTEC) of the KU Leuven under supervision of Moritz Diehl.

This document aims at giving a condensed introduction to `CasADi`. After reading it, you should be able to formulate and manipulate expressions in `CasADi`’s symbolic framework, generate derivative information efficiently using *algorithmic differentiation*, to set up, solve and perform forward and adjoint sensitivity analysis for systems of ordinary differential equations (ODE) or differential-algebraic equations (DAE) as well as to formulate and solve nonlinear programs (NLP) problems and optimal control problems (OCP).

CasADi is available for C++, Python and MATLAB/Octave with little or no difference in performance. In general, the Python API is the best documented and is slightly more stable than the MATLAB API. The C++ API is stable, but is not ideal for getting started with CasADi since there is limited documentation and since it lacks the interactivity of interpreted languages like MATLAB and Python. The MATLAB module has been tested successfully for Octave (version 4.0.2 or later).

What `CasADi` is and what it is *not*
-------------------------------------

`CasADi` started out as a tool for algorithmic differentiation (AD) using a syntax borrowed from computer algebra systems (CAS), which explains its name. While AD still forms one of the core functionalities of the tool, the scope of the tool has since been considerably broadened, with the addition of support for ODE/DAE integration and sensitivity analysis, nonlinear programming and interfaces to other numerical tools. In its current form, it is a general-purpose tool for gradient-based numerical optimization – with a strong focus on optimal control – and “`CasADi`” is just a name without any particular meaning.

It is important to point out that `CasADi` is *not* a conventional AD tool, that can be used to calculate derivative information from existing user code with little to no modification. If you have an existing model written in C++, Python or MATLAB/Octave, you need to be prepared to reimplement the model using `CasADi` syntax.

Secondly, `CasADi` is *not* a computer algebra system. While the symbolic core does include an increasing set of tools for manipulate symbolic expressions, these capabilities are very limited compared to a proper CAS tool.

Finally, `CasADi` is not an “optimal control problem solver”, that allows the user to enter an OCP and then gives the solution back. Instead, it tries to provide the user with a set of “building blocks” that can be used to implement general-purpose or specific-purpose OCP solvers efficiently with a modest programming effort.

Help and support
----------------

If you find simple bugs or lack some feature that you think would be relatively easy for us to add, the simplest thing is simply to write to the forum, located at [http://forum.casadi.org](http://forum.casadi.org/). We check the forum regularly and try to respond as quickly as possible. The only thing we expect for this kind of support is that you cite us, cf. Section citing, whenever you use `CasADi` in scientific work.

If you want more help, we are always open for academic or industrial cooperation. An academic cooperation usually take the form of a co-authorship of a peer reviewed paper, and an industrial cooperation involves a negotiated consulting contract. Please contact us directly if you are interested in this.

Citing `CasADi`
---------------

If you use `CasADi` in published scientific work, please cite the following:

    @PHDTHESIS{Andersson2013b,
      author = {Joel Andersson},
      title = {{A} {G}eneral-{P}urpose {S}oftware {F}ramework for
               {D}ynamic {O}ptimization},
      school = {Arenberg Doctoral School, KU Leuven},
      year = {2013},
      type = {{P}h{D} thesis},
      address = {Department of Electrical Engineering (ESAT/SCD) and
                 Optimization in Engineering Center,
                 Kasteelpark Arenberg 10, 3001-Heverlee, Belgium},
      month = {October}
    }

Reading this document
---------------------

The goal of this document is to make the reader familiar with the syntax of `CasADi` and provide easily available building blocks to build numerical optimization and dynamic optimization software. Our explanation is mostly program code driven and provides little mathematical background knowledge. We assume that the reader already has a fair knowledge of theory of optimization theory, solution of initial-value problems in differential equations and the programming language in question (C++, Python or MATLAB/Octave).

We will use Python and MATLAB syntax side-by-side in this guide, noting that the Python interface is more stable and better documented. Unless otherwise noted, the MATLAB syntax also applies to Octave. We try to point out the instances where has a diverging syntax. To facilitate switching between the programming languages, we also list the major differences in [Syntax Differences](8_lanugage_differences.md)
