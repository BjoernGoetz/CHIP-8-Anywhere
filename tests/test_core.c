#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_placeholder(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x200, 0x200);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_placeholder);
    return UNITY_END();
}
