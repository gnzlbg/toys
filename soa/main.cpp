#include "soa_container.hpp"

// Step-by-step:

// Define a Struct with an extra tag (keys) per data member
struct Struct {
  float  x;
  double y;
  int    i;
  bool   b;
  struct keys { struct x {}; struct y {}; struct i {}; struct b {}; };
};
using k = Struct::keys;  // Lets import the keys, we'll need them often.

// This adapts the struct as an associative fusion sequence
BOOST_FUSION_ADAPT_ASSOC_STRUCT(
    Struct,
    (float,  x, k::x)
    (double, y, k::y)
    (int,    i, k::i)
    (bool,   b, k::b)
)

/// Here we choose the underlying container and get our SOAVector
template<class T> using vector = boost::container::vector<T, std::allocator<T>>;
template<class O> using SOAVector = StructOfArrays<O, vector>;

int main() {
  using namespace fusion;

  /// pretty print:
  auto print = [](auto& o) {
    for (auto i : o) {
      std::cout << "("  << at_key<k::x>(i)  << ", " << at_key<k::y>(i)
                << ", " << at_key<k::i>(i)  << ", " << at_key<k::b>(i) << ")\n";
    }
  };

  /// We can make a vector from our Struct type
  SOAVector<Struct> soa(10);

  /// Iterators/references/value_types are fusion::maps
  int count = 0;
  for (auto i : soa) {
    /// That is, i is a tuple of references, we access each member by key
    at_key<k::x>(i) = static_cast<float>(count);
    at_key<k::y>(i) = static_cast<double>(count);
    at_key<k::i>(i) = count;
    at_key<k::b>(i) = count % 2 == 0;
    ++count;
  }

  std::cout << "Initial values:\n";
  print(soa);

  /// Sort:
  boost::stable_sort(soa, [](auto i, auto j) {
      return at_key<k::x>(i) > at_key<k::x>(j);
  });

  std::cout << "After stable_sort:\n";
  print(soa);

  boost::transform(soa, std::begin(soa), [](auto i) {
      /// To create value_types, we need to pass key value pairs:
      /// (this can be simplified with a ::make_value function(...))
      return typename SOAVector<Struct>::value_type {
        make_pair<k::x>(at_key<k::x>(i) * at_key<k::x>(i)),
        make_pair<k::y>(at_key<k::y>(i) * at_key<k::y>(i)),
        make_pair<k::i>(at_key<k::i>(i) * at_key<k::i>(i)),
        make_pair<k::b>(at_key<k::b>(i) * at_key<k::b>(i)),
      };
  });

  std::cout << "After transform:\n";
  print(soa);

  auto val = typename SOAVector<Struct>::value_type {
    make_pair<k::x>(static_cast<float>(1.0)),
    make_pair<k::y>(static_cast<double>(2.0)),
    make_pair<k::i>(static_cast<int>(3)),
    make_pair<k::b>(true),
  };

  soa.push_back(val);
  std::cout << "After push_back of (1., 2., 3. true):\n";
  print(soa);

  Struct tmp;
  tmp.x = 4.0;
  tmp.y = 3.0;
  tmp.i = 2.0;
  tmp.b = false;

  soa.push_back(tmp);
  std::cout << "After push_back of Struct (4., 3., 2. false):\n";
  print(soa);
}
