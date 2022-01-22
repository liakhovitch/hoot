#ifndef MORSE_H
#define MORSE_H

#define NUM_VALID_CHARS 52
const char valid_chars[NUM_VALID_CHARS] = {
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
  'u', 'v', 'w', 'x', 'y', 'z', '1', '2', '3', '4',
  '5', '6', '7', '8', '9', '0', '.', ',', '?', ':',
  '-', '\"', '(', ')', '=', '*', ';', '/', '\'', '_',
  '+', '@'
  };

const char* morse(char x);

#endif
