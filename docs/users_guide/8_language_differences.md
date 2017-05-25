Difference in usage from different languages
============================================

General usage
-------------

|                                                                                    | Python                                                  | C++                                                              | MATLAB/Octave                                         |
|:-----------------------------------------------------------------------------------|:--------------------------------------------------------|:-----------------------------------------------------------------|:------------------------------------------------------|
| Starting `CasADi`                                                                  | `from casadi import *`                                  | `#include \` `"casadi/casadi.hpp"` `using namespace casadi;`     | `import casadi.*`                                     |
| Printing the *representation* (string representation intended to be *unambiguous*) | `A <ENTER>` (interactive), `print repr(A)` (in scripts) | `std::cout << A;`                                                | `A <ENTER>` or `disp A`                               |
| Printing the *description* (string representation intended to be *readable*)       | `print A`                                               | `A.print();` or `A.print(stream);`                               | `A.print()`                                           |
| Calling a class function                                                           | `SX.zeros(3,4)`                                         | `SX::zeros(3,4)`                                                 | `SX.zeros(3,4)`                                       |
| Creating a dictionary (e.g. for options)                                           | `d = {'opt1':opt1}` or `d = {}; a['opt1'] = opt1`       | `a = Dict();` `a['opt1'] = opt1;`                                | `a = struct;` `a.opt1 = opt1;`                        |
| Creating a symbol                                                                  | `MX.sym("x",2,2)`                                       | `MX::sym("x",2,2)`                                               | `MX.sym('x',2,2)`                                     |
| Creating a function                                                                | `Function("f",[x,y],[x+y])`                             | `Function("f",{x,y},{x+y})`                                      | `Function('f',{x,y},{x+y})`                           |
| Calling a function                                                                 | `z=f(x,y)`                                              | `z = f({x,y})`                                                   | `z=f(x,y)`                                            |
| Create an NLP solver                                                               | `nlp = {"x":x,"f":f}` `nlpsol("S","ipopt",nlp)`         | `MXDict nlp = \` `{{"x",x},{"f",f}};` `nlpsol("S","ipopt",nlp);` | `nlp=struct('x',x,'f',f);` `nlpsol('S','ipopt',nlp);` |

List of operations
------------------

The following is a list of the most important operations. Operations that differ between the different languages are marked with a star (\*). This list is neither complete, nor does it show all the variants of each operation. Further information is available in the API documentation.

|                                                      | Python                                 | C++                                                 | MATLAB/Octave                          |
|:-----------------------------------------------------|:---------------------------------------|:----------------------------------------------------|:---------------------------------------|
| Addition, subtraction                                | `x+y, x-y, -x`                         | `x+y, x-y, -x`                                      | `x+y, x-y, -x`                         |
| \*Elementwise multiplication, division               | `x*y, x/y`                             | `x*y, x/y`                                          | `x.*y, x./y`                           |
| Natural exponential function and logarithm           | `exp(x)` `log(x)`                      | `exp(x)` `log(x)`                                   | `exp(x)` `log(x)`                      |
| \*Exponentiation                                     | `x**y`                                 | `pow(x,y)`                                          | `x^y` or `x.^y`                        |
| Square root                                          | `sqrt(x)`                              | `sqrt(x)`                                           | `sqrt(x)`                              |
| Trigonometric functions                              | `sin(x), cos(x), tan(x)`               | `sin(x), cos(x), tan(x)`                            | `sin(x), cos(x), tan(x)`               |
| Inverse trigonometric                                | `asin(x), acos(x), ...`                | `asin(x), acos(x), ...`                             | `asin(x), acos(x), ...`                |
| Two argument arctangent                              | `atan2(x, y)`                          | `atan2(x, y)`                                       | `atan2(x, y)`                          |
| Hyperbolic functions                                 | `sinh(x), cosh(x), tanh(x)`            | `sinh(x), cosh(x), tanh(x)`                         | `sinh(x), cosh(x), tanh(x)`            |
| Inverse hyperbolic                                   | `asinh(x), acosh(x), ...`              | `asinh(x), acosh(x), ...`                           | `asinh(x), acosh(x), ...`              |
| Inequalities                                         | `a<b, a<=b, a>b, a>=b`                 | `a<b, a<=b, a>b, a>=b`                              | `a<b, a<=b, a>b, a>=b`                 |
| \*(Not) equal to                                     | `a==b, a!=b`                           | `a==b, a!=b`                                        | `a==b, a~=b`                           |
| \*Logical and                                        | `logic_and(a, b)`                      | `a && b`                                            | `a & b`                                |
| \*Logical or                                         | `logic_or(a, b)`                       | `a || b`                                            | `a | b`                                |
| \*Logical not                                        | `logic_not(a)`                         | `!a`                                                | `~a`                                   |
| Round to integer                                     | `floor(x), ceil(x)`                    | `floor(x), ceil(x)`                                 | `floor(x), ceil(x)`                    |
| \*Modulus after division                             | `fmod(x, y)`                           | `fmod(x, y)`                                        | `mod(x, y)`                            |
| \*Absolute value                                     | `fabs(x)`                              | `fabs(x)`                                           | `abs(x)`                               |
| Sign function                                        | `sign(x)`                              | `sign(x)`                                           | `sign(x)`                              |
| (Inverse) error function                             | `erf(x), erfinv(x)`                    | `erf(x), erfinv(x)`                                 | `erf(x), erfinv(x)`                    |
| \*Elementwise min and max                            | `fmin(x, y), fmax(x, y)`               | `fmin(x, y), fmax(x, y)`                            | `min(x, y), max(x, y)`                 |
| Index of first nonzero                               | `find(x)`                              | `find(x)`                                           | `find(x)`                              |
| If-then-else                                         | `if_else(c, x, y)`                     | `if_else(c, x, y)`                                  | `if_else(c, x, y)`                     |
| \*Matrix multiplication                              | `mtimes(x,y)`                          | `mtimes(x,y)`                                       | `mtimes(x,y)` or `x*y`                 |
| \*Transpose                                          | `transpose(A)` or `A.T`                | `transpose(A)` or `A.T()`                           | `transpose(A)` or `A'` or `A.'`        |
| Inner product                                        | `dot(x, y)`                            | `dot(x, y)`                                         | `dot(x, y)`                            |
| \*Horizontal/vertical concatenation                  | `horzcat(x, y)` `vertcat(x, y)`        | `horzcat(v)` `vertcat(v)`, (`v` vector of matrices) | `[x, y]` `[x; y]`                      |
| Horizontal/vertical split (inverse of concatenation) | `vertsplit(x)`, `horzsplit(x)`         | `vertsplit(x)`, `horzsplit(x)`                      | `vertsplit(x)`, `horzsplit(x)`         |
| \*Element access                                     | `A[i,j]` and `A[i]`, *0-based*         | `A(i,j)` and `A(i)`, *0-based*                      | `A(i,j)` and `A(i)`, *1-based*         |
| \*Element assignment                                 | `A[i,j] = b` and `A[i] = b`, *0-based* | `A(i,j) = b` and `A(i) = b`, *0-based*              | `A(i,j) = b` and `A(i) = b`, *1-based* |
| \*Nonzero access                                     | `A.nz[k]`, *0-based*                   | `A.nz(k)`, *0-based*                                | (currently unsupported)                |
| \*Nonzero assignment                                 | `A.nz[k] = b`, *0-based*               | `A.nz(k) = b`, *0-based*                            | (currently unsupported)                |
| Project to a different sparsity                      | `project(x, s)`                        | `project(x, s)`                                     | `project(x, s)`                        |

[1] for problems with free end time, you can always scale time by introducing an extra parameter and substitute *t* for a dimensionless time variable that goes from 0 to 1

[2] An exception is when code is generated for a function that in turn contains calls to external functions.

[3] FMI development group. Functional Mock-up Interface for Model Exchange and Co-Simulation. <https://www.fmi-standard.org/>, July 2014. Specification, FMI 2.0. Section 3.1, pp. 71–72

[4] Lorenz T. Biegler, *<span> empty </span><span>Nonlinear Programming: Concepts, Algorithms, and Applications to Chemical Processes</span><span>http://books.google.es/books/about/Nonlinear\_Programming.html?id=VdB1wJQu0sgC&redir\_esc=y</span>*, SIAM 2010

[5] John T. Betts, *<span> empty </span><span>Practical Methods for Optimal Control Using Nonlinear Programming</span><span>http://books.google.es/books/about/Practical\_Methods\_for\_Optimal\_Control\_Us.html?id=Yn53JcYAeaoC&redir\_esc=y</span>*, SIAM 2001

[6] You can obtain this collection as an archive named `examples_pack.zip` in `CasADi`’s <span> empty </span><span>download area</span><span>http://files.casadi.org</span>
