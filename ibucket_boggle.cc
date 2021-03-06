// Copyright 2008 Google Inc. All Rights Reserved.
// Author: danvk@google.com (Dan Vanderkam)
//
// Play bucketed boggle w/o a bucketed trie. This could potentially be really
// slow, but will give better bounds and allow more flexible bucketing.

#include <iostream>
#include <sys/time.h>
#include "trie.h"
#include "breaking_tree.h"
#include "bucket_solver.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "init.h"

using namespace std;

DEFINE_string(dictionary, "words", "Dictionary file");
DEFINE_int32(size, 44, "Type of boggle board to use (MN = MxN)");
DEFINE_bool(build_tree, false, "Build possibility trees?");

double secs() {
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec + t.tv_usec / 1000000.0;
}

void Usage(char* prog) {
  fprintf(stderr, "Usage: %s <dict> <class1> ... <class16>\n", prog);
  exit(1);
}
void PrintTree(BucketSolver* solver, BreakingNode* node, int indentation = 0);

int main(int argc, char** argv) {
  Init(&argc, &argv);

  BucketSolver* solver = BucketSolver::Create(
    FLAGS_size, FLAGS_dictionary.c_str());
  CHECK(solver != NULL) << "Couldn't load dictionary";
  solver->SetBuildTree(FLAGS_build_tree);

  char buf[400] = "";
  for (int i=1; i<argc; i++) {
    strcat(buf, argv[i]);
    if (i < argc-1) strcat(buf, " ");
  }
  CHECK(solver->ParseBoard(buf)) << "Couldn't parse '" << buf << "'";

  printf("Board: %s\n", solver->as_string());

  double start = secs();
  int score = solver->UpperBound();
  double end = secs();
  printf("Score: %u\n", score);
  printf("%f secs elapsed\n", end - start);

  const BucketSolver::ScoreDetails& d = solver->Details();
  uint64_t reps = solver->NumReps();
  printf("Details:\n");
  printf(" num_reps: %llu = %fB\n", reps, reps / 1.0e9);
  printf(" sum_union: %d\n", d.sum_union);
  printf(" max_nomark: %d\n", d.max_nomark);

  if (solver->Tree()) {
    // PrintTree(solver, solver->Tree()->Prune());
    double a = secs();
    cout << "Recomputed score: " << solver->Tree()->RecomputeScore() << endl;
    double b = secs();
    cout << " elapsed: " << (b - a) << endl;
    solver->Tree()->Prune();
    a = secs();
    cout << "Pruned: " << solver->Tree()->RecomputeScore() << endl;
    b = secs();
    cout << " elapsed: " << (b - a) << endl;

    cout << "Nodes: " << solver->Tree()->NodeCount() << endl;
    // std::map<int, int> counts;
    // solver->Tree()->ChoiceStats(&counts);
    // for (std::map<int, int>::const_iterator it = counts.begin();
    //      it != counts.end(); ++it) {
    //   cout << "  " << it->first << ": " << it->second << endl;
    // }
    solver->Tree()->AttachPossibilities(solver);

    for (int i = 0; i < solver->NumPossibilities(); i++) {
      int cell, letter;
      if (!solver->Possibility(i, &cell, &letter)) {
        fprintf(stderr, "Doh!\n");
        exit(1);
      }

      cout << "Force " << i << " (" << cell << ", " << ('a' + letter) << "): "
           << solver->Tree()->ScoreWithForce(cell, letter) << endl;
    }
  }
}
