#include "Practical.h"

void DieWithUserMessage(string msg, string detail) {
  cerr << msg + ": " + detail + "\n";
  exit(1);
}

void DieWithSystemMessage(string msg) {
  cerr << msg + "\n";
  exit(1);
}
