//
// Created by user on 2022-04-14.
//

#include "doctest.h"

#include <ctime>
#include <cstdlib>
#include <random>

#include "resource_pool.h"

struct alignas(16) Obj {
    int a, b, c, d;
    float x, y;

    Obj(int n = 0) : a(n), b(n), c(n), d(n), x((float)n), y((float)n) {}

    bool operator==(const Obj& other) const {
        return a == other.a && b == other.b && c == other.c && d == other.d
            && x == other.x && y == other.y;
    }

    bool operator!=(const Obj& other) const {
        return !(*this == other);
    }
};

#include "reflection.h"
REFLECTION_REGISTER_TYPE(Obj, void, 1);
REFLECTION_REGISTER_TYPE(char, void, 2);

TEST_CASE("Testing StableResourcePool - simple test 1") {
    StableResourcePool<Obj> pool;

    auto a = pool.new_item(1);
    CHECK(pool.size() == 1);
    auto b = pool.new_item(2);
    CHECK(pool.size() == 2);
    auto c = pool.new_item(3);
    *pool.get(c) = 3;
    CHECK(pool.size() == 3);

    CHECK(*pool.get(a) == 1);
    CHECK(*pool.get(b) == 2);
    CHECK(*pool.get(c) == 3);
    CHECK(pool.is_valid(a));
    CHECK(pool.is_valid(b));
    CHECK(pool.is_valid(c));

    pool.release(a);
    CHECK(!pool.is_valid(a));
    CHECK(pool.is_valid(b));
    CHECK(pool.is_valid(c));
    CHECK(pool.size() == 2);

    pool.release(c);
    CHECK(!pool.is_valid(a));
    CHECK(pool.is_valid(b));
    CHECK(!pool.is_valid(c));
    CHECK(pool.size() == 1);

    pool.release(b);
    CHECK(!pool.is_valid(a));
    CHECK(!pool.is_valid(b));
    CHECK(!pool.is_valid(c));
    CHECK(pool.size() == 0);

    a = pool.new_item(1);
    CHECK(pool.size() == 1);
    b = pool.new_item(2);
    CHECK(pool.size() == 2);
    c = pool.new_item(3);
    CHECK(pool.size() == 3);

    CHECK(*pool.get(a) == 1);
    CHECK(*pool.get(b) == 2);
    CHECK(*pool.get(c) == 3);
    CHECK(pool.is_valid(a));
    CHECK(pool.is_valid(b));
    CHECK(pool.is_valid(c));

}

TEST_CASE("Testing StableResourcePool - simple test 2") {
    StableResourcePool<char> pool;

    auto a = pool.new_item('a');
    CHECK(pool.size() == 1);
    auto b = pool.new_item('b');
    CHECK(pool.size() == 2);
    auto c = pool.new_item('c');
    CHECK(pool.size() == 3);

    CHECK(*pool.get(a) == 'a');
    CHECK(*pool.get(b) == 'b');
    CHECK(*pool.get(c) == 'c');
    CHECK(pool.is_valid(a));
    CHECK(pool.is_valid(b));
    CHECK(pool.is_valid(c));
}

TEST_CASE("Testing StableResourcePool - complex test 1") {
    int test_size = 1024;
    auto pool = StableResourcePool<Obj>(test_size);
    std::vector<Ref<Obj>> refs(test_size);
    for (int i = 0; i < test_size; i++) {
        refs[i] = pool.new_item(i);
    }
    CHECK(pool.size() == test_size);
    int delete_size = test_size/2;
    std::vector<int> shuffled(test_size);
    for (int i = 0; i < test_size; i++) {
        shuffled[i] = i;
    }
    std::shuffle(shuffled.begin(), shuffled.end(), std::mt19937(std::random_device()()));
    for (int i = 0; i < delete_size; i++) {
        pool.release(refs[shuffled[i]]);
    }
    CHECK(pool.size() == test_size - delete_size);

    for (int i = 0; i < delete_size; i++) {
        CHECK(!pool.is_valid(refs[shuffled[i]]));
    }
    for (int i = delete_size; i < test_size; i++) {
        CHECK(pool.is_valid(refs[shuffled[i]]));
        CHECK(*pool.get(refs[shuffled[i]]) == shuffled[i]);
    }

    std::vector<Ref<Obj>> new_ids(delete_size);
    for (int i = 0; i < delete_size; i++) {
        new_ids[i] = pool.new_item(shuffled[i]);
    }
    CHECK(pool.size() == test_size);
    for (int i = 0; i < delete_size; i++) {
        CHECK(pool.is_valid(new_ids[i]));
        CHECK(*pool.get(new_ids[i]) == shuffled[i]);
    }
    for (int i = delete_size; i < test_size; i++) {
        CHECK(pool.is_valid(refs[shuffled[i]]));
        CHECK(*pool.get(refs[shuffled[i]]) == shuffled[i]);
    }
}

TEST_CASE("Testing StableResourcePool - complex test 2") {
    int iter = 0;
    const int test_size = 1024;
    auto pool = StableResourcePool<Obj>(test_size);
    std::vector<Ref<Obj>> refs(test_size);
    for (int i = 0; i < test_size; i++) {
        refs[i] = pool.new_item(iter);
    }
    CHECK(pool.size() == test_size);

    for (iter = 1; iter <= 10; iter++) {
        // MESSAGE("Iter ", iter);
        std::shuffle(refs.begin(), refs.end(), std::mt19937(std::random_device()()));
        const int delete_size = refs.size()/2;
        std::vector<Ref<Obj>> deleted(refs.end() - delete_size, refs.end());
        for (auto id : deleted) {
            pool.release(id);
        }
        for (auto id : deleted) {
            CHECK(!pool.is_valid(id));
        }
        refs.resize(refs.size() - delete_size);
        for (auto id : refs) {
            CHECK(pool.is_valid(id));
        }
        CHECK(pool.size() == refs.size());

        const int add_size = refs.size()/4;
        std::vector<Ref<Obj>> new_ids(add_size);
        for (int i = 0; i < add_size; i++) {
            new_ids[i] = pool.new_item(iter);
            refs.push_back(new_ids[i]);
        }
        CHECK(pool.size() == refs.size());
        for (auto id : new_ids) {
            CHECK(pool.is_valid(id));
            CHECK(*pool.get(id) == iter);
        }
    }
}
