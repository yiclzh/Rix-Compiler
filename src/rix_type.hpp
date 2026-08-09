#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "dim_expr.hpp"
#include "tag.hpp"

namespace rix {

class RixType;

struct MatrixType {
    Tag tag;
    DimExpr rows;
    DimExpr cols;

    bool operator==(const MatrixType& other) const {
        return tag == other.tag && rows == other.rows && cols == other.cols;
    }

    bool operator!=(const MatrixType& other) const { return !(*this == other); }

};

struct ScalarType { bool operator==(const ScalarType&) const { return true; }};
struct BoolType { bool operator==(const BoolType&) const { return true; }};
struct IntType { bool operator==(const IntType&) const { return true; }};
struct NatType { bool operator==(const NatType&) const { return true; }};
struct StringType { bool operator==(const StringType&) const { return true; }};
struct VoidType { bool operator==(const VoidType&) const { return true; }};

struct ResultType {
    std::shared_ptr<RixType> okType;

    bool operator==(const ResultType& other) const;
    bool operator!=(const ResultType& other) const { return !(*this == other); }
};

struct StructType {
    std::string name;
    std::vector<DimExpr> genericArgs;

    bool operator==(const StructType& other) const {
        return name == other.name && genericArgs == other.genericArgs;
    }
    bool operator!=(const StructType& other) const { return !(*this == other); }
};

// RixType is every type a Rix value can have. It's a class wrapping a
// std::variant, rather than a bare using RixType = std::variant<...>

class RixType {
    public:
        using Variant = std::variant<
        MatrixType,
        ScalarType,
        BoolType,
        IntType,
        NatType,
        StringType,
        VoidType,
        ResultType,
        StructType
        >;

        explicit RixType(Variant v) : value_(std::move(v)) {}

        static RixType matrix(Tag tag, DimExpr rows, DimExpr cols);
        static RixType scalar();
        static RixType boolean();
        static RixType integer();
        static RixType nat();
        static RixType string();
        static RixType voidType();
        static RixType result(RixType ok);
        static RixType structType(std::string name, std::vector<DimExpr> genericArgs = {});

        // Allows TypeChecker to inspect a RixType
        template <typename T>
        bool is() const { return std::holds_alternative<T>(value_); }

        template <typename T>
        const T& as() const { return std::get<T>(value_); }

        bool operator==(const RixType& other) const { return value_ == other.value_; }
        bool operator!=(const RixType& other) const { return !(*this == other); }

        std::string toString() const;


    private:
        Variant value_;
};



}