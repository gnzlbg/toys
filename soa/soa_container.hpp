#ifndef SOA_CONTAINER_HPP_
#define SOA_CONTAINER_HPP_
////////////////////////////////////////////////////////////////////////////////
/// \file \brief Implements tests for Thrust related functionality
#include <boost/mpl/vector.hpp>
#include <boost/mpl/zip_view.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/equal.hpp>
/// Fusion:
#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_map.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/algorithm/auxiliary/copy.hpp>
/// MPL-Fusion:
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/include/convert.hpp>
/// Adapt structs to random, associative, fusion sequences
#include <boost/fusion/adapted/struct/adapt_assoc_struct.hpp>

#include <iostream>
#include <algorithm>
#include <type_traits>
#include <boost/container/vector.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/mpl/print.hpp>
#include <boost/tuple/tuple.hpp>

////////////////////////////////////////////////////////////////////////////////
using namespace boost;

namespace boost { namespace fusion {

/// \brief Swap function for fusion types
template <class T> inline void swap(T&& lhs, T&& rhs) noexcept {
  using std::swap;
  std::remove_reference_t<T> tmp = lhs;
  lhs = std::move(rhs);
  rhs = std::move(tmp);
}

}  // namespace fusion
}  // namespace boost

////////////////////////////////////////////////////////////////////////////////
/// The following as_fusion_map functionality was provided by Evgeny Panasyuk,
/// see
/// http://stackoverflow.com/questions/19657065/is-it-possible-to-generate-a-fusion-map-from-an-adapted-struct/19660249?noredirect=1#19660249
/// and the chat discussions of that day in LoungeC++.

namespace SOA_ {

template<typename Vector,typename First,typename Last,typename do_continue>
struct to_fusion_map_iter;

template<typename Vector,typename First,typename Last>
struct to_fusion_map_iter<Vector,First,Last,mpl::false_>
{
    typedef typename fusion::result_of::next<First>::type Next;
    typedef typename mpl::push_front
    <
        typename to_fusion_map_iter
        <
            Vector,
            Next,
            Last,
            typename fusion::result_of::equal_to<Next,Last>::type
        >::type,
        fusion::pair
        <
            typename fusion::result_of::key_of<First>::type,
            typename fusion::result_of::value_of_data<First>::type
        >
    >::type type;
};
template<typename Vector,typename First,typename Last>
struct to_fusion_map_iter<Vector,First,Last,mpl::true_>
{
    typedef Vector type;
};

template<typename FusionAssociativeSequence>
struct as_fusion_map
{
    typedef typename fusion::result_of::begin<FusionAssociativeSequence>::type First;
    typedef typename fusion::result_of::end<FusionAssociativeSequence>::type Last;
    typedef typename fusion::result_of::as_map
    <
        typename to_fusion_map_iter
        <
            mpl::vector<>,
            First,
            Last,
            typename fusion::result_of::equal_to<First,Last>::type
        >::type
    >::type type;
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// The idea behind map mutator was provided by cv_and_he
/// see: http://stackoverflow.com/questions/19540291/assigning-to-a-map-of-references-from-a-map-of-values-references

template<class Output, class Predicate> struct map_mutator {
  map_mutator(Output& map) : map_out(map) {}
  template <class InputPair> void operator()(const InputPair& inputPair) const
  { Predicate()(inputPair, map_out); }
  Output& map_out;
};

struct assign {
  template<class Input, class Output>
  void operator()(const Input& i, Output& o) const
  { fusion::at_key<typename Input::first_type>(o) = i.second; }
};

struct assign_dereference {
  template<class Input, class Output>
  void operator()(const Input& i, Output& o) const
  { fusion::at_key<typename Input::first_type>(o) = *i.second; }
};

template<class Output> using map_assigner = map_mutator<Output, assign>;
template<class Output> using map_dereferencer = map_mutator<Output, assign_dereference>;
////////////////////////////////////////////////////////////////////////////////

template<class column_tags, class column_types> struct zipped {
  /// \brief (column_tags, column_types) -> (column_tags, P(column_types))
  template<template <class> class P>
  using transformed = typename fusion::result_of::as_map<
    typename mpl::transform<
      typename mpl::transform<column_types, P<mpl::_1>>::type,
      column_tags, fusion::pair<mpl::_2, mpl::_1>
        >::type
    >::type;

    /// \brief (column_tags, column_types) -> (column_tags, P(column_types))
  template<template <class> class P>
  using transformed = typename fusion::result_of::as_map<
    typename mpl::transform<
      typename mpl::transform<column_types, P<mpl::_1>>::type,
      column_tags, fusion::pair<mpl::_2, mpl::_1>
        >::type
    >::type;

  using value_map = typename fusion::result_of::as_map<
    typename mpl::transform<
      column_types, column_tags, fusion::pair<mpl::_2, mpl::_1>
      >::type
    >::type;

  using reference_map = transformed<boost::add_reference>;
};

template<class column_types, class column_tags,
         template <class> class iterator_type>
struct iterator {
  using col_types = column_types;
  using col_tags  = column_tags;

