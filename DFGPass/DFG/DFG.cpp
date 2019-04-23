#include"graph.h"

using namespace llvm;
namespace {

	struct DFGPass : public ModulePass {
	public:
		static char ID;
		map<string, Graph*> DFGs;
		map<string, Graph*> CFGs;

		DFGPass() : ModulePass(ID) {}

		bool runOnModule(Module &M) override {
			for (Module::iterator iter_F = M.begin(), FEnd = M.end(); iter_F != FEnd; ++iter_F) {
				Function *F = &*iter_F;
				Graph* control_flow_G = new Graph(F);
				Graph* data_flow_G = new Graph(F);
				// F->viewCFG();
				DFGs.insert(pair<string, Graph*>(F->getName().str(), data_flow_G));
				CFGs.insert(pair<string, Graph*>(F->getName().str(), control_flow_G));

				control_flow_G->head.push_back(pair<Value*, Value*>(&*(F->begin())->begin(), &*(F->begin())->begin()));
				for (Function::iterator BB = F->begin(), BEnd = F->end(); BB != BEnd; ++BB) {
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
								data_flow_G->head.push_back(pair<Value*, Value*>(storeValPtr, storeVal));
								break;
							}

							case llvm::Instruction::Call: {
								CallInst* cinst = dyn_cast<CallInst>(curII);
								string f_name = cinst->getCalledFunction()->getName();
								for(auto iter = DFGs[f_name]->F->arg_begin(), iter_end = DFGs[f_name]->F->arg_end(); iter != iter_end; iter++){
									data_flow_G->link.push_back(pair<Value*, Value*>(cinst, iter));
									errs()<<*cinst<<cinst<<"->"<<*iter<<iter<<"\n";
									// insert(data_flow_G, pair<Value*, Value*>(cinst, iter));
								}
								if(!DFGs[f_name]->F->doesNotReturn()){
									Value* ret_i = &*(--(--DFGs[f_name]->F->end())->end());
									data_flow_G->link.push_back(pair<Value*, Value*>(ret_i, cinst));
									// insert(data_flow_G, pair<Value*, Value*>(ret_i, cinst));
								}
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
			}

			// NOTWITHCFHG indicate the fianl graph represents no CFG information
			writeFileByGraphGloble(NOTWITHCFG);
			errs()<<"end\n";
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

		void writeFileByGraph(Function *F){
			std::error_code error;
			enum sys::fs::OpenFlags F_None;
			StringRef fileName(F->getName().str() + ".dot");
			raw_fd_ostream file(fileName, error, F_None);
			Graph* data_flow_G =  DFGs[F->getName().str()];
			Graph* control_flow_G = CFGs[F->getName().str()];

			file << "digraph \"DFG for'" + F->getName() + "\' function\" {\n";
			for (auto node_iter = DFGs[F->getName()]->v.begin(), node_end = DFGs[F->getName()]->v.end(); node_iter != node_end; ++node_iter) 
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
				DFS_plot(control_flow_G->v[find(control_flow_G->v, iter->second)]->first_out, control_flow_G, file);
			}

			// plot the data flow edge
			file << "edge [color=red]" << "\n";
			mark.clear();
			for(auto iter = data_flow_G->head.begin(), iter_end = data_flow_G->head.end(); iter != iter_end; iter++){
				DFS_plot(data_flow_G->v[find(data_flow_G->v, iter->second)]->first_out, data_flow_G, file);
			}
			file << "}\n";
			file.close();
		}

		void writeFileByGraphGloble(Mode m){
			std::error_code error;
			enum sys::fs::OpenFlags F_None;
			StringRef fileName("all.dot");
			raw_fd_ostream file(fileName, error, F_None);

			file << "digraph \"DFG for all\" {\n";
			for(auto F_iter = DFGs.begin(), F_iter_end = DFGs.end(); F_iter != F_iter_end; F_iter++){
				Graph* data_flow_G =  DFGs[F_iter->first];
				Graph* control_flow_G = CFGs[F_iter->first];
				auto nodes = F_iter->second->v;
				for (auto node_iter = nodes.begin(), node_end =  nodes.end(); node_iter != node_end; ++node_iter) 
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
				if(m != NOTWITHCFG){
					file << "edge [color=black]" << "\n";
					mark.clear();
					for(auto iter = control_flow_G->head.begin(), iter_end = control_flow_G->head.end(); iter != iter_end; iter++){
						DFS_plot(control_flow_G->v[find(control_flow_G->v, iter->second)]->first_out, control_flow_G, file);
					}
				}

				// plot the data flow edge
				vector<string> color_set = {"red", "blue", "cyan", "orange", "yellow"};
				mark.clear();
				int count = 0;
				for(auto iter = data_flow_G->head.begin(), iter_end = data_flow_G->head.end(); iter != iter_end; iter++){
					file << "edge [color=" << color_set[count++] << "]" << "\n";
					DFS_plot(data_flow_G->v[find(data_flow_G->v, iter->second)]->first_out, data_flow_G, file);
				}

				for(auto iter = data_flow_G->link.begin(), iter_end = data_flow_G->link.end(); iter != iter_end; iter++){
					file << "edge [color=grey]" << "\n";
					file << "\tNode" << iter->first << " -> Node" << iter->second << "\n";
					errs() << *iter->first << *iter->second << "\n";
				}
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