#include "mock_data_node.hpp"
#include "yorm_test.hpp"
#include <cassert>
#include <iostream>

#define RUN_TEST(name) \
    std::cout << "[TEST] " << #name << "..." << std::endl; \
    name(); \
    std::cout << "[PASS] " << #name << " passed!\n" << std::endl;

void test_container_and_leafs()
{
    auto root = std::make_shared<yorm::test::MockDataNode>("config");
    yorm_gen::yorm_test config(root);

    config.create_server();
    assert(config.has_server() == true);

    auto srv = config.get_server();
    srv.set_hostname("test-server");
    srv.set_port(8080);

    assert(srv.get_hostname() == "test-server");
    assert(srv.get_port() == 8080);

    config.delete_server();
    assert(config.has_server() == false);
}

void test_list_operations()
{
    auto root = std::make_shared<yorm::test::MockDataNode>("config");
    yorm_gen::yorm_test config(root);
    config.create_server();
    auto srv = config.get_server();

    auto u1 = srv.add_users("admin");
    u1.set_status(yorm_gen::server_ns::users::status_enum::UP);

    auto u2 = srv.add_users("guest");
    u2.set_status(yorm_gen::server_ns::users::status_enum::DOWN);

    auto users = srv.get_users_list();
    assert(users.size() == 2);

    srv.delete_users("admin");
    assert(srv.get_users_list().size() == 1);
    assert(srv.get_users_list()[0].get_username() == "guest");
}

void test_leaflist_operations()
{
    auto root = std::make_shared<yorm::test::MockDataNode>("config");
    yorm_gen::yorm_test config(root);

    config.add_tags("web");
    config.add_tags("database");

    auto tags = config.get_tags();
    assert(tags.size() == 2);
    assert(tags[0] == "web");
    assert(tags[1] == "database");

    config.delete_tags("web");
    assert(config.get_tags().size() == 1);
    assert(config.get_tags()[0] == "database");
}

void test_rpc_operations()
{
    auto root = std::make_shared<yorm::test::MockDataNode>("config");

    yorm_gen::reboot_ns::input req(root);
    req.set_delay(10);
    assert(req.get_delay() == 10);

    auto resp_node = root->execute_rpc("reboot");
    yorm_gen::reboot_ns::output resp(resp_node);
    resp.set_success(true);
    assert(resp.get_success() == true);
}

int main()
{
    std::cout << "==========================================\n";
    std::cout << "   YORM FRAMEWORK - IN-MEMORY UNIT TESTS  \n";
    std::cout << "==========================================\n\n";

    RUN_TEST(test_container_and_leafs);
    RUN_TEST(test_list_operations);
    RUN_TEST(test_leaflist_operations);
    RUN_TEST(test_rpc_operations);

    std::cout << "ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}