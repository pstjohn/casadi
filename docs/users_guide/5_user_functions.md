User-defined function objects
=============================

There are situations when rewriting user-functions using `CasADi` symbolics is not possible or practical. To tackle this, `CasADi` provides a number of ways to embed a call to a “black box” function defined in the language `CasADi` is being used from (C++, MATLAB or Python) or in C. That being said, the recommendation is always to try to avoid this when possible, even if it means investing a lot of time reimplementing existing code. Functions defined using `CasADi` symbolics are almost always more efficient, especially when derivative calculation is involved, since a lot more structure can typically be exploited.

Depending on the circumstances, the user can implement custom `Function` objects in a number of different ways:

-   Subclassing `FunctionInternal`

-   Subclassing `Callback`

-   Importing a function with `external`

We elaborate on this in the following.

Subclassing `FunctionInternal`
------------------------------

All function objects presented in Chapter \[ch:function\] are implemented in `CasADi` as C++ classes inheriting from the `FunctionInternal` abstract base class. In principle, a user with familiarity with C++ programming, should be able to implement a class inheriting from `FunctionInternal`, implementing the virtual methods of this class. The best reference for doing so is the C++ API documentation, choosing “switch to internal” to expose the internal API.

Since `FunctionInternal` is not considered part of the stable, public API, we advice against this in general, unless the plan is to make a contribution to `CasADi`.

Subclassing `Callback`
----------------------

The `Callback` class provides a public API to `FunctionInternal` and inheriting from this class has the same effect as inheriting directly from `FunctionInternal`. Thanks to *cross-language polymorphism*, it is possible to implement the exposed methods of `Callback` from either Python, MATLAB or C++.

The derived class consists of the following parts:

-   A constructor or a static function replacing the constructor

-   A number of *virtual* functions, all optional, that can be overloaded (shadowed) in order to get the desired behavior. This includes the number of of inputs and outputs using `get_n_in` and `get_n_out`, their names using `get_name_in` and `get_name_out` and their sparsity patterns `get_sparsity_in` and `get_sparsity_out`.

-   An optional `init` function called when the construction is complete.

-   A function for numerical evaluation.

-   Optional functions for derivatives. You can choose to supply a full Jacobian (`has_jacobian`, `get_jacobian`), or choose to supply forward/reverse sensitivities (`get_n_forward`, `get_forward`, `get_n_reverse`, `get_reverse`).

For a complete list of functions, see the C++ API documentation for `Callback`.

The usage from the different languages are described in the following.

### Python

In Python, a custom function class can be defined is as follows:

``` python
class MyCallback(Callback):
  def __init__(self, name, d, opts={}):
    Callback.__init__(self)
    self.d = d
    self.construct(name, opts)

  # Number of inputs and outputs
  def get_n_in(self): return 1
  def get_n_out(self): return 1

  # Initialize the object
  def init(self):
     print 'initializing object'

  # Evaluate numerically
  def eval(self, arg):
    x = arg[0]
    f = sin(self.d*x)
    return [f]
```

The implementation should include a constructor, which should call the base class constructor using `Callback.__init__(self)`.

This function can be used as any built-in `CasADi` function with the important caveat that when embedded in graphs, the ownership of the class will *not* be shared between all references. So it is important that the user does not allow the Python class to go out of scope while it is still needed in calculations.

``` python
# Use the function
f = MyCallback('f', 0.5)
res = f(2)
print(res)
```

### MATLAB

In MATLAB, a custom function class can be defined as follows, in a file `MyCallback.m`:

``` matlab
  classdef MyCallback < casadi.Callback
    properties
      d
    end
    methods
      function self = MyCallback(name, d)
        self@casadi.Callback();
        self.d = d;
        construct(self, name);
      end

      % Number of inputs and outputs
      function v=get_n_in(self)
        v=1;
      end
      function v=get_n_out(self)
        v=1;
      end

      % Initialize the object
      function init(self)
        disp('initializing object')
      end

      % Evaluate numerically
      function arg = eval(self, arg)
        x = arg{1};
        f = sin(self.d * x);
        arg = {f};
      end
    end
  end
```

