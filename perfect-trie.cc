#include "perfect-trie.h"
#include <queue>
#include <utility>

typedef PerfectTrie::Trie Trie;
PerfectTrie::PerfectTrie() : bits_(0) {}

int NumChildren(const Trie& t) {
  int num_children = 0;
  for (int i=0; i<26; i++) {
    if (t.StartsWord(i)) num_children += 1;
  }
  return num_children;
}

PerfectTrie* PerfectTrie::CompactTrie(const Trie& t) {
  PerfectTrie* pt = AllocatePT(t);
  int num_children = ::NumChildren(t);

  pt->mark_ = 0;
  pt->bits_ = t.IsWord() ? 1 << 26 : 0;
  int num_written = 0;
  for (int i=0; i<kNumLetters; i++) {
    if (t.StartsWord(i)) {
      pt->bits_ |= (1 << i);
      pt->children_[num_written] = CompactTrie(*t.Descend(i));
      num_written += 1;
      assert(num_written <= num_children);
    }
  }
  assert(num_written == num_children);
  assert(pt->NumChildren() == num_children);
  return pt;
}

// Allocate in BFS order to minimize parent/child spacing in memory.
struct WorkItem {
  const Trie& t;
  PerfectTrie* pt;
  int depth;
  WorkItem(const Trie& tr, PerfectTrie* ptr, int d) : t(tr), pt(ptr), depth(d) {}
};
PerfectTrie* PerfectTrie::CompactTrieBFS(const Trie& t) {
  std::queue<WorkItem> todo;
  PerfectTrie* root = AllocatePT(t);
  todo.push(WorkItem(t, root, 1));

  while (!todo.empty()) {
    WorkItem cur = todo.front();
    todo.pop();

    // Construct the Trie in the PerfectTrie
    const Trie& t = cur.t;
    PerfectTrie* pt = cur.pt;
    pt->mark_ = 0;
    pt->bits_ = t.IsWord() ? 1 << 26 : 0;
    int num_written = 0;
    for (int i=0; i<kNumLetters; i++) {
      if (t.StartsWord(i)) {
	pt->bits_ |= (1 << i);
	//if (cur.depth < 6) {
	pt->children_[num_written] = AllocatePT(*t.Descend(i));
	todo.push(WorkItem(*t.Descend(i),
			    pt->children_[num_written],
			    cur.depth + 1));
	//} else {
	//  pt->children_[num_written] = CompactTrie(*t.Descend(i));
	//}
	num_written += 1;
      }
    }
  }
  return root;
}

PerfectTrie::~PerfectTrie() {
  for (int i=0; i<NumChildren(); i++) {
    delete children_[i];
  }
}

// these are mostly copied from trie.cc
bool PerfectTrie::IsWord(const char* wd) const {
  if (!wd) return false;
  if (!*wd) return IsWord();

  int c = *wd - 'a';
  if (c<0 || c>=kNumLetters) return false;

  if (StartsWord(c)) {
    if (c==kQ && wd[1] == 'u')
      return Descend(c)->IsWord(wd+2);
    return Descend(c)->IsWord(wd+1);
  }
  return false;
}

size_t PerfectTrie::Size() const {
  size_t size = 0;
  if (IsWord()) size++;
  for (int i=0; i<26; i++) {
    if (StartsWord(i)) size += Descend(i)->Size();
  }
  return size;
}

size_t PerfectTrie::MemoryUsage() const {
  size_t size = sizeof(*this) + sizeof(PerfectTrie*) * NumChildren();
  for (int i = 0; i < 26; i++) {
    if (StartsWord(i))
      size += Descend(i)->MemoryUsage();
  }
  return size;
}

void PerfectTrie::MemorySpan(caddr_t* low, caddr_t* high) const {
  if ((unsigned)this & 0x80000000) {
    // Ignore Tries allocated on the stack
    *low = (caddr_t)-1;
    *high = (caddr_t)0;
  } else {
    *low = (caddr_t)this;
    *high = (caddr_t)this; *high += sizeof(*this);
  }
  for (int i=0; i<kNumLetters; i++) {
    if (StartsWord(i)) {
      caddr_t cl, ch;
      Descend(i)->MemorySpan(&cl, &ch);
      if (cl < *low) *low = cl;
      if (ch > *high) *high = ch;
    }
  }
}

void PerfectTrie::PrintTrie(std::string prefix) const {
  if (IsWord()) printf("+"); else printf("-");
  printf("(%08X) ", bits_);
  printf("%s\n", prefix.c_str());
  for (int i=0; i<26; i++) {
    if (StartsWord(i))
      Descend(i)->PrintTrie(prefix + std::string(1, 'a' + i));
  }
}

bool IsBoggleWord(const char* wd) {
  int size = strlen(wd);
  if (size < 3 || size > 17) return false;
  for (int i=0; i<size; ++i) {
    int c = wd[i];
    if (c<'a' || c>'z') return false;
    if (c=='q' && (i+1 >= size || wd[1+i] != 'u')) return false;
  }
  return true;
}

PerfectTrie* PerfectTrie::CreateFromFile(const char* filename) {
  Trie* t = new Trie;
  char line[80];
  FILE* f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "Couldn't open %s\n", filename);
    delete t;
    return NULL;
  }

  while (!feof(f) && fscanf(f, "%s", line)) {
    if (!IsBoggleWord(line)) continue;
    t->AddWord(line);
  }
  fclose(f);

  PerfectTrie* pt = PerfectTrie::CompactTrieBFS(*t);
  delete t;
  return pt;
}

// Various memory-allocation bits
bool PerfectTrie::is_allocated = false;
int PerfectTrie::bytes_allocated = 0;
int PerfectTrie::bytes_used = 0;
char* PerfectTrie::memory_pool;
void* PerfectTrie::GetMemory(size_t amount) {
  if (!is_allocated) {
    // TODO(danvk): Be smarter about this -- allocate just enough for each Trie.
    memory_pool = new char[5 << 20];
    bytes_allocated = 5 << 20;
    is_allocated = true;
  }

  if (bytes_used + amount < bytes_allocated) {
    char* start = memory_pool + bytes_used;
    bytes_used += amount;
    return start;
  } else {
    fprintf(stderr, "Couldn't allocate memory!\n");
    exit(1);
  }
}

// This is messy -- use placement new to get a PerfectTrie of the desired size.
PerfectTrie* PerfectTrie::AllocatePT(const Trie& t) {
  int mem_size = sizeof(PerfectTrie) + ::NumChildren(t) * sizeof(PerfectTrie*);
  void* mem = PerfectTrie::GetMemory(mem_size);
  return new(mem) PerfectTrie;
}

// Plain-vanilla Trie code
inline int idx(char x) { return x - 'a'; }

void Trie::AddWord(const char* wd) {
  if (!wd) return;
  if (!*wd) {
    SetIsWord();
    return;
  }
  int c = idx(*wd);
  if (!StartsWord(c))
    children_[c] = new Trie;
  if (c!=kQ)
    Descend(c)->AddWord(wd+1);
  else
    Descend(c)->AddWord(wd+2);
}

Trie::~Trie() {
  for (int i=0; i<26; i++) {
    if (children_[i]) delete children_[i];
  }
}

// Initially, this node is empty
Trie::Trie() {
  for (int i=0; i<kNumLetters; i++)
    children_[i] = NULL;
  is_word_ = false;
}
