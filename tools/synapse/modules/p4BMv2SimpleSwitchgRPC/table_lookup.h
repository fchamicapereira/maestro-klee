#pragma once

#include "../../execution_plan/context.h"
#include "../../log.h"
#include "../module.h"
#include "call-paths-to-bdd.h"

#include "table_match.h"
#include "table_miss.h"

namespace synapse {
namespace targets {
namespace p4BMv2SimpleSwitchgRPC {

class TableLookup : public Module {
private:
  klee::ref<klee::Expr> condition;
  klee::ref<klee::Expr> key;

public:
  TableLookup()
      : Module(ModuleType::p4BMv2SimpleSwitchgRPC_TableLookup,
               Target::p4BMv2SimpleSwitchgRPC, "TableLookup") {}

  TableLookup(const BDD::Node *node, klee::ref<klee::Expr> _condition,
              klee::ref<klee::Expr> _key)
      : Module(ModuleType::p4BMv2SimpleSwitchgRPC_TableLookup,
               Target::p4BMv2SimpleSwitchgRPC, "TableLookup", node),
        condition(_condition), key(_key) {}

private:
  call_t get_map_get_call(const BDD::Node *current_node,
                          klee::ref<klee::Expr> condition) {
    RetrieveSymbols retriever;
    retriever.visit(condition);

    auto symbols = retriever.get_retrieved_strings();
    assert(symbols.size() == 1);

    auto symbol = symbols[0];
    auto map_get = get_past_node_that_generates_symbol(current_node, symbol);

    assert(map_get->get_type() == BDD::Node::NodeType::CALL);
    auto map_get_call_node = static_cast<const BDD::Call *>(map_get);
    return map_get_call_node->get_call();
  }

  BDD::BDDVisitor::Action visitBranch(const BDD::Branch *node) override {
    if (!query_contains_map_has_key(node)) {
      return BDD::BDDVisitor::Action::STOP;
    }

    assert(!node->get_condition().isNull());
    auto _condition = node->get_condition();

    auto map_get_call = get_map_get_call(node, _condition);

    assert(map_get_call.function_name == "map_get");
    assert(!map_get_call.args["key"].in.isNull());
    assert(!map_get_call.args["value_out"].out.isNull());

    auto _key = map_get_call.args["key"].in;
    auto _value = map_get_call.args["value_out"].out;

    auto new_lookup_module =
        std::make_shared<TableLookup>(node, _condition, _key);
    auto new_match_module = std::make_shared<TableMatch>(node, _value);
    auto new_miss_module = std::make_shared<TableMiss>(node);

    auto lookup_ep_node = ExecutionPlanNode::build(new_lookup_module);
    auto match_ep_node = ExecutionPlanNode::build(new_match_module);
    auto miss_ep_node = ExecutionPlanNode::build(new_miss_module);

    auto lookup_leaf = ExecutionPlan::leaf_t(lookup_ep_node, nullptr);
    auto match_leaf = ExecutionPlan::leaf_t(match_ep_node, node->get_on_true());
    auto miss_leaf = ExecutionPlan::leaf_t(miss_ep_node, node->get_on_false());

    std::vector<ExecutionPlan::leaf_t> lookup_leaves{ lookup_leaf };
    std::vector<ExecutionPlan::leaf_t> match_miss_leaves{ match_leaf,
                                                          miss_leaf };

    auto ep = context->get_current();
    auto ep_lookup = ExecutionPlan(ep, lookup_leaves, bdd);
    auto ep_lookup_match_miss =
        ExecutionPlan(ep_lookup, match_miss_leaves, bdd);

    context->add(ep_lookup_match_miss, new_match_module);

    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action visitCall(const BDD::Call *node) override {
    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action
  visitReturnInit(const BDD::ReturnInit *node) override {
    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action
  visitReturnProcess(const BDD::ReturnProcess *node) override {
    return BDD::BDDVisitor::Action::STOP;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new TableLookup(node, condition, key);
    return std::shared_ptr<Module>(cloned);
  }

  const klee::ref<klee::Expr> &get_condition() const { return condition; }
};
} // namespace p4BMv2SimpleSwitchgRPC
} // namespace targets
} // namespace synapse
