#define NDEBUG
#include "./LLPP.hpp"
void Pexec(span<token> ts, int i) { cout << ts[0].expr << " "; }
void Texec(span<token> ts, int i) { cout << ts[i].expr << " "; }
struct inffix_to_suffix : language<inffix_to_suffix> {
  void build() {
    def_terminal_with_f(term, Texec, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    def_terminal(op, "+", "-");
    def_terminal(eps, "");
    // non-terminals:
    def_non_terminal(expr);
    def_non_terminal(rest);
    auto ptr = getPool().makeScheme("ptr", Pexec);
    // productions:
    typedef vector<something *> p;
    produce(expr) >> p{term, rest};
    produce(rest) >> p{op, term, ptr, rest} | p{eps};
    g.addStart(expr);
  }
};

int main() {
  inffix_to_suffix parser;
  vector<string> ss = {
      "1 + 2 - 3 + 4 ", // valid
      "1 + 2 - ",       // invalid
      "- 8",            // invalid
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
      c = parser.tryParse(test);
    } catch (string &e) {
      msg = e;
    }
    cout << endl;
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
  return 0;
}