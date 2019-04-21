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
namespace {

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
		vector<Vertex*> v;
		vector<Value*> head;

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


	struct DFGPass : public FunctionPass {
	public:
		static char ID;
		map<string, Graph*> DFGs;
		map<string, Graph*> CFGs;

		DFGPass() : FunctionPass(ID) {}

		bool runOnFunction(Function &F) override {
			Graph* control_flow_G = new Graph();
			Graph* data_flow_G = new Graph();
			DFGs.insert(pair<string, Graph*>(F.getName().str(), data_flow_G));
			CFGs.insert(pair<string, Graph*>(F.getName().str(), control_flow_G));

			control_flow_G->head.push_back(&*(F.begin())->begin());
			for (Function::iterator BB = F.begin(), BEnd = F.end(); BB != BEnd; ++BB) {
				BasicBlock *curBB = &*BB;
				for (BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end(); II != IEnd; ++II) {
					Instruction* curII = &*II;
					switch (curII->getOpcode())
					{
						// for the case of load operation, we should save the value of it
						case llvm::Instruction::Load:
						{
							LoadInst* linst = dyn_cast<LoadInst>(curII);
							Value* loadValPtr = linst->getPointerOperand();
							insert(data_flow_G, pair<Value*, Value*>(loadValPtr, curII));
							break;
						}
						// for the case of store operation, both of the pointer and value should be recoded
						case llvm::Instruction::Store: {
							StoreInst* sinst = dyn_cast<StoreInst>(curII);
							Value* storeValPtr = sinst->getPointerOperand();
							Value* storeVal = sinst->getValueOperand();
							insert(data_flow_G, pair<Value*, Value*>(storeVal, curII));
							insert(data_flow_G, pair<Value*, Value*>(curII, storeValPtr));
							data_flow_G->head.push_back(storeVal);
							break;
						}
						// for other operation, we get all the operand point to the current instruction
						default: {
							for (Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op)
							{
								Instruction* tempIns;
								if (dyn_cast<Instruction>(*op))
								{
									insert(data_flow_G, pair<Value*, Value*>(op->get(), curII));
								}
							}
							break;
						}

					}
					BasicBlock::iterator next = II;
					++next;
					if (next != IEnd) {
						insert(control_flow_G, pair<Value*, Value*>(curII, &*next));
					}
				}

				Instruction* terminator = curBB->getTerminator();
				for (BasicBlock* sucBB : successors(curBB)) {
					Instruction* first = &*(sucBB->begin());
					insert(control_flow_G, pair<Value*, Value*>(terminator, first));
				}
			}
			writeFileByGraph(F);

			return false;
		}

		void DFS_plot(Edge* v, Graph* G, raw_fd_ostream& file)
		{
			Edge* p = v;
			while (p)
			{
				if (mark.find(pair<int, int>(p->v_from, p->v_to)) == mark.end()) 
				{
					mark.insert(pair<int, int>(p->v_from, p->v_to));
					file << "\tNode" << G->v[p->v_from]->va << " -> Node" << G->v[p->v_to]->va << "\n";
					DFS_plot(G->v[p->v_to]->first_out, G, file);
				}
				p = p->out_edge;
			}
		}

		void writeFileByGraph(Function &F){
			std::error_code error;
			enum sys::fs::OpenFlags F_None;
			StringRef fileName(F.getName().str() + ".dot");
			raw_fd_ostream file(fileName, error, F_None);
			Graph* data_flow_G =  DFGs[F.getName().str()];
			Graph* control_flow_G = CFGs[F.getName().str()];

			file << "digraph \"DFG for'" + F.getName() + "\' function\" {\n";
			for (auto node_iter = DFGs[F.getName()]->v.begin(), node_end = DFGs[F.getName()]->v.end(); node_iter != node_end; ++node_iter) 
			{
				Value* p = (*node_iter)->va;
				if(isa<Instruction>(*p))
				{
					file << "\tNode" << p << "[shape=record, label=\"" << *p << "\"];\n";
				}
				else
				{
					file << "\tNode" << p << "[shape=ellipse, label=\"" << *p << "\\l" << p << "\"];\n";
				}
			}
			// plot the instruction flow edge
			mark.clear();
			for(auto iter = control_flow_G->head.begin(), iter_end = control_flow_G->head.end(); iter != iter_end; iter++){
				DFS_plot(control_flow_G->v[find(control_flow_G->v, *iter)]->first_out, control_flow_G, file);
			}

			// plot the data flow edge
			file << "edge [color=red]" << "\n";
			mark.clear();
			for(auto iter = data_flow_G->head.begin(), iter_end = data_flow_G->head.end(); iter != iter_end; iter++){
				DFS_plot(data_flow_G->v[find(data_flow_G->v, *iter)]->first_out, data_flow_G, file);
			}
			file << "}\n";
			file.close();
		}

	};
}

char DFGPass::ID = 0;
static RegisterPass<DFGPass> X("DFGPass", "DFG Pass Analyse",
	false, false
);