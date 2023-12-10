#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <stack>

enum Action{
  SHIFT,
  REDUCE,
  ACCEPT,
  ERROR
};

struct TableAction {
  Action action;
  int index;
  char start_symbol;
};

struct Situation {
  char non_terminal;
  std::string rule;
  int dot_pos;
};

class Grammar;

class LR1_algorithm {
public:
  void MakeTable(const Grammar &grammar);
  bool CheckForAccess(std::string str_find, const std::set<char>& alphabet);
private:
  std::vector<std::map<char, TableAction> > shift_reduce_table_;
  std::vector<std::map<char, int> > go_to_;
  std::vector<std::set<Situation> > algo_states_;
  std::map<char, std::vector<std::string>> grammar_;
  std::set<char> alphabet_;
  const char fake_start = '#';
  const char EndPtr = '$';

  void Shift(int cur_state, char cur_symbol, std::stack<int>& stack_states);
  void Reduce(int cur_state, char cur_symbol, std::stack<int>& stack_states);
  void SetTable(const Grammar& grammar);
  int FindRuleNumber(const Situation &situation);
  std::set<Situation> Closure(std::set<Situation>& state_to_close);
  int GoTo(std::set<Situation>& exit_state, char symbol, std::vector<std::set<Situation> >& new_states, bool insert);
  void BuildGrammarStates(const Grammar &grammar);
  bool IsInAlphabet(char symbol, const std::set<char>& alphabet);
};

class Grammar {
public:
  friend class LR1_algorithm;
  friend bool LR1_alg(const std::vector<std::string>& rules, const std::string& str_to_find);
  Grammar() = default;
  explicit Grammar(std::vector<std::string> rules);

private:
  std::map<char, std::vector<std::string>> grammar_;
  std::set<char> alphabet_terminal_;
  std::set<char> alphabet_non_terminal_;
};

//

bool IsNonTerminal(char symbol) { return symbol >= 'A' && symbol <= 'Z'; }

bool IsTerminal(char symbol) { return symbol >= 'a' && symbol <= 'z'; }

bool operator<(const Situation& first, const Situation& second) {
  if (first.non_terminal == second.non_terminal) {
    return first.rule < second.rule ||
           first.rule == second.rule && first.dot_pos < second.dot_pos;
  }
  return first.non_terminal < second.non_terminal;
}

bool operator==(const Situation& first, const Situation& second) {
  return first.non_terminal == second.non_terminal &&
         first.rule == second.rule && first.dot_pos == second.dot_pos;
}

Grammar::Grammar(std::vector<std::string> rules) {
  for (auto& rule : rules) {
    for (size_t i = 0; i < rule.size(); ++i) {
      if (rule[i] == '>') {
        grammar_[rule[0]].push_back(rule.substr(i + 1, rule.size()));
      }
      if (IsNonTerminal(rule[i])) {
        alphabet_non_terminal_.insert(rule[i]);
      }
      if (IsTerminal(rule[i])) {
        alphabet_terminal_.insert(rule[i]);
      }
    }
  }
}

bool LR1_alg(const std::vector<std::string>& rules, const std::string& str_to_find) {
  Grammar grammar(rules);
  LR1_algorithm algorithm;
  algorithm.MakeTable(grammar);
  return algorithm.CheckForAccess(str_to_find, grammar.alphabet_terminal_);
}

void LR1_algorithm::Reduce(int cur_state, char cur_symbol, std::stack<int>& stack_states) {
  std::string rule =
          grammar_[shift_reduce_table_[cur_state][cur_symbol].start_symbol]
          [shift_reduce_table_[cur_state][cur_symbol].index];
  size_t rule_len = rule.size();
  while (rule_len--) {
    stack_states.pop();
  }
  int state_after_reduce = stack_states.top();
  stack_states.push(
          go_to_[state_after_reduce]
          [shift_reduce_table_[cur_state][cur_symbol].start_symbol]);
}

void LR1_algorithm::Shift(int cur_state, char cur_symbol, std::stack<int>& stack_states) {
  stack_states.push(shift_reduce_table_[cur_state][cur_symbol].index);
}

bool LR1_algorithm::CheckForAccess(std::string str_find, const std::set<char>& alphabet) {
  str_find += EndPtr;
  int index = 0;
  std::stack<int> stack_states;
  stack_states.push(0);
  while (index < str_find.size()) {
    int cur_state = stack_states.top();
    char cur_symbol = str_find[index];
    if (!IsInAlphabet(cur_symbol, alphabet)) return false;

    if (shift_reduce_table_[cur_state][cur_symbol].action == SHIFT) {
      Shift(cur_state, cur_symbol, stack_states);
      ++index;
    } else if (shift_reduce_table_[cur_state][cur_symbol].action == REDUCE) {
      Reduce(cur_state, cur_symbol, stack_states);
    } else if (shift_reduce_table_[cur_state][cur_symbol].action == ACCEPT &&
               index == str_find.size() - 1) {
      return true;
    } else if (shift_reduce_table_[cur_state][cur_symbol].action == ERROR) {
      return false;
    }
  }
  return false;
}

