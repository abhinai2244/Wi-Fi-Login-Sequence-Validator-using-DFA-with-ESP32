/*
 * DFA Implementation - Wi-Fi Login Sequence Validator
 *
 * Transition Table:
 * ┌─────────┬───────┬──────┐
 * │ Current │ Input │ Next │
 * ├─────────┼───────┼──────┤
 * │   Q0    │   u   │  Q1  │
 * │   Q1    │   p   │  Q2  │
 * │   Q2    │   s   │  Q3  │
 * │   *     │   x   │  QE  │
 * │   Any   │ Wrong │  QE  │
 * └─────────┴───────┴──────┘
 */

#include "dfa.h"

// DFA Transition Function: δ(current, input) → next state
State transition(State current, char input) {
  switch (current) {
  case Q0:
    return (input == 'u') ? Q1 : QE; // Only username accepted at start
  case Q1:
    return (input == 'p') ? Q2 : QE; // Only password accepted after username
  case Q2:
    return (input == 's') ? Q3 : QE; // Only submit accepted after password
  case Q3:
    return Q3; // Already accepted, stay in final state
  case QE:
  default:
    return QE; // Dead state — no recovery
  }
}

// Check if the current state is the accepting/final state
bool isFinal(State state) { return state == Q3; }

// Check if the current state is the error/dead state
bool isError(State state) { return state == QE; }

// Return human-readable state name
const char *stateName(State state) {
  switch (state) {
  case Q0:
    return "Q0 (Idle)";
  case Q1:
    return "Q1 (Username Entered)";
  case Q2:
    return "Q2 (Password Entered)";
  case Q3:
    return "Q3 (Login Successful)";
  case QE:
    return "QE (Error)";
  default:
    return "Unknown";
  }
}

// Reset DFA to initial state
State resetDFA() { return Q0; }
