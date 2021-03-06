
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "OperationsTest.h"

#include "react/engine/PulsecountEngine.h"
#include "react/engine/ToposortEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqToposort, OperationsTest, ToposortEngine<sequential>);
INSTANTIATE_TYPED_TEST_CASE_P(ParToposort, OperationsTest, ToposortEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(Pulsecount, OperationsTest, PulsecountEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(Subtree, OperationsTest, SubtreeEngine<parallel>);

} // ~namespace