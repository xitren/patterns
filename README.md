# Patterns header only library for C++20

[![Build passed](https://github.com/xitren/patterns/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/xitren/patterns/actions/workflows/cmake-multi-platform.yml)

This project contains a set of patterns for frequent use in embedded projects.

- [Transaction](#transaction)
- [Static Allocator](#static-allocator)
- [LRU cache](#lru-cache)
- [Observer](#observer)
- [Mediator](#mediator)
- [Pipeline](#pipeline)
- [Command-line parameter parser](#command-line-parameter-parser)
- [LSB & MSB abstraction](#lsb--msb-abstraction)
- [PIMPL](#pimpl)
- [Packet serializer/deserializer](#packet-serializerdeserializer)

## Contents

- [Patterns list](#patterns-list)
- [Building and developing](#building-and-developing)
- [Project layout](#project-layout)
- [Contributing](#contributing)
- [Licensing](#licensing)

## Patterns list

### Transaction
Подробнее: `docs/patterns/transaction.md`
A transaction in C++ is a sequence of commands that is either executed completely or, in the case of an error, is not executed at all. In case of an error, the transaction rolls back all changes made to the system from the beginning to the moment the error occurs.
~~~cpp
using namespace xitren::func;

int data1 = 85;
int data2 = 68;

try {
    auto trn = make_transaction(data1, data2);

    data1 = 102;
    data2 = 59;

    // ...

    throw std::runtime_error("Problem!");

    trn.commit();

} catch (std::exception const&) {}

EXPECT_TRUE(data1 == 85);
EXPECT_TRUE(data2 == 68);
~~~

### Static Allocator
Подробнее: `docs/patterns/static-heap-and-allocator.md`
Static Allocation: Static allocation is an allocation procedure that is used to allocate all the data objects in predefined memory area. In this type of allocation, data objects are allocated without MMU unit. 

Memory Location: A variable that is stored in the user defined region of the program.
Lifetime: The memory is available throughout the life cycle of the program from the time it starts until it terminates.
Efficiency: It is fast because in this case the memory location of the array is known in advance and thus dynamic memory allocation is not required.

Can be used with all stdlib containers.
~~~cpp
using namespace xitren::allocators;

constexpr std::size_t                             val = 256;
static_heap<val>                                  manager{};
static_heap_allocator<int, val>                   list{manager};
std::vector<int, static_heap_allocator<int, val>> vec{list};

vec.reserve(8);
~~~

### LRU cache
Подробнее: `docs/patterns/lru-cache.md`
Cache replacement algorithms are efficiently designed to replace the cache when the space is full. The Least Recently Used (LRU) is one of those algorithms. As the name suggests when the cache memory is full, LRU picks the data that is least recently used and removes it in order to make space for the new data. The priority of the data in the cache changes according to the need of that data i.e. if some data is fetched or updated recently then the priority of that data would be changed and assigned to the highest priority , and the priority of the data decreases if it remains unused operations after operations.

Operations on LRU Cache:
lru (expired_after): Initialize LRU cache with expiring duration (recommended: `std::chrono` duration, based on `steady_clock`).
get (key) : Returns `std::optional<Value>` and updates the priority (most recently used). If `Exception=true`, may throw `cache_missed` / `cache_timeout`.
put (key, value): Insert/update a key-value pair. If the number of keys exceeds the capacity, evicts the least recently used key.

~~~cpp
using namespace xitren::cache;

lru<int, std::string, 8, false> inst{1s};
inst.put(11, "First data");
inst.get(11);
~~~


### Observer
Подробнее: `docs/patterns/observer.md` и `docs/patterns/observer-values.md`
An observer is a behavioral design pattern that creates a subscription mechanism that allows one object to monitor and respond to events occurring in other objects.
Imagine that you have two objects: a Customer and a Store. A new product that is interesting to the customer is about to be delivered to the store.

The customer can go to the store every day to check the availability of the product. But at the same time, he will get angry, wasting his precious time.

On the other hand, the store can send spam to each of its customers. This will upset many people, as the product is specific, and not everyone needs it.

It turns out to be a conflict: either the customer spends time on periodic checks, or the store spends resources on useless alerts.

The Observer pattern suggests storing a list of references to subscriber objects inside the publisher's object, and the publisher should not maintain the subscription list independently. It will provide methods by which subscribers could add or remove themselves from the list.

You can use it through the provided template as follows:
~~~cpp
using namespace xitren::comm;

class test_observer : public observer<uint8_t> {
private:
    int i = 0;

public:
    void
    data(void const*, uint8_t const&) override
    {
        i = 1;
    }

    [[nodiscard]] int
    get() const
    {
        return i;
    }
};

test_observer              obs1;
test_observer              obs2;
observable<uint8_t, false> res1;
res1.add_observer(obs1);
res1.add_observer(obs2);

uint8_t nd = {0};
res1.notify_observers(nd);
~~~

### Mediator
Подробнее: `docs/patterns/mediator.md`
Mediator is a behavioral design pattern that reduces the connectivity of multiple classes to each other by moving these connections into a single intermediary class.

Let's assume that you have a dialog for creating a user profile. It consists of all kinds of controls — text fields, checkboxes, buttons.

The individual dialog elements should interact with each other. For example, the "I have a dog" checkbox opens a hidden field for entering the pet's name, and clicking on the submit button starts checking the values of all fields on the form.
By writing this logic directly into the code of the controls, you will put an end to their reuse in other places of the application. They will become too closely related to the elements of the profile editing dialog that are not needed in other contexts. Therefore, you can use either all the elements at once, or none.
The Intermediary pattern forces objects to communicate not directly with each other, but through a separate intermediary object that knows to whom a particular request needs to be redirected. Due to this, the system components will depend only on the intermediary, and not on dozens of other components.

In our example, the mediator could be a dialogue. Most likely, the dialog class already knows what elements it consists of, so you won't have to add any new connections to it.

You can use it through the provided template as follows:
~~~cpp
using namespace xitren::comm;

// Note: mediator messages are byte-serialized, so message types must be trivially copyable.

class data1 {
public:
    int i1;
};
class data2 {
public:
    int i1;
    int i2;
};
class data3 {
public:
    int i1;
    int i2;
};

class m1 : module<data1, data2> {
public:
    explicit m1(base::manager& m) : module(m) {}

    void
    data(data2 const&) override
    {
        std::cout << "Receiving m1!" << std::endl;
    }

    void
    test()
    {
        data1 a{};
        send(a);
    }
};

class m2 : module<data2, data3> {
public:
    explicit m2(base::manager& m) : module(m) {}

    void
    data(data3 const&) override
    {
        std::cout << "Receiving m2!" << std::endl;
    }

    void
    test()
    {
        data2 a{};
        send(a);
    }
};

class m3 : module<data3, data2, data1> {
public:
    explicit m3(base::manager& m) : module(m) {}

    void
    data(data2 const&) override
    {
        std::cout << "Receiving m3 data2!" << std::endl;
    }

    void
    data(data1 const&) override
    {
        std::cout << "Receiving m3 data1!" << std::endl;
    }
};

manager<5> dt;
m1         mod1(dt);
m2         mod2(dt);
m3         mod3(dt);

std::cout << "Sending event!" << std::endl;
mod2.test();
mod1.test();

dt.send(data3{});
~~~

### Pipeline
Подробнее: `docs/patterns/pipeline.md`
Implements a non-blocking handler in a separate thread using atomic operations.
Current implementation uses a bounded buffer and blocks `push()` when the buffer is full (to avoid overwriting and data races).

~~~cpp
using namespace xitren::comm;
using pipeline_type = pipeline_stage<std::string, void, 1024, LogCout>;

static auto func = [&](pipeline_stage_exception, const std::string str, const std::pair<int, int> stat) -> void {
    std::cout << "String: " << str << " Time: " << stat.first << " Util: " << stat.second << "\n";
};

pipeline_type stage(func);
stage.push("First");
stage.push("Second");
stage.push("Third");
for (int i{}; i < 100; i++) {
    stage.push("Next");
    stage.push("Other");
}
~~~

### Command-line parameter parser
Подробнее: `docs/patterns/argv-parser.md`
A command-line parameter handler for applications without using external dependencies.

~~~cpp
using namespace xitren::func;

char const* argv[] = {"app", "--port", "/dev/tty.usbserial-A50285BI", "--baud", "1000000"};
int         argc   = sizeof(argv) / sizeof(argv[0]);

struct options {
    std::string name_port{};
    int         baud_rate{};
};

auto parser = argv_parser<options>::instance({{"--port", &options::name_port}, {"--baud", &options::baud_rate}});

auto detected_opts = parser->parse(argc, argv);

std::cout << "name_port = " << detected_opts.name_port << std::endl;
std::cout << "baud_rate = " << detected_opts.baud_rate << std::endl;
~~~

### LSB & MSB abstraction
Подробнее: `docs/patterns/endian-wrappers.md`
~~~cpp
using namespace xitren::func;

lsb_t<std::uint32_t> data{0xFF00FF00};
data.get();
~~~

### PIMPL
Подробнее: `docs/patterns/fast-pimpl.md`

### Packet serializer/deserializer
Подробнее: `docs/patterns/packet.md`
Serialization is a mechanism of converting the state of an object into a byte stream. Deserialization is the reverse process where the byte stream is used to recreate the actual object in memory. This mechanism is used to persist the object.
~~~cpp
using namespace xitren::func;

struct header_ext {
    uint8_t magic_header[2];
};

struct noise_frame {
    static inline header_ext header = {'N', 'O'};
    uint16_t                 data;
};

struct adc_frame {
    static inline header_ext header = {'L', 'N'};
    uint16_t                 data[11];
};

class crc16ansi {
public:
    using value_type = lsb_t<std::uint16_t>;

    template <xitren::crc::crc_iterator InputIterator>
    static constexpr value_type
    calculate(InputIterator begin, InputIterator end) noexcept
    {
        static_assert(sizeof(*begin) == sizeof(std::uint8_t));
        return xitren::crc::crc16::calculate(begin, end);
    }

    template <std::size_t Size>
    static constexpr value_type
    calculate(std::array<std::uint8_t, Size> const& data) noexcept
    {
        return xitren::crc::crc16::calculate(data);
    }
};


packet<header_ext, noise_frame, crc16ansi> pack(noise_frame::header, adf);
packet<header_ext, noise_frame, crc16ansi> pack_recv(pack.to_array());
auto hdr = pack_recv.header(); // returned by value
~~~

## Building and developing

The recommended way to develop and build this project is to use the Docker image as a dev container.

### Presets

This project makes use of [CMake
presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) to simplify the
process of configuring the project. As a developer, you should use a
`CMakePresets.json` file at the top-level directory of the repository.

### Configure, build and test

You can configure, build and test the `clang_host_release_linux` parts of the project with the following
commands:

~~~shell
cmake --preset=clang_host_release_linux
cmake --build --preset=clang_host_release_linux
ctest --test-dir out/build/clang_host_release_linux --output-on-failure
~~~

### Sanitizers (Linux, clang)

Note: on some Linux environments you may need the compiler runtime package (compiler-rt). For Ubuntu:

~~~shell
sudo apt-get update && sudo apt-get install -y clang llvm compiler-rt
~~~

Note (Alpine/musl): AddressSanitizer/ThreadSanitizer support may be limited or unstable. If you are using the provided
Alpine-based devcontainer, prefer running sanitizers in CI (GitHub Actions) or use the optional Ubuntu/glibc devcontainer
(`.devcontainer/devcontainer.ubuntu.json`).

ASan + UBSan:

~~~shell
cmake --preset=clang_host_asan_ubsan_linux
cmake --build --preset=clang_host_asan_ubsan_linux
ctest --test-dir out/build/clang_host_asan_ubsan_linux --output-on-failure
~~~

TSan:

~~~shell
cmake --preset=clang_host_tsan_linux
cmake --build --preset=clang_host_tsan_linux
ctest --test-dir out/build/clang_host_tsan_linux --output-on-failure
~~~

## Project layout

The following ideas are mainly stolen from [P1204R0 – Canonical Project
Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html).

- Also, all includes, even the "project local" ones use `<>` instead of `""`.
- Subfolders for the source code should comprise somewhat standalone "components".
- There should be a STATIC or INTERFACE library target for each component (this should
  make linking source code dependencies for tests easier).
- Everything test related is in `tests/` and its subdirectories.
- Tests have the `.cpp` extension and are
  named after the class, file, functionality, interface or whatever else they test.
- Hardware tests should be similar to unit tests and check simple functionalities of
  low-level code.
- Golden Tests are used for high level integration/system tests.

The following shows what the directory structure could actually look like.

<details>
  <summary>Directory structure</summary>

  ~~~
  patterns/
  ├── .github/
  ├── docs/
  ├── include/
  │   ├── xitren/
  │   │   ├── allocators/
  │   │   │   ├── static_heap_allocator.hpp
  │   │   │   └── static_heap.hpp
  │   │   ├── cache/
  │   │   │   ├── exceptions.hpp
  │   │   │   └── lru.hpp
  │   │   ├── comm/
  │   │   │   ├── mediator.hpp
  │   │   │   ├── observer_errors.hpp
  │   │   │   ├── observer_static.hpp
  │   │   │   ├── observer.hpp
  │   │   │   ├── pipeline_stage_pool.hpp
  │   │   │   ├── pipeline_stage.hpp
  │   │   │   └── values/
  │   │   │       ├── observable.hpp
  │   │   │       ├── observed.hpp
  │   │   │       └── pre_def.hpp
  │   │   ├── func/
  │   │   │   ├── argv_parser.hpp
  │   │   │   ├── data.hpp
  │   │   │   ├── fast_pimpl.hpp
  │   │   │   ├── interval_event.hpp
  │   │   │   ├── log_adapter.hpp
  │   │   │   └── packet.hpp
  │   │   └── ...
  |   └── ...
  │
  ├── tests/
  │   ├── CMakeLists.txt
  │   ├── patterns_argv_test.cpp
  │   ├── patterns_interval_base_test.cpp
  │   ├── patterns_lru_base_test.cpp
  │   ├── patterns_mediator_base_test.cpp
  │   ├── patterns_observer_base_test.cpp
  │   ├── patterns_observer_static_base_test.cpp
  │   ├── patterns_observer_values_test.cpp
  │   ├── patterns_package_base_test.cpp
  │   ├── patterns_pipeline_test.cpp
  │   ├── patterns_static_heap_allocator_test.cpp
  │   └── ...
  │
  ├── .clang-format
  ├── .clang-tidy
  ├── .gitignore
  ├── CMakeLists.txt
  ├── CMakePresets.json
  ├── Doxyfile
  ├── LICENSE
  ├── README.md
  └── ...
  ~~~

</details>

## Contributing

The best chance of getting a problem fixed is to submit a patch that fixes it (along with a test case that verifies the fix)!
Feel free to create PR.

## Licensing

See the [LICENSE](LICENSE) document.
