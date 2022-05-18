
#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <set>
#define EPSILON '0'
#define START 0
using namespace std;
class Solution {
 public:
  auto AddPossibleNext(
      unordered_map<long, unordered_map<char, set<long>>>& StateTable,
      set<long>& NextStates,
      long OneOfCurrents,
      char Input) -> void {
    if (StateTable[OneOfCurrents].count(Input)) {
      auto possible_states = StateTable[OneOfCurrents][Input];
      for (auto possible_state : possible_states) {
        NextStates.insert(possible_state);
        // DFS + backtrack (insert into set)
        AddPossibleNext(StateTable, NextStates, possible_state, EPSILON);
      }
    }
  }

  bool isMatch(string s, string p) {
    unordered_map<long, unordered_map<char, set<long>>> StateTable;
    // create the first state, as a begining
    StateTable.emplace(START, unordered_map<char, set<long>>{});
    long lastState = 0;
    for (long i = 0; i < p.size(); ++i) {
      if (i + 1 < p.size() && p[i + 1] == '*') {
        // create a new state
        // add a cycle edge to the kleene-star state
        StateTable[i + 1].insert({p[i], {i + 1}});
        // update the last state with a epsilon-edge,
        // if there are chained kleene-stars,
        // just think as chained epsilon-edges...
        if (StateTable[lastState].count(EPSILON) == 0) {
          StateTable[lastState].insert({EPSILON, {i + 1}});
        } else {
          StateTable[lastState][EPSILON].insert(i + 1);
        }
        lastState = i + 1;
      } else if (p[i] != '*') {  // we just skip the star...
        // nornal matching request, we just add a state, and update last state.
        StateTable.emplace(i + 1, unordered_map<char, set<long>>{});
        // add an edge to this state
        if (StateTable[lastState].count(p[i]) == 0) {
          StateTable[lastState].insert({p[i], {i + 1}});
        } else {
          StateTable[lastState][p[i]].insert(i + 1);
        }
        lastState = i + 1;
      }
    }
    StateTable[lastState].insert({EPSILON, {static_cast<long>(p.size()) + 1}});
    // Only
    long AcceptableState = p.size() + 1;
    set<long> CurrentPossibleStates, NextPossibleStates;
    // Start by the START state
    CurrentPossibleStates.insert(START);
    // explore possible epsilon chains
    AddPossibleNext(StateTable, CurrentPossibleStates, START, EPSILON);
    // foreach char in str as an input to move
    for (long i = 0; i < s.size(); i++) {
      NextPossibleStates.clear();
      // foreach possible states, do the move
      for (auto one_of_currents : CurrentPossibleStates) {
        AddPossibleNext(StateTable, NextPossibleStates, one_of_currents, s[i]);
        AddPossibleNext(StateTable, NextPossibleStates, one_of_currents, '.');
      }
      CurrentPossibleStates = NextPossibleStates;
    }
    NextPossibleStates.clear();
    // try to skip any epsilon chain
    for (auto one_of_currents : CurrentPossibleStates) {
      AddPossibleNext(StateTable, NextPossibleStates, one_of_currents, EPSILON);
    }
    CurrentPossibleStates = NextPossibleStates;
    for (auto possible_end : CurrentPossibleStates) {
      if (possible_end == AcceptableState) {
        return true;
      }
    }
    return false;
  }
};

int main(void) {
  Solution sl;
  for (;;) {
    string s, p;
    cout << "str, pattern: " << endl;
    cin >> s >> p;
    cout << sl.isMatch(s, p) << endl;
  }
  return 0;
}