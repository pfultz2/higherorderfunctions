/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    decorate.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef FIT_GUARD_DECORATE_H
#define FIT_GUARD_DECORATE_H

/// decorate
/// ========
/// 
/// Description
/// -----------
/// 
/// The `decorate` function adaptor helps create simple function decorators. 
/// 
/// A function adaptor takes a function and returns a new functions whereas a
/// decorator takes some parameters and returns a function adaptor. The
/// `decorate` function adaptor will return a decorator that returns a
/// function adaptor. Eventually, it will invoke the function with the user-
/// provided parameter and function.
/// 
/// Synopsis
/// --------
/// 
///     template<class F>
///     constexpr decorate_adaptor<F> decorate(F f);
/// 
/// Semantics
/// ---------
/// 
///     assert(decorate(f)(x)(g)(xs...) == f(x, g, xs...));
/// 
/// Requirements
/// ------------
/// 
/// F must be:
/// 
/// * [ConstCallable](ConstCallable)
/// * MoveConstructible
/// 
/// Example
/// -------
/// 
///     #include <fit.hpp>
///     #include <cassert>
///     #include <iostream>
///     #include <string>
///     using namespace fit;
/// 
///     struct logger_f
///     {
///         template<class F, class... Ts>
///         auto operator()(const std::string& message, F&& f, Ts&&... xs) const 
///             -> decltype(f(std::forward<Ts>(xs)...))
///         {
///             // Message to print out when the function is called
///             std::cout << message << std::endl;
///             // Call the function
///             return f(std::forward<Ts>(xs)...);
///         }
///     };
///     // The logger decorator
///     FIT_STATIC_FUNCTION(logger) = fit::decorate(logger_f());
///     
///     struct sum_f
///     {
///         template<class T, class U>
///         T operator()(T x, U y) const
///         {
///             return x+y;
///         }
///     };
///     
///     FIT_STATIC_FUNCTION(sum) = sum_f();
///     int main() {
///         // Use the logger decorator to print "Calling sum" when the function is called
///         assert(3 == logger("Calling sum")(sum)(1, 2));
///     }
/// 

#include <fit/detail/builder.hpp>
#include <fit/detail/compressed_pair.hpp>
#include <fit/detail/builder/unary.hpp>
#include <fit/reveal.hpp>

namespace fit { namespace detail {

template<class D, class T, class F>
struct decorator_invoke
// : compressed_pair<compressed_pair<F, T>, D>
: compressed_pair<compressed_pair<D, T>, F>
{
    // typedef compressed_pair<F, T> base;
    typedef compressed_pair<compressed_pair<D, T>, F> base;

    FIT_INHERIT_CONSTRUCTOR(decorator_invoke, base)

    template<class... Ts>
    constexpr const compressed_pair<D, T>& get_pair(Ts&&... xs) const noexcept
    {
        return this->first(xs...);
    }

    template<class... Ts>
    constexpr const F& base_function(Ts&&... xs) const noexcept
    {
        return this->second(xs...);
    }

    template<class... Ts>
    constexpr const D& get_decorator(Ts&&... xs) const noexcept
    {
        return this->get_pair(xs...).first(xs...);
    }

    template<class... Ts>
    constexpr const T& get_data(Ts&&... xs) const noexcept
    {
        return this->get_pair(xs...).second(xs...);
    }

    FIT_RETURNS_CLASS(decorator_invoke);

    struct decorator_invoke_failure
    {
        template<class Failure>
        struct apply
        {
            template<class... Ts>
            struct of
            : Failure::template of<const T&, const F&, Ts...>
            {};
        };
    };

    struct failure
    : failure_map<decorator_invoke_failure, D>
    {};

    template<class... Ts>
    constexpr FIT_SFINAE_RESULT(const D&, id_<const T&>, id_<const F&>, id_<Ts>...) 
    operator()(Ts&&... xs) const FIT_SFINAE_RETURNS
    (
        FIT_MANGLE_CAST(const D&)(FIT_CONST_THIS->get_decorator(xs...))(
            FIT_MANGLE_CAST(const T&)(FIT_CONST_THIS->get_data(xs...)),
            FIT_MANGLE_CAST(const F&)(FIT_CONST_THIS->base_function(xs...)),
            FIT_FORWARD(Ts)(xs)...
        )
    );
};

template<class D, class T>
struct decoration
: compressed_pair<D, T>
{
    typedef compressed_pair<D, T> base;
    FIT_INHERIT_CONSTRUCTOR(decoration, base)

    template<class... Ts>
    constexpr const D& get_decorator(Ts&&... xs) const noexcept
    {
        return this->first(xs...);
    }

    template<class... Ts>
    constexpr const T& get_data(Ts&&... xs) const noexcept
    {
        return this->second(xs...);
    }

    template<class F>
    constexpr decorator_invoke<D, T, detail::callable_base<F>> operator()(F f) const
    FIT_NOEXCEPT(FIT_IS_NOTHROW_CONSTRUCTIBLE(decorator_invoke<D, T, detail::callable_base<F>>, compressed_pair<D, T>, detail::callable_base<F>&&))
    {
        return decorator_invoke<D, T, detail::callable_base<F>>(
            *this, static_cast<detail::callable_base<F>&&>(f)
        );
    }
};

struct decorate_adaptor_builder
{
    template<class F>
    struct base
    {};

    struct apply
    {
        template<class F, class T>
        constexpr detail::decoration<F, T> operator()(const F& f, T x) const 
        FIT_NOEXCEPT(FIT_IS_NOTHROW_CONSTRUCTIBLE(F, const F&) && FIT_IS_NOTHROW_CONSTRUCTIBLE(T, T&&))
        {
            return detail::decoration<F, T>(f, static_cast<T&&>(x));
        }
    };

};

}

FIT_DECLARE_ADAPTOR(decorate, detail::unary_adaptor_builder<detail::decorate_adaptor_builder>)

} // namespace fit

#endif
