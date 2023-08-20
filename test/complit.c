typedef struct Tree {
  int val;
  struct Tree *lhs;
  struct Tree *rhs;
} Tree;

Tree *tree = &(Tree){
  1,
  &(Tree){
    2,
    &(Tree){ 3, 0, 0 },
    &(Tree){ 4, 0, 0 }
  },
  0
};

int main() {
  _TEST_ASSERT(1, (int){1});
  _TEST_ASSERT(2, ((int[]){0,1,2})[2]);
  _TEST_ASSERT('a', ((struct {char a; int b;}){'a', 3}).a);
  _TEST_ASSERT(3, ({ int x=3; (int){x}; }));
  (int){3} = 5;

  _TEST_ASSERT(1, tree->val);
  _TEST_ASSERT(2, tree->lhs->val);
  _TEST_ASSERT(3, tree->lhs->lhs->val);
  _TEST_ASSERT(4, tree->lhs->rhs->val);

  return 0;
}