  template<class T> struct iterator_lambda {  using type = iterator_type<T>; };
  using iterator_map = typename zipped<col_tags, col_types>::template
                         transformed<iterator_lambda>;

  /// \name Iterator traits
  ///@{
  using value_type    = typename zipped<col_tags, col_types>::value_map;
  using reference_map = typename zipped<col_tags, col_types>::reference_map;
  using iterator_category = std::random_access_iterator_tag;
  using difference_type   = long;
  using pointer = iterator_map;
  ///@}

  using This = iterator<col_types, col_tags, iterator_type>;

  struct reference : reference_map {
    reference(reference& r) noexcept : reference_map(r) {};
    reference(value_type& v) noexcept : reference_map(ref_from_val(v)) {}
    template<class... Ts> reference(Ts&&... ts) noexcept
      : reference_map(std::forward<Ts>(ts)...) {}

    template<class T> reference& operator=(T&& other) noexcept {
      fusion::for_each(other, map_assigner<reference>(*this));
      return *this;
    }
  };

  iterator() = default;
  // template<class... Ts>
  // explicit iterator(Ts... ts) noexcept : it_(std::move(ts)...) {}
  explicit iterator(iterator_map it) noexcept : it_(std::move(it)) {}

  /// \name Comparison operators (==, !=, <, >, <=, >=)
  ///@{
  friend inline bool operator==(const This& l, const This& r) noexcept
  { return fusion::all(fusion::zip(l.it_, r.it_), Eq()); }
  friend inline bool operator<=(const This& l, const This& r) noexcept
  { return fusion::all(fusion::zip(l.it_, r.it_), LEq()); }
  friend inline bool operator>=(const This& l, const This& r) noexcept
  { return fusion::all(fusion::zip(l.it_, r.it_), GEq()); }

  friend inline bool operator!=(const This& l, const This& r) noexcept
  { return !(l == r); }
  friend inline bool operator< (const This& l, const This& r) noexcept
  { return !(l >= r); }
  friend inline bool operator> (const This& l, const This& r) noexcept
  { return !(l <= r); }
  ///@}

  /// \name Traversal operators (++, +=, +, --, -=, -)
  ///@{
  inline This& operator++() noexcept   {
    it_ = fusion::transform(it_, AdvFwd());
    return *this;
  }
  inline This operator++(int) noexcept {
    const auto it = *this;
    ++(*this);
    return it;
  }
  inline This& operator+=(const difference_type value) noexcept {
    it_ = fusion::transform(it_, Increment(value));
    return *this;
  }
  inline This operator--(int) noexcept {
    const auto it = *this;
    --(*this);
    return it;
  }
  inline This& operator-=(const difference_type value) noexcept {
    it_ = fusion::transform(it_, Decrement(value));
    return *this;
  }
  inline This& operator--()   noexcept {
    it_ = fusion::transform(it_, AdvBwd());
    return *this;
  }

  friend inline This operator-(This a, const difference_type value) noexcept
  { return a -= value; }
  friend inline This operator+(This a, const difference_type value) noexcept
  { return a += value; }

  friend inline
  difference_type operator-(const This l, const This r) noexcept {
    const auto result
      = fusion::at_c<0>(l.it_).second - fusion::at_c<0>(r.it_).second;

    assert(fusion::accumulate(fusion::zip(l.it_, r.it_), result,
                              check_distance()));
    return result;
  }
  ///@}

  /// \name Access operators
  ///@{
  inline reference operator* ()       noexcept { return ref_from_it(it_); }
  inline reference operator* () const noexcept { return ref_from_it(it_); }
  inline reference operator->()       noexcept { return *(*this);         }
  inline reference operator[](const difference_type v) noexcept
  { return *fusion::transform(it_, Increment(v)); }
  ///@}

 private:
  iterator_map it_;  ///< Tuple of iterators over column types

  /// \name Wrappers
  ///@{

  /// \brief Equality
  struct Eq { template<class T>   inline bool operator()(T&& i) const noexcept
  { return fusion::at_c<0>(i).second == fusion::at_c<1>(i).second; }};

