// simple for express parser,
// `Compiler` book, the chaper 2.4.2: predictive parsing

#define NDEBUG
#include "./LLPP.hpp"

struct for_if : language<for_if> {
  void build() {
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
    typedef vector<something *> p;
    produce(stmt) >> p{expr, comma}    //
        | p{ifw, lbr, expr, rbr, stmt} //
        | p{forw, lbr, optexpr, comma, optexpr, comma, optexpr, rbr, stmt};
    produce(optexpr) >> p{expr} | p{eps};
    g.addStart(stmt);
  }
  auto try_parse(span<token> series) { return g.parse(series); }
};

int main() {
  for_if calc;
  vector<string> ss = {
      "for ( ; expr ; expr ) for ( expr ; expr ; expr ) if ( expr ) expr ; ",
      "for ( ; expr ; expr ) for ( expr ; expr ; ) if ( expr ) ; ",
      //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^expect expr
      "for ( ; expr ; ; ) for ( expr ; expr ; ) if ( expr ) ; ",
      //^^^^^^^^^^^^^^^expect right bracket
      "for ( ; expr  expr ) for ( expr ; expr ; expr ) if ( expr ) expr ; ",
      //^^^^^^^^^^^^expect comma
      "for ( ; expr ; expr ) for ( expr ; expr ; expr ) if ( expr ) expr ",
      //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^expect
      // comma
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
