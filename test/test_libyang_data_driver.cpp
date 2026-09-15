extern "C" {
#include <libyang.h>
}
#include "libyang_data_driver.hpp"
#include <yorm/tx.hpp>
#include "yorm_test.hpp"
#include <cassert>
#include <iostream>

#define RUN_TEST(name) \
    std::cout << "[TEST] " << #name << "..." << std::endl; \
    name(); \
    std::cout << "[PASS] " << #name << " passed!\n" << std::endl;

ly_ctx *global_ctx = nullptr;

void test_container_and_leafs()
{
    lyd_node *root = nullptr;
    auto driver = yorm::test::LibyangDataDriver{global_ctx, &root};
    yorm_gen::yorm_test config(&driver);

    config.create_server();
    assert(config.has_server() == true);

    auto srv = config.get_server();
    srv.set_hostname("localhost");
    srv.set_port(8080);

    assert(srv.get_hostname() == "localhost");
    assert(srv.get_port() == 8080);

    // TX has hardcoded values cos of no real backend
    assert(yorm::tx(srv).is_added() == true);
    assert(yorm::tx(srv).is_changed() == true);

    config.delete_server();
    assert(config.has_server() == false);

    if (root) {
        lyd_free_all(root);
    }
}

void test_list_operations()
{
    lyd_node *root = nullptr;
    auto driver = yorm::test::LibyangDataDriver{global_ctx, &root};
    yorm_gen::yorm_test config(&driver);

    config.create_server();
    auto srv = config.get_server();

    auto u1 = srv.add_users("admin");
    u1.set_status(yorm_gen::server_ns::users::status_enum::UP);

    auto u2 = srv.add_users("guest");
    u2.set_status(yorm_gen::server_ns::users::status_enum::DOWN);

    int count = 0;
    bool found_admin = false;
    for (auto user : srv.get_users_list()) {
        count++;
        if (user.get_username() == "admin") {
            assert(user.get_status() == yorm_gen::server_ns::users::status_enum::UP);
            found_admin = true;
        }
    }
    assert(count == 2);
    assert(found_admin == true);

    srv.delete_users("guest");
    count = 0;
    for (auto user : srv.get_users_list()) {
        count++;
    }
    assert(count == 1);

    config.delete_server();
    if (root) {
        lyd_free_all(root);
    }
}

void test_leaflist_operations()
{
    lyd_node *root = nullptr;
    auto driver = yorm::test::LibyangDataDriver{global_ctx, &root};
    yorm_gen::yorm_test config(&driver);

    config.add_tags("yorm");
    config.add_tags("c++");
    config.add_tags("zero-cost");

    int tag_count = 0;
    bool found_yorm = false;
    for (auto tag : config.get_tags()) {
        tag_count++;
        if (tag == "yorm")
            found_yorm = true;
    }
    assert(tag_count == 3);
    assert(found_yorm == true);

    config.delete_tags("yorm");
    tag_count = 0;
    for (auto tag : config.get_tags()) {
        tag_count++;
    }
    assert(tag_count == 2);

    std::vector<std::string_view> new_tags = {"rust", "go"};
    config.set_tags(new_tags);

    tag_count = 0;
    for (auto tag : config.get_tags()) {
        tag_count++;
    }
    assert(tag_count == 2);

    if (root) {
        lyd_free_all(root);
    }
}

void test_rpc_operations()
{
    lyd_node *root = nullptr;
    auto driver = yorm::test::LibyangDataDriver{global_ctx, &root};
    yorm_gen::yorm_test config(&driver);

    auto rpc = config.create_reboot_rpc();
    rpc.set_delay(15);

    assert(rpc.get_delay() == 15);

    auto output = rpc.execute();
    assert(output.get_success() == true);

    if (root) {
        lyd_free_all(root);
    }
}

int main()
{
    if (ly_ctx_new("submodules/libyang/modules", 0, &global_ctx) != LY_SUCCESS) {
        std::cerr << "Failed to create context\n";
        return 1;
    }

    lys_module *mod = nullptr;
    if (lys_parse_path(global_ctx, "test/yorm-test.yang", LYS_IN_YANG, &mod) != LY_SUCCESS) {
        std::cerr << "Failed to parse module\n";
        ly_ctx_destroy(global_ctx);
        return 1;
    }

    std::cout << "==========================================\n";
    std::cout << "   YORM FRAMEWORK - LIBYANG UNIT TESTS    \n";
    std::cout << "==========================================\n\n";

    RUN_TEST(test_container_and_leafs);
    RUN_TEST(test_list_operations);
    RUN_TEST(test_leaflist_operations);
    RUN_TEST(test_rpc_operations);

    std::cout << "ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
    ly_ctx_destroy(global_ctx);
    return 0;
}