  /// \brief Less Equal Than
  struct LEq { template<class T>  inline bool operator()(T&& i) const noexcept
  { return fusion::at_c<0>(i).second <= fusion::at_c<1>(i).second; }};

  /// \brief Greater Equal Than
  struct GEq { template<class T>  inline bool operator()(T&& i) const noexcept
  { return fusion::at_c<0>(i).second >= fusion::at_c<1>(i).second; }};

  /// \brief Advance forward
  struct AdvFwd { template<class T> inline auto operator()(T i) const noexcept
  { return i.second = ++(i.second); }};

  /// \brief Advance backward
  struct AdvBwd { template<class T> inline auto operator()(T i) const noexcept
  { return i.second = --(i.second); }};

  /// \brief Increment by value
  struct Increment {
    Increment(const difference_type value) noexcept : value_(value) {}
    template<class T>
    inline std::remove_reference_t<T> operator()(T&& i) const noexcept
    { return {i.second + value_}; }
    const difference_type value_;
  };

  /// \brief Decrent by value
  struct Decrement {
    Decrement(const difference_type value) : value_(value) {}
    template<class T>
    inline std::remove_reference_t<T> operator()(T&& i) const noexcept
    { return {i.second - value_}; }
    const difference_type value_;
  };

  struct check_distance {
    template<class T, class U>
    inline T operator()(const T acc, const U i) const noexcept {
      const auto tmp = fusion::at_c<0>(i).second
                       - fusion::at_c<1>(i).second;
      if (!(acc == tmp)) {
        std::cerr << "column types have different sizes!!\n";
        assert(acc == tmp);
      }
      return tmp;
    }
  };

  ///@}

  template<class U> struct val_to_ref {
    val_to_ref(U& v_) : v(v_) {}
    template<class T>
    inline auto operator()(const T&) const noexcept {
      using key = typename T::first_type;
      using value = typename T::second_type;
      return fusion::make_pair<key, std::add_lvalue_reference_t<value>>
          (fusion::at_key<key>(v));
    }
    U& v;
  };

  template<class U> struct it_to_ref {
    it_to_ref(U v_) : i(v_) {}
    template<class T>
    inline auto operator()(const T&) const noexcept {
      using key = typename T::first_type;
      using value
        = std::remove_reference_t<decltype(*typename T::second_type())>;
      return fusion::make_pair<key, std::add_lvalue_reference_t<value>>
          (*fusion::at_key<key>(i));
    }
    U i;
  };

