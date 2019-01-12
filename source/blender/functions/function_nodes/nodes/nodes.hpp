#pragma once

#include "nodecompiler/core.hpp"
#include "../types/types.hpp"

namespace NC = LLVMNodeCompiler;

class AddIntegersNode : public NC::Node {
public:
	AddIntegersNode(uint amount, NC::Type *type);

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs) const;

private:
	uint amount;
	NC::Type *type;
};

class AddFloatsNode : public NC::Node {
public:
	AddFloatsNode(uint amount, NC::Type *type);

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs) const;

private:
	uint amount;
	NC::Type *type;
};

class Int32InputNode : public NC::Node {
public:
	Int32InputNode(int number);

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &UNUSED(inputs),
		std::vector<llvm::Value *> &r_outputs) const;

private:
	int number;
};

class FloatInputNode : public NC::ExecuteFunctionNode {
public:
	FloatInputNode(float number);

private:
	static void execute(FloatInputNode *node, float *r_number);

	float number;
};

class VectorInputNode : public NC::Node {
public:
	VectorInputNode(float x, float y, float z);

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &UNUSED(inputs),
		std::vector<llvm::Value *> &r_outputs) const;

private:
	float x, y, z;
};

class AddVectorsNode : public NC::Node {
public:
	AddVectorsNode(uint amount);

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs) const;

private:
	uint amount;
};

class PassThroughNode : public NC::Node {
public:
	PassThroughNode(NC::Type *type);

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs) const;
};

class SwitchNode : public NC::Node {
public:
	SwitchNode(NC::Type *type, uint amount);

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs) const;

private:
	uint amount;
	NC::Type *type;
};

class CombineVectorNode : public NC::ExecuteFunctionNode {
public:
	CombineVectorNode();

private:
	static void execute(float *x, float *y, float *z, Vector3 *r_vector);
};

class SeparateVectorNode : public NC::ExecuteFunctionNode {
public:
	SeparateVectorNode();

private:
	static void execute(Vector3 *vector, float *r_x, float *r_y, float *r_z);
};

class FloatToIntNode : public NC::Node {
public:
	FloatToIntNode();

	void buildIR(
		llvm::IRBuilder<> &builder,
		std::vector<llvm::Value *> &inputs,
		std::vector<llvm::Value *> &r_outputs) const;
};