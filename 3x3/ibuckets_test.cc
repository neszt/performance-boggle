#include <assert.h>
#include <iostream>
#include "trie.h"
#include "3x3/ibuckets.h"
#include "3x3/boggler.h"

#define assertEq(a, b) CheckEqual(__FILE__, __LINE__, #a, (a), (b));
template<typename A, typename B>
void CheckEqual(const char* file, int line, const char* expr, A a, B b);


void TestBoards() {
  BucketBoggler bb(NULL);
  assert(!bb.ParseBoard(""));
  assert(!bb.ParseBoard("abc def"));
  assert(!bb.ParseBoard("a b c d e f g h i j"));
  assert(!bb.ParseBoard("a b c d e  g h i"));

  assert(bb.ParseBoard("a b c d e f g h i"));
  char cmp[26];
  for (int i = 0; i < 9; i++) {
    sprintf(cmp, "%c", 'a' + i);
    assertEq(strcmp(bb.Cell(i), cmp), 0);
  }
  assert(1 == bb.NumReps());

  strcpy(bb.Cell(0), "abc");
  strcpy(bb.Cell(8), "pqrs");
  strcpy(bb.Cell(7), "htuv");
  assert(0 != strcmp(bb.Cell(0), "a"));
  assert(0 == strcmp(bb.Cell(0), "abc"));
  assert(0 == strcmp(bb.Cell(7), "htuv"));
  assert(0 == strcmp(bb.Cell(8), "pqrs"));
  assert(3*4*4 == bb.NumReps());
}

void TestBound() {
  SimpleTrie t;
  t.AddWord("sea");
  t.AddWord("seat");
  t.AddWord("seats");
  t.AddWord("tea");
  t.AddWord("teas");

  BucketBoggler bb(&t);
  int score;
  assert(bb.ParseBoard("a b c d e f g h i"));
  assertEq(bb.SimpleUpperBound(), 0);
  assertEq(bb.Details().sum_union, 0);
  assertEq(bb.Details().max_nomark, 0);

  // s e h
  // e a t
  // p u c
  assert(bb.ParseBoard("s e p e a u h t c"));
  score = bb.SimpleUpperBound();
  assertEq(bb.Details().sum_union, 4);   // sea(t), tea(s)
  assertEq(bb.Details().max_nomark, 6);  // seat*2, sea*2, tea
  assertEq(score, 1 + 1 + 1 + 1);  // all words
  assertEq(bb.SimpleUpperBound(), 4);

  // a board where both [st]ea can be found, but not simultaneously
  // st z z
  //  e a s
  assert(bb.ParseBoard("st z z e a s z z z"));
  score = bb.SimpleUpperBound();
  assertEq(bb.Details().sum_union, 3);  // tea(s) + sea
  assertEq(bb.Details().max_nomark, 2);  // tea(s)
  assertEq(score, 2);
  assertEq(bb.SimpleUpperBound(), 2);

  // Add in a "seat", test its (sum union's) shortcomings. Can't have 'seats'
  // and 'teas' on the board simultaneously, but it still counts both.
  // st z z
  //  e a st
  //  z z s
  strcpy(bb.Cell(5), "st");
  strcpy(bb.Cell(8), "s");

  score = bb.SimpleUpperBound();
  assertEq(bb.Details().sum_union, 2 + 4);  // all but "hiccup"
  assertEq(bb.Details().max_nomark, 4);  // sea(t(s))
  assertEq(score, 4);
}

void TestQ() {
  SimpleTrie t;
  t.AddWord("qa");    // qua = 1
  t.AddWord("qas");   // qua = 1
  t.AddWord("qest");  // quest = 2

  BucketBoggler bb(&t);
  int score;

  // q a s
  // a e z
  // s t z
  assert(bb.ParseBoard("q a s a e z s t z"));
  score = bb.SimpleUpperBound();
  assertEq(bb.Details().sum_union, 4);
  assertEq(bb.Details().max_nomark, 6);  // (qa + qas)*2 + qest
  assertEq(score, 4);

  // Make sure "qu" gets parsed as "either 'qu' or 'u'"
  // qu a s
  // a e z
  // s t z
  assert(bb.ParseBoard("qu a s a e z s t z"));
  score = bb.SimpleUpperBound();
  assertEq(bb.Details().sum_union, 4);
  assertEq(bb.Details().max_nomark, 6);  // (qa + qas)*2 + qest
  assertEq(score, 4);
}

template<typename A, typename B>
void CheckEqual(const char* file, int line, const char* expr, A a, B b) {
  if (a != b) {
    std::cout << file << ":" << line << ": " << expr << " = " << a
              << ", expected " << b << std::endl;
    exit(1);
  }
}

int main(int argc, char** argv) {
  TestBoards();
  TestBound();
  TestQ();

  printf("%s: All tests passed!\n", argv[0]);
}
