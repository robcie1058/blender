#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>

#include "BLI_utildefines.h"

#include "ArraySet.hpp"
#include "HashMap.hpp"

namespace LLVMNodeCompiler {

struct AnySocket;
struct SocketInfo;
struct Node;
struct Link;
struct Type;
struct LinkSet;
struct DataFlowGraph;

llvm::CallInst *callPointer(
	llvm::IRBuilder<> &builder,
	void *pointer, llvm::FunctionType *type, llvm::ArrayRef<llvm::Value *> arguments);

llvm::Value *voidPtrToIR(llvm::IRBuilder<> &builder, void *pointer);
llvm::Value *ptrToIR(llvm::IRBuilder<> &builder, void *pointer, llvm::Type *type);

llvm::Type *getVoidPtrTy(llvm::IRBuilder<> &builder);
llvm::Type *getVoidPtrTy(llvm::LLVMContext &context);


class Type {
private:
	HashMap<llvm::LLVMContext *, llvm::Type *> typePerContext;

	/* Will be called at most once for every context. */
	virtual llvm::Type *createLLVMType(llvm::LLVMContext &context) = 0;

public:
	llvm::Type *getLLVMType(llvm::LLVMContext &context);

	virtual llvm::Value *buildCopyIR(llvm::IRBuilder<> &builder, llvm::Value *value);
	virtual void buildFreeIR(llvm::IRBuilder<> &builder, llvm::Value *value);
};

template<typename T>
class PointerType : public Type {
private:
	static void *copy_(PointerType<T> *self, void *value)
	{ return (void *)self->copy((T *)value); }
	static void free_(PointerType<T> *self, void *value)
	{ self->free((T *)value); }

public:
	virtual T *copy(T *value) = 0;
	virtual void free(T *value) = 0;

	llvm::Value *buildCopyIR(llvm::IRBuilder<> &builder, llvm::Value *value)
	{
		llvm::Type *void_ptr = getVoidPtrTy(builder);

		llvm::FunctionType *ftype = llvm::FunctionType::get(
			void_ptr, { void_ptr, void_ptr }, false);

		llvm::Value *this_pointer = voidPtrToIR(builder, this);
		return callPointer(builder, (void *)copy_, ftype, { this_pointer, value });
	}

	void buildFreeIR(llvm::IRBuilder<> &builder, llvm::Value *value)
	{
		llvm::Type *void_ptr = getVoidPtrTy(builder);

		llvm::FunctionType *ftype = llvm::FunctionType::get(
			builder.getVoidTy(), {void_ptr, void_ptr}, false);

		llvm::Value *this_pointer = voidPtrToIR(builder, this);
		callPointer(builder, (void *)free_, ftype, { this_pointer, value });
	}

	llvm::Type *createLLVMType(llvm::LLVMContext &context)
	{
		return getVoidPtrTy(context);
	}
};

struct AnySocket {
	inline bool is_output() const { return this->_is_output; }
	inline bool is_input() const { return !this->_is_output; }
	inline Node *node() const { return this->_node; }
	inline uint index() const { return this->_index; }

	Type *type() const;
	std::string debug_name() const;

	inline static AnySocket NewInput(Node *node, uint index)
	{ return AnySocket(node, false, index); }

	inline static AnySocket NewOutput(Node *node, uint index)
	{ return AnySocket(node, true, index); }

	inline friend bool operator==(const AnySocket &left, const AnySocket &right)
	{
		return (
			   left._node == right._node
			&& left._is_output == right._is_output
			&& left._index == right._index);
	}

private:
	AnySocket(Node *node, bool is_output, uint index)
		: _node(node), _is_output(is_output), _index(index) {}

	const SocketInfo *info() const;

	Node *_node;
	bool _is_output;
	uint _index;
};

using SocketArraySet = ArraySet<AnySocket>;
using SocketSet = SocketArraySet;

template<typename TValue>
using SocketMap = HashMap<AnySocket, TValue>;

using SocketValueMap = SocketMap<llvm::Value *>;

struct SocketInfo {
	std::string debug_name;
	Type *type;

