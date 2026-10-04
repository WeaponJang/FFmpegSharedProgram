/* Force C++ exception and pthread symbols through the static runtime group. */
#include <mutex>
#include <stdexcept>
#include <thread>

int main()
{
    std::mutex mutex;
    int value = 0;
    std::thread worker([&] {
        std::lock_guard<std::mutex> lock(mutex);
        value = 1;
    });
    worker.join();
    try {
        throw std::runtime_error("static linkage probe");
    } catch (const std::runtime_error &) {
        return value == 1 ? 0 : 1;
    }
}