This function can be used as any built-in `CasADi` function, but as for Python, the ownership of the class will *not* be shared between all references. So the user must not allow a class instance to get deleted while it is still in use, e.g. by making it `persistent`.

``` matlab
% Use the function
f = MyCallback('f', 0.5);
res = f(2);
disp(res)
```

### C++

In C++, the syntax is as follows:

    #include "casadi/casadi.hpp"
    using namespace casadi;
    class MyCallback : public Callback {
    private:
      // Data members
      double d;
      // Private constructor
      MyCallback(double d) : d(d) {}
    public:
      // Creator function, creates an owning reference
      static Function create(const std::string& name, double d,
                             const Dict& opts=Dict()) {
        return Callback::create(name, new MyCallback(d), opts);
      }

      // Number of inputs and outputs
      virtual int get_n_in() { return 1;}
      virtual int get_n_out() { return 1;}

      // Initialize the object
      virtual void init() {
        std::cout << "initializing object" << std::endl;
      }

      // Evaluate numerically
      virtual std::vector<DM> eval(const std::vector<DM>& arg) {
        DM x = arg.at(0);
        DM f = sin(d*x);
        return {f};
      }
    };

As seen in the example, the derived class should implement a private constructor that is not called directly, but instead via a static `create` function using the syntax above. This functions returns a `Function` instance which takes ownership of the created object.

A class created this way can be used as any other `Function` instance, with the `create` function replacing a conventional constructor:

    int main() {
      Function f = MyCallback::create("f", 0.5);
      std::vector<DM> arg = {2};
      std::vector<DM> res = f(arg);
      std::cout << res << std::endl;
      return 0;
    }

Importing a function with `external`
------------------------------------

The basic usage of `CasADi`’s `external` function was demonstrated in Section using\_codegen in the context of using autogenerated code. The same function can also be used for importing a user-defined function, as long as it also uses the C API described in Section c\_api.

The following sections expands on this.

### Default functions

It is usually *not* necessary to define all the functions defined in Section c\_api. If `fname_incref` and `fname_decref` are absent, it is assumed that no memory management is needed. If no names of inputs and outputs are provided, they will be given default names. Sparsity patterns are in general assumed to be scalar by default, unless the function corresponds to a derivative of another function (see below), in which case they are assumed to be dense and of the correct dimension.

Futhermore, work vectors are assumed not to be needed if `fname_work` has not been implemented.

### Meta information as comments

If you rely on `CasADi`’s just-in-time compiler, you can provide meta information as a comment in the C code instead of implementing the actual callback function.

The structure of such meta information should be as follows:

``` c
/*CASADIMETA
:fname_N_IN 1
:fname_N_OUT 2
:fname_NAME_IN[0] x
:fname_NAME_OUT[0] r
:fname_NAME_OUT[1] s
:fname_SPARSITY_IN[0] 2 1 0 2
*/
```

### Simplified evaluation signature

If all the inputs and outputs are scalars, the user can choose to replace the function for numerical evaluation:

``` c
int fname(const double** arg, double** res,
          int* iw, double* w, int mem);
```

with a function with simpler syntax:

``` c
void fname_simple(const double* arg, double* res);
```

Note that `_simple` must be appended to the function name. Evaluating a function with this syntax potentially carries less overhead.

### Derivatives

The external function can be made differentiable by providing functions for calculating derivatives. During derivative calculations, `CasADi` will look for symbols in the same file/shared library that follows a certain *naming convention*. For example, you can specify a Jacobian for all the outputs with respect to all inputs for a function named `fname` by implementing a function named `jac_fname`. Similary, you can specify a function for calculating one forward directional derivative by providing a function named `fwd1_fname`, where 1 can be replaced by 2, 4, 8, 16, 32 or 64 for calculating multiple forward directional derivatives at once. For reverse mode directional derivatives, replace `fwd` with `adj`.

This is an experimental feature.


