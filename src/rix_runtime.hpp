#pragma once
#include <Eigen/Dense>
#include <string>

namespace rix { namespace runtime {

template <int T, int N>
Eigen::Matrix<double, T - 1, N> pct_change(const Eigen::Matrix<double, T, N>&) { return {}; }

template <int T, int N>
Eigen::Matrix<double, N, N> cov(const Eigen::Matrix<double, T, N>&) { return {}; }

template <int T, int N>
Eigen::Matrix<double, 1, N> std(const Eigen::Matrix<double, T, N>&) { return {}; }

template <int T, int N>
Eigen::Matrix<double, 1, N> sharpe(const Eigen::Matrix<double, T, N>&, double) { return {}; }

template <int n, int T, int N>
Eigen::Matrix<double, T - n + 1, N> rolling(const Eigen::Matrix<double, T, N>&, const std::string&) { return {}; }

} }
