#include "num2words.h"

#include <string.h>
#include <stdio.h>

static const char* const ONES[] = {
  "zero",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine"
};

static const char* const TEENS[] ={
  "",
  "eleven",
  "twelve"
};

static const char* const TEENS_SPLIT[][2] = {
  {"", ""},
  {"eleven",""},
  {"twelve",""},
  {"thirteen",""},
  {"four","teen"},
  {"fifteen",""},
  {"sixteen",""},
  {"seven","teen"},
  {"eight","teen"},
  {"nine","teen"}
};

static const char* const TENS[] = {
  "",
  "ten",
  "twenty",
  "thirty",
  "forty",
  "fifty",
  "sixty",
  "seventy",
  "eighty",
  "ninety"
};

void hour_to_12h_word(int hours, char *word) {
  hours = hours % 12;
  if (hours == 0) {
    hours = 12;
  }
  strcpy(word, "");

  int tens_val = hours / 10 % 10;
  int ones_val = hours % 10;

  size_t len = 0;

  if (tens_val > 0) {
    if (tens_val == 1 && hours != 10) {
      strcat(word, TEENS[ones_val]);
      return;
    }
    strcat(word, TENS[tens_val]);
    len += strlen(TENS[tens_val]);
    if (ones_val > 0) {
      strcat(word, " ");
      len += 1;
    }
  }

  if (ones_val > 0 || hours == 0) {
    strcat(word, ONES[ones_val]);
    len += strlen(ONES[ones_val]);
  }
}

void minute_to_formal_words(int minutes, char *first_word, char *second_word) {
  strcpy(first_word, "");
  strcpy(second_word, "");

  if (minutes == 0) {
    strcat(first_word, "o'clock");
    return;
  }
  if (minutes < 10) {
    strcat(first_word, ONES[minutes%10]);
    return;
  }
  if (minutes > 10 && minutes < 20) {
    strcat(first_word, TEENS_SPLIT[(minutes - 10) % 10][0]);
    strcat(second_word, TEENS_SPLIT[(minutes - 10) % 10][1]);
    return;
  }

  strcat(first_word, TENS[minutes / 10 % 10]);

  int minute_ones = minutes % 10;
  if (minute_ones) {
    strcat(second_word, ONES[minute_ones]);
  }
}
