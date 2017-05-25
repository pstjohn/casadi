Symbolic framework
==================

At the core of `CasADi` is a self-contained symbolic framework that allows the user to construct symbolic expressions using a MATLAB inspired everything-is-a-matrix syntax, i.e. vectors are treated as n-by-1 matrices and scalars as 1-by-1 matrices. All matrices are *sparse* and use a general sparse format – *compressed column storage* (CCS) – to store matrices. In the following, we introduce the most fundamental classes of this framework.

The `SX` symbolics
------------------

The `SX` data type is used to represent matrices whose elements consist of symbolic expressions made up by a sequence of unary and binary operations. To see how it works in practice, start an interactive Python shell (e.g. by typing `ipython` from a Linux terminal or inside a integrated development environment such as Spyder) or launch MATLAB’s or Octave’s graphical user interface. Assuming `CasADi` has been installed correctly, you can import the symbols into the workspace as follows:

``` python
# Python
from casadi import *
```

``` matlab
% MATLAB
import casadi.*
```

Now create a variable `x` using the syntax:

``` python
# Python
x = MX.sym('x')
```

``` matlab
% MATLAB
x = MX.sym('x');
```

This creates a 1-by-1 matrix, i.e. a scalar containing a symbolic primitive called “x”. This is just the display name, not the identifier. Multiple variables can have the same name, but still be different. The identifier is the return value. You can also create vector- or matrix-valued symbolic variables by supplying additional arguments to `SX.sym`:

``` python
# Python
y = SX.sym('y',5)
Z = SX.sym('Z',4,2)
```

``` matlab
% MATLAB
y = SX.sym('y',5);
Z = SX.sym('Z',4,2);
```

which creates a 5-by-1 matrix, i.e. a vector, and a 4-by-2 matrix with symbolic primitives, respectively.

`SX.sym` is a (static) function which returns an `SX` instance. When variables have been declared, expressions can now be formed in an intuitive way:

``` python
# Python
f = x**2 + 10
f = sqrt(f)
print('f:', f)
```

``` matlab
% MATLAB
f = x^2 + 10;
f = sqrt(f);
display(f)
```

You can also create constant `SX` instances *without* any symbolic primitives:

-   `B1 = SX.zeros(4,5)`: A dense 4-by-5 empty matrix with all zeros

-   `B2 = SX(4,5)`: A sparse 4-by-5 empty matrix with all zeros

-   `B4 = SX.eye(4)`: A sparse 4-by-4 matrix with ones on the diagonal

Note the difference between a sparse matrix with *structural* zeros and a dense matrix with *actual* zeros. When printing an expression with structural zeros, these will be represented as 00 to distinguish them from actual zeros 0:

``` python
# Python
print( 'B4:', B4)
```

``` matlab
% MATLAB
display(B4)
```

The following list summarizes the most commonly used ways of constructing new `SX` expressions:

-   `SX.sym(name,n,m)`: Create an $n$-by-$m$ symbolic primitive

-   `SX.zeros(n,m)`: Create an $n$-by-$m$ dense matrix with all zeros

-   `SX(n,m)`: Create an $n$-by-$m$ sparse matrix with all $structural$ zeros

-   `SX.ones(n,m)`: Create an $n$-by-$m$ dense matrix with all ones

-   `SX.eye(n)`: Create an $n$-by-$n$ diagonal matrix with ones on the diagonal and structural zeros elsewhere.

-   `9 * SX.ones(2,2)`.

-   `SX(matrix_type)`: Create a matrix given a numerical matrix given as a *numpy* or *scipy* matrix (in Python) or as a dense or sparse matrix (in MATLAB). In MATLAB e.g. `SX([1,2;3,4])` for a 2-by-2 matrix. This method can be used explicitly or implicitly.

-   `repmat(SX(3),2,1)` will create a 2-by-1 matrix with all elements 3.

-   (*Python only*) `SX([1,2,3,4])` (note the difference between Python lists and MATLAB horizontal concatenation, which both uses square bracket syntax)

