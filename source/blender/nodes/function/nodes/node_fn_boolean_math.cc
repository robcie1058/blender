/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "BLI_listbase.h"
#include "BLI_string.h"

#include "RNA_enum_types.h"

#include "FN_multi_function_builder.hh"

#include "BKE_node_tree_function.hh"

#include "node_function_util.h"

namespace blender {
namespace node {

static bNodeSocketTemplate fn_node_boolean_math_in[] = {
    {SOCK_BOOLEAN, N_("Boolean")},
    {SOCK_BOOLEAN, N_("Boolean")},
    {-1, ""},
};

static bNodeSocketTemplate fn_node_boolean_math_out[] = {
    {SOCK_BOOLEAN, N_("Boolean")},
    {-1, ""},
};

static void node_boolean_math_update(bNodeTree *UNUSED(ntree), bNode *node)
{
  bNodeSocket *sockB = (bNodeSocket *)BLI_findlink(&node->inputs, 1);

  nodeSetSocketAvailability(sockB,
                            ELEM(node->custom1, NODE_BOOLEAN_MATH_AND, NODE_BOOLEAN_MATH_OR));
}

static void node_boolean_math_label(bNodeTree *UNUSED(ntree), bNode *node, char *label, int maxlen)
{
  const char *name;
  bool enum_label = RNA_enum_name(rna_enum_node_boolean_math_items, node->custom1, &name);
  if (!enum_label) {
    name = "Unknown";
  }
  BLI_strncpy(label, IFACE_(name), maxlen);
}

static const fn::MultiFunction &get_multi_function(bNode &bnode)
{
  static fn::CustomFunction_SI_SI_SO<bool, bool, bool> and_fn{
      "And", [](bool a, bool b) { return a && b; }};
  static fn::CustomFunction_SI_SI_SO<bool, bool, bool> or_fn{
      "Or", [](bool a, bool b) { return a || b; }};
  static fn::CustomFunction_SI_SO<bool, bool> not_fn{"Not", [](bool a) { return !a; }};

  switch (bnode.custom1) {
    case NODE_BOOLEAN_MATH_AND:
      return and_fn;
    case NODE_BOOLEAN_MATH_OR:
      return or_fn;
    case NODE_BOOLEAN_MATH_NOT:
      return not_fn;
  }

  BLI_assert(false);
  return fn::dummy_multi_function;
}

static void node_boolean_build_mf_network(bke::NodeMFNetworkBuilder &builder)
{
  const fn::MultiFunction &fn = get_multi_function(builder.bnode());
  builder.set_matching_fn(fn);
}

}  // namespace node
}  // namespace blender

void register_node_type_fn_boolean_math()
{
  static bNodeType ntype;

  fn_node_type_base(&ntype, FN_NODE_BOOLEAN_MATH, "Boolean Math", 0, 0);
  node_type_socket_templates(
      &ntype, blender::node::fn_node_boolean_math_in, blender::node::fn_node_boolean_math_out);
  node_type_label(&ntype, blender::node::node_boolean_math_label);
  node_type_update(&ntype, blender::node::node_boolean_math_update);
  ntype.build_mf_network = blender::node::node_boolean_build_mf_network;
  nodeRegisterType(&ntype);
}
