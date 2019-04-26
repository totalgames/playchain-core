#pragma once

#include <boost/version.hpp>

// test case defines
#if BOOST_VERSION >= 106000 // boost ver. >= 1.60.0

#define POKER_AUTO_TU_REGISTRAR(test_name)                                                                            \
    BOOST_AUTO_TU_REGISTRAR(test_name)                                                                                 \
    (boost::unit_test::make_test_case(&BOOST_AUTO_TC_INVOKER(test_name), #test_name, __FILE__, __LINE__),              \
     boost::unit_test::decorator::collector::instance());

#else // boost ver. <1.60.0

#define POKER_AUTO_TU_REGISTRAR(test_name)                                                                            \
    BOOST_AUTO_TU_REGISTRAR(test_name)                                                                                 \
    (boost::unit_test::make_test_case(&BOOST_AUTO_TC_INVOKER(test_name), #test_name),                                  \
     boost::unit_test::ut_detail::auto_tc_exp_fail<BOOST_AUTO_TC_UNIQUE_ID(test_name)>::instance()->value());

#endif

#define PLAYCHAIN_TEST_CASE(test_name)                                                                                 \
    struct test_name : public BOOST_AUTO_TEST_CASE_FIXTURE                                                             \
    {                                                                                                                  \
        void test_method()                                                                                             \
        {                                                                                                              \
            try                                                                                                        \
            {                                                                                                          \
                test_method_override();                                                                                \
            }                                                                                                          \
            FC_LOG_AND_RETHROW()                                                                                       \
        }                                                                                                              \
        void test_method_override();                                                                                   \
    };                                                                                                                 \
                                                                                                                       \
    static void BOOST_AUTO_TC_INVOKER(test_name)()                                                                     \
    {                                                                                                                  \
        BOOST_TEST_CHECKPOINT('"' << #test_name << "\" fixture entry.");                                               \
        test_name t;                                                                                                   \
        BOOST_TEST_CHECKPOINT('"' << #test_name << "\" entry.");                                                       \
        t.test_method();                                                                                               \
        BOOST_TEST_CHECKPOINT('"' << #test_name << "\" exit.");                                                        \
    }                                                                                                                  \
                                                                                                                       \
    struct BOOST_AUTO_TC_UNIQUE_ID(test_name)                                                                          \
    {                                                                                                                  \
    };                                                                                                                 \
                                                                                                                       \
    POKER_AUTO_TU_REGISTRAR(test_name)                                                                                \
                                                                                                                       \
    void test_name::test_method_override()
