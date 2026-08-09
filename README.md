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

Rix Compiler Architecture  

```mermaid
flowchart TD
    A[Source file<br/>.rix program text] --> B[Tokenizer<br/>source to tokens]
    B --> C[Parser<br/>builds AST tree]
    C --> D[Type checker<br/>unify + symbol tables]
    D --> E[C++ writer<br/>emits Eigen C++]
    E --> F[Generated code<br/>.cpp file, Eigen]
    F --> G[Compiled binary<br/>run via g++]

    H[Core types<br/>DimExpr, Tag, RixType] --> C
    H --> D
    I[Standard library<br/>signatures + runtime] --> D
    I --> E

    classDef done fill:#9FE1CB,stroke:#0F6E56,color:#04342C;
    classDef next fill:#FAC775,stroke:#854F0B,color:#412402;
    classDef todo fill:#D3D1C7,stroke:#5F5E5A,color:#2C2C2A;

    class B,H done
    class C next
    class A,D,E,F,G,I todo
```






