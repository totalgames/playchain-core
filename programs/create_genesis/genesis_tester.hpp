#pragma once

namespace graphene {

namespace chain {
class genesis_state_type;
}
}

namespace playchain {
namespace util {

using graphene::chain::genesis_state_type;

void test_database(const genesis_state_type&);
}
}