void LR1_algorithm::MakeTable(const Grammar& grammar) {
  BuildGrammarStates(grammar);
  SetTable(grammar);
  for (int i = 0; i < shift_reduce_table_.size(); ++i) {
    for (auto& situation : algo_states_[i]) {
      if (situation.dot_pos == situation.rule.size()) {
        if (situation.non_terminal != fake_start) {
          int rule_number = FindRuleNumber(situation);
          for (auto& alph_symbol : grammar.alphabet_terminal_) {
            shift_reduce_table_[i][alph_symbol] = {REDUCE, rule_number,
                                                   situation.non_terminal};
          }
          shift_reduce_table_[i][EndPtr] = {REDUCE, rule_number,
                                            situation.non_terminal};
        } else {
          shift_reduce_table_[i][EndPtr] = {ACCEPT};
        }
      } else {
        char symbol_on_dot = situation.rule[situation.dot_pos];
        int number_of_state =
                GoTo(algo_states_[i], symbol_on_dot, algo_states_, false);
        if (IsTerminal(symbol_on_dot)) {
          shift_reduce_table_[i][symbol_on_dot] = {SHIFT, number_of_state};
        }
        if (IsNonTerminal(symbol_on_dot)) {
          go_to_[i][symbol_on_dot] = number_of_state;
        }
      }
    }
  }
}

int LR1_algorithm::GoTo(std::set<Situation>& exit_state, char symbol,
                        std::vector<std::set<Situation> >& new_states,
                        bool insert = true) {
  std::set<Situation> new_state;
  for (auto& situations : exit_state) {
    if (situations.dot_pos < situations.rule.size() &&
        situations.rule[situations.dot_pos] == symbol) {
      Situation new_rule = {situations.non_terminal, situations.rule,
                            situations.dot_pos + 1};
      new_state.insert(new_rule);
    }
  }
  if (!new_state.empty()) {
    new_state = Closure(new_state);
    for (int i = 0; i < algo_states_.size(); ++i) {
      if (algo_states_[i] == new_state) return i;
    }
    if (insert) new_states.push_back(new_state);
    return -1;
  }
}

std::set<Situation> LR1_algorithm::Closure(std::set<Situation>& state_to_close) {
  std::set<Situation> state(state_to_close);
  bool non_in_state;
  do {
    non_in_state = false;
    for (auto& situation : state) {
      int dot_pos = situation.dot_pos;
      if (dot_pos < situation.rule.size()) {
        char symbol_for_reveal = situation.rule[dot_pos];
        if (IsNonTerminal(symbol_for_reveal)) {
          for (auto& rule : grammar_[symbol_for_reveal]) {
            Situation new_situation = {symbol_for_reveal, rule, 0};
            if (!state.insert(new_situation).second) non_in_state = true;
          }
        }
      }
    }
  } while (non_in_state);
  return state;
}

void LR1_algorithm::BuildGrammarStates(const Grammar& grammar) {
  grammar_ = grammar.grammar_;
  std::set<Situation> start_rule;
  start_rule.insert(Situation{fake_start, "S", 0});
  algo_states_.push_back(Closure(start_rule));
  while (true) {
    std::vector<std::set<Situation> > new_states;
    for (auto& state : algo_states_) {
      for (auto& symbol : grammar.alphabet_non_terminal_) {
        GoTo(state, symbol, new_states);
      }
      for (auto& symbol : grammar.alphabet_terminal_) {
        GoTo(state, symbol, new_states);
      }
    }
    if (new_states.empty()) break;
    for (auto& state : new_states) {
      algo_states_.push_back(state);
    }
  }
}

bool LR1_algorithm::IsInAlphabet(char symbol, const std::set<char>& alphabet) {
  return symbol == EndPtr || alphabet.find(symbol) != alphabet.end();
}

void LR1_algorithm::SetTable(const Grammar& grammar) {
  shift_reduce_table_.resize(algo_states_.size());
  go_to_.resize(algo_states_.size());
  for (int i = 0; i < shift_reduce_table_.size(); ++i) {
    for (auto& symbol : grammar.alphabet_terminal_) {
      shift_reduce_table_[i][symbol] = {ERROR};
    }
    shift_reduce_table_[i][EndPtr] = {ERROR};
    for (auto& symbol : grammar.alphabet_non_terminal_) {
      go_to_[i][symbol] = -1;
    }
  }
}

int LR1_algorithm::FindRuleNumber(const Situation& situation) {
  for (int i = 0; i < grammar_[situation.non_terminal].size(); ++i) {
    if (grammar_[situation.non_terminal][i] == situation.rule) {
      return i;
    }
  }
}