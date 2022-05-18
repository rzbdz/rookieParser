#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <regex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
using namespace std;
template <typename T>
concept _StringArg = is_constructible_v<string, T>;
struct token {
  string expr;
  operator string() { return expr; }
  template <_StringArg _T> token(_T &&char_tr) : expr{char_tr} {}
  token(int i) : expr{to_string(i)} {}
};
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
  typedef std::vector<token> tokens_t;

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
    map[t].insert(map[t].end(), s.begin(), s.end());
  }

  const auto &produce(anything *from) { return prods[from]; }
  string random_produce(int aprox_len = 20) {
    srand(time(0));
    string result = "";
    auto ti = starts.begin();
    advance(ti, rand() % starts.size());
    anything *cur = *(ti);
    deque<anything *> q;
    q.push_back(cur);
    while (!q.empty()) {
      // cout << result << endl;
      cur = q.front();
      q.pop_front();
      if (cur->isTerminal()) {
        result =
            result + static_cast<string>(map[cur][rand() % map[cur].size()]);
      } else {
        const vector<vector<anything *>> &prds = produce(cur);
        const vector<anything *> *rd = nullptr;
        if (result.size() > aprox_len / 2) {
          for (auto &prd : prds) {
            if (prd[0]->str == "num") {
              rd = &prd;
              break;
            }
          }
        }
        if (!rd) {
          rd = &prds[rand() % prds.size()];
        }
        for (auto r = rd->crbegin(); r != rd->crend(); r++) {
          q.push_front(*r);
        }
      }
    }
    return result;
  }
};
struct pool {
  unordered_map<string, unique_ptr<terminal>> ts{};
  unordered_map<string, unique_ptr<non_terminal>> nts{};
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

grammar boolshit;
pool mypool;
anything *helper(string name, vector<token> ss) {
  auto re = mypool.new_terminal(name);
  boolshit.addTerminal(re, ss);
  return re;
}
anything *helper(string name) {
  auto re = mypool.new_nonterminal(name);
  return re;
}
auto produce(auto &&a) { return boolshit.allow_produce(a); }

#define def_terminal(name, ...) auto name = helper(#name, {__VA_ARGS__})
#define def_non_terminal(name) auto name = helper(#name)
int main(void) {
  // terminals:
  def_terminal(num, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
  def_terminal(lowOp, "+", "-");
  def_terminal(minus, "-");
  def_terminal(highOp, "*", "/");
  def_terminal(lbra, "(");
  def_terminal(rbra, ")");
  // non-terminals:
  def_non_terminal(expr);
  def_non_terminal(term);
  def_non_terminal(unary);
  def_non_terminal(factor);
  // productions:
  typedef vector<anything *> p;
  produce(expr) >> p{expr, lowOp, term} | p{term};
  produce(term) >> p{term, highOp, unary} | p{unary};
  produce(unary) >> p{lbra, lowOp, factor, rbra} | p{unary} | p{factor};
  produce(factor) >> p{num} | p{lbra, expr, rbra};
  boolshit.addStart(expr);
  auto s = boolshit.random_produce(10);
  cout << s << endl;
  return 0;
}