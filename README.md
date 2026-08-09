# Rix-Compiler




Rix Compiler Pipeline 

your .rix file
      ↓
  Tokenizer    →  breaks source into tokens (func, let, {, "Prices", 252, ...)
      ↓
   Parser      →  turns tokens into a tree structure (the AST)
      ↓
 TypeChecker   →  walks the tree, makes sure cov(prices) fails, rolling<60> works, etc.
      ↓
  CppWriter    →  turns the checked tree into real C++ (Eigen) source code
      ↓
   .cpp file   →  you compile that normally with g++





