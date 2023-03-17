/*
 * vector.hpp
 *
 *  Created on: 19.01.2012
 *      Author: zmij
 */

#ifndef PSST_MATH_VECTOR_HPP_
#define PSST_MATH_VECTOR_HPP_

#include <psst/math/detail/conversion.hpp>
#include <psst/math/detail/vector_expressions.hpp>
#include <psst/math/detail/vector_ops.hpp>

#include <assert.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace psst {
namespace math {

template <typename T, std::size_t Size, typename Components>
struct vector : expr::vector_expression<vector<T, Size, Components>>,
                detail::vector_ops<T, Size, Components> {

    using this_type            = vector<T, Size, Components>;
    using traits               = traits::vector_traits<this_type>;
    using base_expression_type = expr::vector_expression<vector<T, Size, Components>>;
    using value_type           = typename traits::value_type;
    using lvalue_reference     = typename traits::lvalue_reference;
    using const_reference      = typename traits::const_reference;
    using magnitude_type       = typename traits::magnitude_type;
    using pointer              = typename traits::pointer;
    using const_pointer        = typename traits::const_pointer;
    using index_sequence_type  = typename traits::index_sequence_type;
    using iterator             = typename traits::iterator;
    using const_iterator       = typename traits::const_iterator;
    using init_list            = std::initializer_list<value_type>;
    using component_access     = typename base_expression_type::component_access;
    template <std::size_t N>
    using value_policy = typename component_access::template value_policy<N>;

    static constexpr auto size = traits::size;

    constexpr vector() : vector{T{0}, index_sequence_type{}} {}

    /**
     * Single value construction, initialize all components to the
     * same value. To precede initializer list constructor, should
     * be called with round parenthesis.
     * @param val
     */
    constexpr explicit vector(value_type val) : vector(val, index_sequence_type{}) {}

    constexpr vector(init_list const& args) : vector(args.begin(), index_sequence_type{}) {}

    constexpr vector(const_pointer p) : vector(p, index_sequence_type{}) {}

    template <typename U, std::size_t SizeR, typename ComponentsR,
              typename = math::traits::enable_if_compatible_components<Components, ComponentsR>>
    constexpr /* implicit */ vector(vector<U, SizeR, ComponentsR> const& rhs)
        : vector(rhs, utils::make_min_index_sequence<Size, SizeR>{})
    {}
    template <typename Expression, typename = math::traits::enable_if_vector_expression<Expression>,
              typename = math::traits::enable_for_compatible_components<this_type, Expression>>
    constexpr /* implicit */ vector(Expression&& rhs)
        : vector(
            std::forward<Expression>(rhs),
            utils::make_min_index_sequence<Size,
                                           math::traits::vector_expression_size_v<Expression>>{})
    {}

    pointer
    data()
    {
        return data_.data();
    }

    constexpr const_pointer
    data() const
    {
        return data_.data();
    }

    template <std::size_t N>
    typename value_policy<N>::accessor_type
    at()
    {
        static_assert(N < size, "Invalid component index in vector");
        return value_policy<N>::accessor(std::get<N>(data_));
    }

    template <std::size_t N>
    constexpr const_reference
    at() const
    {
        static_assert(N < size, "Invalid component index in vector");
        return std::get<N>(data_);
    }

    iterator
    begin()
    {
        return data_.begin();
    }

    constexpr const_iterator
    begin() const
    {
        return cbegin();
    }
    constexpr const_iterator
    cbegin() const
    {
        return data_.cbegin();
    }

    iterator
    end()
    {
        return data_.end();
    }

    constexpr const_iterator
    end() const
    {
        return cend();
    }
    constexpr const_iterator
    cend() const
    {
        return data_.end();
    }

    // FIXME Apply accessors
    lvalue_reference operator[](std::size_t idx)
    {
        assert(idx < size);
        return data_[idx];
    }

    constexpr const_reference operator[](std::size_t idx) const
    {
        assert(idx < size);
        return data_[idx];
    }
    // TODO Make converter a CRTP base
    template <typename U>
    U
    convert() const
    {
        return math::convert<U>(*this);
    }
    /**
     * Implicit conversion to pointer to element
     */
    operator pointer() { return data(); }
    /**
     * Implicit conversion to const pointer to element
     */
    operator const_pointer() const { return data(); }

private:
    template <std::size_t... Indexes>
    constexpr vector(value_type val, std::index_sequence<Indexes...>)
        : data_({value_policy<Indexes>::apply(utils::value_fill<Indexes, T>{val}.value)...})
    {}
    template <std::size_t... Indexes>
    constexpr vector(const_pointer p, std::index_sequence<Indexes...>)
        : data_({value_policy<Indexes>::apply(*(p + Indexes))...})
    {}
    template <typename U, std::size_t SizeR, typename ComponentsR, std::size_t... Indexes>
    constexpr vector(vector<U, SizeR, ComponentsR> const& rhs, std::index_sequence<Indexes...>)
        : data_({value_policy<Indexes>::apply(rhs.template at<Indexes>())...})
    {}
    template <typename Expr, std::size_t... Indexes>
    constexpr vector(Expr&& rhs, std::index_sequence<Indexes...>)
        : data_({value_policy<Indexes>::apply(expr::get<Indexes>(std::forward<Expr>(rhs)))...})
    {}

private:
    using data_type = std::array<T, size>;
    data_type data_;
};

template <std::size_t N, typename T, std::size_t Size, typename Components>
constexpr typename vector<T, Size, Components>::template value_policy<N>::accessor_type
get(vector<T, Size, Components>& v)
{
    return v.template at<N>();
}

/**
 * Projection of vector v onto vector n
 * @param n Target vector
 * @param v Source vector
 * @return Vector that is parallel to n
 */
template <typename T, size_t Size, typename Components>
vector<typename vector<T, Size, Components>::value_type, Size, Components>
projection(vector<T, Size, Components> const& n, vector<T, Size, Components> const& v)
{
    return n * (v * n / n.magnitude_square());
}

/**
 * Vector that is perpendicular to n, such as vǁ + vⱶ = v
 * @param n
 * @param v
 * @return
 */
template <typename T, size_t Size, typename Components>
vector<typename vector<T, Size, Components>::value_type, Size, Components>
perpendicular(vector<T, Size, Components> const& n, vector<T, Size, Components> const& v)
{
    return v - projection(n, v);
}

/**
 * Project vector v onto vector n
 * @param n
 * @param v
 * @return Pair of vectors vǁ, vⱶ. vǁ is parallel to n, vǁ + vⱶ = v
 */
template <typename T, size_t Size, typename Components>
std::pair<vector<typename vector<T, Size, Components>::value_type, Size, Components>,
          vector<typename vector<T, Size, Components>::value_type, Size, Components>>
project(vector<T, Size, Components> const& n, vector<T, Size, Components> const& v)
{
    auto p = projection(n, v);
    return std::make_pair(p, v - p);
}

}    // namespace math
} /* namespace psst */

#ifdef __METASHELL
// Some definitions for debugging templates, not to input them
// every time the metashell starts
#    include <psst/math/colors.hpp>
#    include <psst/math/cylindrical_coord.hpp>
#    include <psst/math/polar_coord.hpp>
#    include <psst/math/quaternion.hpp>
#    include <psst/math/spherical_coord.hpp>
using namespace psst::math;

using vec3f  = vector<float, 3>;
using vec3d  = vector<double, 3>;
using vec3fn = vector<float, 3, components::none>;

using vec4f  = vector<float, 4>;
using vec4d  = vector<double, 4>;
using vec4fn = vector<float, 4, components::none>;

vec3f  v3f_1, v3f_2;
vec4f  v4f_1, v4f_2;
vec3fn v3f_n;
vec4fn v4f_n;

vec3d v3d_1, v3d_2;
vec4d v4d_1, v4d_2;

using polar_f       = polar_coord<float>;
using spherical_f   = spherical_coord<float>;
using cylindrical_f = cylindrical_coord<float>;

polar_f       p_1;
spherical_f   s_1;
cylindrical_f c_1;

#endif

#endif /* PSST_MATH_VECTOR_HPP_ */
