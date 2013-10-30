#include "soa_container.hpp"

struct Object {
  float  x;
  double y;
  int    i;
  bool   b;
  struct keys { struct x {}; struct y {}; struct i {}; struct b {}; };
};
using k = Object::keys;

BOOST_FUSION_ADAPT_ASSOC_STRUCT(
    Object,
    (float,  x, k::x)
    (double, y, k::y)
    (int,    i, k::i)
    (bool,   b, k::b)
)

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

  SOAVector<Object> soa(10);

  // Initialize:
  int count = 0;
  for (auto i : soa) {
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
      return typename SOAVector<Object>::value_type {
        make_pair<k::x>(at_key<k::x>(i) * at_key<k::x>(i)),
        make_pair<k::y>(at_key<k::y>(i) * at_key<k::y>(i)),
        make_pair<k::i>(at_key<k::i>(i) * at_key<k::i>(i)),
        make_pair<k::b>(at_key<k::b>(i) * at_key<k::b>(i)),
            };
  });

  std::cout << "After transform:\n";
  print(soa);

  auto val = typename SOAVector<Object>::value_type {
    make_pair<k::x>(static_cast<float>(1.0)),
    make_pair<k::y>(static_cast<double>(2.0)),
    make_pair<k::i>(static_cast<int>(3)),
    make_pair<k::b>(true),
  };

  soa.push_back(val);
  std::cout << "After push_back of (1., 2., 3. true):\n";
  print(soa);

  Object tmp;
  tmp.x = 4.0;
  tmp.y = 3.0;
  tmp.i = 2.0;
  tmp.b = false;

  soa.push_back(tmp);
  std::cout << "After push_back of Object (4., 3., 2. false):\n";
  print(soa);
}
