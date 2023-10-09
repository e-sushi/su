/*

	Various kinds of stacks, for convenience.

*/

namespace amu {

struct ASTNode;
struct LabelTable;
struct Label;
struct Code;

struct NodeStack {
	Array<ASTNode*> stack;
	ASTNode* current;

	// DEBUG: code object this node stack belongs to 
	Code* code;

	void push(ASTNode* n);
	ASTNode* pop();
};

struct TableStack {
	Array<LabelTable*> stack;
	LabelTable* current;

	void push(LabelTable* l);
	void pop();

	void add(String id, Label* l);
	Label* search(u64 hashed_id);
	Label* search_local(u64 hashed_id);
};

} // namespace amu