-   (*Python only*) `SX([[1,2,3,4]])`.

### Note for MATLAB/Octave users

In MATLAB, if the `import` command is omitted, you can still use CasADi by prefixing all the symbols with the package name, e.g. `from casadi import *`".

Unfortunately, Octave (version 4.0.3) does not implement MATLAB’s `import` command. To work around this issue, we provide a simple function `import.m` that can be placed in Octave’s path enabling the compact syntax used in this guide.

### Note for C++ users

In C++, all public symbols are defined in the `casadi` namespace and require the inclusion of the `casadi/casadi.hpp` header file. The commands above would be equivalent to:

```c++
#include <casadi/casadi.hpp>
using namespace casadi;
int main() {
	SX x = SX::sym("x");
	SX y = SX::sym("y",5);
	SX Z = SX::sym("Z",4,2)
	SX f = pow(x,2) + 10;
	f = sqrt(f);
	std::cout << "f: " << f << std::endl;
	return 0;
}
```

`DM`
----

`DM` is very similar to `SX`, but with the difference that the nonzero elements are numerical values and not symbolic expressions. The syntax is also the same, except for functions such as `SX.sym`, which have no equivalents.

`DM` is mainly used for storing matrices in `CasADi` and as inputs and outputs of functions. It is *not* intended to be used for computationally intensive calculations. For this purpose, use the built-in dense or sparse data types in MATLAB, `numpy` or `scipy` matrices in Python or an expression template based library such as `eigen`, `ublas` or `MTL` in C++. Conversion between the types is usually straightforward:

``` python
# Python
C = DM(2,3)

C_dense = C.full()
from numpy import array
C_dense = array(C) # equivalent

C_sparse = C.sparse()
from scipy.sparse import csc_matrix
C_sparse = csc_matrix(C) # equivalent
```

``` matlab
% MATLAB
C = DM(2,3);

C_dense = full(C);

C_sparse = sparse(C);
```

