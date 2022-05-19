#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
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

#define NDEBUG
#ifndef NDEBUG
auto &prepend(int j) {
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
null_ nnnn_;
auto &prepend(int j) { return nnnn_; }
#endif

template <typename T>
concept _StringArg = is_constructible_v<string, T>;
struct token {
  string expr;
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
struct anything {
  string str;
  virtual bool isTerminal() = 0;
  template <_StringArg _T> anything(_T &&char_tr) : str{char_tr} {}
};
struct terminal : anything {
  template <_StringArg _T> terminal(_T &&char_tr) : anything{char_tr} {}
  bool isTerminal() { return true; }
};
struct non_terminal : anything {
  template <_StringArg _T> non_terminal(_T &&char_tr) : anything{char_tr} {}
  bool isTerminal() { return false; }
};
struct grammar {

  template <typename K, typename V> using map_t = std::unordered_map<K, V>;
  template <typename V> using set_t = std::unordered_set<V>;
  typedef set_t<token> tokens_t;

  // terminals
  map_t<anything *, tokens_t> map{};
  set_t<anything *> starts{};
  map_t<anything *, vector<vector<anything *>>> prods{};

  void addStart(anything *s) { starts.emplace(s); }

  void addProduction(anything *s, vector<anything *> a) {
    prods[s].emplace_back(a);
  }

  struct pipe {
    grammar *g;
    anything *t;
    friend const pipe &operator|(const pipe &p, vector<anything *> a) {
      p.g->addProduction(p.t, a);
      return p;
    }
    friend const pipe &operator>>(const pipe &p, vector<anything *> a) {
      p.g->addProduction(p.t, a);
      return p;
    }
  };

  pipe allow_produce(anything *s) { return {this, s}; }

  void addTerminal(anything *t, vector<token> s) {
    for (auto si : s)
      map[t].emplace(si);
  }

  bool token_in_terminal(anything *ter, token lookahead) {
    if (map[ter].contains(lookahead)) {
      return true;
    } else {
      return false;
    }
  }

  const auto &produce(anything *from) { return prods[from]; }

  span<token> parsing(span<token> subview, anything *from, int pre) {
    prepend(pre - 1) << "inside " << from->str << " find `"
                     << (string)subview[0] << "`" << subview.size() << "\n";
    decltype(auto) prods = produce(from);
    token lookahead = subview[0];
    decltype(auto) go_next = [&]() -> decltype(auto) {
      for (auto &p : prods) {
        prepend(pre) << "scan anything: " << p[0]->str << "\n";
        if (p[0]->isTerminal()) {
          if (token_in_terminal(p[0], lookahead) || p[0]->str == "eps") {
            // epsilon
            prepend(pre) << "matched: " << p[0]->str << " with "
                         << (string)lookahead << "\n";
            return p;
          }
        };
      }
      throw "syntax error: cannot recognize `" + lookahead.expr + "`";
    }();
    for (auto cur : go_next) {
      if (cur->isTerminal() && cur->str == "eps") {
      } else if (cur->isTerminal()) {
        if (subview.size() == 0) {
          throw string{"expect more tokens to match " + cur->str};
        }
        subview = subview.subspan(1, subview.size() - 1);
      } else {
        subview = parsing(subview, cur, pre + 2);
      }
    }
    return subview;
  }
  span<token> parse(span<token> view) {
    for (auto s : starts) {
      return parsing(view, s, 0);
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
  terminal *new_terminal(string name) {
    return (ts[name] = make_unique<terminal>(std::move(name))).get();
  }
  non_terminal *new_nonterminal(string name) {
    return (nts[name] = make_unique<non_terminal>(std::move(name))).get();
  }
  anything *operator[](const char *s) {
    if (ts.contains(s)) {
      return ts[s].get();
    } else {
      return nts[s].get();
    }
  }
};

#define def_terminal(name, ...) auto name = helper(#name, {__VA_ARGS__})
#define def_non_terminal(name) auto name = helper(#name)

struct language {
  grammar g;
  pool p;
  anything *helper(string name, vector<token> ss) {
    auto re = p.new_terminal(name);
    g.addTerminal(re, ss);
    return re;
  }
  anything *helper(string name) {
    auto re = p.new_nonterminal(name);
    return re;
  }
  auto produce(auto &&a) { return g.allow_produce(a); }

  language() {
    // terminals:
    def_terminal(expr, "expr");
    def_terminal(ifw, "if");
    def_terminal(lbr, "(");
    def_terminal(rbr, ")");
    def_terminal(comma, ";");
    def_terminal(forw, "for");
    def_terminal(eps, "");
    // non-terminals:
    def_non_terminal(stmt);
    def_non_terminal(optexpr);
    // productions:
    typedef vector<anything *> p;
    produce(stmt) >> p{expr, comma}    //
        | p{ifw, lbr, expr, rbr, stmt} //
        | p{forw, lbr, optexpr, comma, optexpr, comma, optexpr, rbr, stmt};
    produce(optexpr) >> p{expr} | p{eps};
    g.addStart(stmt);
  }
  auto try_parse(span<token> series) { return g.parse(series); }
};

int main() {
  language calc;
  vector<string> ss = {
      "for ( ; expr ; expr ) for ( expr ; expr ; expr ) if ( expr ) expr ; ",
      "for ( ; expr ; expr ) for ( expr ; expr ; ) if ( expr ) ; ",
      //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^expect expr
      "for ( ; expr ; ; ) for ( expr ; expr ; ) if ( expr ) ; ",
      //^^^^^^^^^^^^^^^expect right bracket
      "for ( ; expr  expr ) for ( expr ; expr ; expr ) if ( expr ) expr ; ",
      //^^^^^^^^^^^^expect comma
      "for ( ; expr ; expr ) for ( expr ; expr ; expr ) if ( expr ) expr ",
      //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^expect comma
      "for (",
      //^^^^
  };
  for (auto str : ss) {
    int last = 0;
    vector<token> test = {};
    for (int i = 0; i < str.size(); i++) {
      if (str[i] == ' ') {
        auto s = str.substr(last, i - last);
        prepend(0) << s << "\n";
        test.push_back(s);
        last = i + 1;
      }
    }
    string msg;
    span<token> c;
    try {
      c = calc.try_parse(test);
    } catch (string &e) {
      msg = e;
    }
    if (c.size() == 0 && msg.empty())
      cout << "`" << str << "` is valid" << endl;
    else {
      cout << "`" << str << "` isn't valid\n┕────";
      for (const auto &r : c) {
        cout << "`" << (string)r << "` ";
      }
      cout << " remains not recognized: " + msg << endl;
    }
  }
}