  static inline reference_map ref_from_val(value_type& v)          noexcept
  { return fusion::transform(v, val_to_ref<value_type>{v}); }
  static inline reference_map ref_from_it(iterator_map& v)       noexcept
  { return fusion::transform(v, it_to_ref<iterator_map>{v}); }
  static inline reference_map ref_from_it(const iterator_map& v) noexcept
  { return fusion::transform(v, it_to_ref<iterator_map>{v}); }
};
} // SOA_ namespace

/// Struct of Arrays container:
/// (T, Container) -> (Container<T.member0>..Container<T.memberN>)
template<class T, template <class> class Container> struct StructOfArrays {

  /// T as a associative fusion map:
  using MemberMap = typename SOA_::as_fusion_map<T>::type;

  /// U -> Container<U>::iterator
  template<class U> using container_it = typename Container<U>::iterator;
  /// P -> P::key_type
  template<class P> struct key_of { using type = typename P::first_type; };
  /// P -> P::value_type
  template<class P> struct val_of { using type = typename P::second_type; };
  /// T -> (key_of<member0>..key_of<memberN>)
  using keys   = typename mpl::transform<MemberMap, key_of<mpl::_1>>::type;
  /// T -> (val_of<member0>..val_of<memberN>)
  using values = typename mpl::transform<MemberMap, val_of<mpl::_1>>::type;
  /// P<key, val> -> P<key, Container<val>>
  template<class P> struct container_of {
    using type = fusion::pair<typename P::first_type,
                              Container<typename P::second_type>>;
  };

  /// T -> (Container<T.member0>..Container<T.memberN>)
  using data = typename mpl::transform<MemberMap, container_of<mpl::_1>>::type;

  /// Container traits:
  using iterator = typename SOA_::iterator<values, keys, container_it>;
  using const_iterator = const typename SOA_::iterator<values, keys, container_it>;
  using value_type = typename iterator::value_type;
  using reference = typename iterator::reference;
  using const_reference = typename iterator::reference;
  using size_type = std::size_t;
  using difference_type = typename iterator::difference_type;
  using pointer = typename iterator::pointer;
  // const_pointer, reverse_iterator, const_reverse_iterator

  /// Initialization:
  explicit StructOfArrays(size_type n) { resize(n); }
  StructOfArrays(const StructOfArrays& other) : data_(other.data_) { }
  StructOfArrays(StructOfArrays&& other) : data_(std::move(other.data_)) { }
  // StructOfArrays(size_type count, const T& value,
  //                const Allocator& alloc = Allocator());
  // StructOfArrays(const vector& other, const Allocator& alloc );
  // StructOfArrays(StructOfArrays&& other, const Allocator& alloc );
  // StructOfArrays(std::initializer_list<T> init,
  //                const Allocator& alloc = Allocator() );
  // assign
  // get_allocator
  StructOfArrays& operator=(const StructOfArrays& other) { data_(other.data_); }
  StructOfArrays& operator=(StructOfArrays&& other) { data_(std::move(other.data_)); }
  // StructOfArrays& operator=(std::initializer_list<T> ilist) { }

  /// Iterators
 private:
  struct begin_iterators {
    template<class U> auto operator()(U& i) const {
      using key_type = typename U::first_type;
      using val_type = typename U::second_type;
      return fusion::make_pair<key_type>(const_cast<val_type&>(i.second).begin());
    }
  };

  struct end_iterators {
    template<class U> auto operator()(U& i) const {
      using key_type = typename U::first_type;
      using val_type = typename U::second_type;
      return fusion::make_pair<key_type>(const_cast<val_type&>(i.second).end());
    }
  };

 public:
  inline iterator begin()
  { return iterator{fusion::transform(data_, begin_iterators())}; }
  inline iterator end()
  { return iterator{fusion::transform(data_, end_iterators())}; }
  inline const_iterator begin() const
  { return iterator{fusion::transform(data_, begin_iterators())}; }
  inline const_iterator end()   const
  { return iterator{fusion::transform(data_, end_iterators())}; }
  // cbegin, cend, rbegin, rend...

  /// Element Access:
  // at
  inline reference operator[](size_type pos) { return begin()[pos]; }
  inline const_reference operator[](size_type pos) const { return begin()[pos]; }
  // front
  // back
  // data


  /// Capacity:
  struct empty_ { template<class U>
  bool operator()(const U& i) const { return i.second.empty(); }
  };
  inline bool empty() const { return fusion::all(data_, empty_()); }
  size_type capacity() const {
    auto cap = fusion::at_c<0>(data_).second.capacity();
    // assert(fusion::accumulate...)
    return cap;
  }
  size_type size() const {
    auto size_ = fusion::at_c<0>(data_).second.size();
    // assert(fusion::accumulate...)
    return size_;
  }
  // reserve
  struct reserve_ {
    reserve_(std::size_t n) : n_(n) {}
    template<class U>  void operator()(U&& i) const { i.second.reserve(n_);  }
    const std::size_t n_;
  };
  void reserve(std::size_t n) { fusion::for_each(data_, reserve_(n)); }
  // shrink_to_fit
  // max_size

  /// Modifiers
  // clear
  // insert
  // emplace
  // erase
  // emplace_back
  // push_back
  struct push_back_obj_ {
    push_back_obj_(data& container_data) : data_(container_data) {}
    template<class U> void operator()(U&& i) const {
      using key = typename std::remove_reference_t<decltype(fusion::at_c<0>(i))>::first_type;
      fusion::at_key<key>(data_).push_back(fusion::at_c<1>(i));
    }
    data& data_;
  };
  void push_back(const T& value) {
    fusion::for_each(zip(MemberMap{}, value), push_back_obj_(data_));
  };
  // void push_back(T&& value) { };
  struct push_back_ {
    push_back_(data& container_data) : data_(container_data) {}
    template<class U> void operator()(U&& i) const {
      using key = typename std::remove_reference_t<U>::first_type;
      fusion::at_key<key>(data_).push_back(i.second);
    }
    data& data_;
  };
  void push_back(const value_type& value) {
    fusion::for_each(value, push_back_(data_));
  };
  // pop_back
  struct resize_ {
    resize_(std::size_t n) : n_(n) {}
    template<class U>  void operator()(U&& i) const { i.second.resize(n_); }
    const std::size_t n_;
  };
  void resize(std::size_t n) { fusion::for_each(data_, resize_(n)); }
  // swap

  // private:
  data data_;
};

// relational operators
// std::swap

////////////////////////////////////////////////////////////////////////////////
#endif
