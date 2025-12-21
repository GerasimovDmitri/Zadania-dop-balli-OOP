#include <type_traits> 
#include <exception> 
#include <string>     

class NotEnoughSlotsError : public std::exception {
private:
    std::string msg_;
public:
    NotEnoughSlotsError(size_t created) {
        msg_ = "Недостаточно слотов. Текущих объектов: " + std::to_string(created);
    }
    const char* what() const noexcept override {
        return msg_.c_str();
    }
};

class EmptySlotError : public std::exception {
public:
    const char* what() const noexcept override {
        return "Объекта по этому номеру не существует";
    }
};

template <typename T, size_t N>
class MemReserver {
private:
    alignas(T) char storage_[N * sizeof(T)]; 
    bool occupied_[N];    

public:
    MemReserver() {
        for (size_t i = 0; i < N; ++i) {
            occupied_[i] = false;
        }
    }

    ~MemReserver() {
        for (size_t i = 0; i < N; ++i) {
            if (occupied_[i]) {
                reinterpret_cast<T*>(&storage_[i * sizeof(T)])->~T();
            }
        }
    }

    template <typename... Args>
    T& create(Args&&... args) {
        for (size_t i = 0; i < N; ++i) {
            if (!occupied_[i]) {
                new (&storage_[i * sizeof(T)]) T(std::forward<Args>(args)...);
                occupied_[i] = true;
                return *reinterpret_cast<T*>(&storage_[i * sizeof(T)]);
            }
        }
        throw NotEnoughSlotsError(count());
    }

    void delete_(size_t index) {
        if (index >= N || !occupied_[index]) {
            throw EmptySlotError();
        }
        reinterpret_cast<T*>(&storage_[index * sizeof(T)])->~T();
        occupied_[index] = false;
    }

    size_t count() const {
        size_t cnt = 0;
        for (size_t i = 0; i < N; ++i) {
            if (occupied_[i]) ++cnt;
        }
        return cnt;
    }

    T& get(size_t index) {
        if (index >= N || !occupied_[index]) {
            throw EmptySlotError();
        }
        return *reinterpret_cast<T*>(&storage_[index * sizeof(T)]);
    }
    size_t position(const T& obj) {
        for (size_t i = 0; i < N; ++i) {
            if (occupied_[i] && reinterpret_cast<T*>(&storage_[i * sizeof(T)]) == &obj) {
                return i;
            }
        }
        throw EmptySlotError();
    }
};
