//
// Created by hannes on 2017-04-12.
//

#ifndef TASER_ATAN_H
#define TASER_ATAN_H

#include "pinhole.h"

#include <Eigen/Core>
#include <ceres/ceres.h>
#include <entity/paramstore/dynamic_pstore.h>

namespace taser {
namespace cameras {

namespace internal {

struct AtanMeta : public PinholeMeta {
  double gamma;
  Eigen::Vector2d wc;

  size_t NumParameters() const override {
    return PinholeMeta::NumParameters();
  }
};

template<typename T, typename MetaType>
class AtanView : public PinholeView<T, MetaType> {
  using Base = PinholeView<T, MetaType>;
  using Vector2 = Eigen::Matrix<T, 2, 1>;
  using Vector3 = Eigen::Matrix<T, 3, 1>;
 public:
  // Inherit constructors
  using Base::PinholeView;

  Vector2 wc() const {
    return this->meta_.wc.template cast<T>();
  }

  void set_wc(const Vector2& wc) {
    this->meta_.wc = wc.template cast<double>();
  }

  T gamma() const {
    return T(this->meta_.gamma);
  }

  void set_gamma(T gamma) {
    this->meta_.gamma = (double) gamma;
  }

  Vector2 Project(const Vector3 &X) const override {
    const T eps = T(1e-32);

    // Common parts
    Vector2 A = X.head(2)/(X(2) + eps);
    Vector2 L = A - wc();
    T r = ceres::sqrt(L.squaredNorm() + eps);
    T f = ceres::atan(r*gamma())/gamma();

    Vector3 Y(T(0), T(0), T(1));
    Y.head(2) = wc() + f*L/r;

    // Apply camera matrix
    // Normalization not needed since Y(2) == 1
    return (this->camera_matrix() * Y).head(2);
  }
  Vector3 Unproject(const Vector2 &y) const override {
    const T eps = T(1e-32);
    Vector3 ph(y(0), y(1), T(1.0));
    Vector3 phn = this->camera_matrix().inverse() * ph;
    Vector2 L = phn.head(2) - wc();

    T r = ceres::sqrt(L.squaredNorm() + eps);
    T f = ceres::tan(r*gamma())/gamma();

    Vector3 Y(T(0), T(0), T(1));
    Y.head(2) = wc() + f*L/r;
    return Y;
  }
};

template<template<typename...> typename ViewTemplate, typename MetaType, typename StoreType>
class AtanEntity : public PinholeEntity<ViewTemplate, MetaType, StoreType> {
  using Base = PinholeEntity<ViewTemplate, MetaType, StoreType>;

 public:
  AtanEntity(size_t cols, size_t rows, double readout, double gamma, const Eigen::Vector2d& wc) :
      Base(cols, rows, readout) {
    this->set_gamma(gamma);
    this->set_wc(wc);
  }
};
} // namespace internal

class AtanCamera : public internal::AtanEntity<internal::AtanView,
                                               internal::AtanMeta,
                                               entity::DynamicParameterStore<double>> {
 public:
  using internal::AtanEntity<internal::AtanView,
                             internal::AtanMeta,
                             entity::DynamicParameterStore<double>>::AtanEntity;

  static constexpr const char *ENTITY_ID = "Atan";
};

} // namespace cameras
} // namespace taser

#endif //TASER_ATAN_H
