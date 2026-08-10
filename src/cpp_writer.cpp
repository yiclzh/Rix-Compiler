#include "cpp_writer.hpp"
#include <stdexcept>

namespace rix {

void CppWriter::writeIndent() {
    for (int i = 0; i < indent_; ++i) out_ << "    ";
}

void CppWriter::writeIncludes() {
    out_ << "#include <Eigen/Dense>\n";
    out_ << "#include <expected>\n";
    out_ << "#include <string>\n";
    out_ << "#include \"rix_runtime.hpp\"\n\n";
    out_ << "struct Error { std::string message; };\n\n";
}

std::string CppWriter::emitType(const RixType& t) {
    if (t.is<MatrixType>()) {
        const auto& m = t.as<MatrixType>();
        if (!m.rows.isConstant() || !m.cols.isConstant()) {
            return "Eigen::MatrixXd";
        }
        return "Eigen::Matrix<double, " + std::to_string(m.rows.constantTerm()) +
               ", " + std::to_string(m.cols.constantTerm()) + ">";
    }
    if (t.is<ScalarType>()) return "double";
    if (t.is<BoolType>())   return "bool";
    if (t.is<IntType>())    return "int";
    if (t.is<NatType>())    return "int";
    if (t.is<StringType>()) return "std::string";
    if (t.is<VoidType>())   return "void";
    if (t.is<ResultType>()) {
        return "std::expected<" + emitType(*t.as<ResultType>().okType) + ", Error>";
    }
    if (t.is<StructType>()) {
        const auto& s = t.as<StructType>();
        if (s.genericArgs.empty()) return s.name;
        std::string out = s.name + "<";
        for (size_t i = 0; i < s.genericArgs.size(); ++i) {
            if (i) out += ", ";
            out += std::to_string(s.genericArgs[i].constantTerm());
        }
        return out + ">";
    }
    throw std::runtime_error("emitType: unrecognized RixType");
}

std::string CppWriter::emitExpr(const Expr& expr) {
    if (expr.is<IntLiteral>())    return std::to_string(expr.as<IntLiteral>().value);
    if (expr.is<FloatLiteral>())  return std::to_string(expr.as<FloatLiteral>().value);
    if (expr.is<StringLiteral>()) return "\"" + expr.as<StringLiteral>().value + "\"";
    if (expr.is<BoolLiteral>())   return expr.as<BoolLiteral>().value ? "true" : "false";
    if (expr.is<Identifier>())    return expr.as<Identifier>().name;
    if (expr.is<CallExpr>())      return emitCall(expr.as<CallExpr>());
    if (expr.is<BinaryExpr>())    return emitBinary(expr.as<BinaryExpr>());
    if (expr.is<UnaryExpr>()) {
        const auto& u = expr.as<UnaryExpr>();
        return "(" + u.op + emitExpr(*u.operand) + ")";
    }
    if (expr.is<StructLiteral>()) {
        const auto& s = expr.as<StructLiteral>();
        std::string out = s.structName + "{";
        for (size_t i = 0; i < s.fields.size(); ++i) {
            if (i) out += ", ";
            out += "." + s.fields[i].first + " = " + emitExpr(*s.fields[i].second);
        }
        return out + "}";
    }
    throw std::runtime_error("emitExpr: unrecognized expression kind");
}

std::string CppWriter::emitCall(const CallExpr& call) {
    if (call.funcName == "Ok")  return emitExpr(*call.args[0]);   // implicit conversion in return position
    if (call.funcName == "Err") return "std::unexpected(" + emitExpr(*call.args[0]) + ")";

    std::string prefix = stdlibFuncNames_.count(call.funcName) ? "rix::runtime::" : "";
    std::string out = prefix + call.funcName;

    if (!call.templateArgs.empty()) {
        out += "<";
        for (size_t i = 0; i < call.templateArgs.size(); ++i) {
            if (i) out += ", ";
            out += std::to_string(call.templateArgs[i].constantTerm());
        }
        out += ">";
    }

    out += "(";
    for (size_t i = 0; i < call.args.size(); ++i) {
        if (i) out += ", ";
        out += emitExpr(*call.args[i]);
    }
    return out + ")";
}

std::string CppWriter::emitBinary(const BinaryExpr& bin) {
    std::string lhs = emitExpr(*bin.lhs);
    std::string rhs = emitExpr(*bin.rhs);
    if (bin.op == ".*") return "(" + lhs + ".cwiseProduct(" + rhs + "))";
    if (bin.op == "./") return "(" + lhs + ".cwiseQuotient(" + rhs + "))";
    return "(" + lhs + " " + bin.op + " " + rhs + ")";
}

void CppWriter::writeStructDecl(const StructDecl& decl) {
    if (!decl.genericParams.empty()) {
        writeIndent();
        out_ << "template<";
        for (size_t i = 0; i < decl.genericParams.size(); ++i) {
            if (i) out_ << ", ";
            out_ << "int " << decl.genericParams[i];
        }
        out_ << ">\n";
    }
    writeIndent();
    out_ << "struct " << decl.name << " {\n";
    indent_++;
    for (const auto& f : decl.fields) {
        writeIndent();
        out_ << emitType(f.type) << " " << f.name << ";\n";
    }
    indent_--;
    writeIndent();
    out_ << "};\n\n";
}

void CppWriter::writeFuncForwardDecl(const FuncDecl& decl) {
    writeIndent();
    std::string returnType = (decl.name == "main") ? "int" : emitType(decl.returnType);
    out_ << returnType << " " << decl.name << "(";
    for (size_t i = 0; i < decl.params.size(); ++i) {
        if (i) out_ << ", ";
        out_ << "const " << emitType(decl.params[i].type) << "& " << decl.params[i].name;
    }
    out_ << ");\n";
}

void CppWriter::writeFuncDecl(const FuncDecl& decl) {
    writeIndent();
    bool isMain = (decl.name == "main");
    std::string returnType = isMain ? "int" : emitType(decl.returnType);
    out_ << returnType << " " << decl.name << "(";
    for (size_t i = 0; i < decl.params.size(); ++i) {
        if (i) out_ << ", ";
        out_ << "const " << emitType(decl.params[i].type) << "& " << decl.params[i].name;
    }
    out_ << ") {\n";
    indent_++;
    inMainFunc_ = isMain;
    writeBlock(decl.body);
    inMainFunc_ = false;
    indent_--;
    writeIndent();
    out_ << "}\n\n";
}

void CppWriter::writeBlock(const Block& block) {
    for (const auto& stmt : block.statements) writeStatement(stmt);
}

void CppWriter::writeStatement(const Statement& stmt) {
    writeIndent();

    if (stmt.is<LetStatement>()) {
        const auto& s = stmt.as<LetStatement>();
        std::string cppType = s.declaredType ? emitType(*s.declaredType) : "auto";
        std::string constPrefix = s.isMut ? "" : "const ";
        out_ << constPrefix << cppType << " " << s.name << " = " << emitExpr(*s.value) << ";\n";
        return;
    }
    if (stmt.is<AssignStatement>()) {
        const auto& s = stmt.as<AssignStatement>();
        out_ << s.name << " = " << emitExpr(*s.value) << ";\n";
        return;
    }
    if (stmt.is<IfStatement>()) {
        const auto& s = stmt.as<IfStatement>();
        out_ << "if (" << emitExpr(*s.condition) << ") {\n";
        indent_++;
        writeBlock(*s.thenBlock);
        indent_--;
        writeIndent();
        if (s.elseBlock) {
            out_ << "} else {\n";
            indent_++;
            writeBlock(*s.elseBlock);
            indent_--;
            writeIndent();
        }
        out_ << "}\n";
        return;
    }
    if (stmt.is<ForStatement>()) {
        const auto& s = stmt.as<ForStatement>();
        out_ << "for (int " << s.varName << " = " << s.start.toString()
             << "; " << s.varName << " < " << s.end.toString()
             << "; ++" << s.varName << ") {\n";
        indent_++;
        writeBlock(*s.body);
        indent_--;
        writeIndent();
        out_ << "}\n";
        return;
    }
    if (stmt.is<ReturnStatement>()) {
        const auto& s = stmt.as<ReturnStatement>();
        if (s.value) out_ << "return " << emitExpr(*s.value) << ";\n";
        else if (inMainFunc_) out_ << "return 0;\n";
        else out_ << "return;\n";
        return;
    }
    if (stmt.is<ExprStatement>()) {
        const auto& s = stmt.as<ExprStatement>();
        out_ << emitExpr(*s.expr) << ";\n";
        return;
    }
}

void CppWriter::writeProgram(const ProgramNode& program) {
    writeIncludes();
    for (const auto& s : program.structs) writeStructDecl(s);

    for (const auto& f : program.funcs) writeFuncForwardDecl(f);
    out_ << "\n";
    for (const auto& f : program.funcs) writeFuncDecl(f);
}

} 
