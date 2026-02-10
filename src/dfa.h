/*
 * DFA Header - Wi-Fi Login Sequence Validator
 *
 * DFA = (Q, Σ, δ, q₀, F)
 * States: Q0 (Start), Q1 (Username), Q2 (Password), Q3 (Success), QE (Error)
 * Alphabet: 'u' (username), 'p' (password), 's' (submit), 'x' (invalid)
 * Start: Q0
 * Final: Q3
 */

#ifndef DFA_H
#define DFA_H

// DFA States
enum State {
  Q0, // Start / Idle
  Q1, // Username entered
  Q2, // Password entered
  Q3, // Login success (Final/Accepting state)
  QE  // Error state (Dead state)
};

// Transition function: δ(current, input) → next state
State transition(State current, char input);

// Check if state is the final/accepting state
bool isFinal(State state);

// Check if state is the error/dead state
bool isError(State state);

// Get human-readable name for a state
const char *stateName(State state);

// Reset DFA to initial state
State resetDFA();

#endif
