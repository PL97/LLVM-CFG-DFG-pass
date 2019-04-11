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
		//指令的节点
		typedef std::pair<Value*, StringRef> node;
		//指令与指令之间的边
		typedef std::pair<node, node> edge;
		//指令的集合
		typedef std::list<node> node_list;
		//边的集合
		typedef std::list<edge> edge_list;
		static char ID;
		//std::error_code error;
		edge_list inst_edges;  //存储每条指令之间的先后执行顺序
		edge_list edges;    //存储data flow的边
		node_list nodes;	//存储每条指令
		int num;
		//raw_fd_ostream *file;
		//enum sys::fs::OpenFlags F_None;
		//std::string str;
		DFGPass() : FunctionPass(ID) { num = 0; }

		// //将指令的内容输出到string中去
		// std::string changeIns2Str(Instruction* ins) {
		// 	std::string temp_str;
		// 	raw_string_ostream os(temp_str);
		// 	ins->print(os);
		// 	return os.str();
		// }

		//如果是变量则获得变量的名字，如果是指令则获得指令的内容
		StringRef getValueName(Value* v)
		{
			std::string temp_result = "val";
			if (v->getName().empty())
			{
				temp_result += std::to_string(num);
				num++;
			}
			else {
				temp_result = v->getName().str();
			}
			StringRef result(temp_result);
			errs() << result;
			return result;
		}

		bool runOnFunction(Function &F) override {
			errs() << "Hello\n";
			edges.clear();
			nodes.clear();
			inst_edges.clear();
			for (Function::iterator BB = F.begin(), BEnd = F.end(); BB != BEnd; ++BB) {
				BasicBlock *curBB = &*BB;
				for (BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end(); II != IEnd; ++II) {
					Instruction* curII = &*II;
					errs() << getValueName(curII) << "\n";
					switch (curII->getOpcode())
					{
						//由于load和store对内存进行操作，需要对load指令和stroe指令单独进行处理
						case llvm::Instruction::Load:
						{
							LoadInst* linst = dyn_cast<LoadInst>(curII);
							Value* loadValPtr = linst->getPointerOperand();
							edges.push_back(edge(node(loadValPtr, getValueName(loadValPtr)), node(curII, getValueName(curII))));
							break;
						}
						case llvm::Instruction::Store: {
							StoreInst* sinst = dyn_cast<StoreInst>(curII);
							Value* storeValPtr = sinst->getPointerOperand();
							Value* storeVal = sinst->getValueOperand();
							// errs()<<"++++++++++++++++"<<getValueName(storeValPtr)<<"+++++++++++++++"<<'\n';
							// errs()<<"++++++++++++++++"<<getValueName(storeVal)<<"+++++++++++++++"<<'\n';
							edges.push_back(edge(node(storeVal, getValueName(storeVal)), node(curII, getValueName(curII))));
							edges.push_back(edge(node(curII, getValueName(curII)), node(storeValPtr, getValueName(storeValPtr))));
							break;
						}
						default: {
							for (Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op)
							{
								Instruction* tempIns;
								if (dyn_cast<Instruction>(*op))
								{
									edges.push_back(edge(node(op->get(), getValueName(op->get())), node(curII, getValueName(curII))));
								}
							}
							break;
						}

					}
					BasicBlock::iterator next = II;
					//errs() << curII << "\n";
					nodes.push_back(node(curII, getValueName(curII)));
					++next;
					if (next != IEnd) {
						inst_edges.push_back(edge(node(curII, getValueName(curII)), node(&*next, getValueName(&*next))));
					}
				}

				Instruction* terminator = curBB->getTerminator();
				for (BasicBlock* sucBB : successors(curBB)) {
					// errs()<<"++++++++++++++"<<getValueName(terminator)<<"+++++++++++++++++"<<'\n';
					Instruction* first = &*(sucBB->begin());
					inst_edges.push_back(edge(node(terminator, getValueName(terminator)), node(first, getValueName(first))));
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
			for (node_list::iterator node = nodes.begin(), node_end = nodes.end(); node != node_end; ++node) {
				if(dyn_cast<Instruction>(node->first))
					file << "\tNode" << node->first << "[shape=record, label=\"" << *(node->first) << "\"];\n";
				else
					file << "\tNode" << node->first << "[shape=record, label=\"" << node->second << "\"];\n";
			}
			//errs() << "Write Done\n";
			//将inst_edges边dump
			for (edge_list::iterator edge = inst_edges.begin(), edge_end = inst_edges.end(); edge != edge_end; ++edge) {
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "\n";
			}
			//将data flow的边dump
			file << "edge [color=red]" << "\n";
			for (edge_list::iterator edge = edges.begin(), edge_end = edges.end(); edge != edge_end; ++edge) {
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "\n";
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