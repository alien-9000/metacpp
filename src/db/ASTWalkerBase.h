/****************************************************************************
* Copyright 2014-2015 Trefilov Dmitrij                                      *
*                                                                           *
* Licensed under the Apache License, Version 2.0 (the "License");           *
* you may not use this file except in compliance with the License.          *
* You may obtain a copy of the License at                                   *
*                                                                           *
*    http://www.apache.org/licenses/LICENSE-2.0                             *
*                                                                           *
* Unless required by applicable law or agreed to in writing, software       *
* distributed under the License is distributed on an "AS IS" BASIS,         *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
* See the License for the specific language governing permissions and       *
* limitations under the License.                                            *
****************************************************************************/
#ifndef METACPP_DB_ASTWALKERBASE_H
#define METACPP_DB_ASTWALKERBASE_H
#include "ExpressionNode.h"

namespace metacpp {
namespace db {
namespace detail {

class ASTWalkerBase
{
public:
    explicit ASTWalkerBase(const detail::ExpressionNodeImplPtr& rootNode);

    virtual ~ASTWalkerBase();

    String doWalk();
protected:
    String evaluateNode(detail::ExpressionNodeImplPtr node);
    virtual String evaluateColumn(std::shared_ptr<detail::ExpressionNodeImplColumn> column) = 0;
    virtual String evaluateLiteral(std::shared_ptr<detail::ExpressionNodeImplLiteral> literal) = 0;
    virtual String evaluateNull(std::shared_ptr<detail::ExpressionNodeImplNull> null) = 0;
    virtual String evaluateUnaryOperator(std::shared_ptr<detail::ExpressionNodeImplUnaryOperator> unary) = 0;
    virtual String evaluateBinaryOperator(std::shared_ptr<detail::ExpressionNodeImplBinaryOperator> binary) = 0;
    virtual String evaluateFunctionCall(std::shared_ptr<detail::ExpressionNodeImplFunctionCall> functionCall) = 0;
    virtual String evaluateWhereClauseRelational(std::shared_ptr<detail::ExpressionNodeImplWhereClauseRelational> whereClauseRelational) = 0;
    virtual String evaluateWhereClauseLogical(std::shared_ptr<detail::ExpressionNodeImplWhereClauseLogical> whereClauseLogical) = 0;
    virtual String evaluateWhereClauseConditional(std::shared_ptr<detail::ExpressionNodeImplWhereClauseConditional> whereClauseComplex) = 0;
private:
    detail::ExpressionNodeImplPtr m_rootNode;

};

} // namespace detail
} // namespace db
} // namespace metacpp

#endif // METACPP_DB_ASTWALKERBASE_H