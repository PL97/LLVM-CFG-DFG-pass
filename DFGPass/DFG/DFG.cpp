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

using namespace llvm;
namespace {
	struct DFGPass : public FunctionPass {
	public:
		// node defination
		typedef Value* node;
		//edge, between nodes
		typedef std::pair<node, node> edge;
		//set of nodes
		typedef std::list<node> node_list;
		//set of edge
		typedef std::list<edge> edge_list;
		static char ID;
		edge_list inst_edges;  //store the edge of the control flow
		edge_list edges;    //store the edge of the data flow
		node_list nodes;	//store all the node(includes instructions and operand)

		DFGPass() : FunctionPass(ID) {}

		bool runOnFunction(Function &F) override {
			errs() << "Hello\n";
			edges.clear();
			nodes.clear();
			inst_edges.clear();
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
							nodes.push_back(loadValPtr);
							edges.push_back(edge(loadValPtr, curII));
							break;
						}
						// for the case of store operation, both of the pointer and value should be recoded
						case llvm::Instruction::Store: {
							StoreInst* sinst = dyn_cast<StoreInst>(curII);
							Value* storeValPtr = sinst->getPointerOperand();
							Value* storeVal = sinst->getValueOperand();
							nodes.push_back(storeVal);
							nodes.push_back(storeValPtr);
							edges.push_back(edge(storeVal, curII));
							edges.push_back(edge(curII, storeValPtr));
							break;
						}
						// for other operation, we get all the operand point to the current instruction
						default: {
							for (Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op)
							{
								Instruction* tempIns;
								if (dyn_cast<Instruction>(*op))
								{
									edges.push_back(edge(op->get(), curII));
								}
							}
							break;
						}

					}
					BasicBlock::iterator next = II;
					//errs() << curII << "\n";
					nodes.push_back(curII);
					++next;
					if (next != IEnd) {
						inst_edges.push_back(edge(curII, &*next));
					}
				}

				Instruction* terminator = curBB->getTerminator();
				for (BasicBlock* sucBB : successors(curBB)) {
					Instruction* first = &*(sucBB->begin());
					inst_edges.push_back(edge(terminator, first));
				}
			}
			writeFile(F);
			return false;
		}

		void writeFile(Function &F){
			std::error_code error;
			enum sys::fs::OpenFlags F_None;
			StringRef fileName(F.getName().str() + ".dot");
			raw_fd_ostream file(fileName, error, F_None);
			file << "digraph \"DFG for'" + F.getName() + "\' function\" {\n";
			for (node_list::iterator node = nodes.begin(), node_end = nodes.end(); node != node_end; ++node) 
			{
				if(isa<Instruction>(*node))
				{
					file << "\tNode" << *node << "[shape=record, label=\"" << **node << "\"];\n";
				}
				else
				{
					file << "\tNode" << *node << "[shape=ellipse, label=\"" << **node << "\\l" << *node << "\"];\n";
				}
			}
			//errs() << "Write Done\n";
			//将inst_edges边dump
			for (edge_list::iterator edge = inst_edges.begin(), edge_end = inst_edges.end(); edge != edge_end; ++edge) {
				file << "\tNode" << edge->first << " -> Node" << edge->second << "\n";
			}
			//将data flow的边dump
			file << "edge [color=red]" << "\n";
			for (edge_list::iterator edge = edges.begin(), edge_end = edges.end(); edge != edge_end; ++edge) {
				file << "\tNode" << edge->first << " -> Node" << edge->second << "\n";
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
