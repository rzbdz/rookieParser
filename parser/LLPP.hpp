// simple for express parser,
// `Compiler` book, 2.5.2
#include <iostream>
#include <random>
#include <regex>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

#ifndef NDEBUG
inline auto &prepend(int j) {
  for (int i = 0; i < j; i++) {
    cout << "-";
  }
  return cout;
}
#else
struct null_ {
  template <typename Anything>
  friend null_ &operator<<(null_ &o, Anything &&an) {
    return o;
  }
};
inline null_ nnnn_;
inline auto &prepend(int j) { return nnnn_; }
#endif

template <typename T>
concept _StringArg = is_constructible_v<string, T>;
struct token {
  string expr;
  operator string_view() const { return expr; }
  operator string() const { return expr; }
  template <_StringArg _T> token(_T &&char_tr) : expr{char_tr} {}
  token(int i) : expr{to_string(i)} {}
  std::strong_ordering operator<=>(const token &) const = default;
};

namespace std {
template <> struct hash<token> {
  size_t operator()(token e) const { return hash<string>()(e.expr); }
};
} // namespace std

struct something {
  string str;
  virtual bool isTerminal() = 0;
  virtual bool isScheme() { return false; }
  template <_StringArg _T> something(_T &&char_tr) : str{char_tr} {}
  // for translation scheme or construct an AST.
  virtual void exec(span<token> t) = 0;
};

struct terminal : something {
  template <_StringArg _T> terminal(_T &&char_tr) : something{char_tr} {}
  bool isTerminal() { return true; }
  virtual void exec(span<token> t) {}
};

struct non_terminal : something {
  template <_StringArg _T> non_terminal(_T &&char_tr) : something{char_tr} {}
  bool isTerminal() { return false; }
  virtual void exec(span<token> t) {}
};

struct scheme : something {
  template <_StringArg _T> scheme(_T &&char_tr) : something{char_tr} {}
  bool isTerminal() { return false; }
  virtual bool isScheme() { return true; }
  virtual void exec(span<token>) = 0;
};

class grammar {
private:
  template <typename K, typename V> using map_t = std::unordered_map<K, V>;
  template <typename V> using set_t = std::unordered_set<V>;
  typedef set_t<token> tokens_t;

  // terminals
  map_t<something *, tokens_t> map{};
  set_t<something *> starts{};
  map_t<something *, vector<vector<something *>>> prods{};

public:
  void addStart(something *s) { starts.emplace(s); }

  template <typename _ArrSth> void addProduction(something *s, _ArrSth a) {
    prods[s].emplace_back(a);
  }

  struct pipe {
    grammar *g;
    something *t;
    friend const pipe &operator|(const pipe &p, vector<something *> a) {
      p.g->addProduction(p.t, a);
      return p;
    }
    friend const pipe &operator>>(const pipe &p, vector<something *> a) {
      p.g->addProduction(p.t, a);
      return p;
    }
  };

  pipe allowProduce(something *s) { return {this, s}; }

  template <typename _ArrToken>
  constexpr void addTerminal(something *t, _ArrToken s) {
    for (auto si : s)
      map[t].emplace(si);
  }

  bool tokenInTerminal(something *ter, token lookahead) {
    if (map[ter].contains(lookahead)) {
      return true;
    } else {
      return false;
    }
  }

  const auto &produce(something *from) { return prods[from]; }

private:
  span<token> parseHelper(span<token> subview, something *from, int pre) {
    if (subview.empty()) {
      prepend(pre - 1) << "inside " << from->str << " nil sequence"
                       << "\n";
    } else {
      prepend(pre - 1) << "inside " << from->str << " find `"
                       << (string)subview[0] << "`"
                       << "\n";
    }
    decltype(auto) prods = produce(from);
    token lookahead = !subview.empty() ? subview[0] : "";
    decltype(auto) go_next = [&]() -> decltype(auto) {
      for (auto &p : prods) {
        prepend(pre) << "scan anything: " << p[0]->str << "\n";
        if (p[0]->isTerminal()) {
          if (p[0]->str == "eps" || tokenInTerminal(p[0], lookahead)) {
            // epsilon
            prepend(pre) << "matched: " << p[0]->str << " with "
                         << (string)lookahead << "\n";
            return p;
          }
        };
      }
      throw "syntax error: cannot recognize `" + lookahead.expr + "`";
    }();
    decltype(subview) info = subview; // keep this level of production as info
    for (auto cur : go_next) {
      cur->exec(info);
      if (cur->isScheme() || cur->str == "eps") {
      } else if (cur->isTerminal()) {
        if (subview.size() == 0) {
          throw string{"expect token to match " + cur->str};
        }
        subview = subview.subspan(1, subview.size() - 1);
      } else {
        subview = parseHelper(subview, cur, pre + 2);
      }
    }
    return subview;
  }

public:
  span<token> parse(span<token> view) {
    for (auto s : starts) {
      return parseHelper(view, s, 0);
    }
    throw "syntax error: nothing canbe started from " + view[0].expr;
  }
};

struct pool {
  unordered_map<string, unique_ptr<terminal>> ts{};
  unordered_map<string, unique_ptr<non_terminal>> nts{};
  static auto &getInstance() {
    static pool p;
    return p;
  }
  template <typename T> terminal *makeTerminal(string name) {
    return (ts[name] = make_unique<T>(std::move(name))).get();
  }
  non_terminal *makeNonterminal(string name) {
    return (nts[name] = make_unique<non_terminal>(std::move(name))).get();
  }
  something *operator[](const char *s) {
    if (ts.contains(s)) {
      return ts[s].get();
    } else {
      return nts[s].get();
    }
  }
};

template <typename langImpl> struct language {
  grammar g;
  pool p;
  template <typename T> something *buildHelper(string name, vector<token> ss) {
    auto re = p.makeTerminal<T>(name);
    g.addTerminal(re, ss);
    return re;
  }
  something *buildHelper(string name) {
    auto re = p.makeNonterminal(name);
    return re;
  }
  auto produce(auto &&a) { return g.allowProduce(a); }

  language() { static_cast<langImpl *>(this)->build(); }
  auto tryParse(span<token> series) { return g.parse(series); }
};

#define def_terminal(name, ...)                                                \
  auto name = buildHelper<terminal>(#name, {__VA_ARGS__})
#define def_other_terminal(name, type, ...)                                    \
  auto name = buildHelper<type>(#name, {__VA_ARGS__})
#define def_non_terminal(name) auto name = buildHelper(#name)