#include <array>
#include <type_traits>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

template <size_t N>
class Mask {
private:
    std::array<int, N> data_;
public:
    template <typename... Ts, typename = std::enable_if_t<sizeof...(Ts) == N>>
    Mask(Ts... vals) : data_{vals...} {
        for (int v : data_) {
            if (v != 0 && v != 1) {
                throw std::domain_error("Mask must contain only 0 or 1");
            }
        }
    }

    constexpr size_t size() const {
        return N;
    }

    int at(size_t index) const {
        if (index >= N) {
            throw std::out_of_range("Index out of range in Mask");
        }
        return data_[index];
    }

    template <typename Container>
    void slice(Container& container) {
        auto it = container.begin();
        size_t mask_idx = 0;
        while (it != container.end()) {
            bool keep = this->at(mask_idx % N) == 1;
            if (!keep) {
                it = container.erase(it);
            } else {
                ++it;
            }
            ++mask_idx;
        }
    }

    template <typename Container, typename Func>
    Container transform(const Container& container, Func&& f) const {
        Container result = container;
        size_t mask_idx = 0;
        auto result_it = result.begin();
        for (; result_it != result.end(); ++result_it) {
            if (this->at(mask_idx % N) == 1) {
                *result_it = std::invoke(f, *result_it);
            }
            ++mask_idx;
        }
        return result;
    }

    template <typename Container, typename Func>
    Container slice_and_transform(const Container& container, Func&& f) const {
        Container result; 
        size_t mask_idx = 0;
        for (const auto& elem : container) {
            if (this->at(mask_idx % N) == 1) {
                result.push_back(std::invoke(f, elem));
            }
            ++mask_idx;
        }
        return result;
    }
};
