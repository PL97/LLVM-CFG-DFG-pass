#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Use.h>
#include <llvm/Analysis/CFG.h>
#include <list>
#include<vector>
#include<string>
#include<set>
#include<map>

using std::string;
using std::vector;
using std::set;
using std::pair;
using std::map;

using namespace llvm;

struct Edge
{
	int v_from;
	int v_to;
	Edge* in_edge;
	Edge* out_edge;

	Edge(int f, int t, Edge* ie, Edge* oe)
	{
		v_from = f;
		v_to = t;
		in_edge = ie;
		out_edge = oe;
	}
};

struct Vertex
{
	Edge* first_in;
	Edge* first_out;
	Value* va;

	Vertex(Edge* fi, Edge* fo, Value* v) {
		first_in = fi;
		first_out = fo;
		va = v;
	}
};

struct Graph
{
	Function* F;
	vector<Vertex*> v;
	vector<pair<Value*, Value*>> head;
	vector<pair<Value*, Value*>> link;

	Graph(){}
	Graph(Function* f):F(f){}

};

void getGraphInfo(Graph* g)
{
	for (auto iter = g->v.begin(), iter_e = g->v.end(); iter != iter_e; iter++)
	{
		Edge* p = (*iter)->first_in;
		while (p->in_edge)
		{
			errs() << p->in_edge->v_from << '\n';
			p = p->in_edge;
		}
	}
}

int find(vector<Vertex*> l, Value* e)
{
	int count = 0;
	for (auto iter = l.begin(), iter_end = l.end(); iter != iter_end; iter++)
	{
		if ((*iter)->va == e) {
			return count;
		}
		//errs() << (*iter)->va << '\n';
		count++;
	}
	return -1;
}

void insert(Graph *G, pair<Value*, Value*>e)
{
	Value* from = e.first;
	Value* to = e.second;
	int from_idx, to_idx;

	// add vertex
	if ((from_idx = find(G->v, from)) == -1)
	{
		G->v.push_back(new Vertex(NULL, NULL, from));
		from_idx = G->v.size()-1;
	}
	if ((to_idx = find(G->v, to)) == -1)
	{
		G->v.push_back(new Vertex(NULL, NULL, to));
		to_idx = G->v.size()-1;
	}

	// add edge
	Edge* p = G->v[from_idx]->first_out, *new_edge = NULL;
	if (p != NULL) {
		while (p->out_edge && p->v_to != to_idx) 
		{
			p = p->out_edge;
		}
		if (p->v_to == to_idx) 
		{
			new_edge = p;
		}
		else
		{
			new_edge = new Edge(from_idx, to_idx, NULL, NULL);
			p->out_edge = new_edge;
		}
	}
	else
	{
		new_edge = new Edge(from_idx, to_idx, NULL, NULL);
		G->v[from_idx]->first_out = new_edge;
	}

	p = G->v[to_idx]->first_in;
	if (p != NULL) {
		while (p->in_edge && p->v_from != from_idx) 
		{
			p = p->in_edge;
		}
		if (p->v_from != from_idx)
		{
			p->in_edge = new_edge;
		}
	}
	else
	{
		G->v[to_idx]->first_in = new_edge;
	}
}

set<pair<int, int>> mark;
enum Mode{WITHCFG, NOTWITHCFG};

void DFS(Edge* v, Graph* G)
{
	mark.clear();
	Edge* p = v;
	while (p)
	{
		if (mark.find(pair<int, int>(p->v_from, p->v_to)) == mark.end()) 
		{
			mark.insert(pair<int, int>(p->v_from, p->v_to));
			errs() << *G->v[p->v_from]->va << "->" << *G->v[p->v_to]->va << '\n';
			errs() << '\n';
			DFS(G->v[p->v_to]->first_out, G);
		}
		p = p->out_edge;
	}
}