More usage examples for `SX` can be found in the tutorials at [http://docs.casadi.org](http://docs.casadi.org/). For documentation of particular functions of this class (and others), find the “C++ API docs” on the website and search for information about `casadi::Matrix`.

The `MX` symbolics
------------------

Let us perform a simple operation using the `SX` above:

``` python
# Python
x = SX.sym('x',2,2)
y = SX.sym('y')
f = 3*x + y
print(f)
print(f.shape)
```

``` matlab
% MATLAB
x = SX.sym('x',2,2);
y = SX.sym('y');
f = 3*x + y;
disp(f)
disp(size(f))
```

As you can see, the output of this operation is a 2-by-2 matrix. Note how the multiplication and the addition were performed elementwise and new expressions (of type `SX`) were created for each entry of the result matrix.

We shall now introduce a second, more general *matrix expression* type `MX`.
The `MX` type allows, like `SX`, to build up expressions consisting of a sequence of elementary operations.
But unlike `SX`, these elementary operations are not restricted to be scalar unary or binary operations (ℝ → ℝ or ℝ × ℝ → ℝ.
Instead, the elementary operations that are used to form `MX` expressions are allowed to be general *multiple sparse-matrix valued* input, *multiple sparse-matrix valued* output functions: $mathbb{R}^{n_1 times m_1}  times ldots times mathbb{R}^{n_N times m_N} rightarrow mathbb{R}^{p_1 times q_1} times ldots times mathbb{R}^{p_M times q_M}$

The syntax of `MX` mirrors that of `SX`:

``` python
# Python
x = MX.sym('x',2,2)
y = MX.sym('y')
f = 3*x + y
print(f)
print(f.shape)
```

``` matlab
% MATLAB
x = MX.sym('x',2,2);
y = MX.sym('y');
f = 3*x + y;
disp(f)
disp(size(f))
```

Note how the result consists of only two operations (one multiplication and one addition) using `MX` symbolics, whereas the `SX` equivalent has eight (two for each element of the resulting matrix). As a consequence, `MX` can be more economical when working with operations that are naturally vector or matrix valued with many elements. As we shall see in [Functions](3_functions.md), it is also much more general since we allow calls to arbitrary functions that cannot be expanded in terms of elementary operations.

`MX` supports getting and setting elements, using the same syntax as `SX`, but the way it is implemented is very different. Test, for example, to print the element in the upper-left corner of a 2-by-2 symbolic variable:

``` python
# Python
x = MX.sym('x',2,2)
print(x[0,0])
```

``` matlab
% MATLAB
x = MX.sym('x',2,2);
x(1,1)
```

The output should be understood as an expression that is equal to the first (i.e. index 0 in C++) structurally non-zero element of `x`, unlike `x_0` in the `SX` case above, which is the name of a symbolic primitive in the first (index 0) location of the matrix.

Similar results can be expected when trying to set elements:

``` python
# Python
x = MX.sym('x',2)
A = MX(2,2)
A[0,0] = x[0]
A[1,1] = x[0]+x[1]
print('A:', A)
```

``` matlab
% MATLAB
x = MX.sym('x',2);
A = MX(2,2);
A(1,1) = x(1);
A(2,2) = x(1)+x(2);
display(A)
```

The interpretation of the (admittedly cryptic) output is that starting with an all zero sparse matrix, an element is assigned to `x_0`. It is then projected to a matrix of different sparsity and an another element is assigned to `x_0+x_1`.

Element access and assignment, of the type you have just seen, are examples of operations that can be used to construct expressions. Other examples of operations are matrix multiplications, transposes, concatenations, resizings, reshapings and function calls.

Mixing `SX` and `MX`
--------------------

You can *not* multiply an `SX` object with an `MX` object, or perform any other operation to mix the two in the same expression graph. You can, however, in an `MX` graph include calls to a *function* defined by `SX` expressions. This will be demonstrated in Chapter [ch:function]. Mixing `SX` and `MX` is often a good idea since functions defined by `SX` expressions have a much lower overhead per operation making it much faster for operations that are naturally written as a sequence of scalar operations. The `SX` expressions are thus intended to be used for low level operations (for example the DAE right hand side in Section integrator), whereas the `MX` expressions act as a glue and enables the formulation of e.g. the constraint function of an NLP (which might contain calls to ODE/DAE integrators, or might simply be too large to expand as one big expression).

The `Sparsity` class
--------------------

As mentioned above, matrices in `CasADi` are stored using the *compressed column storage* (CCS) format. This is a standard format for sparse matrices that allows linear algebra operations such as elementwise operations, matrix multiplication and transposes to be performed efficiently. In the CCS format, the sparsity pattern is decoded using the dimensions – the number of rows and number of columns – and two vectors. The first vector contains the index of the first structurally nonzero element of each column and the second vector contains the row index for every nonzero element. For more details on the CCS format, see e.g. T[emplates for the Solution of Linear Systems](http://netlib.org/linalg/html_templates/node92.html) on Netlib. Note that `CasADi` uses the CCS format for sparse as well as dense matrices.

Sparsity patterns in `CasADi` are stored as instances of the `Sparsity` class, which is *reference-counted*, meaning that multiple matrices can share the same sparsity pattern, including `MX` expression graphs and instances of `SX` and `DM`. The `Sparsity` class is also *cached*, meaning that the creation of multiple instances of the same sparsity patterns is always avoided.

The following list summarizes the most commonly used ways of constructing new sparsity patterns:

-   `Sparsity.dense(n,m)`: Create a dense *n*-by-*m* sparsity pattern

-   `Sparsity(n,m)`: Create a sparse *n*-by-*m* sparsity pattern

-   `Sparsity.diag(n)`: Create a diagonal *n*-by-*n* sparsity pattern

-   `Sparsity.upper(n)`: Create an upper triangular *n*-by-*n* sparsity pattern

-   `Sparsity.lower(n)`: Create a lower triangular *n*-by-*n* sparsity pattern

The `Sparsity` class can be used to create non-standard matrices, e.g.

``` python
# Python
print(SX.sym('x',Sparsity.lower(3)))
```

``` matlab
% MATLAB
disp(SX.sym('x',Sparsity.lower(3)))
```

### Getting and setting elements in matrices

To get or set an element or a set of elements in `CasADi`’s matrix types (`SX`, `MX` and `DM`), we use square brackets in Python and round brackets in C++ and MATLAB. As is conventional in these languages, indexing starts from zero in C++ and Python but from one in MATLAB. In Python and C++, we allow negative indices to specify an index counted from the end. In MATLAB, use the `end` keyword for indexing from the end.

Indexing can be done with one index or two indices. With two indices, you reference a particular row (or set or rows) and a particular column (or set of columns). With one index, you reference an element (or set of elements) starting from the upper left corner and columnwise to the lower right corner. All elements are counted regardless of whether they are structurally zero or not.

``` python
# Python
M = SX([[3,7],[4,5]])
print(M[0,:])
M[0,:] = 1
print(M)
```

``` matlab
% MATLAB
M = SX([3,7;4,5]);
disp(M(1,:))
M(1,:) = 1;
disp(M)
```

Unlike Python’s NumPy, `CasADi` slices are not views into the data of the left hand side; rather, a slice access copies the data. As a result, the matrix *M* is not changed at all in the following example:

```python
M = SX([[3,7],[4,5]])
M[0,:][0,0] = 1
print(M)
```

The getting and setting matrix elements is elaborated in the following. The discussion applies to all of `CasADi`’s matrix types.

##### Single element access

is getting or setting by providing a row-column pair or its flattened index (columnwise starting in the upper left corner of the matrix):

``` python
# Python
M = diag(SX([3,4,5,6]))
print(M)
```

``` matlab
% MATLAB
M = diag(SX([3,4,5,6]));
disp(M)
```

``` python
print(M[0,0], M[1,0], M[-1,-1])
```

``` matlab
M(1,1), M(2,1), M(end,end)
```

``` python
print(M[5], M[-6])
```

``` matlab
M(6), M(end-5)
```

##### Slice access

means setting multiple elements at once. This is significantly more efficient than setting the elements one at a time. You get or set a slice by providing a (*start*,*stop*,*step*) triple. In Python and MATLAB, `CasADi` uses standard syntax:

``` python
print(M[:,1])
```

``` matlab
disp(M(:,2))
```

``` python
print(M[1:,1:4:2])
```

``` matlab
disp(M(2:end,2:2:4))
```

In C++, `CasADi`’s `Slice` helper class can be used. For the example above, this means `M(Slice(1,-1),Slice(1,4,2))`, respectively.

##### List access

is similar to (but potentially less efficient than) slice access:

``` python
M = SX([[3,7,8,9],[4,5,6,1]])
print(M)
```

``` matlab
M = SX([3 7 8 9; 4 5 6 1]);
disp(M)
```

``` python
print(M[0,[0,3]], M[[5,-6]])
```

``` matlab
M(1,[1,4]), M([6,numel(M)-5])
```

Arithmetic operations
---------------------

`CasADi` supports most standard arithmetic operations such as addition, multiplications, powers, trigonometric functions etc:

``` python
x = SX.sym('x')
y = SX.sym('y',2,2)
print(sin(y)-x)
```

``` matlab
x = SX.sym('x');
y = SX.sym('y',2,2);
sin(y)-x
```

In C++ and Python (but not in MATLAB), the standard multiplication operation (using `*`) is reserved for elementwise multiplication (in MATLAB `.*`). For **matrix multiplication**, use `mtimes(A,B)`:

``` python
print(y*y, mtimes(y,y))
```

``` matlab
y.*y, y*y
```

As is customary in MATLAB, multiplication using `*` and `.*` are equivalent when either of the arguments is a scalar.

**Transposes** are formed using the syntax `A.T()` in C++ and with `A.’` in MATLAB:

``` python
print(y.T)
```

``` matlab
y'
```

**Reshaping** means changing the number of rows and columns but retaining the number of elements and the relative location of the nonzeros. This is a computationally very cheap operation which is performed using the syntax:

``` python
x = SX.eye(4)
print(reshape(x,2,8))
```

``` matlab
x = SX.eye(4);
reshape(x,2,8)
```

**Concatenation** means stacking matrices horizontally or vertically. Due to the column-major way of storing elements in `CasADi`, it is most efficient to stack matrices horizontally. Matrices that are in fact column vectors (i.e. consisting of a single column), can also be stacked efficiently vertically. Vertical and horizontal concatenation is performed using the functions `vertcat` and `horzcat` (that take a list of input arguments) in Python and C++ and with square brackets in MATLAB:

``` python
x = SX.sym('x',5)
y = SX.sym('y',5)
print(vertcat(x,y))
```

``` matlab
x = SX.sym('x',5);
y = SX.sym('y',5);
[x;y]
```

``` python
print(horzcat(x,y))
```

``` matlab
[x,y]
```

$$Horizontal and vertical splitting$$ are the inverse operations of the above introduced horizontal and vertical concatenation. To split up an expression horizontally into $n$ smaller expressions, you need to provide, in addition to the expression being splitted, a vector *offset* of length $n$ + 1. The first element of the $offset$ vector must be 0 and the last element must be the number of columns. Remaining elements must follow in a non-decreasing order. The output $i$ of the splitting operation then contains the columns $c$ with $textit{offset}[i] le c < textit{offset}[i+1]$. The following demonstrates the syntax:

``` python
x = SX.sym('x',5,2)
w = horzsplit(x,[0,1,2])
print(w[0], w[1])
```

``` matlab
x = SX.sym('x',5,2);
w = horzsplit(x,[0,1,2]);
w{1}, w{2}
```

The `vertsplit` operation works analogously, but with the *offset* vector referring to rows:

``` python
w = vertsplit(x,[0,3,5])
print(w[0], w[1])
```

``` matlab
w = vertsplit(x,[0,3,5]);
w{1}, w{2}
```

Note that it is always possible to use slice element access instead of horizontal and vertical splitting, for the above vertical splitting:

``` python
w = [x[0:3,:], x[3:5,:]]
print(w[0], w[1])
```

``` matlab
w = {x(1:3,:), x(4:5,:)};
w{1}, w{2}
```

For `SX` graphs, this alternative way is completely equivalent, but for `MX` graphs using `horzsplit`/`vertsplit` is *significantly more efficient when all the splitted expressions are needed*.

**Inner product**, defined as $<A,B> := trace{A , B} = sum_{i,j} , A_{i,j} , B_{i, j}$ are created as follows:

``` python
x = SX.sym('x',2,2)
print(dot(x,x))
```

``` matlab
x = SX.sym('x',2,2)
dot(x,x)
```

Many of the above operations are also defined for the `Sparsity` class (Section sparsity_class), e.g. `vertcat`, `horzsplit`, transposing, addition (which returns the *union* of two sparsity patterns) and multiplication (which returns the *intersection* of two sparsity patterns).

Querying properties
-------------------

You can check if a matrix or sparsity pattern has a certain property by calling an appropriate member function. e.g.

``` python
y = SX.sym('y',10,1)
print(y.shape)
```

``` matlab
y = SX.sym('y',10,1);
size(y)
```

Note that in MATLAB, `myfcn(obj, arg)` are both valid ways of calling a member function `myfcn`. The latter variant is probably preferable from a style viewpoint.

Some commonly used properties for a matrix *A* are:

-   `A.size1()`: The number of rows

-   `A.size2()`: The number of columns

-   `A.shape`: (in MATLAB “size”) The shape, i.e. the pair (*nrow*,*ncol*)

-   `A.numel()`: The number of elements, i.e `nrow` * `ncol`

-   `A.nnz()`: The number of structurally nonzero elements, equal to *A*.numel() if *dense*.

-   `A.sparsity()`: Retrieve a reference to the sparsity pattern

-   `A.is_dense()`: Is a matrix dense, i.e. having no structural zeros

-   `A.is_scalar()`: Is the matrix a scalar, i.e. having dimensions 1-by-1?

-   `A.is_column()`: Is the matrix a vector, i.e. having dimensions *n*-by-1?

-   `A.is_square()`: Is the matrix square?

-   `A.is_triu()`: Is the matrix upper triangular?

-   `A.is_constant()`: Are the matrix entries all constant?

-   `A.is_integer()`: Are the matrix entries all integer-valued?

The last queries are examples of queries for which *false negative* returns are allowed. A matrix for which *A*.is_constant() is *true* is guaranteed to be constant, but is *not* guaranteed to be non-constant if *A*.is_constant() is *false*. We recommend you to check the API documentation for a particular function before using it for the first time.

Linear algebra
--------------

`CasADi` supports a limited number of linear algebra operations, e.g. for solution of linear systems of equations:

``` python
A = MX.sym('A',3,3)
b = MX.sym('b',3)
print(solve(A,b))
```

``` matlab
A = MX.sym('A',3,3);
b = MX.sym('b',3);
solve(A,b)
```

Calculus – algorithmic differentiation
--------------------------------------

The single most central functionality of `CasADi` is *algorithmic (or automatic) differentiation* (AD). For a function $f$ : $f: mathbb{R}^N rightarrow mathbb{R}^M$
$y = f(x)$,
 *Forward mode* directional derivatives can be used to calculate Jacobian-times-vector products:
$$hat{y} = frac{partial f}{partial x} , hat{x}.$$

Similarly, *reverse mode* directional derivatives can be used to calculate Jacobian-transposed-times-vector products:
$$bar{x} = left(frac{partial f}{partial x}right)^{text{T}} , bar{y}.$$

Both forward and reverse mode directional derivatives are calculated at a cost proportional to evaluating $f(x)$, *regardless of the dimension of $x$*.

CasADi is also able to generate complete, *sparse* Jacobians efficiently. The algorithm for this is very complex, but essentially consists of the following steps:

-   Automatically detect the sparsity pattern of the Jacobian

-   Use graph coloring techniques to find a few forward and/or directional derivatives needed to construct the complete Jacobian

-   Calculate the directional derivatives numerically or symbolically

-   Assemble the complete Jacobian

Hessians are calculated by first calculating the gradient and then performing the same steps as above to calculate the Jacobian of the gradient in the same way as above, while exploiting symmetry.

### Syntax

An expression for a Jacobian is obtained using the syntax:

``` python
A = SX.sym('A',3,2)
x = SX.sym('x',2)
print(jacobian(mtimes(A,x),x))
```

``` matlab
A = SX.sym('A',3,2);
x = SX.sym('x',2);
jacobian(A*x,x)
```

When the differentiated expression is a scalar, you can also calculate the gradient in the matrix sense:

``` python
print(gradient(dot(A,A),A))
```

``` matlab
gradient(dot(A,A),A)
```

Hessians, and as a by-product gradients, are obtained as follows:

``` python
[H,g] = hessian(dot(x,x),x)
print('H:', H)
```

``` matlab
[H,g] = hessian(dot(x,x),x);
display(H)
```

For calculating a Jacobian-times-vector product, the `jtimes` function – performing forward mode AD – is often more efficient than creating the full Jacobian and performing a matrix-vector multiplication:

``` python
v = SX.sym('v',2)
f = mtimes(A,x)
print(jtimes(f,x,v))
```

``` matlab
v = SX.sym('v',2);
f = A*x;
jtimes(f,x,v)
```

The `jtimes` function optionally calculates the transposed-Jacobian-times-vector product, i.e. reverse mode AD:

``` python
w = SX.sym('w',3)
f = mtimes(A,x)
print(jtimes(f,x,w,True))
```

``` matlab
w = SX.sym('w',3);
f = A*x
jtimes(f,x,w,true)
```