	SocketInfo(std::string debug_name, Type *type)
		: debug_name(debug_name), type(type) {}
};

struct NodeSockets {
private:
	using sockets_t = std::vector<SocketInfo>;
	sockets_t sockets;

public:
	using const_iterator = typename sockets_t::const_iterator;

	NodeSockets() {}

	inline void add(SocketInfo socket)
	{ this->sockets.push_back(socket); }

	inline void add(std::string debug_name, Type *type)
	{ this->sockets.push_back(SocketInfo(debug_name, type)); }

	inline uint size() const
	{ return this->sockets.size(); }

	const SocketInfo &operator[](const int index) const
	{ return this->sockets[index]; }

	const_iterator begin() const
	{ return this->sockets.begin(); }
	const_iterator end() const
	{ return this->sockets.end(); }
};

class Node {
public:
	inline const NodeSockets &inputs()
	{ return this->m_inputs; }
	inline const NodeSockets &outputs()
	{ return this->m_outputs; }

	inline AnySocket Input(const uint index)
	{ return AnySocket::NewInput(this, index); }
	inline AnySocket Output(const uint index)
	{ return AnySocket::NewOutput(this, index); }

	inline void addInput(std::string debug_name, Type *type)
	{ this->m_inputs.add(debug_name, type); }
	inline void addOutput(std::string debug_name, Type *type)
	{ this->m_outputs.add(debug_name, type); }

	virtual std::string debug_id() const;

	virtual void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs) = 0;

private:
	NodeSockets m_inputs, m_outputs;
};

class ExecuteFunctionNode : public Node {
public:
	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs);

protected:
	void *execute_function = nullptr;
	bool use_this = false;
};

struct Link {
	AnySocket from, to;

	Link(AnySocket from, AnySocket to)
		: from(from), to(to) {}
};

struct LinkSet {
	std::vector<Link> links;

	AnySocket getOriginSocket(AnySocket socket) const;
	SocketSet getTargetSockets(AnySocket socket) const;
};

class DataFlowCallable {
	void *function_pointer;
	llvm::Module *module;
	llvm::ExecutionEngine *ee;
public:
	DataFlowCallable(llvm::Module *module, llvm::ExecutionEngine *ee, std::string function_name)
		: module(module), ee(ee)
	{
		this->function_pointer = (void *)this->ee->getFunctionAddress(function_name);
	}

	inline void *getFunctionPointer()
	{ return this->function_pointer; }

	void printCode();
};

class DataFlowGraph {
public:
	std::vector<Node *> nodes;
	LinkSet links;

	DataFlowCallable *generateCallable(
		std::string debug_name,
		SocketArraySet &inputs, SocketArraySet &outputs);

	llvm::Module *generateModule(
		llvm::LLVMContext &context,
		std::string module_name, std::string function_name,
		SocketArraySet &inputs, SocketArraySet &outputs);

	llvm::Function *generateFunction(
		llvm::Module *module, std::string name,
		SocketArraySet &inputs, SocketArraySet &outputs);

	void generateCode(
		llvm::IRBuilder<> &builder,
		SocketArraySet &inputs, SocketArraySet &outputs, std::vector<llvm::Value *> &input_values,
		std::vector<llvm::Value *> &r_output_values);

	AnySocket getOriginSocket(AnySocket socket) const;
	SocketSet getTargetSockets(AnySocket socket) const;

	std::string toDotFormat(std::vector<Node *> marked_nodes = {}) const;

	SocketSet findRequiredSockets(SocketSet &inputs, SocketSet &outputs);
private:
	void findRequiredSockets(AnySocket socket, SocketSet &inputs, SocketSet &required_sockets);

	void generateCodeForSocket(
		llvm::IRBuilder<> &builder,
		AnySocket socket,
		SocketValueMap &values,
		SocketSet &required_sockets,
		SocketSet &forwarded_sockets);

	void forwardOutputIfNecessary(
		llvm::IRBuilder<> &builder,
		AnySocket output,
		SocketValueMap &values,
		SocketSet &required_sockets,
		SocketSet &forwarded_sockets);

	void forwardOutput(
		llvm::IRBuilder<> &builder,
		AnySocket output,
		SocketValueMap &values,
		SocketSet &required_sockets);
};

} /* namespace LLVMNodeCompiler */
