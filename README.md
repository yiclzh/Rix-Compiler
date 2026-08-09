# Rix-Compiler

Rix

A statically-shape-checked matrix language for quantitative finance, compiling to C++ (Eigen).

What is Rix?

Rix catches an entire class of bug that plain C++/numpy/Eigen can't: code that's structurally valid but semantically wrong. cov(prices) — computing a covariance matrix directly on price levels instead of returns — runs fine in most languages and produces a quietly wrong number. In Rix, Prices and Returns are distinct types, even when their numeric shape is identical, so that call is a compile error.

rix:
let prices: Matrix<Prices, 252, 3> = load_csv("universe.csv")
let bad = cov(prices)

error: cov() expects Matrix<Returns,T,N>, found Matrix<Prices,252,3>
note: Prices and Returns are distinct tags — did you mean pct_change(prices)?

Key ideas
Nominal tags (Prices, Returns, CovMatrix, ...) — two matrices with identical shape but different meaning are different types.
Symbolic, statically-checked shapes — Matrix<Returns, T-1, N> is checked at compile time via unification, not just at runtime.
Compiles to real C++ — DimExpr maps to Eigen's fixed-size template parameters, so shape checks disappear entirely by the time the code runs; tags cost nothing at runtime, since they're erased during codegen.
A stdlib built for quant work — covariance, correlation, EWMA, rolling windows, Cholesky, portfolio construction, and more, each with a signature that only accepts the inputs that actually make sense.


Rix Compiler Pipeline 

```mermaid
flowchart TD
%% Main Compiler Pipeline
Input["Your .rix File"] --> Tokenizer
Tokenizer["Rix Tokenizer<br/>breaks source into tokens<br/>func, let, &#123;, &#34;Prices&#34;, 252, ..."] --> Parser
Parser["Parser<br/>builds AST tree"] --> TypeCheck
TypeCheck["Type Checker<br/>unify + symbol tables"] --> CPPWrite
CPPWrite["C++ Writer<br/>emits Eigen C++"] --> GenCode
GenCode["Generated Code<br/>.cpp file"] --> Binary
Binary["Compiled binary<br/>run via g++"]

%% Side Inputs
CoreTypes["Core Types<br/>Dim, Tag, RixType"] -.-> TypeCheck
StdLibrary["Standard Library<br/>signatures + runtime"] -.-> TypeCheck
```






