#include "../src/policy.h"
#include <iostream>
#include <stdexcept>

void check(bool ok, const char *name) {
    if (!ok) throw std::runtime_error(name);
    std::cout << "PASS " << name << '\n';
}
int main() {
    check(policy::validName("ugm_ngan"), "valid account");
    check(!policy::validName("root"), "reject root");
    check(!policy::validName("ugm_test;id"), "reject shell characters");
    check(!policy::validName("ugm_a b"), "reject whitespace");
    check(!policy::validName("-f"), "reject option injection");
    check(policy::validName("ugm_" + std::string(27, 'a')), "31 character boundary");
    check(!policy::validName("ugm_" + std::string(28, 'a')), "reject long name");
    check(!policy::canManage("ugm_a", 0, "other"), "protect system UID");
    check(!policy::canManage("ugm_a", 65534, "other"), "protect nobody UID");
    check(!policy::canManage("ugm_a", 1001, "ugm_a"), "protect operator");
    check(policy::canManage("ugm_a", 1001, "other"), "allow regular scoped user");
    check(!policy::confirmDelete("ugm_a", "yes"), "reject loose confirmation");
    check(policy::confirmDelete("ugm_a", "DELETE ugm_a"), "exact delete confirmation");
    check(!policy::hasPassword("!!"), "reject missing password");
    check(!policy::hasPassword("!*"), "reject disabled password");
    check(policy::hasPassword("!$6$hash"), "allow locked existing password");
}
