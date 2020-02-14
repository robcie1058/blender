#include "FN_expression_multi_function.h"
#include "FN_expression_parser.h"
#include "FN_multi_function_network.h"
#include "FN_multi_functions.h"

#include "BLI_string_map.h"

namespace FN {
namespace Expr {

using BLI::StringMap;

static void insert_implicit_conversions(ResourceCollector &resources,
                                        MFBuilderOutputSocket **sub1,
                                        MFBuilderOutputSocket **sub2)
{
  MFNetworkBuilder &network_builder = (*sub1)->node().network();
  const CPPType &type1 = (*sub1)->data_type().single__cpp_type();
  const CPPType &type2 = (*sub2)->data_type().single__cpp_type();
  if (type1 == type2) {
  }
  else if (type1 == CPPType_float && type2 == CPPType_int32) {
    MFBuilderFunctionNode &node = network_builder.add_function<MF_Convert<int, float>>(resources);
    network_builder.add_link(**sub2, node.input(0));
    *sub2 = &node.output(0);
  }
  else if (type1 == CPPType_int32 && type2 == CPPType_float) {
    MFBuilderFunctionNode &node = network_builder.add_function<MF_Convert<int, float>>(resources);
    network_builder.add_link(**sub1, node.input(0));
    *sub1 = &node.output(0);
  }
  else {
    BLI_assert(false);
  }
}

struct AddFunc {
  template<typename T> T execute(T a, T b) const
  {
    return a + b;
  }
};

struct SubFunc {
  template<typename T> T execute(T a, T b) const
  {
    return a - b;
  }
};

struct MulFunc {
  template<typename T> T execute(T a, T b) const
  {
    return a * b;
  }
};

struct SafeDivFunc {
  template<typename T> T execute(T a, T b) const
  {
    return (b != 0) ? a / b : 0;
  }
};

static MFBuilderOutputSocket &build_node(AstNode &ast_node,
                                         MFNetworkBuilder &network_builder,
                                         ResourceCollector &resources,
                                         const StringMap<MFBuilderOutputSocket *> &inputs,
                                         const ConstantsTable &constants_table);

template<typename FuncT>
static MFBuilderOutputSocket &build_binary_node(AstNode &ast_node,
                                                StringRef name,
                                                MFNetworkBuilder &network_builder,
                                                ResourceCollector &resources,
                                                const StringMap<MFBuilderOutputSocket *> &inputs,
                                                const ConstantsTable &constants_table,
                                                const FuncT &func)
{
  MFBuilderOutputSocket *sub1 = &build_node(
      *ast_node.children[0], network_builder, resources, inputs, constants_table);
  MFBuilderOutputSocket *sub2 = &build_node(
      *ast_node.children[1], network_builder, resources, inputs, constants_table);
  insert_implicit_conversions(resources, &sub1, &sub2);

  MFBuilderFunctionNode *node = nullptr;
  const CPPType &type = sub1->data_type().single__cpp_type();
  if (type == CPPType_int32) {
    node = &network_builder.add_function<MF_Custom_In2_Out1<int, int, int>>(
        resources, name, [=](int a, int b) { return func.execute(a, b); });
  }
  else if (type == CPPType_float) {
    node = &network_builder.add_function<MF_Custom_In2_Out1<float, float, float>>(
        resources, name, [=](float a, float b) { return func.execute(a, b); });
  }
  else {
    BLI_assert(false);
  }

  network_builder.add_link(*sub1, node->input(0));
  network_builder.add_link(*sub2, node->input(1));
  return node->output(0);
}

static MFBuilderOutputSocket &build_node(AstNode &ast_node,
                                         MFNetworkBuilder &network_builder,
                                         ResourceCollector &resources,
                                         const StringMap<MFBuilderOutputSocket *> &inputs,
                                         const ConstantsTable &constants_table)
{
  switch (ast_node.type) {
    case AstNodeType::Less: {
      BLI_assert(false);
      break;
    }
    case AstNodeType::Greater: {
      BLI_assert(false);
      break;
    }
    case AstNodeType::Equal: {
      BLI_assert(false);
      break;
    }
    case AstNodeType::LessOrEqual: {
      BLI_assert(false);
      break;
    }
    case AstNodeType::GreaterOrEqual: {
      BLI_assert(false);
      break;
    }
    case AstNodeType::Plus: {
      return build_binary_node(
          ast_node, "add", network_builder, resources, inputs, constants_table, AddFunc());
    }
    case AstNodeType::Minus: {
      return build_binary_node(
          ast_node, "subtract", network_builder, resources, inputs, constants_table, SubFunc());
    }
    case AstNodeType::Multiply: {
      return build_binary_node(
          ast_node, "multiply", network_builder, resources, inputs, constants_table, MulFunc());
    }
    case AstNodeType::Divide: {
      return build_binary_node(
          ast_node, "divide", network_builder, resources, inputs, constants_table, SafeDivFunc());
    }
    case AstNodeType::Identifier: {
      IdentifierNode &identifier_node = (IdentifierNode &)ast_node;
      StringRef identifier = identifier_node.value;
      MFBuilderOutputSocket *expression_input_socket = inputs.lookup_default(identifier, nullptr);
      if (expression_input_socket != nullptr) {
        return *expression_input_socket;
      }
      Optional<SingleConstant> constant = constants_table.try_lookup(identifier);
      BLI_assert(constant.has_value());
      return network_builder
          .add_function<MF_GenericConstantValue>(resources, *constant->type, constant->buffer)
          .output(0);
    }
    case AstNodeType::ConstantInt: {
      ConstantIntNode &int_node = (ConstantIntNode &)ast_node;
      MFBuilderFunctionNode &node = network_builder.add_function<MF_ConstantValue<int>>(
          resources, int_node.value);
      return node.output(0);
    }
    case AstNodeType::ConstantFloat: {
      ConstantFloatNode &float_node = (ConstantFloatNode &)ast_node;
      MFBuilderFunctionNode &node = network_builder.add_function<MF_ConstantValue<float>>(
          resources, float_node.value);
      return node.output(0);
    }
    case AstNodeType::ConstantString: {
      BLI_assert(false);
      break;
    }
    case AstNodeType::Negate: {
      MFBuilderOutputSocket &sub_output = build_node(
          *ast_node.children[0], network_builder, resources, inputs, constants_table);
      MFBuilderFunctionNode *node = nullptr;
      if (sub_output.data_type().single__cpp_type() == CPPType_int32) {
        node = &network_builder.add_function<MF_Custom_In1_Out1<int, int>>(
            resources, "negate", [](int a) { return -a; });
      }
      else if (sub_output.data_type().single__cpp_type() == CPPType_float) {
        node = &network_builder.add_function<MF_Custom_In1_Out1<float, float>>(
            resources, "negate", [](float a) { return -a; });
      }
      else {
        BLI_assert(false);
      }
      network_builder.add_link(sub_output, node->input(0));
      return node->output(0);
    }
    case AstNodeType::Power: {
      BLI_assert(false);
      break;
    }
    case AstNodeType::Call: {
      BLI_assert(false);
      break;
    }
  }
  BLI_assert(false);
  return network_builder.node_by_id(0).output(0);
}

const MultiFunction &expression_to_multi_function(StringRef str,
                                                  ResourceCollector &resources,
                                                  ArrayRef<StringRef> variable_names,
                                                  ArrayRef<MFDataType> variable_types,
                                                  const ConstantsTable &constants_table)
{
  BLI::assert_same_size(variable_names, variable_types);
  AstNode &ast_node = parse_expression(str, resources.allocator());

  MFNetworkBuilder network_builder;
  StringMap<MFBuilderOutputSocket *> builder_dummy_inputs;
  for (uint i : variable_names.index_range()) {
    StringRef identifier = variable_names[i];
    MFBuilderDummyNode &node = network_builder.add_dummy(
        identifier, {}, {variable_types[i]}, {}, {"Value"});
    builder_dummy_inputs.add_new(identifier, &node.output(0));
  }

  MFBuilderOutputSocket &builder_output_socket = build_node(
      ast_node, network_builder, resources, builder_dummy_inputs, constants_table);
  MFBuilderDummyNode &builder_output = network_builder.add_output_dummy("Result",
                                                                        builder_output_socket);

  MFNetwork &network = resources.construct<MFNetwork>("expression network", network_builder);
  const MFInputSocket &output_socket = network.find_dummy_socket(builder_output.input(0));

  Vector<const MFOutputSocket *> inputs;
  builder_dummy_inputs.foreach_value([&](MFBuilderOutputSocket *builder_input) {
    inputs.append(&network.find_dummy_socket(*builder_input));
  });
  Vector<const MFInputSocket *> outputs;
  outputs.append(&output_socket);

  network_builder.to_dot__clipboard();

  const MultiFunction &fn = resources.construct<MF_EvaluateNetwork>(
      "expression function", inputs, outputs);

  return fn;
}

}  // namespace Expr
}  // namespace FN
