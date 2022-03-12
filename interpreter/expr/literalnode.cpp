/**
  *  \file interpreter/expr/literalnode.cpp
  *  \brief Class interpreter::expr::LiteralNode
  */

#include "interpreter/expr/literalnode.hpp"

interpreter::expr::LiteralNode::LiteralNode()
    : RValueNode(),
      m_value()
{ }

interpreter::expr::LiteralNode::~LiteralNode()
{ }

void
interpreter::expr::LiteralNode::setNewValue(afl::data::Value* value) throw()
{
    // ex IntLiteralExprNode::setValue
    m_value.reset(value);
}

void
interpreter::expr::LiteralNode::compileValue(BytecodeObject& bco, const CompilationContext& /*cc*/) const
{
    // ex IntLiteralExprNode::compileValue
    bco.addPushLiteral(m_value.get());
}

void
interpreter::expr::LiteralNode::compileEffect(BytecodeObject& bco, const interpreter::CompilationContext& cc) const
{
    defaultCompileEffect(bco, cc);
}

void
interpreter::expr::LiteralNode::compileCondition(BytecodeObject& bco, const CompilationContext& cc, BytecodeObject::Label_t ift, BytecodeObject::Label_t iff) const
{
    defaultCompileCondition(bco, cc, ift, iff);
}
